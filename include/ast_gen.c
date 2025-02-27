#include "common.h"
#include "scanner.h"
#include "memory.h"
#include "string.h"
#include "trie.h"
#include "expr.h"
#include "stmt.h"

// #define DEBUG_EXPR_TREE_S_FORMAT
#define DEBUG_EXPR_TREE

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

static void print_expr(Expr *expr, int indent);
static void print_indent(int indent);
static const char *token_type_to_string(TokenType type);
static void print_expr_as_s_expr(Expr *expr);
void print_stmt(Stmt *stmt, int indent);
static void print_indent(int indent);
void print_stmt_list(Stmt **statements, int count, int indent);

Trie trie;
INIT_ARRAY(Tokens, Token);
GENERIC_ARRAY_OPS(Tokens, Token);
GENERIC_ARRAY_IMPL(Tokens, Token, array, token);
typedef enum
{
    PARSER_SUCCESS,
    PARSER_ERROR_UNEXPECTED_TOKEN,
    PARSER_ERROR_EOF
} ParserResult;

typedef struct
{
    Tokens *tokens;
    int current;
    ParserResult last_error;
} Parser;

static Expr *parse_equality(Parser *parser);
static Expr *parse_comparison(Parser *parser);
static Expr *parse_term(Parser *parser);
static Expr *parse_factor(Parser *parser);
static Expr *parse_unary(Parser *parser);
static Expr *parse_primary(Parser *parser);
static char *parse_lexeme(Token);
static Token parser_peek(const Parser *parser);
void parser_advance(Parser *parser);

static Stmt *parse_declaration(Parser *parser);
static Stmt *parse_statement(Parser *parser);
static Stmt **parse_program(Parser *parser, int *count);
static Stmt *parse_print_statement(Parser *parser);
static Stmt *parse_debug_statement(Parser *parser);
static Stmt *parse_block_statement(Parser *parser);
static Stmt *parse_if_statement(Parser *parser);
static Stmt *parse_while_statement(Parser *parser);
static Stmt *parse_for_statement(Parser *parser);
static Stmt *parse_var_declaration(Parser *parser);
static Expr *parse_expression(Parser *parser);
ParserResult parser_consume(Parser *parser, TokenType expected_type, const char *error_message);
bool parser_is_at_end(const Parser *parser);
bool parser_match(Parser *parser, TokenType type);

static const char *token_type_to_string(TokenType type)
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
    ADD_KEYWORD("debug", TOKEN_DEBUG)
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
            write_Tokens(tokens, string(scanner));
            continue;
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

static Stmt *parse_declaration(Parser *parser)
{
    if (parser_match(parser, TOKEN_VAR))
    {
        parser_advance(parser);
        return parse_var_declaration(parser);
    }
    return parse_statement(parser);
}

static Stmt *parse_debug_statement(Parser *parser)
{
    Expr *expr = parse_expression(parser);
    if (expr == NULL)
        return NULL;

    if (parser_consume(parser, TOKEN_SEMICOLON, "Expect ';' after value.") != PARSER_SUCCESS)
    {
        free(expr);
        return NULL;
    }

    Stmt *stmt = malloc(sizeof(Stmt));
    if (!stmt)
        return NULL;

    stmt->tag = STMT_DEBUG;
    stmt->as.debugStatement.expression = expr;
    stmt->as.debugStatement.line = parser_peek(parser).line;
    return stmt;
}

static Stmt *parse_print_statement(Parser *parser)
{
    Expr *expr = parse_expression(parser);
    if (expr == NULL)
        return NULL;

    if (parser_consume(parser, TOKEN_SEMICOLON, "Expect ';' after value.") != PARSER_SUCCESS)
    {
        free(expr);
        return NULL;
    }

    Stmt *stmt = malloc(sizeof(Stmt));
    if (!stmt)
        return NULL;

    stmt->tag = STMT_PRINT;
    stmt->as.printStmt.expression = expr;
    return stmt;
}

