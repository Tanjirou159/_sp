#include "tiny_compiler.h"

ASTNode* program();
ASTNode* function();
ASTNode* statement();
ASTNode* expression();
ASTNode* equality();
ASTNode* comparison();
ASTNode* term();
ASTNode* factor();
ASTNode* unary();
ASTNode* primary();

void error(const char* msg) {
    printf("Error at line %d: %s\n", lexer.line, msg);
    exit(1);
}

void expect(TokenType type) {
    if (current_token.type != type) {
        char msg[100];
        sprintf(msg, "Expected token type %d, got %d", type, current_token.type);
        error(msg);
    }
}

void advance_token() {
    current_token = get_next_token();
}

ASTNode* new_ast_node(NodeType type) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    node->type = type;
    return node;
}

ASTNode* program() {
    ASTNode* prog = new_ast_node(NODE_PROGRAM);
    ASTNode* last = prog;
    
    while (current_token.type != TOKEN_EOF) {
        last->next = function();
        while (last->next) last = last->next;
    }
    return prog;
}

ASTNode* function() {
    expect(TOKEN_FUNC);
    advance_token();
    expect(TOKEN_IDENT);
    
    ASTNode* func = new_ast_node(NODE_FUNCTION);
    func->func.name = strdup(current_token.lexeme);
    advance_token();
    
    expect(TOKEN_LPAREN);
    advance_token();
    // Skip parameters parsing for simplicity
    while (current_token.type != TOKEN_RPAREN && current_token.type != TOKEN_EOF) {
        if (current_token.type == TOKEN_IDENT) advance_token();
        if (current_token.type == TOKEN_COLON) advance_token();
        if (current_token.type == TOKEN_INT) advance_token();
        if (current_token.type == TOKEN_COMMA) advance_token();
    }
    expect(TOKEN_RPAREN);
    advance_token();
    
    if (current_token.type == TOKEN_COLON) {
        advance_token();
        expect(TOKEN_INT);
        func->func.ret_type = strdup(current_token.lexeme);
        advance_token();
    } else {
        func->func.ret_type = strdup("void");
    }
    
    expect(TOKEN_LBRACE);
    advance_token();
    
    func->func.body = new_ast_node(NODE_PROGRAM);
    ASTNode* last_stmt = func->func.body;
    
    while (current_token.type != TOKEN_RBRACE && current_token.type != TOKEN_EOF) {
        last_stmt->next = statement();
        while (last_stmt->next) last_stmt = last_stmt->next;
    }
    
    expect(TOKEN_RBRACE);
    advance_token();
    
    return func;
}

