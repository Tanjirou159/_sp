#!/usr/bin/env python3
import sys
import re
from enum import Enum
from dataclasses import dataclass
from typing import List, Dict, Optional, Tuple

# ============================================================
# TOKEN DEFINITION
# ============================================================

class TokenType(Enum):
    # Keywords
    CONST = 1
    VAR = 2
    PROCEDURE = 3
    CALL = 4
    BEGIN = 5
    END = 6
    IF = 7
    THEN = 8
    WHILE = 9
    DO = 10
    ODD = 11
    
    # Operators
    PLUS = 20
    MINUS = 21
    TIMES = 22
    DIVIDE = 23
    ASSIGN = 24
    EQ = 25
    NEQ = 26
    LT = 27
    LE = 28
    GT = 29
    GE = 30
    
    # Delimiters
    LPAREN = 40
    RPAREN = 41
    COMMA = 42
    SEMICOLON = 43
    PERIOD = 44
    
    # Other
    IDENT = 50
    NUMBER = 51
    EOF = 52

@dataclass
class Token:
    type: TokenType
    value: any
    line: int

# ============================================================
# LEXER
# ============================================================

class Lexer:
    def __init__(self, source: str):
        self.source = source
        self.pos = 0
        self.line = 1
        self.keywords = {
            'const': TokenType.CONST,
            'var': TokenType.VAR,
            'procedure': TokenType.PROCEDURE,
            'call': TokenType.CALL,
            'begin': TokenType.BEGIN,
            'end': TokenType.END,
            'if': TokenType.IF,
            'then': TokenType.THEN,
            'while': TokenType.WHILE,
            'do': TokenType.DO,
            'odd': TokenType.ODD,
        }
    
    def current_char(self) -> str:
        if self.pos >= len(self.source):
            return '\0'
        return self.source[self.pos]
    
    def advance(self) -> None:
        if self.current_char() == '\n':
            self.line += 1
        self.pos += 1
    
    def skip_whitespace(self) -> None:
        while self.current_char() in ' \t\r\n':
            self.advance()
    
    def read_ident(self) -> str:
        start = self.pos
        while self.current_char().isalnum() or self.current_char() == '_':
            self.advance()
        return self.source[start:self.pos]
    
    def read_number(self) -> int:
        start = self.pos
        while self.current_char().isdigit():
            self.advance()
        return int(self.source[start:self.pos])
    
    def get_token(self) -> Token:
        self.skip_whitespace()
        line = self.line
        ch = self.current_char()
        
        if ch == '\0':
            return Token(TokenType.EOF, None, line)
        
        # Identifiers and keywords
        if ch.isalpha():
            ident = self.read_ident()
            token_type = self.keywords.get(ident.lower(), TokenType.IDENT)
            return Token(token_type, ident if token_type == TokenType.IDENT else None, line)
        
        # Numbers
        if ch.isdigit():
            return Token(TokenType.NUMBER, self.read_number(), line)
        
        # Single-character tokens
        self.advance()  # consume the character
        
        if ch == '+': return Token(TokenType.PLUS, None, line)
        if ch == '-': return Token(TokenType.MINUS, None, line)
        if ch == '*': return Token(TokenType.TIMES, None, line)
        if ch == '/': return Token(TokenType.DIVIDE, None, line)
        if ch == '(': return Token(TokenType.LPAREN, None, line)
        if ch == ')': return Token(TokenType.RPAREN, None, line)
        if ch == ',': return Token(TokenType.COMMA, None, line)
        if ch == ';': return Token(TokenType.SEMICOLON, None, line)
        if ch == '.': return Token(TokenType.PERIOD, None, line)
        if ch == ':':
            if self.current_char() == '=':
                self.advance()
                return Token(TokenType.ASSIGN, None, line)
            raise SyntaxError(f"Expected '=' after ':' at line {line}")
        if ch == '=':
            return Token(TokenType.EQ, None, line)
        if ch == '<':
            if self.current_char() == '>':
                self.advance()
                return Token(TokenType.NEQ, None, line)
            elif self.current_char() == '=':
                self.advance()
                return Token(TokenType.LE, None, line)
            return Token(TokenType.LT, None, line)
        if ch == '>':
            if self.current_char() == '=':
                self.advance()
                return Token(TokenType.GE, None, line)
            return Token(TokenType.GT, None, line)
        
        raise SyntaxError(f"Unexpected character '{ch}' at line {line}")

