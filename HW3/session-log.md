# Session Log: HW3 - Extended TinyLang Compiler

**Date**: 2026-06-09  
**Project**: `<repo>/HW3/`  
**Base**: Extended from `<repo>/HW2/` (C-based TinyLang compiler)

---

## Context

Working within a systems programming course repository containing:
- `HW1/` - Python PL/0 compiler (lexer, parser, VM)
- `HW2/` - C-based TinyLang compiler (lexer, parser, codegen, VM)
- `HW3/` - Initially empty

## Goal

Extend the HW2 TinyLang compiler with new language features and create
accompanying documentation (`Documentation.md`).

---

## Decisions

1. **For-loop desugaring in codegen** - For-loops are desugared in the code
   generator rather than the parser. This keeps the AST representation clean
   and allows break/continue to work naturally through the loop context stack.

2. **Loop context stack** - Break/continue implemented via a stack of
   `LoopContext` structs. Break emits placeholder JMP instructions that are
   patched when the enclosing loop exits. Continue emits direct JMP to the
   loop's continue-target address.

3. **Separate arrays pool** - Array elements stored in a dedicated `arrays_pool`
   memory region in the VM rather than inline in globals. This enables bounds
   checking and avoids alignment issues. Pool size determined at compile time.

4. **Operator precedence** - Restructured expression parser with new levels:
   `logical_or → logical_and → equality → comparison → term → factor → unary`

5. **Comments in lexer** - `//` and `/* */` skipped during tokenization,
   invisible to the parser.

---

## Files Created/Modified

### `HW3/tiny_compiler.h` (created, ~180 lines)
- Extended `TokenType` enum: +8 tokens (`TOKEN_FOR`, `TOKEN_BREAK`, `TOKEN_CONTINUE`, `TOKEN_LBRACKET`, `TOKEN_RBRACKET`, `TOKEN_AND`, `TOKEN_OR`, `TOKEN_MOD`)
- Extended `NodeType` enum: +5 nodes (`NODE_ARRAY_ACCESS`, `NODE_ARRAY_ASSIGN`, `NODE_FOR`, `NODE_BREAK`, `NODE_CONTINUE`)
- Extended `ASTNode` union: `is_array`/`array_size` in var_decl, `array_access`, `array_assign`, `for_stmt` fields
- Extended `OpCode` enum: +5 opcodes (`OP_MOD`, `OP_AND`, `OP_OR`, `OP_ARRAY_LOAD`, `OP_ARRAY_STORE`)
- Extended `VM` struct: `arrays_pool`, `num_array_cells`

### `HW3/lexer.c` (created, ~190 lines)
- Added keywords: `for`, `break`, `continue`
- Added operators: `[`, `]`, `%`, `&&`, `||`
- Added comment skipping: `//` line comments, `/* */` block comments
- Error on lone `&` or `|` without second character

### `HW3/parser.c` (created, ~420 lines)
- Restructured expression parsing: `logical_or()` → `logical_and()` → `equality()` → ...
- Added `for_statement()` with init/cond/incr clause parsing
- Added array declaration parsing (`var arr: int[5]`)
- Added array access (`arr[i]`) in `primary()`
- Added array assignment (`arr[i] = value`) in `statement()`
- Added `break` and `continue` statement parsing
- Added `else if` chain support in if-statement parsing

### `HW3/codegen.c` (created, ~310 lines)
- Added `LoopContext` stack with `push_loop()`/`pop_loop()`/`add_break_patch()`
- Added `NODE_FOR` desugaring codegen:
  ```
  init → JMP cond_check → body → incr+pop → cond_check → JMP_IF exit → JMP body_start → exit
  ```
- Added `NODE_BREAK`: emit placeholder JMP, patch on loop exit
- Added `NODE_CONTINUE`: emit direct JMP to loop's continue_addr
- Added `NODE_ARRAY_ACCESS`: `LOAD base; eval index; ARRAY_LOAD`
- Added `NODE_ARRAY_ASSIGN`: `eval value; LOAD base; eval index; ARRAY_STORE`
- Added `OP_MOD`, `OP_AND`, `OP_OR` emit for binary operators

### `HW3/vm.c` (created, ~150 lines)
- Added `arrays_pool` allocation and cleanup
- Added `OP_MOD` with safe zero-divisor handling
- Added `OP_AND`, `OP_OR` (logical operations)
- Added `OP_ARRAY_LOAD`: `index=pop(); base=pop(); push(pool[base+index])`
- Added `OP_ARRAY_STORE`: `index=pop(); base=pop(); value=pop(); pool[base+index]=value`
- Added bounds checking on array access

### `HW3/main.c` (created, ~220 lines)
- 10 test programs covering all new features
- Extended `print_bytecode()` for all new opcodes
- Test coverage: arrays, for-loop, logical ops, break, continue, modulo, for+break, nested loops+continue, else-if, original HW2 features

### `HW3/Documentation.md` (created, ~400 lines)
- HW2 → HW3 feature comparison table
- Full EBNF grammar with new productions
- Code layout diagrams for for-loop desugaring
- Operator precedence table
- File-by-file change summary
- Design decision rationale
- 3 complete example programs (Array Summation, Prime Checker, FizzBuzz)

---

## Architecture

```
Source Code → Lexer → Tokens → Parser → AST → Code Generator → Bytecode → VM
```

Key differences from HW2:
- Lexer now skips comments and recognizes 8 new token types
- Parser has 3 additional precedence levels (||, &&, postfix[])
- Codegen manages a loop-context stack for break/continue backpatching
- VM has a separate arrays_pool memory region with bounds checking

## Status

- All 7 source files written and self-consistent
- 10 test programs covering all new language features
- **Not yet compiled** - no C compiler (gcc/clang) found on this system

## Build Instructions

```bash
cd HW3
gcc -o tinylang *.c -std=c99 -Wall
./tinylang
```

Requires: GCC or Clang with C99 support. On Windows, install via MSYS2:
`pacman -S mingw-w64-x86_64-gcc`
