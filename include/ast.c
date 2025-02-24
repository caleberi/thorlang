#include "common.h"
#include "scanner.h"
#include "memory.h"
#include "string.h"
#include "trie.h"
#include "expr.h"

#define MEM_CHECK(variable, action, ...)                                 \
    if (variable == NULL)                                                \
    {                                                                    \
        fprintf(stderr, "Memory allocation failed for %s\n", #variable); \
        action __VA_ARGS__;                                              \
    }

#define TOKEN_TYPES                         \
    X(TOKEN_LEFT_PAREN, "LEFT_PAREN")       \
    X(TOKEN_RIGHT_PAREN, "RIGHT_PAREN")     \
    X(TOKEN_LEFT_BRACE, "LEFT_BRACE")       \
    X(TOKEN_RIGHT_BRACE, "RIGHT_BRACE")     \
    X(TOKEN_COMMA, "COMMA")                 \
    X(TOKEN_DOT, "DOT")                     \
    X(TOKEN_MINUS, "MINUS")                 \
    X(TOKEN_PLUS, "PLUS")                   \
    X(TOKEN_SEMICOLON, "SEMICOLON")         \
    X(TOKEN_SLASH, "SLASH")                 \
    X(TOKEN_STAR, "STAR")                   \
    X(TOKEN_BANG, "BANG")                   \
    X(TOKEN_BANG_EQUAL, "BANG_EQUAL")       \
    X(TOKEN_EQUAL, "EQUAL")                 \
    X(TOKEN_EQUAL_EQUAL, "EQUAL_EQUAL")     \
    X(TOKEN_GREATER, "GREATER")             \
    X(TOKEN_GREATER_EQUAL, "GREATER_EQUAL") \
    X(TOKEN_LESS, "LESS")                   \
    X(TOKEN_LESS_EQUAL, "LESS_EQUAL")       \
    X(TOKEN_QUESTION, "QUESTION")           \
    X(TOKEN_COLON, "COLON")                 \
    X(TOKEN_IDENTIFIER, "IDENTIFIER")       \
    X(TOKEN_STRING, "STRING")               \
    X(TOKEN_NUMBER, "NUMBER")               \
    X(TOKEN_AND, "AND")                     \
    X(TOKEN_CLASS, "CLASS")                 \
    X(TOKEN_ELSE, "ELSE")                   \
    X(TOKEN_FALSE, "FALSE")                 \
    X(TOKEN_FOR, "FOR")                     \
    X(TOKEN_FUN, "FUN")                     \
    X(TOKEN_IF, "IF")                       \
    X(TOKEN_NIL, "NIL")                     \
    X(TOKEN_OR, "OR")                       \
    X(TOKEN_PRINT, "PRINT")                 \
    X(TOKEN_RETURN, "RETURN")               \
    X(TOKEN_SUPER, "SUPER")                 \
    X(TOKEN_THIS, "THIS")                   \
    X(TOKEN_TRUE, "TRUE")                   \
    X(TOKEN_VAR, "VAR")                     \
    X(TOKEN_WHILE, "WHILE")                 \
    X(TOKEN_ERROR, "ERROR")                 \
    X(TOKEN_EOF, "EOF")

Trie trie;
INIT_ARRAY(Tokens, Token);
GENERIC_ARRAY_OPS(Tokens, Token);
GENERIC_ARRAY_IMPL(Tokens, Token, array, token);

const char *token_type_to_string(TokenType type)
{
    switch (type)
    {
#define X(token, str) \
    case token:       \
        return str;
        TOKEN_TYPES
#undef X
    default:
        return "UNKNOWN_TOKEN";
    }
}

void boostrap_keyword()
{
#define ADD_KEYWORD(word, token_type) \
    insert_trie(&trie, word, token_type);

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
    ADD_KEYWORD("func", TOKEN_FUN)
    ADD_KEYWORD("this", TOKEN_THIS)
    ADD_KEYWORD("true", TOKEN_TRUE)
    ADD_KEYWORD("var", TOKEN_VAR)
#undef ADD_KEYWORD
}

void cleanup_keyword() { free_trie(&trie); }

bool has_error = false;

static void check_error(
    size_t actual,
    size_t expected,
    const char *message,
    const char *path)
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
    token.start = scanner->start;
    token.length = (int)(scanner->current - scanner->start);
    token.line = scanner->line;
    return token;
}

