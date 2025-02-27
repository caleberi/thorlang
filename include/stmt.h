#ifndef __THOR_STMT_H__
#define __THOR_STMT_H__

#include "expr.h"
#include "scanner.h"
#include "common.h"

typedef enum StmtType
{
  STMT_WHILE,
  STMT_FOR,
  STMT_FOREACH,
  STMT_IF,
  STMT_CLASS,
  STMT_FUNCTION,
  STMT_PRINT,
  STMT_BLOCK,
  STMT_VAR_DECL,
  STMT_DEBUG,
} StmtType;

typedef struct Stmt Stmt;

#define DEFINE_STMT_TYPES(V) \
  V(While,                   \
    Expr *condition;         \
    Stmt * body;)            \
  V(For,                     \
    Stmt *initializer;       \
    Expr * condition;        \
    Expr * increment;        \
    Stmt * body;)            \
  V(ForEach,                 \
    Token iterator;          \
    Expr * collection;       \
    Stmt * body;)            \
  V(If,                      \
    Expr *condition;         \
    Stmt * thenBranch;       \
    Stmt * elseBranch;)      \
  V(Class,                   \
    Token name;              \
    Stmt * *methods;         \
    int methodCount;)        \
  V(Function,                \
    Token name;              \
    Token * params;          \
    int paramCount;          \
    Stmt * *body;            \
    int bodyCount;)          \
  V(Print,                   \
    Expr *expression;)       \
  V(Debug,                   \
    Expr *expression;        \
    int line;                \
    char *filename;)         \
  V(Block,                   \
    Stmt **statements;       \
    int count;)              \
  V(VarDecl,                 \
    Token name;              \
    Expr * initializer;)

#define STMT_STRUCT_FORWARD_DECL(name, ...) \
  typedef struct name##Stmt name##Stmt;

DEFINE_STMT_TYPES(STMT_STRUCT_FORWARD_DECL)

#define STMT_STRUCT_DEF(name, fields) \
  struct name##Stmt                   \
  {                                   \
    fields                            \
  };

DEFINE_STMT_TYPES(STMT_STRUCT_DEF)

struct Stmt
{
  StmtType tag;
  union
  {
    WhileStmt whileStmt;
    ForStmt forStmt;
    ForEachStmt forEachStmt;
    IfStmt ifStmt;
    ClassStmt classStmt;
    FunctionStmt functionStmt;
    PrintStmt printStmt;
    BlockStmt blockStmt;
    VarDeclStmt varDeclStmt;
    DebugStmt debugStatement;
  } as;
};

#endif // __THOR_STMT_H__