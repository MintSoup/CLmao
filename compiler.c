#include "compiler.h"
#include "commons.h"
#include "dbg.h"
#include "object.h"
#include "value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	Token previous;
	Token current;
	bool hadError;
	bool panicMode;
} Parser;

typedef enum {
	PREC_NONE,
	PREC_ASSIGNMENT, // =
	PREC_OR,		 // or
	PREC_AND,		 // and
	PREC_EQUALITY,   // == !=
	PREC_COMPARISON, // < > <= >=
	PREC_TERM,		 // + -
	PREC_FACTOR,	 // *
	PREC_UNARY,		 // ! -
	PREC_CALL,		 // . ()
	PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool);

typedef struct {
	ParseFn prefix;
	ParseFn infix;
	Precedence precedence;
} ParseRule;

typedef struct {
	Token name;
	int depth;
} Local;

typedef struct {
	Local locals[UINT8_COUNT];
	int localCount;
	int scopeDepth;
} Compiler;

Parser parser;
Chunk *compilingChunk;
Compiler *current = NULL;

static void errorAt(Token *token, const char *msg) {
	if (parser.panicMode)
		return;

	parser.panicMode = true;
	fprintf(stderr, "[LINE %d] Error ", token->line);
	if (token->type == TOKEN_EOF) {
		fprintf(stderr, "at EOF");
	} else if (token->type == TOKEN_ERROR) {

	} else {
		fprintf(stderr, "at '%.*s'", token->length, token->start);
	}
	fprintf(stderr, ": %s\n", msg);
	parser.hadError = true;
}

static void errorAtCurrent(const char *msg) { errorAt(&(parser.current), msg); }

static void error(const char *msg) { errorAt(&(parser.previous), msg); }

static void advance() {
	parser.previous = parser.current;
	while (true) {
		parser.current = scanToken();
		if (parser.current.type != TOKEN_ERROR)
			break;
		errorAtCurrent(parser.current.start);
	}
}

static bool consume(TokenType type, char *msg) {
	if (parser.current.type == type) {
		advance();
		return true;
	}
	errorAtCurrent(msg);
	return false;
}

static Chunk *currentChunk() { return compilingChunk; }

static void emitByte(uint8_t byte) {
	writeChunk(currentChunk(), byte, parser.previous.line);
}
static void emitBytes(uint8_t b1, uint8_t b2) {
	emitByte(b1);
	emitByte(b2);
}

static void emitReturn() { emitByte(OP_RETURN); }

uint8_t makeConstant(Value val) {
	int c = addConstant(currentChunk(), val);
	if (c >= UINT8_MAX) {
		error("Too many constants in one chunk");
		return 0;
	}
	return (uint8_t)c;
}

static void emitConstant(Value value) {
	emitBytes(OP_CONSTANT, makeConstant(value));
}

static void initCompiler(Compiler *compiler) {
	compiler->localCount = 0;
	compiler->scopeDepth = 0;
	current = compiler;
}

static void endCompiler() {
	emitReturn();
#ifdef DEBUG_TRACE_BYTECODE
	if (!parser.hadError)
		disassembleChunk(currentChunk(), "OUTPUT");
#endif
}

static void beginScope() { current->scopeDepth++; }
static void endScope() {
	current->scopeDepth--;
	while (current->localCount > 0 &&
		   current->locals[current->localCount - 1].depth >
			   current->scopeDepth) {
		emitByte(OP_POP);
		current->localCount--;
	}
}

static void expression();

static void statement();
static void declaration();
static bool match(TokenType e);
static void printStatement();
static void expressionStatement();
static void synchronize();
static void varDeclaration();
static uint8_t identifierConstant(Token *token);

static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence p);

static void binary(bool canAssign) {
	TokenType operator= parser.previous.type;
	ParseRule *rule = getRule(operator);
	parsePrecedence((int)rule->precedence + 1);

	switch (operator) {
	case TOKEN_PLUS:
		emitByte(OP_ADD);
		break;
	case TOKEN_MINUS:
		emitByte(OP_SUB);
		break;
	case TOKEN_STAR:
		emitByte(OP_MUL);
		break;
	case TOKEN_SLASH:
		emitByte(OP_DIV);
		break;
	case TOKEN_LESS:
		emitByte(OP_LESS);
		break;
	case TOKEN_GREATER:
		emitByte(OP_GREATER);
		break;
	case TOKEN_LESS_EQUAL:
		emitByte(OP_LESS_EQUAL);
		break;
	case TOKEN_GREATER_EQUAL:
		emitByte(OP_GREATER_EQUAL);
		break;
	case TOKEN_EQUAL_EQUAL:
		emitByte(OP_EQUALS);
		break;
	case TOKEN_BANG_EQUAL:
		emitByte(OP_NOT_EQUALS);
		break;
	default:;
	}
}