static bool is_digit(char c) { return c >= '0' && c <= '9'; }

static bool is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

static bool is_at_end(Scanner *scanner)
{
    return *(scanner->current) == '\0';
}

static char advance(Scanner *scanner)
{
    scanner->current++;
    return *(scanner->current - 1);
}

static char peek(Scanner *scanner)
{
    return scanner->current[0];
}

static char peek_next(Scanner *scanner)
{
    if (is_at_end(scanner))
        return '\0';
    return scanner->current[1];
}

static bool skip_whitespace(Scanner *scanner, char c)
{
    switch (c)
    {
    case ' ':
    case '\r':
    case '\t':
        return true;
    case '\n':
        scanner->line++;
        return true;
    }
    return false;
}

static TokenType identifier_type(Scanner *scanner)
{
    int length = scanner->current - scanner->start;
    char *word = (char *)malloc(sizeof(char) * ((length) + 1));
    MEM_CHECK(word, return, TOKEN_ERROR);
    memcpy(word, scanner->start, length);
    word[length + 1] = '\0';
    SearchResult result = search_trie(&trie, word);
    return result.found ? result.type : TOKEN_IDENTIFIER;
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
    while (peek(scanner) != '"' && !is_at_end(scanner))
    {
        if (peek(scanner) == '\n')
            scanner->line++;
        advance(scanner);
    }
    if (is_at_end(scanner))
    {
        has_error = true;
        return emit_token(scanner, TOKEN_ERROR);
    }
    advance(scanner);
    return emit_token(scanner, TOKEN_STRING);
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
#define CASE_TOKEN(c, t)             \
    case c:                          \
    {                                \
        write_Tokens(                \
            tokens,                  \
            emit_token(scanner, t)); \
        continue;                    \
    }

    for (;;)
    {
        if (has_error)
            break;
        scanner->start = scanner->current;
        if (is_at_end(scanner))
            break;
        char c = advance(scanner);
        if (skip_whitespace(scanner, c))
            continue;

        if (is_alpha(c))
        {
            write_Tokens(tokens, identifier(scanner));
            continue;
        }
        if (is_digit(c))
        {
            write_Tokens(tokens, number(scanner));
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
            string(scanner);
        case '#': // comment
            while (peek(scanner) != '\n' && !is_at_end(scanner))
                advance(scanner);
            break;
        }
    }
    write_Tokens(tokens, emit_token(scanner, TOKEN_EOF));
    cleanup_keyword();
#undef CASE_TOKEN
    return;
}

typedef enum
{
    PARSER_SUCCESS,
    PARSER_ERROR_UNEXPECTED_TOKEN,
    PARSER_ERROR_EOF
} ParserResult;

typedef struct
{
    Tokens *tokens;
    size_t current;
    ParserResult last_error;
} Parser;

ParserResult parser_get_error(const Parser *parser) { return parser ? parser->last_error : PARSER_ERROR_EOF; }
void parser_reset_error(Parser *parser)
{
    if (parser)
    {
        parser->last_error = PARSER_SUCCESS;
    }
}
Parser *parser_create(Tokens *tokens)
{
    Parser *parser = malloc(sizeof(Parser));
    if (!parser)
        return NULL;

    parser->tokens = tokens;
    parser->current = 0;
    parser->last_error = PARSER_SUCCESS;
    return parser;
}

void parser_destroy(Parser *parser)
{
    if (!parser)
        return;
    free_Tokens(parser->tokens);
    free(parser);
}

bool parser_is_at_end(const Parser *parser)
{
    if (!parser)
        return true;
    return parser->current >= parser->tokens->count ||
           parser_peek(parser).type == TOKEN_EOF;
}

Token parser_advance(Parser *parser)
{
    if (!parser_is_at_end(parser))
        parser->current++;

    return parser_peek(parser);
}

Token parser_peek(const Parser *parser)
{
    if (!parser || parser_is_at_end(parser))
    {
        return (Token){.type = TOKEN_EOF};
    }
    return parser->tokens->entries[parser->current];
}