ASTNode* statement() {
    if (current_token.type == TOKEN_VAR) {
        advance_token();
        expect(TOKEN_IDENT);
        
        ASTNode* decl = new_ast_node(NODE_VAR_DECL);
        decl->var_decl.name = strdup(current_token.lexeme);
        advance_token();
        
        expect(TOKEN_COLON);
        advance_token();
        
        if (current_token.type == TOKEN_INT || current_token.type == TOKEN_BOOL || 
            current_token.type == TOKEN_STRING_TYPE) {
            decl->var_decl.var_type = strdup(current_token.lexeme);
            advance_token();
        }
        
        if (current_token.type == TOKEN_ASSIGN) {
            advance_token();
            decl->var_decl.init = expression();
        }
        
        expect(TOKEN_SEMICOLON);
        advance_token();
        return decl;
    }
    else if (current_token.type == TOKEN_IDENT) {
        char* name = strdup(current_token.lexeme);
        advance_token();
        
        if (current_token.type == TOKEN_ASSIGN) {
            advance_token();
            ASTNode* assign = new_ast_node(NODE_ASSIGN);
            assign->assign.name = name;
            assign->assign.value = expression();
            expect(TOKEN_SEMICOLON);
            advance_token();
            return assign;
        }
    }
    else if (current_token.type == TOKEN_IF) {
        advance_token();
        expect(TOKEN_LPAREN);
        advance_token();
        
        ASTNode* if_node = new_ast_node(NODE_IF);
        if_node->if_stmt.cond = expression();
        
        expect(TOKEN_RPAREN);
        advance_token();
        
        if_node->if_stmt.then_branch = new_ast_node(NODE_PROGRAM);
        ASTNode* then_last = if_node->if_stmt.then_branch;
        
        expect(TOKEN_LBRACE);
        advance_token();
        while (current_token.type != TOKEN_RBRACE && current_token.type != TOKEN_EOF) {
            then_last->next = statement();
            while (then_last->next) then_last = then_last->next;
        }
        expect(TOKEN_RBRACE);
        advance_token();
        
        if (current_token.type == TOKEN_ELSE) {
            advance_token();
            if_node->if_stmt.else_branch = new_ast_node(NODE_PROGRAM);
            ASTNode* else_last = if_node->if_stmt.else_branch;
            
            expect(TOKEN_LBRACE);
            advance_token();
            while (current_token.type != TOKEN_RBRACE && current_token.type != TOKEN_EOF) {
                else_last->next = statement();
                while (else_last->next) else_last = else_last->next;
            }
            expect(TOKEN_RBRACE);
            advance_token();
        }
        
        return if_node;
    }
    else if (current_token.type == TOKEN_WHILE) {
        advance_token();
        expect(TOKEN_LPAREN);
        advance_token();
        
        ASTNode* while_node = new_ast_node(NODE_WHILE);
        while_node->while_stmt.cond = expression();
        
        expect(TOKEN_RPAREN);
        advance_token();
        
        while_node->while_stmt.body = new_ast_node(NODE_PROGRAM);
        ASTNode* body_last = while_node->while_stmt.body;
        
        expect(TOKEN_LBRACE);
        advance_token();
        while (current_token.type != TOKEN_RBRACE && current_token.type != TOKEN_EOF) {
            body_last->next = statement();
            while (body_last->next) body_last = body_last->next;
        }
        expect(TOKEN_RBRACE);
        advance_token();
        
        return while_node;
    }
    else if (current_token.type == TOKEN_RETURN) {
        advance_token();
        ASTNode* ret_node = new_ast_node(NODE_RETURN);
        if (current_token.type != TOKEN_SEMICOLON) {
            ret_node->return_stmt.value = expression();
        }
        expect(TOKEN_SEMICOLON);
        advance_token();
        return ret_node;
    }
    else if (current_token.type == TOKEN_PRINT) {
        advance_token();
        expect(TOKEN_LPAREN);
        advance_token();
        
        ASTNode* print_node = new_ast_node(NODE_PRINT);
        if (current_token.type == TOKEN_STRING) {
            print_node->print_stmt.is_string_lit = true;
            print_node->print_stmt.str_value = strdup(current_token.str_value);
            advance_token();
        } else {
            print_node->print_stmt.is_string_lit = false;
            print_node->print_stmt.expr = expression();
        }
        
        expect(TOKEN_RPAREN);
        advance_token();
        expect(TOKEN_SEMICOLON);
        advance_token();
        return print_node;
    }
    else {
        error("Unexpected token in statement");
    }
    
    return NULL;
}

ASTNode* expression() {
    return equality();
}

ASTNode* equality() {
    ASTNode* node = comparison();
    
    while (current_token.type == TOKEN_EQUAL || current_token.type == TOKEN_NOT_EQUAL) {
        ASTNode* binary = new_ast_node(NODE_BINARY);
        binary->binary.op = (current_token.type == TOKEN_EQUAL) ? "==" : "!=";
        binary->binary.left = node;
        advance_token();
        binary->binary.right = comparison();
        node = binary;
    }
    
    return node;
}