static void literal(bool canAssign) {
	switch (parser.previous.type) {
	case TOKEN_NULL:
		emitByte(OP_NULL);
		return;
	case TOKEN_FALSE:
		emitByte(OP_FALSE);
		return;
	case TOKEN_TRUE:
		emitByte(OP_TRUE);
		return;
	default:;
	}
}

static void grouping(bool canAssign) {
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expected ')' after grouping expression");
}

static void number(bool canAssign) {
	double value = strtod(parser.previous.start, NULL);
	emitConstant(NUM_VALUE(value));
}

static void string(bool canAssign) {
	emitConstant(OBJ_VALUE((Obj *)copyString(parser.previous.start + 1,
											 parser.previous.length - 2)));
}

static bool identifiersEqual(Token *a, Token *b) {
	if (a->length != b->length)
		return false;
	return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Compiler *compiler, Token *name) {
	for (int i = compiler->localCount - 1; i >= 0; i--) {
		Local *local = &compiler->locals[i];
		if (identifiersEqual(name, &local->name)) {
			if(local->depth == -1){
				error("Cannot reference variable in its own initializer");
			}
			return i;
		}
	}
	return -1;
}

static void namedVariable(Token t, bool canAssign) {
	uint8_t getOp, setOp;
	int arg = resolveLocal(current, &t);
	if (arg != -1) {
		getOp = OP_GET_LOCAL;
		setOp = OP_SET_LOCAL;
	} else {
		arg = identifierConstant(&t);
		getOp = OP_GET_GLOBAL;
		setOp = OP_SET_GLOBAL;
	}

	if (canAssign && match(TOKEN_EQUAL)) {
		expression();
		emitBytes(setOp, (uint8_t)arg);
	} else {
		emitBytes(getOp, (uint8_t)arg);
	}
}
static void variable(bool canAssign) {
	namedVariable(parser.previous, canAssign);
}

static void factorial(bool canAssign) { emitByte(OP_FACTORIAL); }

static void unary(bool canAssign) {
	TokenType operandType = parser.previous.type;
	parsePrecedence(PREC_UNARY);
	switch (operandType) {
	case TOKEN_MINUS:
		emitByte(OP_NEGATE);
		break;
	case TOKEN_BANG:
		emitByte(OP_NOT);
		break;
	default:
		return;
	}
}

ParseRule rules[] = {
	{grouping, NULL, PREC_NONE},	 // TOKEN_LEFT_PAREN
	{NULL, NULL, PREC_NONE},		 // TOKEN_RIGHT_PAREN
	{NULL, NULL, PREC_NONE},		 // TOKEN_LEFT_BRACE
	{NULL, NULL, PREC_NONE},		 // TOKEN_RIGHT_BRACE
	{NULL, NULL, PREC_NONE},		 // TOKEN_COMMA
	{NULL, NULL, PREC_NONE},		 // TOKEN_DOT
	{unary, binary, PREC_TERM},		 // TOKEN_MINUS
	{NULL, binary, PREC_TERM},		 // TOKEN_PLUS
	{NULL, NULL, PREC_NONE},		 // TOKEN_SEMICOLON
	{NULL, binary, PREC_FACTOR},	 // TOKEN_SLASH
	{NULL, binary, PREC_FACTOR},	 // TOKEN_STAR
	{unary, factorial, PREC_UNARY},  // TOKEN_BANG
	{NULL, binary, PREC_EQUALITY},   // TOKEN_BANG_EQUAL
	{NULL, NULL, PREC_NONE},		 // TOKEN_EQUAL
	{NULL, binary, PREC_EQUALITY},   // TOKEN_EQUAL_EQUAL
	{NULL, binary, PREC_COMPARISON}, // TOKEN_GREATER
	{NULL, binary, PREC_COMPARISON}, // TOKEN_GREATER_EQUAL
	{NULL, binary, PREC_COMPARISON}, // TOKEN_LESS
	{NULL, binary, PREC_COMPARISON}, // TOKEN_LESS_EQUAL
	{variable, NULL, PREC_NONE},	 // TOKEN_IDENTIFIER
	{string, NULL, PREC_NONE},		 // TOKEN_STRING
	{number, NULL, PREC_NONE},		 // TOKEN_NUMBER
	{NULL, NULL, PREC_NONE},		 // TOKEN_AND
	{NULL, NULL, PREC_NONE},		 // TOKEN_CLASS
	{NULL, NULL, PREC_NONE},		 // TOKEN_ELSE
	{literal, NULL, PREC_NONE},		 // TOKEN_FALSE
	{NULL, NULL, PREC_NONE},		 // TOKEN_FOR
	{NULL, NULL, PREC_NONE},		 // TOKEN_FUN
	{NULL, NULL, PREC_NONE},		 // TOKEN_IF
	{literal, NULL, PREC_NONE},		 // TOKEN_NULL
	{NULL, NULL, PREC_NONE},		 // TOKEN_OR
	{NULL, NULL, PREC_NONE},		 // TOKEN_PRINT
	{NULL, NULL, PREC_NONE},		 // TOKEN_RETURN
	{NULL, NULL, PREC_NONE},		 // TOKEN_SUPER
	{NULL, NULL, PREC_NONE},		 // TOKEN_THIS
	{literal, NULL, PREC_NONE},		 // TOKEN_TRUE
	{NULL, NULL, PREC_NONE},		 // TOKEN_VAR
	{NULL, NULL, PREC_NONE},		 // TOKEN_WHILE
	{NULL, NULL, PREC_NONE},		 // TOKEN_ERROR
	{NULL, NULL, PREC_NONE},		 // TOKEN_EOF
};

