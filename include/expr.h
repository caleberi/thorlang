#ifndef __THOR_EXPR_H__
#define __THOR_EXPR_H__
#include "scanner.h"
#include "common.h"

typedef enum ExprType
{
  EXPR_EQUALITY,
  EXPR_COMPARISON,
  EXPR_TERM,
  EXPR_FACTOR,
  EXPR_UNARY,
  EXPR_NUMBER,
  EXPR_STRING,
  EXPR_CLASS,
  EXPR_CALL,
  EXPR_TRUE,
  EXPR_FALSE,
  EXPR_NIL,
  EXPR_BINARY,
  EXPR_GROUPING,
  EXPR_ASSIGN,
  EXPR_VARIABLE,
} ExprType;

typedef enum NumberType
{
  DOUBLE,
  INT,
  FLOAT
} NumberType;

#define DEFINE_EXPR_TYPES(V)               \
  V(Binary,                                \
    Expr *left;                            \
    Expr * right;                          \
    TokenType op;)                         \
  V(Unary,                                 \
    Expr *expr;                            \
    TokenType op;)                         \
  V(Grouping,                              \
    Expr *expr;)                           \
  V(Number, NumberType type; union { \
            int ivalue; \
            float fvalue; \
            double dvalue; } v;) \
  V(String,                                \
    char *value;)                          \
  V(Var, char *name;)                      \
  V(Assign,                                \
    char *name;                            \
    Expr * value;)

#define EXPR_STRUCT_FORWARD_DECL(name, ...) \
  typedef struct name##Expr name##Expr;

DEFINE_EXPR_TYPES(EXPR_STRUCT_FORWARD_DECL)

typedef struct Expr Expr;
#define EXPR_STRUCT_DEF(name, fields) \
  struct name##Expr                   \
  {                                   \
    fields                            \
  };
DEFINE_EXPR_TYPES(EXPR_STRUCT_DEF)

typedef struct Expr
{
  ExprType tag;
  union
  {
    BinaryExpr binary;
    UnaryExpr unary;
    GroupingExpr group;
    NumberExpr number;
    StringExpr string;
    VarExpr variable;
    AssignExpr assign;
  } as;
} Expr;

#endif // __THOR_EXPR_H__