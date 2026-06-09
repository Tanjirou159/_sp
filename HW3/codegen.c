#include "tiny_compiler.h"
#include <stdarg.h>

typedef struct {
    Instruction* code;
    int size;
    int capacity;
} CodeBuffer;

typedef struct LoopPatch {
    int addr;
    struct LoopPatch* next;
} LoopPatch;

typedef struct {
    int continue_addr;
    LoopPatch* break_list;
} LoopContext;

CodeBuffer code_buffer;
int num_globals = 0;
int next_array_base = 0;

LoopContext loop_stack[128];
int loop_depth = 0;

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
    static char** global_names = NULL;
    static int num_names = 0;

    for (int i = 0; i < num_names; i++) {
        if (strcmp(global_names[i], name) == 0)
            return i;
    }

    global_names = realloc(global_names, (num_names + 1) * sizeof(char*));
    global_names[num_names] = strdup(name);
    num_globals++;
    return num_names++;
}

void push_loop(int continue_addr) {
    loop_stack[loop_depth].continue_addr = continue_addr;
    loop_stack[loop_depth].break_list = NULL;
    loop_depth++;
}

void pop_loop(int break_target) {
    loop_depth--;
    LoopPatch* p = loop_stack[loop_depth].break_list;
    while (p) {
        code_buffer.code[p->addr].operand = break_target;
        LoopPatch* next = p->next;
        free(p);
        p = next;
    }
}

void add_break_patch(int addr) {
    LoopPatch* p = (LoopPatch*)malloc(sizeof(LoopPatch));
    p->addr = addr;
    p->next = loop_stack[loop_depth - 1].break_list;
    loop_stack[loop_depth - 1].break_list = p;
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
            emit(OP_PUSH, (int)node->string.value);
            emit(OP_PRINT_STR, 0);
            break;

        case NODE_VAR:
            emit(OP_LOAD, get_global_offset(node->var.name));
            break;

        case NODE_ARRAY_ACCESS: {
            int base_offset = get_global_offset(node->array_access.name);
            emit(OP_LOAD, base_offset);
            generate_expr(node->array_access.index);
            emit(OP_ARRAY_LOAD, 0);
            break;
        }

        case NODE_BINARY:
            generate_expr(node->binary.left);
            generate_expr(node->binary.right);
            if (strcmp(node->binary.op, "+") == 0) emit(OP_ADD, 0);
            else if (strcmp(node->binary.op, "-") == 0) emit(OP_SUB, 0);
            else if (strcmp(node->binary.op, "*") == 0) emit(OP_MUL, 0);
            else if (strcmp(node->binary.op, "/") == 0) emit(OP_DIV, 0);
            else if (strcmp(node->binary.op, "%") == 0) emit(OP_MOD, 0);
            else if (strcmp(node->binary.op, "==") == 0) emit(OP_EQ, 0);
            else if (strcmp(node->binary.op, "!=") == 0) emit(OP_NE, 0);
            else if (strcmp(node->binary.op, "<") == 0) emit(OP_LT, 0);
            else if (strcmp(node->binary.op, "<=") == 0) emit(OP_LTE, 0);
            else if (strcmp(node->binary.op, ">") == 0) emit(OP_GT, 0);
            else if (strcmp(node->binary.op, ">=") == 0) emit(OP_GTE, 0);
            else if (strcmp(node->binary.op, "&&") == 0) emit(OP_AND, 0);
            else if (strcmp(node->binary.op, "||") == 0) emit(OP_OR, 0);
            break;

        case NODE_UNARY:
            generate_expr(node->unary.expr);
            if (strcmp(node->unary.op, "-") == 0) emit(OP_NEG, 0);
            else if (strcmp(node->unary.op, "!") == 0) emit(OP_NOT, 0);
            break;

        case NODE_CALL: {
            ASTNode* arg = node->call.args->next;
            while (arg) {
                generate_expr(arg);
                arg = arg->next;
            }
            emit(OP_CALL, (int)node->call.name);
            break;
        }

        default:
            break;
    }
}