# ============================================================
# SYMBOL TABLE
# ============================================================

class SymbolKind(Enum):
    CONST = 1
    VAR = 2
    PROCEDURE = 3

@dataclass
class Symbol:
    name: str
    kind: SymbolKind
    level: int
    address: int

class SymbolTable:
    def __init__(self):
        self.symbols: List[Symbol] = []
    
    def lookup(self, name: str) -> Optional[Symbol]:
        for sym in reversed(self.symbols):
            if sym.name == name:
                return sym
        return None
    
    def insert(self, name: str, kind: SymbolKind, level: int, address: int) -> Symbol:
        sym = Symbol(name, kind, level, address)
        self.symbols.append(sym)
        return sym

# ============================================================
# VM INSTRUCTIONS
# ============================================================

class OpCode(Enum):
    LIT = 1   # Load literal constant
    LOD = 2   # Load variable (level, address)
    STO = 3   # Store variable (level, address)
    CAL = 4   # Call procedure (level, address)
    INT = 5   # Allocate stack space (size)
    JMP = 6   # Unconditional jump (address)
    JPC = 7   # Jump if condition false (address)
    OPR = 8   # Operation (subtype: 0=ret, 1=neg, 2=add, 3=sub, 4=mul, 5=div, 6=odd, 7=eq, 8=neq, 9=lt, 10=le, 11=gt, 12=ge)

@dataclass
class Instruction:
    op: OpCode
    l: int  # Level (lexical nesting)
    a: int  # Address (for LOD/STO) or value (for LIT)

# ============================================================
# PARSER / COMPILER
# ============================================================

