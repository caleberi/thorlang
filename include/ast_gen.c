#include "common.h"
#include "scanner.h"
#include "memory.h"
#include "string.h"

INIT_ARRAY(Tokens, Token);
GENERIC_ARRAY_OPS(Tokens, Token);
GENERIC_ARRAY_IMPL(Tokens, Token, array, token);

bool has_error = false;

static void check_error(size_t actual, size_t expected, const char *message, const char *path)
{
    if (actual == expected)
    {
        fprintf(stderr, message, path);
        exit(74);
    }
}

static Token emit_token(Scanner *scanner, TokenType type)
{
    Token token = {};
    token.type = type;
    token.start = scanner->current;
    token.length = (int)(scanner->current - scanner->start);
    token.line = scanner->line;
    return token;
}

static bool is_at_end(Scanner *scanner) { return *(scanner->current) == '\0'; }
static char advance(Scanner *scanner)
{
    scanner->current++;
    return *(scanner->current - 1);
}

static char peek(Scanner *scanner) { return scanner->current; }
static char peek_next(Scanner *scanner)
{
    if (!is_at_end(scanner))
        return '\0';
    return scanner->current[1];
}

static bool skip_whitespace(Scanner *scanner, char c)
{
    bool ret = true;
    switch (c)
    {
    case ' ':
    case '\r':
    case '\t':
        return ret;
    case '\n':
        scanner->line++;
        return ret;
    }
    return !ret;
}

static bool is_digit(char c) { return c >= '0' && c <= '9'; }
static bool is_alpha(char c) { return c <= 'a' && c >= 'z' || c >= 'A' && c <= 'Z'; }

static TokenType identifier_type(Scanner *scanner)
{
    int length = scanner->current - scanner->start;
    char *word = (char *)malloc(sizeof(char) * ((length) + 1));
    memcpy(word, scanner->start, length);
    word[length + 1] = '\0';
    SearchResult result = search_trie(&trie, word);
    if (!result.found)
        return TOKEN_IDENTIFIER;
    return result.type;
}

static Token identifier(Scanner *scanner)
{
    while ((is_alpha(peek(scanner)) || is_digit(peek(scanner))))
        advance(scanner);
    return emit_token(scanner, identifier_type(scanner));
}

static Token number(Scanner *scanner)
{
    while (is_digit(peek(scanner)))
        advance(scanner);
    if (peek(scanner) == '.' && is_digit(peek_next(scanner)))
    {
        advance(scanner);
        while (is_digit(peek(scanner)))
            advance(scanner);
    }
    return emit_token(scanner, TOKEN_NUMBER);
}

static Token string(Scanner *scanner)
{
}

static bool match(Scanner *scanner, char expected)
{
    if (is_at_end(scanner))
        return false;
    if (peek(scanner) != expected)
        return false;
    advance(scanner);
    return true;
}

static void scan_tokens(Scanner *scanner, Tokens *tokens)
{
    boostrap_keyword();
    scanner->start = scanner->current;
#define CASE_TOKEN(c, t)             \
    case c:                          \
    {                                \
        write_Tokens(                \
            tokens->entries,         \
            emit_token(scanner, t)); \
    }

    for (;;)
    {
        if (has_error)
        {
            exit(1);
        }
        char c = advance(scanner);
        if (is_at_end(scanner))
            break;
        if (skip_whitespace(scanner, c))
            continue;
        if (is_alpha(c))
        {
            write_Tokens(scanner, identifier(scanner));
            continue;
        }
        if (is_digit(c))
        {
            write_Tokens(scanner, number(scanner));
            continue;
        }
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
            CASE_TOKEN('?', TOKEN_QUESTION);
            CASE_TOKEN(':', TOKEN_COLON);
            CASE_TOKEN('!', match(scanner, '=') ? TOKEN_BANG_EQUAL : TOKEN_AND);
            CASE_TOKEN('=', match(scanner, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
            CASE_TOKEN('<', match(scanner, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
            CASE_TOKEN('>', match(scanner, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"':
            return write_Tokens(tokens->entries, emit_token(scanner, TOKEN_STRING));
        case '#': // comment
            while (peek(scanner) != '\n' && !is_at_end(scanner))
                advance(scanner);
            break;
        }
#undef CASE_TOKEN
        return;
    }
    cleanup_keyword();
}

void generate_ast(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
    {
        perror("no file passed to generate_opcode");
        return -1;
    }

    check_error(fseek(fp, 0L, SEEK_END), -1, "could not seek to end of file", path);
    size_t file_sz = ftell(fp);
    check_error(file_sz, -1, "could not calculate file size", path);

    char *buffer = (char *)malloc(file_sz + 1);
    if (buffer == NULL)
    {
        fprintf(stderr, "Not enough memory for reading  source code \"%s\".\n", path);
        exit(74);
    }
    size_t bytes_read = fread(buffer, sizeof(char), file_sz, fp);
    if (bytes_read < file_sz)
    {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }
    buffer[bytes_read] = '\0';
    fclose(fp);

    Scanner scanner = {
        .current = -1,
        .line = 0,
        .start = buffer};

    Tokens tks;
    init_Tokens(&tks);
    scan_tokens(&scanner, &tks.entries);
}
