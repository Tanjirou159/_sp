#include "tiny_compiler.h"

typedef struct {
    const char* source;
    int pos;
    int line;
} Lexer;

Lexer lexer;
Token current_token;
int num_global_names = 0;

void init_lexer(const char* source) {
    lexer.source = source;
    lexer.pos = 0;
    lexer.line = 1;
}

char peek() {
    return lexer.source[lexer.pos];
}

char advance() {
    return lexer.source[lexer.pos++];
}

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

void skip_whitespace() {
    while (peek() == ' ' || peek() == '\t' || peek() == '\r' || peek() == '\n') {
        if (peek() == '\n') lexer.line++;
        advance();
    }
}

void skip_line_comment() {
    while (peek() != '\n' && peek() != '\0') advance();
}

void skip_block_comment() {
    advance(); // consume '*'
    while (peek() != '\0') {
        if (peek() == '*' && lexer.source[lexer.pos + 1] == '/') {
            advance(); advance(); // consume '*/'
            return;
        }
        if (peek() == '\n') lexer.line++;
        advance();
    }
}

Token make_token(TokenType type) {
    Token t;
    t.type = type;
    t.line = lexer.line;
    t.lexeme = NULL;
    return t;
}

Token make_token_str(TokenType type, const char* start, int length) {
    Token t;
    t.type = type;
    t.line = lexer.line;
    t.lexeme = malloc(length + 1);
    strncpy(t.lexeme, start, length);
    t.lexeme[length] = '\0';
    return t;
}

Token make_number_token(int value) {
    Token t = make_token(TOKEN_NUMBER);
    t.int_value = value;
    return t;
}

Token make_string_token(const char* value) {
    Token t = make_token(TOKEN_STRING);
    t.str_value = strdup(value);
    return t;
}

Token get_next_token() {
    skip_whitespace();

    char c = peek();
    if (c == '\0') return make_token(TOKEN_EOF);

    // Comments
    if (c == '/') {
        if (lexer.source[lexer.pos + 1] == '/') {
            advance(); advance();
            skip_line_comment();
            return get_next_token();
        }
        if (lexer.source[lexer.pos + 1] == '*') {
            advance(); advance();
            skip_block_comment();
            return get_next_token();
        }
    }

    // Numbers
    if (is_digit(c)) {
        const char* start = lexer.source + lexer.pos;
        int value = 0;
        while (is_digit(peek())) {
            value = value * 10 + (advance() - '0');
        }
        Token t = make_number_token(value);
        t.lexeme = malloc(lexer.pos - (start - lexer.source) + 1);
        strncpy(t.lexeme, start, lexer.pos - (start - lexer.source));
        t.lexeme[lexer.pos - (start - lexer.source)] = '\0';
        return t;
    }

    // Strings
    if (c == '"') {
        advance();
        const char* start = lexer.source + lexer.pos;
        while (peek() != '"' && peek() != '\0') advance();
        int length = lexer.pos - (start - lexer.source);
        char* str_value = malloc(length + 1);
        strncpy(str_value, start, length);
        str_value[length] = '\0';
        advance(); // Consume closing quote
        return make_string_token(str_value);
    }

    // Identifiers and keywords
    if (is_alpha(c)) {
        const char* start = lexer.source + lexer.pos;
        while (is_alpha(peek()) || is_digit(peek())) advance();
        int length = lexer.pos - (start - lexer.source);

        // Check multi-char keywords first
        if (length == 6 && strncmp(start, "string", 6) == 0)
            return make_token_str(TOKEN_STRING_TYPE, start, length);
        if (length == 6 && strncmp(start, "return", 6) == 0)
            return make_token_str(TOKEN_RETURN, start, length);
        if (length == 5 && strncmp(start, "while", 5) == 0)
            return make_token_str(TOKEN_WHILE, start, length);
        if (length == 5 && strncmp(start, "print", 5) == 0)
            return make_token_str(TOKEN_PRINT, start, length);
        if (length == 5 && strncmp(start, "break", 5) == 0)
            return make_token_str(TOKEN_BREAK, start, length);
        if (length == 5 && strncmp(start, "false", 5) == 0)
            return make_token_str(TOKEN_FALSE, start, length);
        if (length == 4 && strncmp(start, "bool", 4) == 0)
            return make_token_str(TOKEN_BOOL, start, length);
        if (length == 4 && strncmp(start, "func", 4) == 0)
            return make_token_str(TOKEN_FUNC, start, length);
        if (length == 4 && strncmp(start, "else", 4) == 0)
            return make_token_str(TOKEN_ELSE, start, length);
        if (length == 4 && strncmp(start, "true", 4) == 0)
            return make_token_str(TOKEN_TRUE, start, length);
        if (length == 3 && strncmp(start, "int", 3) == 0)
            return make_token_str(TOKEN_INT, start, length);
        if (length == 3 && strncmp(start, "var", 3) == 0)
            return make_token_str(TOKEN_VAR, start, length);
        if (length == 3 && strncmp(start, "for", 3) == 0)
            return make_token_str(TOKEN_FOR, start, length);
        if (length == 2 && strncmp(start, "if", 2) == 0)
            return make_token_str(TOKEN_IF, start, length);
        if (length == 8 && strncmp(start, "continue", 8) == 0)
            return make_token_str(TOKEN_CONTINUE, start, length);

        return make_token_str(TOKEN_IDENT, start, length);
    }

    // Operators and punctuation
    advance();
    switch (c) {
        case '(': return make_token(TOKEN_LPAREN);
        case ')': return make_token(TOKEN_RPAREN);
        case '{': return make_token(TOKEN_LBRACE);
        case '}': return make_token(TOKEN_RBRACE);
        case '[': return make_token(TOKEN_LBRACKET);
        case ']': return make_token(TOKEN_RBRACKET);
        case ';': return make_token(TOKEN_SEMICOLON);
        case ',': return make_token(TOKEN_COMMA);
        case ':': return make_token(TOKEN_COLON);
        case '+': return make_token(TOKEN_PLUS);
        case '-': return make_token(TOKEN_MINUS);
        case '*': return make_token(TOKEN_MUL);
        case '/': return make_token(TOKEN_DIV);
        case '%': return make_token(TOKEN_MOD);
        case '!':
            if (peek() == '=') {
                advance();
                return make_token(TOKEN_NOT_EQUAL);
            }
            return make_token(TOKEN_NOT);
        case '=':
            if (peek() == '=') {
                advance();
                return make_token(TOKEN_EQUAL);
            }
            return make_token(TOKEN_ASSIGN);
        case '<':
            if (peek() == '=') {
                advance();
                return make_token(TOKEN_LTE);
            }
            return make_token(TOKEN_LT);
        case '>':
            if (peek() == '=') {
                advance();
                return make_token(TOKEN_GTE);
            }
            return make_token(TOKEN_GT);
        case '&':
            if (peek() == '&') {
                advance();
                return make_token(TOKEN_AND);
            }
            return make_token(TOKEN_ERROR);
        case '|':
            if (peek() == '|') {
                advance();
                return make_token(TOKEN_OR);
            }
            return make_token(TOKEN_ERROR);
        default:
            return make_token(TOKEN_ERROR);
    }
}