class Compiler:
    def __init__(self):
        self.lexer = None
        self.current_token = None
        self.symbol_table = SymbolTable()
        self.code: List[Instruction] = []
        self.current_level = 0
        self.variable_count = 0
        self.next_procedure_address = 0
        self.line_number = 1
    
    def error(self, msg: str):
        raise SyntaxError(f"Line {self.line_number}: {msg}")
    
    def expect(self, expected_type: TokenType) -> Token:
        if self.current_token.type == expected_type:
            token = self.current_token
            self.advance()
            return token
        self.error(f"Expected {expected_type}, got {self.current_token.type}")
    
    def advance(self):
        self.current_token = self.lexer.get_token()
        self.line_number = self.current_token.line
    
    def accept(self, token_type: TokenType) -> bool:
        if self.current_token.type == token_type:
            self.advance()
            return True
        return False
    
    def emit(self, op: OpCode, l: int, a: int) -> int:
        """Emit instruction and return its index for backpatching"""
        self.code.append(Instruction(op, l, a))
        return len(self.code) - 1
    
    def patch(self, index: int, address: int):
        """Patch instruction at index with new address"""
        self.code[index].a = address
    
    # ============================================================
    # CODE GENERATION (WITH WHILE LOOP SUPPORT)
    # ============================================================
    
    def compile(self, source: str) -> List[Instruction]:
        self.lexer = Lexer(source)
        self.advance()
        
        # Generate main program header
        self.emit(OpCode.JMP, 0, 0)  # Jump over procedure code
        self.next_procedure_address = 1
        
        # Parse procedures
        while self.current_token.type == TokenType.PROCEDURE:
            self.procedure()
        
        # Patch main program jump
        main_start = len(self.code)
        self.patch(0, main_start)
        
        # Parse main statement
        self.emit(OpCode.INT, 0, self.variable_count)
        self.statement()
        self.emit(OpCode.OPR, 0, 0)  # Return (halt)
        
        self.expect(TokenType.PERIOD)
        
        return self.code
    
    def procedure(self):
        """Compile a procedure declaration"""
        self.expect(TokenType.PROCEDURE)
        name_token = self.expect(TokenType.IDENT)
        name = name_token.value
        
        # Add procedure to symbol table
        proc_addr = self.next_procedure_address
        self.symbol_table.insert(name, SymbolKind.PROCEDURE, self.current_level, proc_addr)
        
        # Set up procedure code address (will be patched)
        proc_start = len(self.code)
        self.next_procedure_address = proc_start
        self.emit(OpCode.JMP, 0, 0)  # Placeholder, will be patched to skip proc body
        proc_jump = len(self.code) - 1
        
        # Enter new lexical level
        self.current_level += 1
        previous_var_count = self.variable_count
        self.variable_count = 0
        
        # Parse parameters (P0 doesn't have actual parameters, but has local vars)
        if self.accept(TokenType.LPAREN):
            self.expect(TokenType.RPAREN)
        
        self.expect(TokenType.SEMICOLON)
        
        # Parse constants and variables
        if self.accept(TokenType.CONST):
            self.const_declaration()
            self.expect(TokenType.SEMICOLON)
        
        if self.accept(TokenType.VAR):
            self.var_declaration()
            self.expect(TokenType.SEMICOLON)
        
        # Parse nested procedures
        while self.current_token.type == TokenType.PROCEDURE:
            self.procedure()
            self.expect(TokenType.SEMICOLON)
        
        # Parse statement
        self.emit(OpCode.INT, 0, self.variable_count)  # Allocate local vars
        self.statement()
        self.emit(OpCode.OPR, 0, 0)  # Return
        
        # Exit procedure level
        self.current_level -= 1
        self.variable_count = previous_var_count
        
        # Patch the jump to skip procedure body
        self.patch(proc_jump, len(self.code))
    
    def const_declaration(self):
        """const name = number {, name = number}"""
        while True:
            name_token = self.expect(TokenType.IDENT)
            name = name_token.value
            self.expect(TokenType.EQ)
            num_token = self.expect(TokenType.NUMBER)
            value = num_token.value
            
            self.symbol_table.insert(name, SymbolKind.CONST, self.current_level, value)
            
            if not self.accept(TokenType.COMMA):
                break
    
    def var_declaration(self):
        """var name {, name}"""
        while True:
            name_token = self.expect(TokenType.IDENT)
            name = name_token.value
            self.symbol_table.insert(name, SymbolKind.VAR, self.current_level, self.variable_count)
            self.variable_count += 1
            
            if not self.accept(TokenType.COMMA):
                break
    
    def statement(self):
        """Parse a statement"""
        if self.accept(TokenType.IDENT):
            # Assignment
            var_name = self.current_token.value
            symbol = self.symbol_table.lookup(var_name)
            if symbol is None:
                self.error(f"Undefined variable: {var_name}")
            if symbol.kind != SymbolKind.VAR:
                self.error(f"Cannot assign to {var_name}")
            
            self.advance()
            self.expect(TokenType.ASSIGN)
            self.expression()
            self.emit(OpCode.STO, symbol.level, symbol.address)
            
        elif self.accept(TokenType.CALL):
            # Procedure call
            proc_name = self.current_token.value
            symbol = self.symbol_table.lookup(proc_name)
            if symbol is None or symbol.kind != SymbolKind.PROCEDURE:
                self.error(f"Undefined procedure: {proc_name}")
            self.advance()
            self.emit(OpCode.CAL, symbol.level, symbol.address)
            
        elif self.accept(TokenType.BEGIN):
            # Statement sequence
            self.statement()
            while self.accept(TokenType.SEMICOLON):
                self.statement()
            self.expect(TokenType.END)
            
        elif self.accept(TokenType.IF):
            # If statement
            self.condition()
            self.expect(TokenType.THEN)
            
            # Emit conditional jump (to be patched)
            jpc_addr = self.emit(OpCode.JPC, 0, 0)
            self.statement()
            
            # Patch JPC to skip then-part if condition false
            self.patch(jpc_addr, len(self.code))
            
        elif self.accept(TokenType.WHILE):
            # ====================================================
            # WHILE LOOP IMPLEMENTATION - THE KEY ADDITION
            # ====================================================
            # Design: Test-at-Top loop pattern
            #   start_addr: [condition code]
            #               JPC exit_addr  (jump to exit if false)
            #               [statement body code]
            #               JMP start_addr
            #   exit_addr:  ...
            
            # 1. Mark the start address where condition will be evaluated
            start_addr = len(self.code)
            
            # 2. Generate condition evaluation code
            self.condition()
            self.expect(TokenType.DO)
            
            # 3. Emit conditional jump (false -> exit)
            #    Address 0 is a placeholder, will be patched later
            exit_addr = self.emit(OpCode.JPC, 0, 0)
            
            # 4. Generate the loop body statement
            self.statement()
            
            # 5. Emit unconditional jump back to condition test
            self.emit(OpCode.JMP, 0, start_addr)
            
            # 6. Backpatch: Fill the JPC's target address with current position
            #    This is where execution continues after the loop
            self.patch(exit_addr, len(self.code))
            
        elif self.accept(TokenType.WHILE):
            # Alternative: DO-WHILE style (uncomment if needed)
            start_addr = len(self.code)
            self.statement()
            self.expect(TokenType.WHILE)
            self.condition()
            self.expect(TokenType.SEMICOLON)
            self.emit(OpCode.JPC, 0, start_addr)
    
    def condition(self):
        """Parse and generate code for a condition"""
        if self.accept(TokenType.ODD):
            self.expression()
            self.emit(OpCode.OPR, 0, 6)  # ODD operation
        else:
            self.expression()
            op = self.current_token.type
            if op in (TokenType.EQ, TokenType.NEQ, TokenType.LT, 
                      TokenType.LE, TokenType.GT, TokenType.GE):
                self.advance()
                self.expression()
                
                # Map token to OPR operation code
                op_map = {
                    TokenType.EQ: 7,
                    TokenType.NEQ: 8,
                    TokenType.LT: 9,
                    TokenType.LE: 10,
                    TokenType.GT: 11,
                    TokenType.GE: 12,
                }
                self.emit(OpCode.OPR, 0, op_map[op])
            else:
                # Relational operator required
                self.error("Invalid condition")
    
    def expression(self):
        """Parse and generate code for an expression"""
        if self.accept(TokenType.PLUS):
            self.term()
        elif self.accept(TokenType.MINUS):
            self.term()
            self.emit(OpCode.OPR, 0, 1)  # NEGATE
        else:
            self.term()
        
        while self.current_token.type in (TokenType.PLUS, TokenType.MINUS):
            op = self.current_token.type
            self.advance()
            self.term()
            if op == TokenType.PLUS:
                self.emit(OpCode.OPR, 0, 2)  # ADD
            else:
                self.emit(OpCode.OPR, 0, 3)  # SUB
    
    def term(self):
        """Parse and generate code for a term"""
        self.factor()
        
        while self.current_token.type in (TokenType.TIMES, TokenType.DIVIDE):
            op = self.current_token.type
            self.advance()
            self.factor()
            if op == TokenType.TIMES:
                self.emit(OpCode.OPR, 0, 4)  # MUL
            else:
                self.emit(OpCode.OPR, 0, 5)  # DIV
    
    def factor(self):
        """Parse and generate code for a factor"""
        if self.accept(TokenType.NUMBER):
            self.emit(OpCode.LIT, 0, self.current_token.value)
        elif self.accept(TokenType.IDENT):
            name = self.current_token.value
            symbol = self.symbol_table.lookup(name)
            if symbol is None:
                self.error(f"Undefined identifier: {name}")
            
            if symbol.kind == SymbolKind.CONST:
                self.emit(OpCode.LIT, 0, symbol.address)
            elif symbol.kind == SymbolKind.VAR:
                self.emit(OpCode.LOD, symbol.level, symbol.address)
            else:
                self.error(f"Cannot use procedure {name} in expression")
        elif self.accept(TokenType.LPAREN):
            self.expression()
            self.expect(TokenType.RPAREN)
        else:
            self.error("Invalid factor")