static Stmt *parse_block_statement(Parser *parser)
{
    int capacity = 8;
    int stmt_count = 0;
    Stmt **statements = malloc(sizeof(Stmt *) * capacity);
    if (statements == NULL)
        return NULL;

    while (!parser_is_at_end(parser) && !parser_match(parser, TOKEN_RIGHT_BRACE))
    {
        Stmt *stmt = parse_declaration(parser);
        if (stmt == NULL)
        {
            if (stmt_count >= capacity)
            {
                capacity *= 2;
                statements = realloc(statements, sizeof(Stmt *) * ((capacity * 0.5) + 1));
                if (!statements)
                    return NULL;
            }
            statements[stmt_count++] = stmt;
        }
    }

    if (parser_is_at_end(parser) && parser_peek(parser).type != TOKEN_RIGHT_BRACE)
    {
        parser->last_error = PARSER_ERROR_UNEXPECTED_TOKEN;
        return NULL;
    }

    Stmt *stmt = malloc(sizeof(Stmt));
    if (stmt == NULL)
        return NULL;

    stmt->tag = STMT_BLOCK;
    stmt->as.blockStmt.statements = statements;
    stmt->as.blockStmt.count = stmt_count;
    return stmt;
}

static Stmt *parse_if_statement(Parser *parser)
{
    if (parser_consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'if'.") != PARSER_SUCCESS)
        return NULL;

    Expr *condition = parse_expression(parser);

    if (condition == NULL)
        return NULL;

    if (parser_consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after condition.") != PARSER_SUCCESS)
    {
        free(condition);
        return NULL;
    }

    Stmt *then_branch = parse_statement(parser);
    if (then_branch == NULL)
    {
        free(condition);
        return NULL;
    }

    Stmt *else_branch = NULL;
    if (parser_match(parser, TOKEN_ELSE))
    {
        parser_advance(parser);
        else_branch = parse_statement(parser);
        if (else_branch == NULL)
        {
            free(condition);
            free(else_branch);
            return NULL;
        }
    }

    Stmt *stmt = malloc(sizeof(Stmt));
    if (stmt == NULL)
        return NULL;

    stmt->tag = STMT_IF;
    stmt->as.ifStmt.condition = condition;
    stmt->as.ifStmt.thenBranch = then_branch;
    stmt->as.ifStmt.elseBranch = else_branch;
    return stmt;
}

static Stmt *parse_while_statement(Parser *parser)
{
    if (parser_consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'while'.") != PARSER_SUCCESS)
        return NULL;
    Expr *condition = parse_expression(parser);
    if (condition == NULL)
        return NULL;

    if (parser_consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after condition.") != PARSER_SUCCESS)
    {
        free(condition);
        return NULL;
    }

    Stmt *body = parse_statement(parser);
    if (body == NULL)
    {
        free(condition);
        return NULL;
    }

    Stmt *stmt = malloc(sizeof(Stmt));
    if (stmt == NULL)
        return NULL;

    stmt->tag = STMT_WHILE;
    stmt->as.whileStmt.condition = condition;
    stmt->as.whileStmt.body = body;
    return stmt;
}

static Stmt *parse_for_statement(Parser *parser)
{
    if (parser_consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'for'.") != PARSER_SUCCESS)
        return NULL;

    Stmt *initializer = NULL;
    if (parser_match(parser, TOKEN_SEMICOLON))
        parser_advance(parser);
    else if (parser_match(parser, TOKEN_VAR))
    {
        parser_advance(parser);
        initializer = parse_var_declaration(parser);
        if (initializer == NULL)
            return NULL;
    }
    else
    {
        Expr *init_expr = parse_expression(parser);
        if (init_expr == NULL)
            return NULL;

        if (parser_consume(parser, TOKEN_SEMICOLON, "Expect ';' after for-loop initializer.") != PARSER_SUCCESS)
        {
            free(init_expr);
            return NULL;
        }
    }

    Expr *condition = NULL;
    if (!parser_match(parser, TOKEN_SEMICOLON))
    {
        parser_advance(parser);
        condition = parse_expression(parser);
        if (condition == NULL)
        {
            free(initializer);
            return NULL;
        }
    }

    if (parser_consume(parser, TOKEN_SEMICOLON, "Expect ';' after for-loop condition.") != PARSER_SUCCESS)
    {
        free(initializer);
        free(condition);
        return NULL;
    }

    Expr *increment = NULL;
    if (!parser_match(parser, TOKEN_RIGHT_PAREN))
    {
        parser_advance(parser);
        increment = parse_expression(parser);
        if (increment == NULL)
        {
            free(condition);
            free(initializer);
            return NULL;
        }
    }

    if (parser_consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after for-loop clauses.") != PARSER_SUCCESS)
    {
        free(increment);
        free(condition);
        free(initializer);
        return NULL;
    }

    Stmt *body = parse_statement(parser);
    if (body == NULL)
    {
        free(increment);
        free(condition);
        free(initializer);
        return NULL;
    }

    Stmt *stmt = malloc(sizeof(Stmt));
    if (stmt == NULL)
    {
        free(increment);
        free(condition);
        free(initializer);
        free(body);
        return NULL;
    }

    stmt->tag = STMT_FOR;
    stmt->as.forStmt.initializer = initializer;
    stmt->as.forStmt.condition = condition;
    stmt->as.forStmt.increment = increment;
    stmt->as.forStmt.body = body;

    return stmt;
}

static Stmt *parse_var_declaration(Parser *parser)
{
    if (parser_consume(parser, TOKEN_IDENTIFIER, "Expect variable name.") != PARSER_SUCCESS)
        return NULL;

    Token name = parser_peek(parser);
    parser_advance(parser);

    Expr *initializer = NULL;
    if (parser_match(parser, TOKEN_EQUAL))
    {
        parser_advance(parser);
        initializer = parse_expression(parser);
        if (!initializer)
            return NULL;
    }

    if (parser_consume(parser, TOKEN_SEMICOLON, "Expect ';' after variable declaration.") != PARSER_SUCCESS)
    {
        free(initializer);
        return NULL;
    }

    Stmt *stmt = malloc(sizeof(Stmt));
    if (!stmt)
        return NULL;

    stmt->tag = STMT_VAR_DECL;
    stmt->as.varDeclStmt.name = name;
    stmt->as.varDeclStmt.initializer = initializer;

    return stmt;
}

static Stmt *parse_statement(Parser *parser)
{
    if (parser_match(parser, TOKEN_PRINT))
    {
        parser_advance(parser);
        return parse_print_statement(parser);
    }

    if (parser_match(parser, TOKEN_DEBUG))
    {
        parser_advance(parser);
        return parse_debug_statement(parser);
    }

    if (parser_match(parser, TOKEN_LEFT_BRACE))
    {
        parser_advance(parser);
        return parse_block_statement(parser);
    }

    if (parser_match(parser, TOKEN_IF))
    {
        parser_advance(parser);
        return parse_if_statement(parser);
    }

    if (parser_match(parser, TOKEN_WHILE))
    {
        parser_advance(parser);
        return parse_while_statement(parser);
    }

    if (parser_match(parser, TOKEN_FOR))
    {
        parser_advance(parser);
        return parse_for_statement(parser);
    }

    Expr *expr = parse_expression(parser);
    if (expr == NULL)
        return NULL;

    if (parser_consume(parser, TOKEN_SEMICOLON, "Expect ';' after expression.") != PARSER_SUCCESS)
    {
        free(expr);
        return NULL;
    }

    return NULL;
}

static Stmt **parse_program(Parser *parser, int *count)
{

    int capacity = 8;
    int stmt_count = 0;
    Stmt **statements = malloc(sizeof(Stmt *) * capacity);
    if (statements == NULL)
        return NULL;
    while (!parser_is_at_end(parser))
    {
        Stmt *stmt = parse_declaration(parser);
        if (stmt != NULL)
        {
            if (stmt_count >= capacity)
            {
                capacity *= 2;
                statements = realloc(statements, sizeof(Stmt *) * ((capacity * 0.5) + 1));
                if (!statements)
                    return NULL;
            }
            statements[stmt_count++] = stmt;
        }
        else
        {
            // Error recovery - skip to next statement
            // This could be more sophisticated with synchronization
            while (!parser_is_at_end(parser))
            {
                if (parser_match(parser, TOKEN_SEMICOLON))
                    break;
                parser_advance(parser);
            }
        }
    }

    *count = stmt_count;
    return statements;
}

static char *parse_lexeme(Token token)
{
    char *lexeme = malloc(sizeof(char) * (token.length + 1));
    MEM_CHECK(lexeme, return, NULL);
    memcpy(lexeme, token.start, token.length);
    lexeme[token.length] = '\0';
    return lexeme;
}

ParserResult parser_get_error(const Parser *parser) { return parser ? parser->last_error : PARSER_ERROR_EOF; }
void parser_reset_error(Parser *parser)
{
    if (parser != NULL)
        parser->last_error = PARSER_SUCCESS;
}

Parser *parser_create(Tokens *tokens)
{
    Parser *parser = malloc(sizeof(Parser));
    if (parser == NULL)
        return NULL;

    parser->tokens = tokens;
    parser->current = 0;
    parser->last_error = PARSER_SUCCESS;
    return parser;
}

void parser_destroy(Parser *parser)
{
    if (parser == NULL)
        return;
    free_Tokens(parser->tokens);
    parser->tokens = NULL;
    free(parser);
}

bool parser_is_at_end(const Parser *parser)
{
    if (parser == NULL)
        return false;
    return parser->current >= parser->tokens->count || parser_peek(parser).type == TOKEN_EOF;
}

void parser_advance(Parser *parser)
{
    if (!parser_is_at_end(parser))
        parser->current++;
}

Token parser_peek(const Parser *parser)
{
    return parser->tokens->entries[parser->current];
}

bool parser_match(Parser *parser, TokenType type)
{
    if (parser == NULL || parser_is_at_end(parser))
        return false;

    if (parser_peek(parser).type == type)
        return true;
    return false;
}

ParserResult parser_consume(Parser *parser, TokenType expected_type, const char *error_message)
{
    if (parser_match(parser, expected_type))
    {
        parser_advance(parser);
        return PARSER_SUCCESS;
    }

    parser->last_error = PARSER_ERROR_UNEXPECTED_TOKEN;
    printf("[ERROR]: %s\n", error_message);
    return parser->last_error;
}

static Expr *create_binary_expr(Expr *left, TokenType op, Expr *right)
{
    Expr *expr = malloc(sizeof(Expr));
    if (expr == NULL)
        return NULL;

    expr->tag = EXPR_BINARY;
    expr->as.binary.left = left;
    expr->as.binary.right = right;
    expr->as.binary.op = op;
    return expr;
}

static Expr *create_unary_expr(TokenType op, Expr *right)
{
    Expr *expr = malloc(sizeof(Expr));
    if (expr == NULL)
        return NULL;

    expr->tag = EXPR_UNARY;
    expr->as.unary.op = op;
    expr->as.unary.expr = right;
    return expr;
}

Expr *parse_expression(Parser *parser) { return parse_equality(parser); }

// Equailty -> Comparison (( "!=" | "==" ) Comparison)*
static Expr *parse_equality(Parser *parser)
{
    Expr *expr = parse_comparison(parser);
    if (expr == NULL)
        return NULL;

    while (parser_match(parser, TOKEN_BANG_EQUAL) ||
           parser_match(parser, TOKEN_EQUAL_EQUAL))
    {
        TokenType op = parser_peek(parser).type;
        parser_advance(parser);
        Expr *right = parse_comparison(parser);
        if (right == NULL)
            return NULL;
        expr = create_binary_expr(expr, op, right);
    }
    return expr;
}

// Comparison -> Term ((">" | ">=" | "<" | "<=") Term)*
static Expr *parse_comparison(Parser *parser)
{
    Expr *expr = parse_term(parser);
    if (expr == NULL)
        return NULL;

    while (parser_match(parser, TOKEN_GREATER) ||
           parser_match(parser, TOKEN_GREATER_EQUAL) ||
           parser_match(parser, TOKEN_LESS) ||
           parser_match(parser, TOKEN_LESS_EQUAL))
    {
        TokenType op = parser_peek(parser).type;
        parser_advance(parser);
        Expr *right = parse_term(parser);
        if (right == NULL)
            return NULL;
        expr = create_binary_expr(expr, op, right);
    }
    return expr;
}

// Term -> Factor (("-" | "+") Factor)*
static Expr *parse_term(Parser *parser)
{
    Expr *expr = parse_factor(parser);
    if (expr == NULL)
        return NULL;

    while (parser_match(parser, TOKEN_MINUS) ||
           parser_match(parser, TOKEN_PLUS))
    {
        TokenType op = parser_peek(parser).type;
        parser_advance(parser);
        Expr *right = parse_factor(parser);
        if (right == NULL)
            return NULL;
        expr = create_binary_expr(expr, op, right);
    }
    return expr;
}

// Factor -> Unary (("/" | "*") Unary)*
static Expr *parse_factor(Parser *parser)
{
    Expr *expr = parse_unary(parser);
    if (expr == NULL)
        return NULL;

    while (parser_match(parser, TOKEN_SLASH) ||
           parser_match(parser, TOKEN_STAR))
    {
        TokenType op = parser_peek(parser).type;
        parser_advance(parser);
        Expr *right = parse_unary(parser);
        if (right == NULL)
            return NULL;
        expr = create_binary_expr(expr, op, right);
    }
    return expr;
}

// Unary -> ("!" | "-") Unary | Primary
static Expr *parse_unary(Parser *parser)
{
    if (parser_match(parser, TOKEN_BANG) ||
        parser_match(parser, TOKEN_MINUS))
    {
        TokenType op = parser_peek(parser).type;
        parser_advance(parser);
        Expr *right = parse_unary(parser);
        if (right == NULL)
            return NULL;
        return create_unary_expr(op, right);
    }
    return parse_primary(parser);
}

static Expr *parse_primary(Parser *parser)
{
    Expr *expr = malloc(sizeof(Expr));
    if (expr == NULL)
        return NULL;

    if (parser_match(parser, TOKEN_NUMBER))
    {
        Token token = parser_peek(parser);
        expr->tag = EXPR_NUMBER;
        expr->as.number.type = DOUBLE;
        char *lexeme = parse_lexeme(token);
        expr->as.number.v.dvalue = atof(lexeme);
        free(lexeme);
        parser_advance(parser);
        return expr;
    }

    if (parser_match(parser, TOKEN_STRING))
    {
        Token token = parser_peek(parser);
        expr->tag = EXPR_STRING;
        char *lexeme = parse_lexeme(token);
        expr->as.string.value = strdup(lexeme);
        free(lexeme);
        parser_advance(parser);
        return expr;
    }

    if (parser_match(parser, TOKEN_TRUE))
    {
        expr->tag = EXPR_TRUE;
        parser_advance(parser);
        return expr;
    }

    if (parser_match(parser, TOKEN_FALSE))
    {
        expr->tag = EXPR_FALSE;
        parser_advance(parser);
        return expr;
    }

    if (parser_match(parser, TOKEN_NIL))
    {
        expr->tag = EXPR_NIL;
        parser_advance(parser);
        return expr;
    }

    if (parser_match(parser, TOKEN_LEFT_PAREN))
    {
        parser_advance(parser);
        Expr *inside = parse_expression(parser);
        if (inside == NULL || parser_consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.") != PARSER_SUCCESS)
        {
            free(expr);
            return NULL;
        }
        expr->tag = EXPR_GROUPING;
        expr->as.group.expr = inside;
        return expr;
    }

    free(expr);
    parser->last_error = PARSER_ERROR_UNEXPECTED_TOKEN;
    return NULL;
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

    Parser *parser = parser_create(&tks);
    Expr *expr = parse_expression(parser);
    if (expr != NULL)
    {
#ifdef DEBUG_EXPR_TREE_S_FORMAT
        print_expr_as_s_expr(expr);
#endif

#ifdef DEBUG_EXPR_TREE
        print_expr(expr, 1);
#endif
        printf("\n");
        free(expr);
    }
    free(buffer);
    parser_destroy(parser);
}

void print_expr(Expr *expr, int indent)
{
    if (!expr)
    {
        printf("NULL\n");
        return;
    }

    print_indent(indent);

    switch (expr->tag)
    {
    case EXPR_BINARY:
        printf("Binary Expression:\n");
        print_indent(indent + 2);
        printf("Operator: %s\n", token_type_to_string(expr->as.binary.op));
        print_indent(indent + 2);
        printf("Left:\n");
        print_expr(expr->as.binary.left, indent + 4);
        print_indent(indent + 2);
        printf("Right:\n");
        print_expr(expr->as.binary.right, indent + 4);
        break;

    case EXPR_UNARY:
        printf("Unary Expression:\n");
        print_indent(indent + 2);
        printf("Operator: %s\n", token_type_to_string(expr->as.unary.op));
        print_indent(indent + 2);
        printf("Operand:\n");
        print_expr(expr->as.unary.expr, indent + 4);
        break;

    case EXPR_GROUPING:
        printf("Grouping Expression:\n");
        print_indent(indent + 2);
        printf("Expression:\n");
        print_expr(expr->as.group.expr, indent + 4);
        break;

    case EXPR_NUMBER:
        printf("Number: ");
        switch (expr->as.number.type)
        {
        case INT:
            printf("%d\n", expr->as.number.v.ivalue);
            break;
        case FLOAT:
            printf("%f\n", expr->as.number.v.fvalue);
            break;
        case DOUBLE:
            printf("%f\n", expr->as.number.v.dvalue);
            break;
        }
        break;

    case EXPR_STRING:
        printf("String: \"%s\"\n", expr->as.string.value);
        break;

    case EXPR_TRUE:
        printf("Boolean: true\n");
        break;

    case EXPR_FALSE:
        printf("Boolean: false\n");
        break;

    case EXPR_NIL:
        printf("nil\n");
        break;

    default:
        printf("Unknown expression type: %d\n", expr->tag);
        break;
    }
}

void print_expr_as_s_expr(Expr *expr)
{
    if (expr == NULL)
    {
        printf("nil");
        return;
    }

    switch (expr->tag)
    {
    case EXPR_BINARY:
        printf("(%s ", token_type_to_string(expr->as.binary.op));
        print_expr_as_s_expr(expr->as.binary.left);
        printf(" ");
        print_expr_as_s_expr(expr->as.binary.right);
        printf(")");
        break;

    case EXPR_UNARY:
        printf("(%s ", token_type_to_string(expr->as.unary.op));
        print_expr_as_s_expr(expr->as.unary.expr);
        printf(")");
        break;

    case EXPR_GROUPING:
        printf("(group ");
        print_expr_as_s_expr(expr->as.group.expr);
        printf(")");
        break;

    case EXPR_NUMBER:
        switch (expr->as.number.type)
        {
        case INT:
            printf("%d", expr->as.number.v.ivalue);
            break;
        case FLOAT:
            printf("%f", expr->as.number.v.fvalue);
            break;
        case DOUBLE:
            printf("%f", expr->as.number.v.dvalue);
            break;
        }
        break;

    case EXPR_STRING:
        printf("\"%s\"", expr->as.string.value);
        break;

    case EXPR_TRUE:
        printf("true");
        break;

    case EXPR_FALSE:
        printf("false");
        break;

    case EXPR_NIL:
        printf("nil");
        break;

    default:
        printf("unknown");
        break;
    }
}

static void print_indent(int indent)
{
    for (int i = 0; i < indent; i++)
    {
        putchar(' ');
    }
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

void test_simple_expression_tokens()
{
    printf("Testing simple expression tokens...\n");
    const char *test_input =
        "(( -5 + 8 ) * 9) + 1238.348 - 20;\n";

    write_test_file(test_input, "test_simple_expression.txt");
    generate_ast("test_simple_expression.txt");
    printf("Basic simple test completed\n\n");
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
    test_simple_expression_tokens();
    // test_basic_tokens();
    // test_numbers_and_strings();
    // test_operators();
    // test_comments();
    // test_complex_code();

    printf("All scanner tests completed.\n");
    return 0;
}