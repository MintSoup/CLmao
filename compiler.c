#include "compiler.h"
#include "commons.h"
#include "dbg.h"
#include "object.h"
#include <stdio.h>
#include <stdlib.h>

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

typedef void (*ParseFn)();

typedef struct {
	ParseFn prefix;
	ParseFn infix;
	Precedence precedence;
} ParseRule;

Parser parser;
Chunk *compilingChunk;

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

static void endCompiler() {
	emitByte(OP_PRINT);
	emitReturn();
#ifdef DEBUG_TRACE_BYTECODE
	if (!parser.hadError)
		disassembleChunk(currentChunk(), "code");
#endif
}

static void expression();
static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence p);

static void binary() {
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
	}
}

static void literal() {
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
	}
}

static void grouping() {
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expected ')' after grouping expression");
}

static void number() {
	double value = strtod(parser.previous.start, NULL);
	emitConstant(NUM_VALUE(value));
}

static void string() {
	emitConstant(OBJ_VALUE((Obj *)copyString(parser.previous.start + 1,
											 parser.previous.length - 2)));
}

static void factorial() { emitByte(OP_FACTORIAL); }

static void unary() {
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
	{NULL, NULL, PREC_NONE},		 // TOKEN_IDENTIFIER
	{string, NULL, PREC_PRIMARY},		 // TOKEN_STRING
	{number, NULL, PREC_PRIMARY},		 // TOKEN_NUMBER
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
	prefixRule();
	while (getRule(parser.current.type)->precedence >= p) {
		advance();
		ParseFn infixRule = getRule(parser.previous.type)->infix;
		infixRule();
	}
}

static ParseRule *getRule(TokenType p) { return &rules[p]; }

static void expression() { parsePrecedence(PREC_ASSIGNMENT); }

bool compile(const char *src, Chunk *chunk) {
	parser.panicMode = parser.hadError = false;
	initScanner(src);
	compilingChunk = chunk;
	advance();
	expression();
	consume(TOKEN_EOF, "Expected end of expression");

	endCompiler();
	return !parser.hadError;
}