# ============================================================
# VIRTUAL MACHINE
# ============================================================

class VM:
    def __init__(self, code: List[Instruction]):
        self.code = code
        self.stack = [0] * 1000  # Stack memory
        self.sp = 0  # Stack pointer
        self.bp = 0  # Base pointer (frame pointer)
        self.pc = 0  # Program counter
    
    def run(self) -> None:
        print("\n=== VM Execution ===\n")
        
        while self.pc < len(self.code):
            instr = self.code[self.pc]
            self.pc += 1
            
            if instr.op == OpCode.LIT:
                # Load literal
                self.stack[self.sp] = instr.a
                self.sp += 1
                
            elif instr.op == OpCode.LOD:
                # Load variable (using static links for lexical scoping)
                # Find frame: follow static link 'l' times
                frame = self.bp
                for _ in range(instr.l):
                    frame = self.stack[frame]  # Static link stored at frame start
                value = self.stack[frame + instr.a]
                self.stack[self.sp] = value
                self.sp += 1
                
            elif instr.op == OpCode.STO:
                # Store variable
                value = self.stack[self.sp - 1]
                self.sp -= 1
                
                # Find frame using static links
                frame = self.bp
                for _ in range(instr.l):
                    frame = self.stack[frame]
                self.stack[frame + instr.a] = value
                
            elif instr.op == OpCode.CAL:
                # Call procedure
                # Save return address, dynamic link, and static link
                self.stack[self.sp] = self.bp  # Dynamic link
                self.stack[self.sp + 1] = self.pc  # Return address
                
                # Calculate static link
                # Lexical level difference determines how many static links to follow
                num_links = self.current_level() - instr.l
                sl = self.bp
                for _ in range(num_links):
                    sl = self.stack[sl]
                self.stack[self.sp + 2] = sl  # Static link
                
                self.sp += 3
                self.bp = self.sp
                self.pc = instr.a
                
            elif instr.op == OpCode.INT:
                # Allocate local variables
                self.sp += instr.a
                
            elif instr.op == OpCode.JMP:
                # Unconditional jump
                self.pc = instr.a
                
            elif instr.op == OpCode.JPC:
                # Conditional jump (pop condition value)
                cond = self.stack[self.sp - 1]
                self.sp -= 1
                if cond == 0:  # False - jump to exit
                    self.pc = instr.a
                    
            elif instr.op == OpCode.OPR:
                # Operation
                if instr.a == 0:  # Return from procedure
                    self.sp = self.bp
                    self.bp = self.stack[self.sp - 3]  # Restore dynamic link
                    self.pc = self.stack[self.sp - 2]  # Return address
                elif instr.a == 1:  # Negate
                    self.stack[self.sp - 1] = -self.stack[self.sp - 1]
                elif instr.a == 2:  # Add
                    b = self.stack[self.sp - 1]
                    a = self.stack[self.sp - 2]
                    self.sp -= 1
                    self.stack[self.sp - 1] = a + b
                elif instr.a == 3:  # Subtract
                    b = self.stack[self.sp - 1]
                    a = self.stack[self.sp - 2]
                    self.sp -= 1
                    self.stack[self.sp - 1] = a - b
                elif instr.a == 4:  # Multiply
                    b = self.stack[self.sp - 1]
                    a = self.stack[self.sp - 2]
                    self.sp -= 1
                    self.stack[self.sp - 1] = a * b
                elif instr.a == 5:  # Divide
                    b = self.stack[self.sp - 1]
                    a = self.stack[self.sp - 2]
                    self.sp -= 1
                    self.stack[self.sp - 1] = a // b
                elif instr.a == 6:  # ODD
                    self.stack[self.sp - 1] = 1 if self.stack[self.sp - 1] % 2 == 1 else 0
                elif instr.a == 7:  # Equal
                    b = self.stack[self.sp - 1]
                    a = self.stack[self.sp - 2]
                    self.sp -= 1
                    self.stack[self.sp - 1] = 1 if a == b else 0
                elif instr.a == 8:  # Not equal
                    b = self.stack[self.sp - 1]
                    a = self.stack[self.sp - 2]
                    self.sp -= 1
                    self.stack[self.sp - 1] = 1 if a != b else 0
                elif instr.a == 9:  # Less than
                    b = self.stack[self.sp - 1]
                    a = self.stack[self.sp - 2]
                    self.sp -= 1
                    self.stack[self.sp - 1] = 1 if a < b else 0
                elif instr.a == 10:  # Less equal
                    b = self.stack[self.sp - 1]
                    a = self.stack[self.sp - 2]
                    self.sp -= 1
                    self.stack[self.sp - 1] = 1 if a <= b else 0
                elif instr.a == 11:  # Greater than
                    b = self.stack[self.sp - 1]
                    a = self.stack[self.sp - 2]
                    self.sp -= 1
                    self.stack[self.sp - 1] = 1 if a > b else 0
                elif instr.a == 12:  # Greater equal
                    b = self.stack[self.sp - 1]
                    a = self.stack[self.sp - 2]
                    self.sp -= 1
                    self.stack[self.sp - 1] = 1 if a >= b else 0
    
    def current_level(self) -> int:
        """Determine current lexical level from frame structure"""
        # In a real implementation, level would be stored in the frame
        # For now, we return 0 for main and adjust accordingly
        # This is a simplification - full implementation would track level in stack
        if self.bp == 0:
            return 0
        return 1  # Simplified

