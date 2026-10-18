#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "utils.h"

typedef struct Arena Arena;
typedef struct Token Token;
typedef struct Node Node;
typedef struct DataType DataType;

typedef struct BinOpNode BinOpNode;
typedef struct UnaryNode UnaryNode;
typedef struct LiteralNode LiteralNode;
typedef struct IdentifierNode IdentifierNode;
typedef struct BlockNode BlockNode;
typedef struct IfNode IfNode;
typedef struct WhileNode WhileNode;
typedef struct ForNode ForNode;
typedef struct ReturnNode ReturnNode;
typedef struct DeferNode DeferNode;
typedef struct ExprStmtNode ExprStmtNode;
typedef struct VarDeclNode VarDeclNode;
typedef struct FuncDeclNode FuncDeclNode;
typedef struct StructDeclNode StructDeclNode;
typedef struct EnumDeclNode EnumDeclNode;
typedef struct TypeDeclNode TypeDeclNode;
typedef struct ImportNode ImportNode;
typedef struct CallNode CallNode;
typedef struct MemberAccessNode MemberAccessNode;
typedef struct IndexNode IndexNode;
typedef struct StructInitNode StructInitNode;
typedef struct ArrayInitNode ArrayInitNode;
typedef struct SwitchNode SwitchNode;
typedef struct CaseNode CaseNode;

typedef enum {
    // SYMBOLS
    TOKEN_PLUS, TOKEN_MINUS,
    TOKEN_STAR, TOKEN_SLASH,
    TOKEN_EQ,
    TOKEN_PAREN_LEFT, TOKEN_PAREN_RIGHT,
    TOKEN_BRACE_LEFT, TOKEN_BRACE_RIGHT,
    TOKEN_CURLY_LEFT, TOKEN_CURLY_RIGHT,
    TOKEN_BANG, TOKEN_AT,
    TOKEN_HASH, TOKEN_DOLLAR,
    TOKEN_PERCENT,
    TOKEN_QUOTE, TOKEN_DOUBLE_QUOTE,
    TOKEN_QUESTION,
    TOKEN_COMMA, TOKEN_SEMI, TOKEN_COLON,
    TOKEN_DOT,

    // OPERATORS
    TOKEN_PLUS_PLUS, TOKEN_PLUS_EQ,
    TOKEN_MINUS_MINUS, TOKEN_MINUS_EQ,
    TOKEN_MULTI_EQ, TOKEN_DIV_EQ,
    TOKEN_EQ_EQ, TOKEN_NOT_EQ,
    TOKEN_LT, TOKEN_LT_EQ,
    TOKEN_GT, TOKEN_GT_EQ,
    TOKEN_POW, TOKEN_XOR,
    TOKEN_LOGICAL_AND, TOKEN_LOGICAL_OR,
    TOKEN_BIT_OR, TOKEN_BIT_OR_EQ,
    TOKEN_BIT_AND, TOKEN_BIT_AND_EQ,
    TOKEN_SHIFT_LEFT, TOKEN_SHIFT_LEFT_EQ,
    TOKEN_SHIFT_RIGHT, TOKEN_SHIFT_RIGHT_EQ,
    TOKEN_BIT_NOT, TOKEN_XOR_EQ,

    // KEYWORDS
    TOKEN_WHILE, TOKEN_FOR,
    TOKEN_BREAK, TOKEN_RETURN,
    TOKEN_CONTINUE,
    TOKEN_IF, TOKEN_ELSE,
    TOKEN_IMPORT, TOKEN_STATIC,
    TOKEN_DEFER, TOKEN_COMPTIME,
    TOKEN_STRUCT, TOKEN_TYPE,
    TOKEN_CONST, TOKEN_ENUM,
    TOKEN_TRUE, TOKEN_FALSE,
    TOKEN_SWITCH, TOKEN_CASE,
    TOKEN_DEFAULT,

    // TYPES
    TOKEN_CHAR,
    TOKEN_INT, TOKEN_FLOAT,
    TOKEN_BOOL,
    TOKEN_VOID, TOKEN_NULL,

    // LITERALS
    TOKEN_LIT_STR, TOKEN_LIT_CHAR,
    TOKEN_LIT_INT, TOKEN_LIT_FLOAT,
    TOKEN_IDENT,

    // OTHER
    TOKEN_EOF,
    TOKEN_UNKNOWN,
    TOKEN_COUNT,
} TokenType;

