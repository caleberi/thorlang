#ifndef _THOR_SCANNER_H_
#define _THOR_SCANNER_H_
#include "trie.h"
typedef enum
{
    // Single character tokens
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_MINUS,
    TOKEN_PLUS,
    TOKEN_SEMICOLON,
    TOKEN_SLASH,
    TOKEN_STAR,

    // One or more character tokens
    TOKEN_BANG,
    TOKEN_BANG_EQUAL,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,

    TOKEN_QUESTION,
    TOKEN_COLON,

    // Literals
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,

    // Keywords
    TOKEN_AND,
    TOKEN_CLASS,
    TOKEN_ELSE,
    TOKEN_FALSE,
    TOKEN_FOR,
    TOKEN_FUN,
    TOKEN_IF,
    TOKEN_NIL,
    TOKEN_OR,
    TOKEN_PRINT,
    TOKEN_RETURN,
    TOKEN_SUPER,
    TOKEN_THIS,
    TOKEN_TRUE,
    TOKEN_VAR,
    TOKEN_WHILE,

    TOKEN_ERROR,
    TOKEN_EOF
} TokenType;

typedef struct
{
    TokenType type;
    const char *start;
    int length;
    int line;
} Token;

typedef struct
{
    const char *start;
    const char *current;
    int line;
} Scanner;

void init_scanner(const char *);
Token scan_token();

// This is crappy
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

#endif // _THOR_SCANNER_H_