# ============================================================
# MAIN
# ============================================================

def print_instructions(code: List[Instruction]):
    """Pretty print the generated instructions"""
    print("\n=== Generated Code ===\n")
    for i, instr in enumerate(code):
        print(f"{i:3d}: {instr.op.name:3s}  l={instr.l:2d}  a={instr.a:3d}")
    print()

def test_while_loop():
    """Test program demonstrating while loop"""
    program = """
    var i, sum;
    begin
        i := 1;
        sum := 0;
        while i <= 10 do
        begin
            sum := sum + i;
            i := i + 1
        end;
        write sum
    end.
    """
    # Note: 'write' is a built-in procedure (would need implementation)
    # For demonstration, we'll use a simpler program
    
    # Simple factorial using while loop
    factorial = """
    var n, result;
    begin
        n := 5;
        result := 1;
        while n > 1 do
        begin
            result := result * n;
            n := n - 1
        end
    end.
    """
    
    return factorial

def test_nested_while():
    """Test nested while loops"""
    program = """
    var i, j, sum;
    begin
        i := 1;
        sum := 0;
        while i <= 3 do
        begin
            j := 1;
            while j <= 3 do
            begin
                sum := sum + i + j;
                j := j + 1
            end;
            i := i + 1
        end
    end.
    """
    return program

