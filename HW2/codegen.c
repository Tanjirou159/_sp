#include "tiny_compiler.h"
#include <stdarg.h>

typedef struct {
    Instruction* code;
    int size;
    int capacity;
} CodeBuffer;

CodeBuffer code_buffer;
int num_globals = 0;
int* global_offsets;

void emit(OpCode op, int operand) {
    if (code_buffer.size >= code_buffer.capacity) {
        code_buffer.capacity = code_buffer.capacity * 2 + 16;
        code_buffer.code = realloc(code_buffer.code, code_buffer.capacity * sizeof(Instruction));
    }
    code_buffer.code[code_buffer.size].op = op;
    code_buffer.code[code_buffer.size].operand = operand;
    code_buffer.size++;
}

int get_global_offset(const char* name) {
    // Simple symbol table
    static char** global_names = NULL;
    static int num_names = 0;
    
    for (int i = 0; i < num_names; i++) {
        if (strcmp(global_names[i], name) == 0)
            return i;
    }
    
    // Add new global
    global_names = realloc(global_names, (num_names + 1) * sizeof(char*));
    global_names[num_names] = strdup(name);
    num_globals++;
    return num_names++;
}

void generate_expr(ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_NUMBER_LIT:
            emit(OP_PUSH, node->number.value);
            break;
            
        case NODE_BOOL_LIT:
            emit(OP_PUSH, node->boolean.value);
            break;
            
        case NODE_STRING_LIT:
            // For simplicity, treat string literals as addresses
            emit(OP_PUSH, (int)node->string.value);
            emit(OP_PRINT_STR, 0);
            break;
            
        case NODE_VAR:
            emit(OP_LOAD, get_global_offset(node->var.name));
            break;
            
        case NODE_BINARY:
            generate_expr(node->binary.left);
            generate_expr(node->binary.right);
            if (strcmp(node->binary.op, "+") == 0) emit(OP_ADD, 0);
            else if (strcmp(node->binary.op, "-") == 0) emit(OP_SUB, 0);
            else if (strcmp(node->binary.op, "*") == 0) emit(OP_MUL, 0);
            else if (strcmp(node->binary.op, "/") == 0) emit(OP_DIV, 0);
            else if (strcmp(node->binary.op, "==") == 0) emit(OP_EQ, 0);
            else if (strcmp(node->binary.op, "!=") == 0) emit(OP_NE, 0);
            else if (strcmp(node->binary.op, "<") == 0) emit(OP_LT, 0);
            else if (strcmp(node->binary.op, "<=") == 0) emit(OP_LTE, 0);
            else if (strcmp(node->binary.op, ">") == 0) emit(OP_GT, 0);
            else if (strcmp(node->binary.op, ">=") == 0) emit(OP_GTE, 0);
            break;
            
        case NODE_UNARY:
            generate_expr(node->unary.expr);
            if (strcmp(node->unary.op, "-") == 0) emit(OP_NEG, 0);
            else if (strcmp(node->unary.op, "!") == 0) emit(OP_NOT, 0);
            break;
            
        case NODE_CALL:
            // Simplified: just evaluate args and push
            {
                ASTNode* arg = node->call.args->next;
                while (arg) {
                    generate_expr(arg);
                    arg = arg->next;
                }
                emit(OP_CALL, (int)node->call.name);
            }
            break;
            
        default:
            break;
    }
}

void generate_statement(ASTNode* stmt) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case NODE_VAR_DECL:
            if (stmt->var_decl.init) {
                generate_expr(stmt->var_decl.init);
                emit(OP_STORE, get_global_offset(stmt->var_decl.name));
            }
            break;
            
        case NODE_ASSIGN:
            generate_expr(stmt->assign.value);
            emit(OP_STORE, get_global_offset(stmt->assign.name));
            break;
            
        case NODE_IF: {
            generate_expr(stmt->if_stmt.cond);
            int jmp_addr = code_buffer.size;
            emit(OP_JMP_IF, 0);  // Jump if false
            // Generate then branch
            ASTNode* then_stmt = stmt->if_stmt.then_branch->next;
            while (then_stmt) {
                generate_statement(then_stmt);
                then_stmt = then_stmt->next;
            }
            int else_addr = code_buffer.size;
            emit(OP_JMP, 0);  // Jump over else
            
            // Patch JMP_IF to skip then branch
            code_buffer.code[jmp_addr].operand = code_buffer.size;
            
            // Generate else branch
            if (stmt->if_stmt.else_branch) {
                ASTNode* else_stmt = stmt->if_stmt.else_branch->next;
                while (else_stmt) {
                    generate_statement(else_stmt);
                    else_stmt = else_stmt->next;
                }
            }
            code_buffer.code[else_addr].operand = code_buffer.size;
            break;
        }
        
        case NODE_WHILE: {
            int loop_start = code_buffer.size;
            generate_expr(stmt->while_stmt.cond);
            int jmp_addr = code_buffer.size;
            emit(OP_JMP_IF, 0);
            
            ASTNode* body_stmt = stmt->while_stmt.body->next;
            while (body_stmt) {
                generate_statement(body_stmt);
                body_stmt = body_stmt->next;
            }
            
            emit(OP_JMP, loop_start);
            code_buffer.code[jmp_addr].operand = code_buffer.size;
            break;
        }
        
        case NODE_RETURN:
            if (stmt->return_stmt.value)
                generate_expr(stmt->return_stmt.value);
            emit(OP_RET, 0);
            break;
            
        case NODE_PRINT:
            if (stmt->print_stmt.is_string_lit) {
                emit(OP_PRINT_STR, (int)stmt->print_stmt.str_value);
            } else {
                generate_expr(stmt->print_stmt.expr);
                emit(OP_PRINT, 0);
            }
            break;
            
        default:
            break;
    }
}

void generate_program(ASTNode* prog) {
    code_buffer.code = malloc(1024 * sizeof(Instruction));
    code_buffer.capacity = 1024;
    code_buffer.size = 0;
    
    ASTNode* func = prog->next;
    while (func) {
        if (func->type == NODE_FUNCTION) {
            if (strcmp(func->func.name, "main") == 0) {
                ASTNode* stmt = func->func.body->next;
                while (stmt) {
                    generate_statement(stmt);
                    stmt = stmt->next;
                }
            }
        }
        func = func->next;
    }
    
    emit(OP_HALT, 0);
}