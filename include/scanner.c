#include "common.h"
#include "compiler.h"
#include "scanner.h"
#include "trie.h"

Scanner scanner;
Trie trie;

void boostrap_keyword()
{
#define ADD_KEYWORD(word, token_type)     \
    insert_trie(&trie, word, token_type); \
    init_trie(&trie);
    ADD_KEYWORD("and", TOKEN_AND)
    ADD_KEYWORD("class", TOKEN_CLASS)
    ADD_KEYWORD("else", TOKEN_ELSE)
    ADD_KEYWORD("if", TOKEN_IF)
    ADD_KEYWORD("nil", TOKEN_NIL)
    ADD_KEYWORD("or", TOKEN_OR)
    ADD_KEYWORD("print", TOKEN_PRINT)
    ADD_KEYWORD("return", TOKEN_RETURN)
    ADD_KEYWORD("super", TOKEN_SUPER)
    ADD_KEYWORD("while", TOKEN_WHILE)
    ADD_KEYWORD("false", TOKEN_FALSE)
    ADD_KEYWORD("for", TOKEN_FOR)
    ADD_KEYWORD("fun", TOKEN_FUN)
    ADD_KEYWORD("this", TOKEN_THIS)
    ADD_KEYWORD("true", TOKEN_TRUE)
    ADD_KEYWORD("var", TOKEN_VAR)
#undef ADD_KEYWORD
}

void cleanup_keyword()
{
    free_trie(&trie);
}

void init_scanner(const char *source)
{
    scanner.current = source;
    scanner.start = source;
    scanner.line = 1;
}

static bool is_digit(char c) { return c >= '0' && c <= '9'; }
static bool is_at_end() { return *scanner.current == '\0'; }
static bool is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
static char peek() { return *scanner.current; }

static Token emit_token(TokenType type)
{
    Token token = {};
    token.type = type;
    token.start = scanner.current;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

static Token emit_error_token(const char *message)
{
    Token token = {
        .length = (int)strlen(message),
        .line = scanner.line,
        .type = TOKEN_ERROR,
        .start = message,
    };
    return token;
}

static char advance()
{
    scanner.current++;
    return *(scanner.current - 1);
}

static char peek_next()
{
    if (is_at_end())
        return '\0';
    return scanner.current[1];
}

static bool match(char expected)
{
    if (is_at_end())
        return false;
    if (*scanner.current != expected)
        return false;
    scanner.current++;
    return true;
}

static void skip_whitespace()
{
    for (;;)
    {
        switch (peek())
        {
        case ' ':
        case '\r':
        case '\t':
            advance();
            break;
        case '\n':
            scanner.line++;
            advance();
            break;
        default:
            return;
        }
    }
}

static Token string()
{
    while (peek() != '"' && !is_at_end())
    {
        if (peek() == '\n')
            scanner.line++;
        advance();
    }
    if (is_at_end())
        return emit_error_token("Unterminated string.");
    advance(); // closing quote
    return emit_token(TOKEN_STRING);
}

static Token number()
{
    while (is_digit(peek()))
        advance();
    if (peek() == '.' && is_digit(peek_next()))
    {
        advance();
        while (is_digit(peek()))
            advance();
    }
    return emit_token(TOKEN_NUMBER);
}

static TokenType identifier_type()
{
    int length = scanner.current - scanner.start;
    char *word = (char *)malloc(sizeof(char) * ((length) + 1));
    memcpy(word, scanner.start, length);
    word[length + 1] = '\0';
    // O(N) - Compared to the original
    SearchResult result = search_trie(&trie, word);
    if (!result.found)
        return TOKEN_IDENTIFIER;
    return result.type;
}

static Token identifier()
{
    while ((is_alpha(peek()) || is_digit(peek())))
        advance();
    return emit_token(identifier_type());
}

Token scan_token()
{
    skip_whitespace();
#define CASE_TOKEN(char, token) \
    case char:                  \
        return emit_token(token);
    scanner.start = scanner.current;
    if (is_at_end())
        return emit_token(TOKEN_EOF);

    char c = advance();
    if (is_alpha(c))
        return identifier();
    if (is_digit(c))
        return number();
    switch (c)
    {
        CASE_TOKEN('.', TOKEN_DOT);
        CASE_TOKEN(',', TOKEN_COMMA);
        CASE_TOKEN('-', TOKEN_MINUS);
        CASE_TOKEN('+', TOKEN_PLUS);
        CASE_TOKEN('/', TOKEN_SLASH);
        CASE_TOKEN('*', TOKEN_STAR);
        CASE_TOKEN('(', TOKEN_LEFT_PAREN);
        CASE_TOKEN(')', TOKEN_RIGHT_PAREN);
        CASE_TOKEN('{', TOKEN_LEFT_BRACE);
        CASE_TOKEN('}', TOKEN_RIGHT_BRACE);
        CASE_TOKEN(';', TOKEN_SEMICOLON);
        CASE_TOKEN('!', match('=') ? TOKEN_BANG_EQUAL : TOKEN_AND);
        CASE_TOKEN('=', match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        CASE_TOKEN('<', match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        CASE_TOKEN('>', match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '"':
        return string();
    case '#': // comment
        while (peek() != '\n' && !is_at_end())
            advance();
        break;
    }
#undef CASE_TOKEN
    return emit_error_token("Unexpected character.");
}