void generate_statement(ASTNode* stmt) {
    if (!stmt) return;

    switch (stmt->type) {
        case NODE_VAR_DECL:
            get_global_offset(stmt->var_decl.name);
            if (stmt->var_decl.is_array) {
                int base = next_array_base;
                next_array_base += stmt->var_decl.array_size;
                emit(OP_PUSH, base);
                emit(OP_STORE, get_global_offset(stmt->var_decl.name));
            } else if (stmt->var_decl.init) {
                generate_expr(stmt->var_decl.init);
                emit(OP_STORE, get_global_offset(stmt->var_decl.name));
            }
            break;

        case NODE_ASSIGN:
            generate_expr(stmt->assign.value);
            emit(OP_STORE, get_global_offset(stmt->assign.name));
            break;

        case NODE_ARRAY_ASSIGN: {
            generate_expr(stmt->array_assign.value);
            emit(OP_LOAD, get_global_offset(stmt->array_assign.name));
            generate_expr(stmt->array_assign.index);
            emit(OP_ARRAY_STORE, 0);
            break;
        }

        case NODE_IF: {
            generate_expr(stmt->if_stmt.cond);
            int jmp_addr = code_buffer.size;
            emit(OP_JMP_IF, 0);

            ASTNode* then_stmt = stmt->if_stmt.then_branch->next;
            while (then_stmt) {
                generate_statement(then_stmt);
                then_stmt = then_stmt->next;
            }

            int else_jmp_addr = code_buffer.size;
            emit(OP_JMP, 0);

            code_buffer.code[jmp_addr].operand = code_buffer.size;

            if (stmt->if_stmt.else_branch) {
                ASTNode* else_stmt = stmt->if_stmt.else_branch->next;
                while (else_stmt) {
                    generate_statement(else_stmt);
                    else_stmt = else_stmt->next;
                }
            }

            code_buffer.code[else_jmp_addr].operand = code_buffer.size;
            break;
        }

        case NODE_WHILE: {
            int cond_check = code_buffer.size;
            generate_expr(stmt->while_stmt.cond);

            int jmp_exit = code_buffer.size;
            emit(OP_JMP_IF, 0);

            push_loop(cond_check);

            ASTNode* body_stmt = stmt->while_stmt.body->next;
            while (body_stmt) {
                generate_statement(body_stmt);
                body_stmt = body_stmt->next;
            }

            emit(OP_JMP, cond_check);

            code_buffer.code[jmp_exit].operand = code_buffer.size;
            pop_loop(code_buffer.size);
            break;
        }

        case NODE_FOR: {
            // Layout:
            //   init
            //   JMP cond_check
            // body_start:
            //   body
            // incr_target:
            //   incr; POP
            // cond_check:
            //   push cond
            //   JMP_IF exit     (jumps if cond==false)
            //   JMP body_start
            // exit:

            // 1. Init
            if (stmt->for_stmt.init) {
                if (stmt->for_stmt.init->type == NODE_VAR_DECL) {
                    generate_statement(stmt->for_stmt.init);
                } else {
                    generate_expr(stmt->for_stmt.init);
                    emit(OP_POP, 0);
                }
            }

            // 2. Jump over body+incr to condition check
            int skip_to_cond = code_buffer.size;
            emit(OP_JMP, 0);

            // 3. Body
            int body_start = code_buffer.size;

            // Push loop context with dummy continue_addr (patched after body)
            // continue_addr=0 is safe because body won't be empty with continue
            push_loop(0);
            int ctx_idx = loop_depth - 1;

            ASTNode* body_stmt = stmt->for_stmt.body->next;
            while (body_stmt) {
                generate_statement(body_stmt);
                body_stmt = body_stmt->next;
            }

            // 4. Increment target - patch the continue_addr NOW
            int incr_target = code_buffer.size;
            loop_stack[ctx_idx].continue_addr = incr_target;

            if (stmt->for_stmt.incr) {
                generate_expr(stmt->for_stmt.incr);
                emit(OP_POP, 0);
            }

            // 5. Condition check
            int cond_check = code_buffer.size;
            code_buffer.code[skip_to_cond].operand = cond_check;

            if (stmt->for_stmt.cond) {
                generate_expr(stmt->for_stmt.cond);
            } else {
                emit(OP_PUSH, 1);
            }

            int jmp_exit = code_buffer.size;
            emit(OP_JMP_IF, 0);

            emit(OP_JMP, body_start);

            // 6. Exit
            code_buffer.code[jmp_exit].operand = code_buffer.size;
            pop_loop(code_buffer.size);
            break;
        }

        case NODE_BREAK:
            if (loop_depth == 0) {
                printf("Error: break outside of loop\n");
                exit(1);
            }
            emit(OP_JMP, 0);
            add_break_patch(code_buffer.size - 1);
            break;

        case NODE_CONTINUE:
            if (loop_depth == 0) {
                printf("Error: continue outside of loop\n");
                exit(1);
            }
            emit(OP_JMP, loop_stack[loop_depth - 1].continue_addr);
            break;

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
    next_array_base = 0;
    loop_depth = 0;

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