def test_procedure_call():
    """Test procedure calls with lexical scoping"""
    program = """
    var x;
    procedure proc1;
        var y;
        begin
            y := 10;
            x := x + y
        end;
    begin
        x := 5;
        call proc1
    end.
    """
    return program

if __name__ == "__main__":
    # Test 1: While loop (factorial)
    print("=" * 50)
    print("TEST 1: WHILE LOOP (Factorial of 5)")
    print("=" * 50)
    
    compiler = Compiler()
    program = test_while_loop()
    print("Source program:")
    print(program)
    
    try:
        code = compiler.compile(program)
        print_instructions(code)
        
        vm = VM(code)
        vm.run()
        
        # Show result from stack
        print(f"\nResult (should be 120 for factorial!): {vm.stack[vm.sp-1] if vm.sp > 0 else 'N/A'}")
    except SyntaxError as e:
        print(f"Compilation error: {e}")
    
    print("\n" + "=" * 50)
    print("TEST 2: NESTED WHILE LOOPS")
    print("=" * 50)
    
    compiler2 = Compiler()
    program2 = test_nested_while()
    print("Source program:")
    print(program2)
    
    try:
        code2 = compiler2.compile(program2)
        print_instructions(code2)
        
        vm2 = VM(code2)
        vm2.run()
        print(f"\nSum result (should be 36): {vm2.stack[vm2.sp-1] if vm2.sp > 0 else 'N/A'}")
    except SyntaxError as e:
        print(f"Compilation error: {e}")

    print("\n" + "=" * 50)
    print("TEST 3: PROCEDURE CALLS")
    print("=" * 50)
    
    compiler3 = Compiler()
    program3 = test_procedure_call()
    print("Source program:")
    print(program3)
    
    try:
        code3 = compiler3.compile(program3)
        print_instructions(code3)
        
        vm3 = VM(code3)
        vm3.run()
        print(f"\nResult: x = {vm3.stack[1] if len(vm3.stack) > 1 else 'N/A'} (should be 15)")
    except SyntaxError as e:
        print(f"Compilation error: {e}")