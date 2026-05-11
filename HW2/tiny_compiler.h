#ifndef TINY_COMPILER_H
#define TINY_COMPILER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

// Token types
typedef enum {
    TOKEN_NUMBER, TOKEN_IDENT, TOKEN_STRING,
    TOKEN_INT, TOKEN_BOOL, TOKEN_STRING_TYPE,
    TOKEN_VAR, TOKEN_FUNC, TOKEN_IF, TOKEN_ELSE, TOKEN_WHILE,
    TOKEN_RETURN, TOKEN_PRINT,
    TOKEN_TRUE, TOKEN_FALSE,
    TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_LBRACE, TOKEN_RBRACE,
    TOKEN_SEMICOLON, TOKEN_COMMA, TOKEN_COLON,
    TOKEN_ASSIGN, TOKEN_EQUAL, TOKEN_NOT_EQUAL,
    TOKEN_LT, TOKEN_LTE, TOKEN_GT, TOKEN_GTE,
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_MUL, TOKEN_DIV, TOKEN_NOT,
    TOKEN_EOF, TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char* lexeme;
    int line;
    union {
        int int_value;
        char* str_value;
    };
} Token;

// AST node types
typedef enum {
    NODE_PROGRAM, NODE_FUNCTION, NODE_VAR_DECL, NODE_ASSIGN,
    NODE_IF, NODE_WHILE, NODE_RETURN, NODE_PRINT,
    NODE_BINARY, NODE_UNARY, NODE_CALL, NODE_VAR, NODE_NUMBER_LIT,
    NODE_BOOL_LIT, NODE_STRING_LIT
} NodeType;

typedef struct ASTNode {
    NodeType type;
    struct ASTNode* next;  // For lists
    union {
        struct {
            char* name;
            struct ASTNode* params;
            struct ASTNode* body;
            char* ret_type;
        } func;
        struct {
            char* name;
            char* var_type;
            struct ASTNode* init;
        } var_decl;
        struct {
            char* name;
            struct ASTNode* value;
        } assign;
        struct {
            struct ASTNode* cond;
            struct ASTNode* then_branch;
            struct ASTNode* else_branch;
        } if_stmt;
        struct {
            struct ASTNode* cond;
            struct ASTNode* body;
        } while_stmt;
        struct {
            struct ASTNode* value;
        } return_stmt;
        struct {
            struct ASTNode* expr;
            int is_string_lit;
            char* str_value;
        } print_stmt;
        struct {
            char* op;
            struct ASTNode* left;
            struct ASTNode* right;
        } binary;
        struct {
            char* op;
            struct ASTNode* expr;
        } unary;
        struct {
            char* name;
            struct ASTNode* args;
        } call;
        struct {
            char* name;
        } var;
        struct {
            int value;
        } number;
        struct {
            int value;
        } boolean;
        struct {
            char* value;
        } string;
    };
} ASTNode;

// Bytecode definitions
typedef enum {
    OP_PUSH, OP_POP, OP_LOAD, OP_STORE,
    OP_ADD, OP_SUB, OP_MUL, OP_DIV,
    OP_EQ, OP_NE, OP_LT, OP_LTE, OP_GT, OP_GTE,
    OP_AND, OP_OR, OP_NOT, OP_NEG,
    OP_JMP, OP_JMP_IF, OP_CALL, OP_RET,
    OP_PRINT, OP_PRINT_STR, OP_HALT
} OpCode;

typedef struct {
    OpCode op;
    int operand;
} Instruction;

// VM state
typedef struct {
    int* stack;
    int sp;
    int fp;
    int* globals;
    Instruction* code;
    int ip;
    int code_size;
    int stack_size;
    int num_globals;
} VM;

// Compiler functions
void compile(const char* source);
void run_vm(Instruction* code, int code_size, int num_globals);
void print_bytecode(Instruction* code, int size);

#endif