typedef enum {
    TYPE_VOID, TYPE_NULL,
    TYPE_INT, TYPE_FLOAT,
    TYPE_CHAR,
    TYPE_BOOL, TYPE_CUSTOM,
} TypeKind;

struct DataType {
    TypeKind kind;
    StringStream name;
    int pointer_depth;
};

struct Token {
    TokenType type;
    StringStream view;
    size_t line;
    size_t col;
};

// NODES
typedef enum {
    // DECLARATIONS
    NODE_VAR_DECL,
    NODE_FUNC_DECL,
    NODE_STRUCT_DECL,
    NODE_ENUM_DECL,
    NODE_TYPE_DECL,

    // STATEMENTS
    NODE_BLOCK,
    NODE_IF,
    NODE_WHILE, NODE_FOR,
    NODE_RETURN, NODE_BREAK,
    NODE_CONTINUE,
    NODE_DEFER,
    NODE_EXPR_STMT,
    NODE_IMPORT,
    NODE_SWITCH,
    NODE_CASE,

    // EXPRESSIONS
    NODE_LITERAL,
    NODE_IDENTIFIER,
    NODE_BINOP,
    NODE_UNARY,
    NODE_CALL,
    NODE_MEMBER_ACCESS,
    NODE_INDEX,
    NODE_STRUCT_INIT,
    NODE_ARRAY_INIT,

    // OTHER
    NODE_COUNT
} NodeKind;

struct BinOpNode {
    Node *left;
    Node *right;
    TokenType op;
};

struct UnaryNode {
    Node *operand;
    TokenType op;
};

struct LiteralNode {
    Token token;
};

struct IdentifierNode {
    Token name;
};

struct BlockNode {
    Node **statements;
    size_t count;
};

struct IfNode {
    Node *condition;
    Node *then_branch;
    Node *else_branch;
};

struct WhileNode {
    Node *condition;
    Node *body;
};

struct ForNode {
    Node *init;
    Node *condition;
    Node *increment;
    Node *body;
};

struct ReturnNode {
    Node *expr;
};

struct DeferNode {
    Node *stmt;
};

struct ExprStmtNode {
    Node *expr;
};

struct VarDeclNode {
    Token name;
    DataType type;
    Node *initializer; // Can be NULL

    bool is_static;
    bool is_const;
};

struct FuncDeclNode {
    Token name;
    size_t param_count;
    DataType return_type;
    Node *params; // Linked list
    Node *body;

    bool is_static;
    bool is_const;
};

struct StructDeclNode {
    Token name;
    Node *fields; // Linked list
    size_t field_count;
};

struct EnumDeclNode {
    Token name;
    Node *variants; // Linked list
    size_t variant_count;
};

struct TypeDeclNode {
    Token alias_name;
    DataType target_type;
};

struct ImportNode {
    StringStream path;
};

struct CallNode {
    Node *callee;
    Node **args;
    size_t arg_count;
};

struct MemberAccessNode {
    Node *object;
    Token property;
};

struct IndexNode {
    Node *array;
    Node *index;
};

struct StructInitNode {
    Token type_name;
    Token *field_names;
    Node **field_values;
    size_t field_count;
};

struct ArrayInitNode {
    Node **elements;
    size_t element_count;
};

struct SwitchNode {
    Node *expr;
    Node *cases; // Linked list of NODE_CASE
    size_t case_count;
};

struct CaseNode {
    Node *value; // set to NULL in case of `default`
    Node *body;
};

struct Node {
    NodeKind kind;
    size_t line;
    size_t col;
    bool _export;
    const char *path;

    union {
        BinOpNode binop;
        UnaryNode unary;
        LiteralNode literal;
        IdentifierNode identifier;
        BlockNode block;
        IfNode if_stmt;
        WhileNode while_stmt;
        ForNode for_stmt;
        ReturnNode return_stmt;
        DeferNode defer_stmt;
        ExprStmtNode expr_stmt;
        VarDeclNode var_decl;
        FuncDeclNode func_decl;
        StructDeclNode struct_decl;
        EnumDeclNode enum_decl;
        TypeDeclNode type_decl;
        ImportNode import_stmt;
        CallNode call;
        MemberAccessNode member_access;
        IndexNode index;
        StructInitNode struct_init;
        ArrayInitNode array_init;
        SwitchNode switch_stmt;
        CaseNode case_stmt;
    } as;
    bool _comptime;
    Node *next;
};

Node *node_new(Arena *a, NodeKind kind, size_t line, size_t col, const char *path);
