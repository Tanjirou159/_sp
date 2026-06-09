#include "tiny_compiler.h"

ASTNode* program();
ASTNode* function();
ASTNode* statement();
ASTNode* for_statement();
ASTNode* block();
ASTNode* expression();
ASTNode* logical_or();
ASTNode* logical_and();
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
    // Skip parameters parsing
    while (current_token.type != TOKEN_RPAREN && current_token.type != TOKEN_EOF) {
        if (current_token.type == TOKEN_IDENT) advance_token();
        if (current_token.type == TOKEN_COLON) advance_token();
        if (current_token.type == TOKEN_INT) advance_token();
        if (current_token.type == TOKEN_BOOL) advance_token();
        if (current_token.type == TOKEN_STRING_TYPE) advance_token();
        if (current_token.type == TOKEN_COMMA) advance_token();
    }
    expect(TOKEN_RPAREN);
    advance_token();

    if (current_token.type == TOKEN_COLON) {
        advance_token();
        if (current_token.type == TOKEN_INT || current_token.type == TOKEN_BOOL) {
            func->func.ret_type = strdup(current_token.lexeme);
            advance_token();
        } else {
            func->func.ret_type = strdup("void");
        }
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

ASTNode* block() {
    ASTNode* blk = new_ast_node(NODE_PROGRAM);
    ASTNode* last = blk;

    while (current_token.type != TOKEN_RBRACE && current_token.type != TOKEN_EOF) {
        last->next = statement();
        while (last->next) last = last->next;
    }

    return blk;
}

ASTNode* statement() {
    if (current_token.type == TOKEN_VAR) {
        advance_token();
        expect(TOKEN_IDENT);

        ASTNode* decl = new_ast_node(NODE_VAR_DECL);
        decl->var_decl.name = strdup(current_token.lexeme);
        decl->var_decl.is_array = false;
        decl->var_decl.array_size = 0;
        advance_token();

        expect(TOKEN_COLON);
        advance_token();

        if (current_token.type == TOKEN_INT || current_token.type == TOKEN_BOOL ||
            current_token.type == TOKEN_STRING_TYPE) {
            decl->var_decl.var_type = strdup(current_token.lexeme);
            advance_token();

            // Check for array declaration: int[size]
            if (current_token.type == TOKEN_LBRACKET) {
                advance_token();
                expect(TOKEN_NUMBER);
                decl->var_decl.is_array = true;
                decl->var_decl.array_size = current_token.int_value;
                advance_token();
                expect(TOKEN_RBRACKET);
                advance_token();
            }
        }

        if (current_token.type == TOKEN_ASSIGN) {
            advance_token();
            decl->var_decl.init = expression();
        } else if (decl->var_decl.is_array) {
            // Arrays auto-initialized to zero by default
            decl->var_decl.init = NULL;
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
        else if (current_token.type == TOKEN_LBRACKET) {
            // Array assignment: arr[index] = value
            advance_token();
            ASTNode* arr_assign = new_ast_node(NODE_ARRAY_ASSIGN);
            arr_assign->array_assign.name = name;
            arr_assign->array_assign.index = expression();
            expect(TOKEN_RBRACKET);
            advance_token();
            expect(TOKEN_ASSIGN);
            advance_token();
            arr_assign->array_assign.value = expression();
            expect(TOKEN_SEMICOLON);
            advance_token();
            return arr_assign;
        }
        else if (current_token.type == TOKEN_LPAREN) {
            // Function call as statement
            ASTNode* call = new_ast_node(NODE_CALL);
            call->call.name = name;
            advance_token();

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
            expect(TOKEN_SEMICOLON);
            advance_token();
            return call;
        }
        else {
            error("Expected assignment, array access, or function call after identifier");
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

        if (current_token.type == TOKEN_LBRACE) {
            advance_token();
            while (current_token.type != TOKEN_RBRACE && current_token.type != TOKEN_EOF) {
                then_last->next = statement();
                while (then_last->next) then_last = then_last->next;
            }
            expect(TOKEN_RBRACE);
            advance_token();
        } else {
            then_last->next = statement();
        }

        if (current_token.type == TOKEN_ELSE) {
            advance_token();
            if_node->if_stmt.else_branch = new_ast_node(NODE_PROGRAM);
            ASTNode* else_last = if_node->if_stmt.else_branch;

            if (current_token.type == TOKEN_LBRACE) {
                advance_token();
                while (current_token.type != TOKEN_RBRACE && current_token.type != TOKEN_EOF) {
                    else_last->next = statement();
                    while (else_last->next) else_last = else_last->next;
                }
                expect(TOKEN_RBRACE);
                advance_token();
            } else if (current_token.type == TOKEN_IF) {
                // else if support
                else_last->next = statement();
            } else {
                error("Expected '{' or 'if' after else");
            }
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

        if (current_token.type == TOKEN_LBRACE) {
            advance_token();
            while (current_token.type != TOKEN_RBRACE && current_token.type != TOKEN_EOF) {
                body_last->next = statement();
                while (body_last->next) body_last = body_last->next;
            }
            expect(TOKEN_RBRACE);
            advance_token();
        } else {
            body_last->next = statement();
        }

        return while_node;
    }
    else if (current_token.type == TOKEN_FOR) {
        return for_statement();
    }
    else if (current_token.type == TOKEN_BREAK) {
        advance_token();
        ASTNode* brk = new_ast_node(NODE_BREAK);
        expect(TOKEN_SEMICOLON);
        advance_token();
        return brk;
    }
    else if (current_token.type == TOKEN_CONTINUE) {
        advance_token();
        ASTNode* cont = new_ast_node(NODE_CONTINUE);
        expect(TOKEN_SEMICOLON);
        advance_token();
        return cont;
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

ASTNode* for_statement() {
    // for (init; cond; incr) { body }
    advance_token(); // consume 'for'
    expect(TOKEN_LPAREN);
    advance_token();

    ASTNode* for_node = new_ast_node(NODE_FOR);

    // Parse init (var decl or expression)
    if (current_token.type != TOKEN_SEMICOLON) {
        if (current_token.type == TOKEN_VAR) {
            advance_token();
            expect(TOKEN_IDENT);

            ASTNode* decl = new_ast_node(NODE_VAR_DECL);
            decl->var_decl.name = strdup(current_token.lexeme);
            decl->var_decl.is_array = false;
            decl->var_decl.array_size = 0;
            advance_token();

            expect(TOKEN_COLON);
            advance_token();

            if (current_token.type == TOKEN_INT || current_token.type == TOKEN_BOOL) {
                decl->var_decl.var_type = strdup(current_token.lexeme);
                advance_token();
            }

            if (current_token.type == TOKEN_ASSIGN) {
                advance_token();
                decl->var_decl.init = expression();
            }

            for_node->for_stmt.init = decl;
        } else {
            for_node->for_stmt.init = expression();
        }
    }

    expect(TOKEN_SEMICOLON);
    advance_token();

    // Parse condition
    if (current_token.type != TOKEN_SEMICOLON) {
        for_node->for_stmt.cond = expression();
    }

    expect(TOKEN_SEMICOLON);
    advance_token();

    // Parse increment
    if (current_token.type != TOKEN_RPAREN) {
        for_node->for_stmt.incr = expression();
    }

    expect(TOKEN_RPAREN);
    advance_token();

    // Parse body
    for_node->for_stmt.body = new_ast_node(NODE_PROGRAM);
    ASTNode* body_last = for_node->for_stmt.body;

    if (current_token.type == TOKEN_LBRACE) {
        advance_token();
        while (current_token.type != TOKEN_RBRACE && current_token.type != TOKEN_EOF) {
            body_last->next = statement();
            while (body_last->next) body_last = body_last->next;
        }
        expect(TOKEN_RBRACE);
        advance_token();
    } else {
        body_last->next = statement();
    }

    return for_node;
}

ASTNode* expression() {
    return logical_or();
}

ASTNode* logical_or() {
    ASTNode* node = logical_and();

    while (current_token.type == TOKEN_OR) {
        ASTNode* binary = new_ast_node(NODE_BINARY);
        binary->binary.op = "||";
        binary->binary.left = node;
        advance_token();
        binary->binary.right = logical_and();
        node = binary;
    }

    return node;
}

ASTNode* logical_and() {
    ASTNode* node = equality();

    while (current_token.type == TOKEN_AND) {
        ASTNode* binary = new_ast_node(NODE_BINARY);
        binary->binary.op = "&&";
        binary->binary.left = node;
        advance_token();
        binary->binary.right = equality();
        node = binary;
    }

    return node;
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

    while (current_token.type == TOKEN_MUL || current_token.type == TOKEN_DIV ||
           current_token.type == TOKEN_MOD) {
        ASTNode* binary = new_ast_node(NODE_BINARY);
        if (current_token.type == TOKEN_MUL) binary->binary.op = "*";
        else if (current_token.type == TOKEN_DIV) binary->binary.op = "/";
        else binary->binary.op = "%";
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

        if (current_token.type == TOKEN_LBRACKET) {
            // Array access: arr[index]
            ASTNode* arr_acc = new_ast_node(NODE_ARRAY_ACCESS);
            arr_acc->array_access.name = var->var.name;
            free(var);
            advance_token();
            arr_acc->array_access.index = expression();
            expect(TOKEN_RBRACKET);
            advance_token();
            return arr_acc;
        }
        else if (current_token.type == TOKEN_LPAREN) {
            // Function call
            ASTNode* call = new_ast_node(NODE_CALL);
            call->call.name = var->var.name;
            free(var);
            advance_token();

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
