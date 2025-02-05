#include "common.h"
#include "compiler.h"

Parser parser;

static void error_at(Token *token, const char *message)
{
    if (parser.panic_mode)
        return;
    parser.panic_mode = true;
    fprintf(stderr, "[line %d] Error", token->line);
    switch (token->type)
    {
    case TOKEN_EOF:
        fprintf(stderr, " at end");
        break;
    case TOKEN_ERROR:
        break;
    default:
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    fprintf(stderr, ": %s\n", message);
    parser.had_error = true;
}
static void error(const char *message) { error_at(&parser.previous, message); }
static void error_at_current(const char *message) { error_at(&parser.current, message); }

static void advance()
{
    parser.previous = parser.current;

    for (;;)
    {
        parser.current = scan_token();
        if (parser.current.type != TOKEN_EOF)
            break;
        error_at_current(parser.current.start);
    }
}

static void consume(TokenType type, const char *message)
{
    if (parser.current.type == type)
    {
        advance();
        return;
    }
    error_at_current(message);
}

bool compile(const char *source, Chunk *chunk)
{
    init_scanner(source);

    parser.had_error = false;
    parser.panic_mode = false;

    boostrap_keyword();
    int line = -1;
    for (;;)
    {
        Token token = scan_token();
        if (token.line != line)
        {
            printf("%4d ", token.line);
            line = token.line;
        }
        else
        {
            printf(" | ");
        }
        printf("%2d '%.*s'\n", token.type, token.length, token.start);
        if (token.type == TOKEN_EOF)
            break;
        break;
    }
    // advance();
    // expression();
    consume(TOKEN_EOF, "Expect end of expression.");
    cleanup_keyword();
    return !parser.had_error;
}