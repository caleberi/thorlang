#include "common.h"
#include "compiler.h"

typedef struct
{
    PREC_NONE,
        PREC_ASSIGNMENT, // =
        PREC_OR,         // or
        PREC_AND,        // and
        PREC_EQUALITY,   // == !=
        PREC_COMPARISON, // < > <= >=
        PREC_TERM,       // + -
        PREC_FACTOR,     // * /
        PREC_UNARY,      // ! -
        PREC_CALL,       // . ()
        PREC_PRIMARY
} Precedence;

Parser parser;
Chunk *compiling_chunk;

static Chunk *current_chunk() { return compiling_chunk; }
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

static void emit_byte_code(uint8_t byte) { write_chunk(current_chunk(), byte, parser.previous.line); }
static void emit_return() { emit_byte_code(OP_RETURN); }
static void emit_byte_codes(uint8_t b1, uint8_t b2)
{
    emit_byte_code(b1);
    emit_byte_code(b2);
}

static void end_compiler() { emit_return(); }

static void grouping()
{
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression");
}

static uint8_t make_constant(Value value)
{
    int constant = add_constant(currentChunk(), value);
    if (constant > UINT8_MAX)
    {
        error("Too many constants in one chunk.");
        return 0;
    }
    return (uint8_t)constant;
}

static emit_constant(Value value) { emit_byte_codes(OP_CONSTANT, make_constant(value)); }
static void number()
{
    double value = strtod(parser.current.start, NULL);
    emit_constant(value);
}

static void unary()
{
    TokenType operator_type = parser.previous.type;
    expression();
    switch (operator_type)
    {
    case TOKEN_MINUS:
        emit_byte_code(OP_NEGATE);
        break;
    default:
        return;
    }
}

static void parse_precedence(Precedence precedence)
{
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
    compiling_chunk = chunk;

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
    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression.");
    cleanup_keyword();
    return !parser.had_error;
}