bool parser_match(Parser *parser, TokenType type)
{
    if (!parser || parser_is_at_end(parser))
        return false;

    if (parser_peek(parser).type == type)
    {
        parser_advance(parser);
        return true;
    }
    return false;
}

ParserResult parser_consume(Parser *parser, TokenType expected_type, const char *error_message)
{
    if (parser_match(parser, expected_type))
    {
        return PARSER_SUCCESS;
    }

    parser->last_error = PARSER_ERROR_UNEXPECTED_TOKEN;
    return parser->last_error;
}

void generate_ast(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
    {
        perror("no file passed to generate_opcode");
        return;
    }

    check_error(
        fseek(fp, 0L, SEEK_END),
        (size_t)-1,
        "could not seek to end of file", path);
    size_t file_sz = ftell(fp);
    check_error(
        file_sz,
        (size_t)-1,
        "could not calculate file size", path);
    rewind(fp);
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
        .current = buffer,
        .line = 1,
        .start = buffer};

    printf("buffer: %s\n", buffer);
    Tokens tks;
    init_Tokens(&tks);
    scan_tokens(&scanner, &tks);

    printf("Token List:\n");
    for (size_t i = 0; i < tks.count; i++)
    {
        Token *ptr = &tks.entries[i];
        char *lexeme = malloc(sizeof(char) * (ptr->length + 1));
        MEM_CHECK(lexeme, break);
        memcpy(lexeme, ptr->start, ptr->length);
        lexeme[ptr->length] = '\0';

        printf("Token: type=%s, lexeme='%s', line=%d\n",
               token_type_to_string(ptr->type),
               lexeme,
               ptr->line);
        free(lexeme);
    }

    free(buffer);
    free_Tokens(&tks);
}

void write_test_file(const char *content, const char *filename)
{
    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        fprintf(stderr, "Could not create test file\n");
        exit(1);
    }
    fprintf(fp, "%s", content);
    fclose(fp);
}

void test_basic_tokens()
{
    printf("Testing basic tokens...\n");
    const char *test_input =
        "var x = 42;\n"
        "if (x > 10) {\n"
        "    print x;\n"
        "}\n";

    write_test_file(test_input, "test_basic.txt");
    generate_ast("test_basic.txt");
    printf("Basic tokens test completed\n\n");
}

void test_numbers_and_strings()
{
    printf("Testing numbers and strings...\n");
    const char *test_input =
        "123\n"
        "45.67\n"
        "\"Hello, World!\"\n"
        "3.14159\n";

    write_test_file(test_input, "test_numbers.txt");
    generate_ast("test_numbers.txt");
    printf("Numbers and strings test completed\n\n");
}

void test_operators()
{
    printf("Testing operators...\n");
    const char *test_input =
        "a + b\n"
        "x - y\n"
        "m * n\n"
        "p / q\n"
        "a == b\n"
        "x != y\n"
        "m <= n\n"
        "p >= q\n";

    write_test_file(test_input, "test_operators.txt");
    generate_ast("test_operators.txt");
    printf("Operators test completed\n\n");
}

void test_comments()
{
    printf("Testing comments...\n");
    const char *test_input =
        "# This is a comment\n"
        "var x = 10; # Inline comment\n"
        "# Another comment\n"
        "print x;\n";

    write_test_file(test_input, "test_comments.txt");
    generate_ast("test_comments.txt");
    printf("Comments test completed\n\n");
}

void test_complex_code()
{
    printf("Testing complex code...\n");
    const char *test_input =
        "func factorial(n) {\n"
        "    if (n <= 1) return 1;\n"
        "    return n * factorial(n - 1);\n"
        "}\n"
        "\n"
        "var result = factorial(5);\n"
        "print result;\n";

    write_test_file(test_input, "test_complex.txt");
    generate_ast("test_complex.txt");
    printf("Complex code test completed\n\n");
}

int main()
{
    printf("Starting scanner tests...\n\n");

    test_basic_tokens();
    test_numbers_and_strings();
    test_operators();
    test_comments();
    test_complex_code();

    printf("All scanner tests completed.\n");
    return 0;
}