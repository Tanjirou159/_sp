# Extended TinyLang Compiler Documentation

## Table of Contents
1. [Introduction](#introduction)
2. [What's New in HW3](#whats-new-in-hw3)
3. [Getting Started](#getting-started)
4. [Language Reference](#language-reference)
5. [Compiler Architecture](#compiler-architecture)
6. [Virtual Machine](#virtual-machine)
7. [Examples](#examples)
8. [Grammar (EBNF)](#grammar-ebnf)
9. [HW2 to HW3: Extension Details](#hw2-to-hw3-extension-details)

---

## Introduction

TinyLang is a statically-typed, compiled programming language built for educational purposes. The HW3 version extends the HW2 compiler with **arrays**, **for loops**, **break/continue**, **logical AND/OR**, **modulo operator**, **comments**, and **else-if** support.

### Key Features
- **Static typing** (`int`, `bool`, `string`)
- **Arrays** - fixed-size arrays with bounds checking
- **For loops** with init/cond/incr
- **Break & continue** for loop control
- **Logical operators** (`&&`, `||`, `!`)
- **Modulo operator** (`%`)
- **Else-if** chains
- **Comments** (`//` line, `/* */` block)
- **Stack-based VM** with array memory pool
- **Portable bytecode** - runs on any TinyLang VM

---

## What's New in HW3

This table summarizes the extensions added on top of the HW2 baseline:

| Feature | HW2 | HW3 |
|---------|-----|-----|
| Variables (`var`) | Yes | Yes |
| If/Else | Yes | Yes |
| **Else-if chains** | No | **Yes** |
| While loops | Yes | Yes |
| **For loops** | No | **Yes** |
| **Break** | No | **Yes** |
| **Continue** | No | **Yes** |
| Print | Yes | Yes |
| **Arrays** | No | **Yes** |
| Arithmetic (`+`, `-`, `*`, `/`) | Yes | Yes |
| **Modulo (`%`)** | No | **Yes** |
| Comparison (`==`, `!=`, `<`, `<=`, `>`, `>=`) | Yes | Yes |
| **Logical AND (`&&`)** | No | **Yes** |
| **Logical OR (`||`)** | No | **Yes** |
| Logical NOT (`!`) | Yes | Yes |
| Negation (`-`) | Yes | Yes |
| **Comments** | No | **Yes** |

### New Token Types
`TOKEN_LBRACKET`, `TOKEN_RBRACKET`, `TOKEN_FOR`, `TOKEN_BREAK`, `TOKEN_CONTINUE`, `TOKEN_AND`, `TOKEN_OR`, `TOKEN_MOD`

### New AST Nodes
`NODE_ARRAY_ACCESS`, `NODE_ARRAY_ASSIGN`, `NODE_FOR`, `NODE_BREAK`, `NODE_CONTINUE`

### New VM Opcodes
`OP_MOD`, `OP_AND`, `OP_OR`, `OP_ARRAY_LOAD`, `OP_ARRAY_STORE`

---

## Getting Started

### Compilation

```bash
gcc -o tinylang *.c -std=c99 -Wall
```

### Running

```bash
./tinylang
```

The program runs all 10 built-in test cases automatically, demonstrating each new feature.

---

## Language Reference

### Data Types

| Type | Description | Example |
|------|-------------|---------|
| `int` | 32-bit signed integer | `42`, `-17` |
| `bool` | Boolean value | `true`, `false` |
| `string` | Text literal (print only) | `"Hello"` |
| `int[N]` | Fixed-size integer array | `int[10]` |

### Variables

```tinylang
var x: int = 10;
var flag: bool = true;
var arr: int[5];         // Array of 5 integers
arr[0] = 42;             // Array element assignment
var y: int = arr[0];     // Read from array
```

### Arrays

Arrays are fixed-size, zero-initialized integer containers. Elements are accessed
with `name[index]` syntax, using zero-based indexing. Bounds checking is
performed at runtime.

```tinylang
var scores: int[3];
scores[0] = 95;
scores[1] = 87;
scores[2] = scores[0] + scores[1];  // 182
```

**Array memory layout**: Arrays are stored in a contiguous memory pool separate
from scalar globals. Each array variable holds the base offset into this pool.

### For Loops

For loops have three clauses (init, condition, increment), all optional:

```tinylang
for (var i: int = 0; i < 10; i = i + 1) {
    print(i);
    print("\n");
}
```

**Empty condition** creates an infinite loop:
```tinylang
for (;;) { break; }
```

**Implementation detail**: For loops are desugared in the code generator:

```
Pseudo-code transformation:
  for (INIT; COND; INCR) { BODY }
becomes:
  INIT
  JMP cond_check
body_start:
  BODY
incr_target:            <-- continue jumps here
  INCR; POP
cond_check:
  push COND
  JMP_IF exit
  JMP body_start
exit:
```

### Break and Continue

- **`break`** - immediately exits the innermost enclosing loop
- **`continue`** - skips the rest of the current iteration and continues
  - In `while` loops: jumps to the condition check
  - In `for` loops: jumps to the increment clause before condition check

```tinylang
var i: int = 0;
while (i < 10) {
    i = i + 1;
    if (i == 3) { continue; }   // Skip 3
    if (i == 7) { break; }      // Stop at 7
    print(i);
}
// Output: 1 2 4 5 6
```

### Logical Operators

| Operator | Description | Example | Result |
|----------|-------------|---------|--------|
| `&&` | Logical AND | `a && b` | 1 if both non-zero |
| `\|\|` | Logical OR | `a \|\| b` | 1 if either non-zero |
| `!` | Logical NOT | `!a` | 1 if a is 0 |

```tinylang
if (x > 0 && y > 0) {
    print("Both positive\n");
}
if (a == 0 || b == 0) {
    print("At least one is zero\n");
}
```

**Operator precedence** (lowest to highest):
1. `||` (logical or)
2. `&&` (logical and)
3. `==`, `!=` (equality)
4. `<`, `<=`, `>`, `>=` (comparison)
5. `+`, `-` (term)
6. `*`, `/`, `%` (factor)
7. `!`, `-` (unary)

### Modulo

```tinylang
var rem: int = 17 % 5;  // 2
```

Zero divisor yields 0 (same as division safety):

```tinylang
var x: int = 10 % 0;    // 0 (safe)
```

### Comments

```tinylang
// Single-line comment

/*
   Multi-line
   comment
*/
```

### Else-If Chains

```tinylang
if (score >= 90) {
    print("A\n");
} else if (score >= 80) {
    print("B\n");
} else if (score >= 70) {
    print("C\n");
} else {
    print("F\n");
}
```

---

## Compiler Architecture

```
Source Code -> Lexer -> Tokens -> Parser -> AST -> Code Generator -> Bytecode -> VM
```

### 1. Lexer (`lexer.c`)
- Tokenizes source into tokens
- Added: `[`, `]`, `%`, `&&`, `||`, `for`, `break`, `continue`
- Added: comment skipping (`//` and `/* */`)

### 2. Parser (`parser.c`)
- Recursive descent parser
- Added precedence levels for `||` and `&&` in expression parsing
- Added: array declaration, array access, array assignment
- Added: for-loop parsing with init/cond/incr clauses
- Added: break/continue statements
- Added: else-if handling

### 3. Code Generator (`codegen.c`)
- Traverses AST, emits bytecode
- **Loop context stack** for break/continue:
  - `push_loop(continue_addr)` on entering loop
  - `pop_loop(break_target)` on exiting loop
  - `add_break_patch(addr)` for unresolved break jumps
- **For-loop desugaring** into while-equivalent code
- **Array allocation**: `next_array_base` counter tracks pool offsets

### 4. Virtual Machine (`vm.c`)
- Stack-based execution engine
- **Arrays pool**: separate memory region for array elements (1024 cells max)
- Added: `OP_MOD`, `OP_AND`, `OP_OR`, `OP_ARRAY_LOAD`, `OP_ARRAY_STORE`
- Bounds checking on array access

---

## Virtual Machine

### Memory Layout

```
+----------------+
|   Stack        |  (4096 cells)
|     ...        |
+----------------+
|   Globals      |  (num_globals cells)
+----------------+
|   Arrays Pool  |  (num_array_cells cells)
+----------------+
```

### Stack Effects for New Opcodes

| Instruction | Stack Before | Stack After | Description |
|-------------|-------------|-------------|-------------|
| `OP_MOD` | `a, b` | `a%b` | Modulo (safe for b=0) |
| `OP_AND` | `a, b` | `a&&b` | Logical AND |
| `OP_OR` | `a, b` | `a\|\|b` | Logical OR |
| `OP_ARRAY_LOAD` | `base, index` | `arrays[base+index]` | Array read |
| `OP_ARRAY_STORE` | `value, base, index` | -- | Array write |

---

## Examples

### Array Summation

```tinylang
func main() {
    var nums: int[5];
    nums[0] = 10;
    nums[1] = 20;
    nums[2] = 30;
    nums[3] = 40;
    nums[4] = 50;

    var sum: int = 0;
    for (var i: int = 0; i < 5; i = i + 1) {
        sum = sum + nums[i];
    }
    print("Sum: ");
    print(sum);
    print("\n");
}
// Output: Sum: 150
```

### Prime Checker with Break

```tinylang
func main() {
    var n: int = 29;
    var is_prime: bool = true;

    for (var i: int = 2; i * i <= n; i = i + 1) {
        if (n % i == 0) {
            is_prime = false;
            break;
        }
    }

    print(n);
    if (is_prime) {
        print(" is prime\n");
    } else {
        print(" is not prime\n");
    }
}
```

### FizzBuzz with Continue and Modulo

```tinylang
func main() {
    for (var i: int = 1; i <= 15; i = i + 1) {
        if (i % 3 == 0 && i % 5 == 0) {
            print("FizzBuzz\n");
            continue;
        }
        if (i % 3 == 0) {
            print("Fizz\n");
            continue;
        }
        if (i % 5 == 0) {
            print("Buzz\n");
            continue;
        }
        print(i);
        print("\n");
    }
}
```

---

## Grammar (EBNF)

```ebnf
program         = { function } ;
function        = "func", identifier, "(", [param_list], ")", [":", type], block ;
param_list      = parameter, { ",", parameter } ;
parameter       = identifier, ":", type ;
type            = "int" | "bool" | "string" | array_type ;
array_type      = "int", "[", integer, "]" ;
block           = "{", { statement }, "}" ;

statement       = variable_declaration
                | array_assignment
                | assignment
                | if_statement
                | while_loop
                | for_loop
                | break_stmt
                | continue_stmt
                | return_statement
                | print_statement
                | function_call, ";" ;

variable_declaration = "var", identifier, ":", type, [ "=", expression ], ";" ;
array_assignment = identifier, "[", expression, "]", "=", expression, ";" ;
assignment      = identifier, "=", expression, ";" ;
if_statement    = "if", "(", expression, ")", block
                , { "else", "if", "(", expression, ")", block }
                , [ "else", block ] ;
while_loop      = "while", "(", expression, ")", block ;
for_loop        = "for", "(", for_init, ";", [ expression ], ";", [ expression ], ")", block ;
for_init        = variable_declaration | expression | ;
break_stmt      = "break", ";" ;
continue_stmt   = "continue", ";" ;
return_statement = "return", [ expression ], ";" ;
print_statement = "print", "(", (string | expression), ")", ";" ;
function_call   = identifier, "(", [arg_list], ")" ;
arg_list        = expression, { ",", expression } ;

expression      = logical_or ;
logical_or      = logical_and, { "||", logical_and } ;
logical_and     = equality, { "&&", equality } ;
equality        = comparison, { ("==" | "!="), comparison } ;
comparison      = term, { ("<" | "<=" | ">" | ">="), term } ;
term            = factor, { ("+" | "-"), factor } ;
factor          = unary, { ("*" | "/" | "%"), unary } ;
unary           = ("+" | "-" | "!"), unary | postfix ;
postfix         = primary, { "[", expression, "]" } ;
primary         = integer
                | boolean
                | string
                | identifier [ "(", [arg_list], ")" ]
                | "(", expression, ")" ;
```

---

## HW2 to HW3: Extension Details

### File-by-file changes

#### `tiny_compiler.h`
- Added 8 new token types (`TOKEN_FOR`, `TOKEN_BREAK`, `TOKEN_CONTINUE`, `TOKEN_LBRACKET`, `TOKEN_RBRACKET`, `TOKEN_AND`, `TOKEN_OR`, `TOKEN_MOD`)
- Added 5 new AST node types (`NODE_ARRAY_ACCESS`, `NODE_ARRAY_ASSIGN`, `NODE_FOR`, `NODE_BREAK`, `NODE_CONTINUE`)
- Extended `var_decl` with `is_array` and `array_size` fields
- Added `array_access` and `array_assign` unions to `ASTNode`
- Added `for_stmt` union for for-loop components
- Added 5 new opcodes: `OP_MOD`, `OP_AND`, `OP_OR`, `OP_ARRAY_LOAD`, `OP_ARRAY_STORE`
- Added `arrays_pool` and `num_array_cells` to `VM` struct

#### `lexer.c`
- Added keyword recognition for `for`, `break`, `continue`
- Added `[`, `]`, `%` single-character tokens
- Added `&&`, `||` double-character operators (with error on single `&` or `|`)
- Added `//` line comment and `/* */` block comment skipping

#### `parser.c`
- Restructured expression parsing hierarchy: `logical_or → logical_and → equality → comparison → term → factor → unary → primary`
- Added `for_statement()` parsing with init/cond/incr extraction
- Added `break`/`continue` statement parsing
- Added array declaration (`var arr: int[5]`) in variable_declaration
- Added array access (`arr[i]`) in `primary()`
- Added array assignment (`arr[i] = value`) in `statement()`
- Added `else if` handling in if_statement parsing

#### `codegen.c`
- Added `LoopContext` stack with `push_loop`/`pop_loop`/`add_break_patch` for break/continue support
- Added `NODE_FOR` code generation with desugaring pattern:
  - Init → JMP cond_check → body → incr+pop → cond_check → JMP_IF/JMP
- Added `NODE_BREAK` (emit placeholder JMP, patch on loop exit)
- Added `NODE_CONTINUE` (emit JMP to loop's continue_addr)
- Added `NODE_ARRAY_ACCESS` (LOAD base + generate index + ARRAY_LOAD)
- Added `NODE_ARRAY_ASSIGN` (value + LOAD base + index + ARRAY_STORE)
- Added `OP_MOD`, `OP_AND`, `OP_OR` emit for binary operators

#### `vm.c`
- Added `arrays_pool` allocation and bounds checking
- Added `OP_MOD` (safe modulo, returns 0 on division by zero)
- Added `OP_AND` and `OP_OR` (logical operations on stack operands)
- Added `OP_ARRAY_LOAD` (base + index → arrays_pool read)
- Added `OP_ARRAY_STORE` (value + base + index → arrays_pool write)
- Memory cleanup for arrays_pool on HALT

#### `main.c`
- Replaced single test program with 10 test cases
- Added `print_bytecode` support for all new opcodes
- Test coverage: arrays, for loops, logical ops, break, continue, modulo, for+break, nested loops+continue, else-if, original HW2 features

### Design Decisions

1. **For-loop desugaring**: For loops are desugared in the code generator rather than the parser, keeping the AST representation clean and allowing break/continue to work naturally.

2. **Loop context stack**: Break/continue require tracking of loop exit/continue addresses. A stack of loop contexts allows proper nesting. Break instructions emit placeholder jumps that are patched when the enclosing loop exits.

3. **Arrays pool**: Array elements are stored in a separate memory pool rather than globals. This avoids alignment issues and allows bounds checking. The pool size is determined at compile time based on total array allocation.

4. **Operator precedence**: The expression parser was restructured to handle `||` (lowest), `&&`, then equality/comparison/arithmetic according to C-like conventions.

5. **Comments**: Comments are skipped in the lexer, making them invisible to the parser. Both `//` and `/* */` styles are supported.

---

## License

TinyLang is released under the MIT License.

---

*Documentation version 2.0 - Extended for HW3*
