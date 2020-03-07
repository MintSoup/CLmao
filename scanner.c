#include "scanner.h"
#include "commons.h"
#include <string.h>

typedef struct {
	const char *start;
	const char *current;
	int line;
} Scanner;

Scanner scanner;


static bool isAtEnd() { return *scanner.current == 0; }

static Token makeToken(TokenType type) {
	Token t;
	t.line = scanner.line;
	t.type = type;
	t.start = scanner.start;
	t.length = (int)(scanner.current - scanner.start);
	return t;
}
static Token errorToken(char *msg) {
	Token t;
	t.line = scanner.line;
	t.type = TOKEN_ERROR;
	t.start = scanner.start;
	t.length = (int)strlen(msg);
	return t;
}
static char advance() { return *(scanner.current++); }
static bool match(char t) {
	if (isAtEnd())
		return false;
	else {
		if (*scanner.current == t) {
			scanner.current++;
			return true;
		} else
			return false;
	}
}

static char peek() { return *scanner.current; }
static char peekNext() {
	if (isAtEnd())
		return 0;
	return scanner.current[1];
}
static void skipWhitespace() {
	for (;;) {
		char c = peek();
		switch (c) {
		case ' ':
		case '\r':
		case '\t':
			advance();
			break;
		case '\n': {
			scanner.line++;
			advance();
			break;
		}
		case '/': {
			if (peekNext() == '/') {
				while (peek() != '\n' && !isAtEnd())
					advance();
			} else
				return;
			break;
		}

		default:
			return;
		}
	}
}

static bool isDigit(char c) { return c >= '0' && c <= '9'; }
static bool isAlpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
static bool isAlphaNumeric(char c) { return isAlpha(c) || isDigit(c); }
static TokenType checkKeyword(int start, int len, const char *rest,
							  TokenType type) {
	if (scanner.current - scanner.start == start + len)
		if (memcmp(scanner.start + start, rest, len) == 0) {
			return type;
		}
	return TOKEN_IDENTIFIER;
}

static TokenType identifierType() {
	switch (scanner.start[0]) {
	case 'a':
		return checkKeyword(1, 2, "nd", TOKEN_AND);
	case 'c':
		return checkKeyword(1, 4, "lass", TOKEN_CLASS);
	case 'e':
		return checkKeyword(1, 3, "lse", TOKEN_ELSE);
	case 'i':
		return checkKeyword(1, 1, "f", TOKEN_IF);
	case 'n':
		return checkKeyword(1, 3, "ull", TOKEN_NULL);
	case 'o':
		return checkKeyword(1, 1, "r", TOKEN_OR);
	case 'p':
		return checkKeyword(1, 4, "rint", TOKEN_PRINT);
	case 'r':
		return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
	case 's':
		return checkKeyword(1, 4, "uper", TOKEN_SUPER);
	case 'l':
		return checkKeyword(1, 2, "et", TOKEN_LET);
	case 'w':
		return checkKeyword(1, 4, "hile", TOKEN_WHILE);
	case 'f':
		switch (scanner.start[1]) {
		case 'o':
			return checkKeyword(2, 1, "r", TOKEN_FOR);
		case 'a':
			return checkKeyword(2, 3, "lse", TOKEN_FALSE);
		case 'u':
			return checkKeyword(2, 3, "unc", TOKEN_FUNC);
		}
	case 't':
		switch (scanner.start[1]) {
		case 'r':
			return checkKeyword(2, 2, "ue", TOKEN_TRUE);
		case 'h':
			return checkKeyword(2, 2, "is", TOKEN_THIS);
		}
		break;
	}
}





void initScanner(const char *src) {
	scanner.start = src;
	scanner.line = 1;
	scanner.current = scanner.start;
}
Token scanToken() {
	skipWhitespace();
	scanner.start = scanner.current;
	if (isAtEnd())
		return makeToken(TOKEN_EOF);

	char c = advance();

	switch (c) {
	case '(':
		return makeToken(TOKEN_LEFT_PAREN);
	case ')':
		return makeToken(TOKEN_RIGHT_PAREN);
	case '{':
		return makeToken(TOKEN_LEFT_BRACE);
	case '}':
		return makeToken(TOKEN_RIGHT_BRACE);
	case ';':
		return makeToken(TOKEN_SEMICOLON);
	case ',':
		return makeToken(TOKEN_COMMA);
	case '.':
		return makeToken(TOKEN_DOT);
	case '-':
		return makeToken(TOKEN_MINUS);
	case '+':
		return makeToken(TOKEN_PLUS);
	case '/':
		return makeToken(TOKEN_SLASH);
	case '*':
		return makeToken(TOKEN_STAR);
	case '!':
		return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
	case '=':
		return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
	case '<':
		return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
	case '>':
		return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
	case '"': {
		while (!isAtEnd() && peek() != '"') {
			if (peek() == '\n')
				scanner.line++;
			advance();
		}
		if (isAtEnd())
			return makeToken(TOKEN_ERROR);
		advance();
		return makeToken(TOKEN_STRING);
	}
	default: {
		if (isAlpha(c)) {
			while (isAlphaNumeric(peek()))
				advance();

			return makeToken(identifierType());

		}

		else if (isDigit(c)) {
			while (isDigit(peek()))
				advance();
			if (peek() == '.' && isDigit(peekNext())) {
				advance();
				while (isDigit(peek()))
					advance();
			}
			return makeToken(TOKEN_NUMBER);
		}
		break;
	}
	}

	return errorToken("Unexpected character");
}

