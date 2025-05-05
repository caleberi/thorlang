#include "common.h"
#include "compiler.h"

Parser parser;
Chunk *compiling_chunk;

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_NONE},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_NONE},
    [TOKEN_GREATER] = {NULL, binary, PREC_NONE},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_NONE},
    [TOKEN_LESS] = {NULL, binary, PREC_NONE},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_NONE},
    [TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
    [TOKEN_QUESTION] = {NULL, ternary, PREC_CONDITIONAL},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
};

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

static void parse_precedence(Precedence precedence)
{
    advance();
    ParseFn prefix_rule = get_rule(parser.previous.type)->prefix;
    if (prefix_rule == NULL)
    {
        error("Expect expression.");
        return;
    }
    prefix_rule();

    while (precedence <= get_rule(parser.current.type)->precedence)
    {
        advance();
        ParseFn infix_rule = get_rule(parser.previous.type)->infix;
        infix_rule();
    }
}

static uint8_t identifier_constant(Token *name)
{
    return make_constant(OBJ_VAL(copy_string(name->start,
                                             name->length)));
}

static void expression();
static void statement();
static void declaration();
static bool check(TokenType type);
static bool match(TokenType type);

static ParseRule *get_rule(TokenType type) { return &rules[type]; }
static void end_compiler()
{
    emit_return();

#ifdef DEBUG_PRINT_CODE
    if (!parser.had_error)
    {
        disassemble_chunk(current_chunk(), "code");
    }
#endif
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

static emit_constant(Value value)
{
    emit_byte_codes(OP_CONSTANT, make_constant(value));
}

static void number()
{
    double v = strtod(parser.current.start, NULL);
    emit_constant(NUMBER_VAL(v));
}

static void expression()
{
    parse_precedence(PREC_ASSIGNMENT);
}

static void grouping()
{
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression");
}

static void string()
{
    emit_constant(OBJ_VAL(copy_string(parser.previous.start + 1,
                                      parser.previous.length - 2)));
}
static void ternary()
{
    parse_precedence(PREC_CONDITIONAL);
    consume(TOKEN_COLON, "Expected ':' after expression");
    parse_precedence(PREC_CONDITIONAL);
    emit_byte_code(OP_TENARY);
}

static void unary()
{
    TokenType operator_type = parser.previous.type;
    parse_precedence(PREC_UNARY);
    switch (operator_type)
    {
    case TOKEN_BANG:
        emit_byte_code(OP_NOT);
        break;
    case TOKEN_MINUS:
        emit_byte_code(OP_NEGATE);
        break;
    default:
        return;
    }
}

static void literal()
{
    switch (parser.previous.type)
    {
    case TOKEN_FALSE:
        emit_byte_code(OP_FALSE);
        break;
    case TOKEN_NIL:
        emit_byte_code(OP_NIL);
        break;
    case TOKEN_TRUE:
        emit_byte_code(OP_TRUE);
        break;
    default:
        return;
    }
}

static void binary()
{
    TokenType operator_type = parser.previous.type;
    ParseRule *rule = get_rule(operator_type);
    parse_precedence((Precedence)rule->precedence + 1);
    switch (operator_type)
    {
    case TOKEN_BANG_EQUAL:
        emit_byte_codes(OP_EQUAL, OP_NOT);
        break;
    case TOKEN_EQUAL_EQUAL:
        emit_byte_code(OP_EQUAL);
        break;
    case TOKEN_GREATER:
        emit_byte_code(OP_GREATER);
        break;
    case TOKEN_GREATER_EQUAL:
        emit_byte_codes(OP_LESS, OP_NOT);
        break;
    case TOKEN_LESS:
        emit_byte_code(OP_LESS);
        break;
    case TOKEN_LESS_EQUAL:
        emit_byte_codes(OP_GREATER, OP_NOT);
        break;
    case TOKEN_PLUS:
        emit_byte_code(OP_ADD);
        break;
    case TOKEN_MINUS:
        emit_byte_code(OP_SUBTRACT);
        break;
    case TOKEN_STAR:
        emit_byte_code(OP_MULTIPLY);
        break;
    case TOKEN_SLASH:
        emit_byte_code(OP_DIVIDE);
        break;
    default:
        return;
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

static bool match(TokenType type)
{
    if (!check(type))
        return false;
    advance();
    return true;
}

static bool check(TokenType type)
{
    return parser.current.type == type;
}

static void print_statement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emit_byte_code(OP_PRINT);
}

static void expression_statement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emit_byte_code(OP_POP);
}

static void statement()
{
    if (match(TOKEN_PRINT))
        print_statement();
    else
        expression_statement();
}

static void synchronize()
{
    parser.panic_mode = false;
    while (parser.current.type != TOKEN_EOF)
    {
        if (parser.previous.type == TOKEN_SEMICOLON)
            return;
        switch (parser.current.type)
        {
        case TOKEN_CLASS:
        case TOKEN_FUN:
        case TOKEN_VAR:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_PRINT:
        case TOKEN_RETURN:
            return;
        default:
        }
        advance();
    }
}

static uint8_t parse_variable(const char *error_message)
{
    consume(TOKEN_IDENTIFIER, error_message);
    return identifier_constant(&parser.previous);
}
static void var_declaration()
{
    uint8_t global = parse_variable("Expect variable name.");
    if (match(TOKEN_EQUAL))
        expression();
    else
        emitByte(OP_NIL);
    consume(TOKEN_SEMICOLON,
            "Expect ';' after variable declaration.");
    define_variable(global);
}

static void declaration()
{
    if (match(TOKEN_VAR))
        var_declaration();
    else
        statement();
    if (parser.panic_mode)
        synchronize();
    return statement();
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
    // expression();
    // consume(TOKEN_EOF, "Expect end of expression.");

    while (!match(TOKEN_EOF))
    {
        declaration();
    }
    cleanup_keyword();
    return !parser.had_error;
}