static void parsePrecedence(Precedence p) {
	advance();
	ParseFn prefixRule = getRule(parser.previous.type)->prefix;
	if (prefixRule == NULL) {
		error("Expect expression");
		return;
	}
	bool canAssign = p <= PREC_ASSIGNMENT;
	prefixRule(canAssign);
	while (getRule(parser.current.type)->precedence >= p) {
		advance();
		ParseFn infixRule = getRule(parser.previous.type)->infix;
		infixRule(canAssign);
	}
	if (canAssign && match(TOKEN_EQUAL))
		error("Invalid assignment target");
}

static ParseRule *getRule(TokenType p) { return &rules[p]; }

static void expression() { parsePrecedence(PREC_ASSIGNMENT); }

#define check(x) (parser.current.type == x)

static void block() {
	while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
		declaration();
	consume(TOKEN_RIGHT_BRACE, "Expected '}' after block.");
}

bool compile(const char *src, Chunk *chunk) {
	parser.panicMode = parser.hadError = false;
	initScanner(src);
	compilingChunk = chunk;
	Compiler compiler;
	initCompiler(&compiler);
	advance();
	while (!match(TOKEN_EOF)) {
		declaration();
	}

	endCompiler();
	return !parser.hadError;
}

static void declaration() {
	if (match(TOKEN_LET)) {
		varDeclaration();
	} else {
		statement();
	}
	if (parser.panicMode)
		synchronize();
}

static void statement() {
	if (match(TOKEN_PRINT)) {
		printStatement();
	} else if (match(TOKEN_LEFT_BRACE)) {
		beginScope();
		block();
		endScope();
	} else {
		expressionStatement();
	}
}

static bool match(TokenType e) {
	if (!check(e))
		return false;

	advance();
	return true;
}

static void printStatement() {
	expression();
	consume(TOKEN_SEMICOLON, "Expected ';' after print statement");
	emitByte(OP_PRINT);
}

static void expressionStatement() {
	expression();
	consume(TOKEN_SEMICOLON, "Expected ';' after expression statement.");
	emitByte(OP_POP);
}

static void synchronize() {
	parser.panicMode = false;
	while (!check(TOKEN_EOF)) {
		if (parser.previous.type == TOKEN_SEMICOLON)
			return;
		switch (parser.current.type) {
		case TOKEN_CLASS:
		case TOKEN_FUNC:
		case TOKEN_LET:
		case TOKEN_FOR:
		case TOKEN_IF:
		case TOKEN_WHILE:
		case TOKEN_PRINT:
		case TOKEN_RETURN:
			return;
		default:;
		}
		advance();
	}
}

static uint8_t identifierConstant(Token *token) {
	return makeConstant(
		OBJ_VALUE((Obj *)copyString(token->start, token->length)));
}

static void addLocal(Token t) {
	if (current->localCount >= UINT8_COUNT) {
		error("Too many local variables");
		return;
	}
	Local *loc = &current->locals[current->localCount++];
	loc->name = t;
	loc->depth = -1;
}

static void declareVariable() {
	if (current->scopeDepth == 0)
		return;
	Token *name = &parser.previous;

	for (int i = current->localCount - 1; i >= 0; i--) {
		Local *l = &current->locals[i];
		if (l->depth != -1 && l->depth < current->scopeDepth) {
			break;
		}

		if (identifiersEqual(name, &l->name)) {
			error("Variable with that name already declared in this scope");
		}
	}

	addLocal(*name);
}

static uint8_t parseVariable(char *error) {
	consume(TOKEN_IDENTIFIER, error);
	declareVariable();
	if (current->scopeDepth > 0)
		return 0;
	return identifierConstant(&parser.previous);
}

static void defineVariable(uint8_t global) {
	if (current->scopeDepth > 0) {
		current->locals[current->localCount - 1].depth = current->scopeDepth;
		return;
	}
	emitBytes(OP_DEFINE_GLOBAL, global);
}

static void varDeclaration() {
	uint8_t global = parseVariable("Expected variable name");

	if (match(TOKEN_EQUAL))
		expression();
	else
		emitByte(OP_NULL);

	consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration");

	defineVariable(global);
}

#undef check
