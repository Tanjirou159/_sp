# TinyLang Documentation

## Table of Contents
1. [Introduction](#introduction)
2. [Getting Started](#getting-started)
3. [Language Reference](#language-reference)
4. [Compiler Architecture](#compiler-architecture)
5. [Virtual Machine](#virtual-machine)
6. [Examples](#examples)
7. [Error Handling](#error-handling)

---

## Introduction

TinyLang is a statically-typed, compiled programming language designed for educational purposes and embedded systems. It features a custom stack-based virtual machine (VM) for portability across different platforms.

### Key Features
- **Static typing** - Type checking at compile time
- **Manual memory management** - No garbage collector overhead
- **Clean C-like syntax** - Easy to learn and use
- **Portable bytecode** - Runs on any platform with a TinyLang VM
- **Tiny footprint** - Minimal runtime requirements

### Design Philosophy
- Simplicity over complexity
- Safety through static analysis
- Predictable performance
- Educational value

---

## Getting Started

### Installation

```bash
# Clone the repository
git clone https://github.com/example/tinylang.git
cd tinylang

# Compile the compiler
gcc -o tinylang *.c -std=c99 -Wall

# Run a TinyLang program
./tinylang examples/factorial.tl
```

### Hello World Program

```tinylang
func main() {
    print("Hello, World!");
}
```

### Compilation Process

1. **Lexical Analysis** - Converts source code to tokens
2. **Parsing** - Builds Abstract Syntax Tree (AST)
3. **Code Generation** - Produces bytecode for the VM
4. **Execution** - VM interprets the bytecode

---

## Language Reference

### Data Types

TinyLang supports three primitive types:

| Type | Description | Example |
|------|-------------|---------|
| `int` | 32-bit signed integer | `42`, `-17`, `0` |
| `bool` | Boolean value | `true`, `false` |
| `string` | Text string | `"Hello"`, `"World"` |

### Variables

Variables must be declared with a type before use:

```tinylang
var x: int = 10;
var flag: bool = true;
var name: string = "TinyLang";
var y: int;  // Uninitialized variable
```

### Functions

Functions are declared using the `func` keyword:

```tinylang
// Function with parameters and return type
func add(a: int, b: int): int {
    return a + b;
}

// Void function (no return value)
func greet(name: string) {
    print("Hello, ");
    print(name);
}

// Main function - program entry point
func main() {
    var result: int = add(5, 3);
    print(result);
}
```

### Control Flow

#### If-Else Statements

```tinylang
var score: int = 85;

if (score >= 90) {
    print("A grade");
} else if (score >= 80) {
    print("B grade");
} else {
    print("C grade or below");
}
```

#### While Loops

```tinylang
var i: int = 0;
while (i < 10) {
    print(i);
    i = i + 1;
}
```

### Operators

#### Arithmetic Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `+` | Addition | `x + y` |
| `-` | Subtraction | `x - y` |
| `*` | Multiplication | `x * y` |
| `/` | Division | `x / y` |
| `-` | Unary negation | `-x` |

#### Comparison Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `==` | Equal to | `x == y` |
| `!=` | Not equal to | `x != y` |
| `<` | Less than | `x < y` |
| `<=` | Less than or equal | `x <= y` |
| `>` | Greater than | `x > y` |
| `>=` | Greater than or equal | `x >= y` |

#### Logical Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `!` | Logical NOT | `!flag` |

### Print Statement

The `print` function outputs values to the console:

```tinylang
print("Text string");
print(x);           // Print variable
print(42);          // Print literal
print("Value: ");   // Print string literal
```

### Comments

```tinylang
// This is a single-line comment

/*
   This is a
   multi-line comment
*/
```

---

## Compiler Architecture

### Overview

```
Source Code → Lexer → Tokens → Parser → AST → Code Generator → Bytecode → VM
```

### Component Details

#### 1. Lexer (lexer.c)
- Converts source text into tokens
- Handles numbers, identifiers, keywords, operators, and punctuation
- Tracks line numbers for error reporting

#### 2. Parser (parser.c)
- Recursive descent parser
- Builds Abstract Syntax Tree (AST)
- Implements precedence climbing for expressions
- Performs syntactic analysis

#### 3. Code Generator (codegen.c)
- Traverses AST to generate bytecode
- Manages symbol table for variable locations
- Emits instructions for the stack-based VM
- Handles control flow with jump instructions

#### 4. Virtual Machine (vm.c)
- Stack-based execution engine
- 16-bit instruction set
- Direct threading for performance
- Memory-safe operations with bounds checking

### Bytecode Format

Each instruction consists of:
- **Opcode** (1 byte) - Operation to perform
- **Operand** (4 bytes) - Parameter for the operation

Example bytecode sequence:
```
0: PUSH 42      ; Push literal 42 onto stack
1: STORE 0      ; Store top of stack to global 0
2: LOAD 0       ; Load global 0 onto stack
3: PRINT        ; Print top of stack
4: HALT         ; Stop execution
```

---

## Virtual Machine

### Architecture

The VM uses a classic stack-based design:

```
+----------------+
|   Instruction  |
|    Pointer     |
+----------------+
|   Frame Base   |
+----------------+
|   Stack Top    |
+----------------+
|                |
|     Stack      |
|      ↓         |
|                |
+----------------+
|    Globals     |
+----------------+
```

### Stack Operations

| Instruction | Description | Stack Effect |
|-------------|-------------|--------------|
| `PUSH n` | Push integer n | `→ n` |
| `POP` | Discard top value | `x →` |
| `LOAD addr` | Load global variable | `→ global[addr]` |
| `STORE addr` | Store to global | `x →` (stores to global[addr]) |

### Arithmetic Operations

| Instruction | Description | Stack Effect |
|-------------|-------------|--------------|
| `ADD` | Addition | `a b → a+b` |
| `SUB` | Subtraction | `a b → a-b` |
| `MUL` | Multiplication | `a b → a*b` |
| `DIV` | Division | `a b → a/b` |
| `NEG` | Negation | `a → -a` |

### Comparison Operations

| Instruction | Description | Stack Effect |
|-------------|-------------|--------------|
| `EQ` | Equal | `a b → (a==b)` |
| `NE` | Not equal | `a b → (a!=b)` |
| `LT` | Less than | `a b → (a<b)` |
| `LTE` | Less or equal | `a b → (a<=b)` |
| `GT` | Greater than | `a b → (a>b)` |
| `GTE` | Greater or equal | `a b → (a>=b)` |

### Control Flow

| Instruction | Description |
|-------------|-------------|
| `JMP addr` | Unconditional jump to address |
| `JMP_IF addr` | Jump if top of stack is false |
| `CALL func` | Call function (simplified) |
| `RET` | Return from function |

### I/O Operations

| Instruction | Description |
|-------------|-------------|
| `PRINT` | Print top of stack as integer |
| `PRINT_STR addr` | Print string at address |
| `HALT` | Stop VM execution |

---

## Examples

### Factorial Calculation

```tinylang
func factorial(n: int): int {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

func main() {
    var result: int = factorial(5);
    print("5! = ");
    print(result);
    print("\n");
}
```

### Fibonacci Sequence

```tinylang
func fibonacci(n: int): int {
    if (n <= 1) {
        return n;
    } else {
        return fibonacci(n - 1) + fibonacci(n - 2);
    }
}

func main() {
    var i: int = 0;
    while (i < 10) {
        print("fib(");
        print(i);
        print(") = ");
        print(fibonacci(i));
        print("\n");
        i = i + 1;
    }
}
```

### Prime Number Checker

```tinylang
func is_prime(n: int): bool {
    if (n <= 1) {
        return false;
    }
    
    var i: int = 2;
    while (i * i <= n) {
        if (n % i == 0) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

func main() {
    var num: int = 17;
    if (is_prime(num)) {
        print(num);
        print(" is prime\n");
    } else {
        print(num);
        print(" is not prime\n");
    }
}
```

### Simple Calculator

```tinylang
func calculate(a: int, b: int, op: string): int {
    if (op == "+") {
        return a + b;
    }
    if (op == "-") {
        return a - b;
    }
    if (op == "*") {
        return a * b;
    }
    if (op == "/") {
        if (b != 0) {
            return a / b;
        }
        print("Error: Division by zero\n");
        return 0;
    }
    print("Error: Unknown operator\n");
    return 0;
}

func main() {
    var x: int = 10;
    var y: int = 5;
    
    print("10 + 5 = ");
    print(calculate(x, y, "+"));
    print("\n");
    
    print("10 - 5 = ");
    print(calculate(x, y, "-"));
    print("\n");
    
    print("10 * 5 = ");
    print(calculate(x, y, "*"));
    print("\n");
    
    print("10 / 5 = ");
    print(calculate(x, y, "/"));
    print("\n");
}
```

---

## Error Handling

### Compile-Time Errors

TinyLang detects the following errors during compilation:

| Error Type | Example | Message |
|------------|---------|---------|
| Lexical Error | `@#$` | "Unknown character" |
| Syntax Error | `if x > 5` | "Expected '(' after if" |
| Type Error | `var x: int = true` | "Type mismatch" |
| Undeclared Variable | `print(y)` where y not declared | "Undeclared variable" |

### Runtime Errors

The VM detects runtime errors:

| Error | Cause | Behavior |
|-------|-------|----------|
| Stack Overflow | Too many nested calls | Program termination with error message |
| Stack Underflow | Invalid bytecode | Program termination |
| Division by Zero | Division or modulo by zero | Returns 0 with warning |

### Error Example

```tinylang
func main() {
    var x: int = 10;
    var y: int = 0;
    var z: int = x / y;  // Division by zero warning
    print(z);            // Prints 0
}
```

---

## Grammar (EBNF)

```ebnf
program         = { function } ;
function        = "func", identifier, "(", [param_list], ")", [":", type], block ;
param_list      = parameter, { ",", parameter } ;
parameter       = identifier, ":", type ;
type            = "int" | "bool" | "string" ;
block           = "{", { statement }, "}" ;
statement       = variable_declaration
                | assignment
                | if_statement
                | loop_statement
                | return_statement
                | print_statement
                | function_call, ";" ;

variable_declaration = "var", identifier, ":", type, [ "=", expression ], ";" ;
assignment      = identifier, "=", expression, ";" ;
if_statement    = "if", "(", expression, ")", block, [ "else", block ] ;
loop_statement  = "while", "(", expression, ")", block ;
return_statement = "return", [ expression ], ";" ;
print_statement = "print", "(", (string | expression), ")", ";" ;
function_call   = identifier, "(", [arg_list], ")" ;
arg_list        = expression, { ",", expression } ;

expression      = equality ;
equality        = comparison, { ("==" | "!="), comparison } ;
comparison      = term, { ("<" | "<=" | ">" | ">="), term } ;
term            = factor, { ("+" | "-"), factor } ;
factor          = unary, { ("*" | "/"), unary } ;
unary           = ("+" | "-" | "!"), unary | primary ;
primary         = integer
                | boolean
                | string
                | identifier
                | "(", expression, ")"
                | function_call ;

integer         = digit, { digit } ;
boolean         = "true" | "false" ;
string          = '"', { character }, '"' ;
identifier      = letter, { letter | digit } ;
```

---

## Performance Considerations

### Memory Usage
- **Stack size**: Configurable (default 4096 cells)
- **Global variables**: Limited by available memory
- **Bytecode**: ~8 bytes per instruction average

### Optimization Tips
1. Minimize function call depth to avoid stack overflow
2. Use local variables for temporary calculations
3. Avoid deep recursion for large inputs
4. Prefer iterative solutions for simple loops

### Benchmark Example

```tinylang
// Calculate sum of 1 to 1,000,000
func sum_iterative(n: int): int {
    var result: int = 0;
    var i: int = 1;
    while (i <= n) {
        result = result + i;
        i = i + 1;
    }
    return result;
}

func main() {
    var result: int = sum_iterative(1000000);
    print("Sum: ");
    print(result);
    print("\n");
}
```

---

## Extending TinyLang

### Adding New Features

To add a new feature to TinyLang:

1. **Update the grammar** (EBNF)
2. **Add new tokens** (lexer)
3. **Extend AST node types** (parser)
4. **Implement code generation** (codegen)
5. **Add VM instructions** (vm)

### Example: Adding Arrays

```c
// Add new token
TOKEN_LBRACKET, TOKEN_RBRACKET

// Add AST node
typedef struct {
    char* name;
    ASTNode* index;
} array_access;

// Add VM instructions
OP_ARRAY_LOAD, OP_ARRAY_STORE
```

---

## Troubleshooting

### Common Issues

1. **"Stack overflow" error**
   - Solution: Increase stack size in VM initialization
   - Or optimize recursive functions

2. **"Undeclared variable" error**
   - Solution: Declare all variables before use
   - Check variable scope

3. **Bytecode won't run**
   - Solution: Ensure main function exists
   - Check for infinite loops

### Debugging Techniques

1. **Print bytecode**: Use `print_bytecode()` function
2. **Trace VM execution**: Add logging in each VM instruction
3. **Dump AST**: Add AST visualization function

---

## License

TinyLang is released under the MIT License.

---

## References

- [Compiler Design](https://en.wikipedia.org/wiki/Compiler)
- [Stack-based Virtual Machines](https://en.wikipedia.org/wiki/Stack_machine)
- [Recursive Descent Parsing](https://en.wikipedia.org/wiki/Recursive_descent_parser)

---

## Appendix: EBNF Notation Reference

| Symbol | Meaning |
|--------|---------|
| `=` | Definition |
| `\|` | Alternation (OR) |
| `{ ... }` | Repetition (0 or more) |
| `[ ... ]` | Optional |
| `( ... )` | Grouping |
| `" ... "` | Terminal symbol |

---

*Documentation version 1.0 - Last updated: 2026*