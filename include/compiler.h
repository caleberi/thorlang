#ifndef _THOR_COMPILER_H_
#define _THOR_COMPILER_H_
#include "chunk.h"
#include "scanner.h"
#include "debug.h"

typedef struct
{
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
} Parser;

bool compile(const char *source, Chunk *chunk);
void boostrap_keyword();
void cleanup_keyword();

static void binary();
static void unary();
static void grouping();
static void expression();
static void number();
static void ternary();

typedef enum
{
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_CONDITIONAL, // ?:
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)();
typedef struct ParseRule
{
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

static ParseRule *get_rule(TokenType);

#endif // _THOR_COMPILE_H_