ASTNode* comparison() {
    ASTNode* node = term();
    
    while (current_token.type == TOKEN_LT || current_token.type == TOKEN_LTE ||
           current_token.type == TOKEN_GT || current_token.type == TOKEN_GTE) {
        ASTNode* binary = new_ast_node(NODE_BINARY);
        const char* op;
        if (current_token.type == TOKEN_LT) op = "<";
        else if (current_token.type == TOKEN_LTE) op = "<=";
        else if (current_token.type == TOKEN_GT) op = ">";
        else op = ">=";
        binary->binary.op = (char*)op;
        binary->binary.left = node;
        advance_token();
        binary->binary.right = term();
        node = binary;
    }
    
    return node;
}

ASTNode* term() {
    ASTNode* node = factor();
    
    while (current_token.type == TOKEN_PLUS || current_token.type == TOKEN_MINUS) {
        ASTNode* binary = new_ast_node(NODE_BINARY);
        binary->binary.op = (current_token.type == TOKEN_PLUS) ? "+" : "-";
        binary->binary.left = node;
        advance_token();
        binary->binary.right = factor();
        node = binary;
    }
    
    return node;
}

ASTNode* factor() {
    ASTNode* node = unary();
    
    while (current_token.type == TOKEN_MUL || current_token.type == TOKEN_DIV) {
        ASTNode* binary = new_ast_node(NODE_BINARY);
        binary->binary.op = (current_token.type == TOKEN_MUL) ? "*" : "/";
        binary->binary.left = node;
        advance_token();
        binary->binary.right = unary();
        node = binary;
    }
    
    return node;
}

ASTNode* unary() {
    if (current_token.type == TOKEN_MINUS || current_token.type == TOKEN_NOT) {
        ASTNode* unary_node = new_ast_node(NODE_UNARY);
        unary_node->unary.op = (current_token.type == TOKEN_MINUS) ? "-" : "!";
        advance_token();
        unary_node->unary.expr = unary();
        return unary_node;
    }
    
    return primary();
}

ASTNode* primary() {
    if (current_token.type == TOKEN_NUMBER) {
        ASTNode* num = new_ast_node(NODE_NUMBER_LIT);
        num->number.value = current_token.int_value;
        advance_token();
        return num;
    }
    else if (current_token.type == TOKEN_TRUE || current_token.type == TOKEN_FALSE) {
        ASTNode* bool_lit = new_ast_node(NODE_BOOL_LIT);
        bool_lit->boolean.value = (current_token.type == TOKEN_TRUE);
        advance_token();
        return bool_lit;
    }
    else if (current_token.type == TOKEN_STRING) {
        ASTNode* str_lit = new_ast_node(NODE_STRING_LIT);
        str_lit->string.value = strdup(current_token.str_value);
        advance_token();
        return str_lit;
    }
    else if (current_token.type == TOKEN_IDENT) {
        ASTNode* var = new_ast_node(NODE_VAR);
        var->var.name = strdup(current_token.lexeme);
        advance_token();
        
        if (current_token.type == TOKEN_LPAREN) {
            // Function call
            ASTNode* call = new_ast_node(NODE_CALL);
            call->call.name = var->var.name;
            free(var);
            advance_token(); // Consume '('
            
            call->call.args = new_ast_node(NODE_PROGRAM);
            ASTNode* last_arg = call->call.args;
            
            if (current_token.type != TOKEN_RPAREN) {
                last_arg->next = expression();
                while (last_arg->next) last_arg = last_arg->next;
                while (current_token.type == TOKEN_COMMA) {
                    advance_token();
                    last_arg->next = expression();
                    while (last_arg->next) last_arg = last_arg->next;
                }
            }
            
            expect(TOKEN_RPAREN);
            advance_token();
            return call;
        }
        
        return var;
    }
    else if (current_token.type == TOKEN_LPAREN) {
        advance_token();
        ASTNode* expr = expression();
        expect(TOKEN_RPAREN);
        advance_token();
        return expr;
    }
    
    error("Expected primary expression");
    return NULL;
}