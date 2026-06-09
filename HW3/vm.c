#include "tiny_compiler.h"

VM vm;

void init_vm(int stack_size, int num_globals, int num_array_cells) {
    vm.stack = malloc(stack_size * sizeof(int));
    vm.sp = -1;
    vm.fp = 0;
    vm.globals = calloc(num_globals, sizeof(int));
    vm.arrays_pool = calloc(num_array_cells, sizeof(int));
    vm.num_array_cells = num_array_cells;
    vm.ip = 0;
    vm.stack_size = stack_size;
    vm.num_globals = num_globals;
}

void push(int value) {
    if (vm.sp + 1 >= vm.stack_size) {
        printf("Stack overflow\n");
        exit(1);
    }
    vm.stack[++vm.sp] = value;
}

int pop() {
    if (vm.sp < 0) {
        printf("Stack underflow\n");
        exit(1);
    }
    return vm.stack[vm.sp--];
}

void run_vm(Instruction* code, int code_size, int num_globals, int num_array_cells) {
    init_vm(4096, num_globals, num_array_cells);
    vm.code = code;

    while (vm.ip < code_size) {
        Instruction instr = code[vm.ip++];

        switch (instr.op) {
            case OP_PUSH:
                push(instr.operand);
                break;
            case OP_POP:
                pop();
                break;
            case OP_LOAD:
                push(vm.globals[instr.operand]);
                break;
            case OP_STORE:
                vm.globals[instr.operand] = pop();
                break;
            case OP_ADD:
                push(pop() + pop());
                break;
            case OP_SUB: {
                int b = pop();
                int a = pop();
                push(a - b);
                break;
            }
            case OP_MUL:
                push(pop() * pop());
                break;
            case OP_DIV: {
                int b = pop();
                int a = pop();
                push(b != 0 ? a / b : 0);
                break;
            }
            case OP_MOD: {
                int b = pop();
                int a = pop();
                push(b != 0 ? a % b : 0);
                break;
            }
            case OP_EQ:
                push(pop() == pop());
                break;
            case OP_NE:
                push(pop() != pop());
                break;
            case OP_LT: {
                int b = pop();
                int a = pop();
                push(a < b);
                break;
            }
            case OP_LTE: {
                int b = pop();
                int a = pop();
                push(a <= b);
                break;
            }
            case OP_GT: {
                int b = pop();
                int a = pop();
                push(a > b);
                break;
            }
            case OP_GTE: {
                int b = pop();
                int a = pop();
                push(a >= b);
                break;
            }
            case OP_AND: {
                int b = pop();
                int a = pop();
                push(a && b);
                break;
            }
            case OP_OR: {
                int b = pop();
                int a = pop();
                push(a || b);
                break;
            }
            case OP_NOT:
                push(!pop());
                break;
            case OP_NEG:
                push(-pop());
                break;
            case OP_JMP:
                vm.ip = instr.operand;
                break;
            case OP_JMP_IF:
                if (!pop())
                    vm.ip = instr.operand;
                break;
            case OP_ARRAY_LOAD: {
                int index = pop();
                int base = pop();
                if (base + index < vm.num_array_cells) {
                    push(vm.arrays_pool[base + index]);
                } else {
                    printf("Array index out of bounds: base=%d, index=%d\n", base, index);
                    push(0);
                }
                break;
            }
            case OP_ARRAY_STORE: {
                int index = pop();
                int base = pop();
                int value = pop();
                if (base + index < vm.num_array_cells) {
                    vm.arrays_pool[base + index] = value;
                } else {
                    printf("Array index out of bounds: base=%d, index=%d\n", base, index);
                }
                break;
            }
            case OP_PRINT:
                printf("%d", pop());
                break;
            case OP_PRINT_STR:
                printf("%s", (char*)instr.operand);
                break;
            case OP_HALT:
                printf("\n");
                free(vm.stack);
                free(vm.globals);
                free(vm.arrays_pool);
                return;
            default:
                printf("Unknown opcode %d\n", instr.op);
                return;
        }
    }
}
