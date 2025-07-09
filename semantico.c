#include "compilador.h"

void check_variable_declaration(SymbolTable *table, Token *type_token, Token *name_token) {
    insert_symbol(table, name_token->fila, VARIAVEL, type_token->fila, name_token->row);
}

void check_function_declaration(SymbolTable *table, Token *type_token, Token *name_token) {
    insert_symbol(table, name_token->fila, FUNCAO, type_token->fila, name_token->row);
}

void check_variable_usage(SymbolTable *table, Token *var_token) {
    Symbol *sym = find_symbol(table, var_token->fila);
    if (!sym) {
        Symbol *new_sym = malloc(sizeof(Symbol));
        new_sym->name = strdup(var_token->fila);
        new_sym->type = VARIAVEL;
        new_sym->data_type = strdup("UNKNOWN");
        new_sym->scope_level = table->scope_level;
        new_sym->declared = 0;
        new_sym->first_occurrence_line = var_token->row;
        new_sym->next = table->head;
        table->head = new_sym;
        
        fprintf(stderr, "Aviso: variável '%s' usada antes de declará-la (linha %d)\n",
                var_token->fila, var_token->row);
    }
}


void check_assignment(SymbolTable *table, Token *var_token, SyntaxNode *expr_node, int line) {
    Symbol *var_sym = find_symbol(table, var_token->fila);
    if (!var_sym) return;

    char *expr_type = infer_expr_type(table, expr_node);
    if (strcmp(var_sym->data_type, expr_type) != 0 && (strcmp(var_sym->data_type, "unknown") == 1)) {
        fprintf(stderr, "Erro semântico: tipos incompatíveis (esperado %s, recebido %s) na linha %d\n",
                var_sym->data_type, expr_type, line);
    }
}

void check_function_call(SymbolTable *table, Token *func_token, int line) {
        
    Symbol *sym = find_symbol(table, func_token->fila);
    if (!sym || sym->type != FUNCAO) {
        fprintf(stderr, "Erro semântico: função '%s' não declarada (linha %d)\n",
                func_token->fila, line);
    }
}

void check_return_type(SymbolTable *table, SyntaxNode *return_node, const char *expected_type, int line) {
    if (strstr(return_node->value, "\"") != NULL) { 
        if (strcmp(expected_type, "char*") != 0) {
            fprintf(stderr, "Erro semântico: retorno de string em função do tipo %s (linha %d)\n",
                    expected_type, line);
        }
    } else {
        if (strcmp(expected_type, "int") != 0 && strcmp(expected_type, "float") != 0) {
            fprintf(stderr, "Erro semântico: retorno de número em função do tipo %s (linha %d)\n",
                    expected_type, line);
        }
    }
}

static char* bin_op_result_type(char* left_type, char* op, char* right_type) {
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 || 
        strcmp(op, "*") == 0 || strcmp(op, "/") == 0) {
        
        if (strcmp(left_type, "float") == 0 || strcmp(right_type, "float") == 0) {
            return "float";
        }
        if (strcmp(left_type, "int") == 0 && strcmp(right_type, "int") == 0) {
            return "int";
        }
        fprintf(stderr, "Erro semântico: operação inválida entre %s e %s\n", left_type, right_type);
        return "unknown";
    }
    
    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 || 
        strcmp(op, "<") == 0 || strcmp(op, ">") == 0 || 
        strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0) {
        return "int"; 
    }
    
    if (strcmp(op, "&&") == 0 || strcmp(op, "||") == 0) {
        if (strcmp(left_type, "int") == 0 && strcmp(right_type, "int") == 0) {
            return "int";
        }
        fprintf(stderr, "Erro semântico: operação lógica requer operandos inteiros\n");
        return "unknown";
    }
    
    // Atribuição
    if (strcmp(op, "=") == 0) {
        if (strcmp(left_type, right_type) == 0) {
            return left_type;
        }
        if (strcmp(left_type, "float") == 0 && strcmp(right_type, "int") == 0) {
            return left_type;
        }
        if(strcmp(left_type, "int") == 0 && strcmp(right_type, "float") == 0) {
            fprintf(stderr, "Aviso: conversão implícita de float para int na atribuição\n");
            return left_type; 
        }
        fprintf(stderr, "Erro semântico: tipos incompatíveis em atribuição: %s = %s\n", 
                left_type, right_type);
        return "unknown";
    }
    
    return "unknown";
}

char* infer_expr_type(SymbolTable *table, SyntaxNode *node) {
    if (node->num_descendant == 0) {
        if (strcmp(node->type, "NUMBER") == 0) {
            return (strchr(node->value, '.') != NULL) ? "float" : "int";
        }
        else if (strcmp(node->type, "LITERAL") == 0) {
            return "char*";
        }
        else if (strcmp(node->type, "IDENTIFIER") == 0) {
            Symbol *sym = find_symbol(table, node->value);
            return sym ? sym->data_type : "unknown";
        }
        return "unknown";
    }

    if (node->num_descendant == 1) {
        SyntaxNode *child = node->descendants[0];
        char *child_type = infer_expr_type(table, child);
        
        if (strcmp(node->type, "OPERATOR") == 0) {
            if (strcmp(node->value, "!") == 0) {
                if (strcmp(child_type, "int") != 0) {
                    fprintf(stderr, "Erro semântico: operador '!' requer operando inteiro\n");
                }
                return "int";
            }
            else if (strcmp(node->value, "-") == 0) {
                if (strcmp(child_type, "int") == 0 || strcmp(child_type, "float") == 0) {
                    return child_type;
                }
                fprintf(stderr, "Erro semântico: operador '-' unário requer tipo numérico\n");
            }
        }
        return child_type;
    }

    if (strcmp(node->type, "FUNCTION_CALL") == 0) {
        for (int i = 0; i < node->num_descendant; i++) {
            if (strcmp(node->descendants[i]->type, "FUNCTION") == 0) {
                Symbol *func_sym = find_symbol(table, node->descendants[i]->value);
                return func_sym ? func_sym->data_type : "unknown";
            }
        }
        return "unknown";
    }

    if (node->num_descendant >= 3) {
        if (strcmp(node->descendants[0]->type, "SEPARATOR") == 0 && 
            strcmp(node->descendants[0]->value, "(") == 0 &&
            strcmp(node->descendants[node->num_descendant-1]->type, "SEPARATOR") == 0 && 
            strcmp(node->descendants[node->num_descendant-1]->value, ")") == 0) {
            
            SyntaxNode *sub_expr = create_node("EXPRESSION", "");
            
            for (int i = 1; i < node->num_descendant - 1; i++) {
                add_descendant(sub_expr, node->descendants[i]);
            }
            
            char *type = infer_expr_type(table, sub_expr);
            
            free(sub_expr);
            
            return type;
        }
    }

    int split_index = -1;
    int min_precedence = 100;
    int paren_level = 0;
    
    for (int i = 0; i < node->num_descendant; i++) {
        SyntaxNode *child = node->descendants[i];
        
        if (strcmp(child->type, "SEPARATOR") == 0) {
            if (strcmp(child->value, "(") == 0) paren_level++;
            else if (strcmp(child->value, ")") == 0) paren_level--;
            continue;
        }
        
        if (paren_level > 0) continue;
        
        if (strcmp(child->type, "OPERATOR") == 0) {
            int prec = 0;
            char *op = child->value;
            
            if (strcmp(op, "*") == 0 || strcmp(op, "/") == 0) prec = 3;
            else if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) prec = 2;
            else if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 || 
                     strcmp(op, "<") == 0 || strcmp(op, ">") == 0 || 
                     strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0) prec = 1;
            else if (strcmp(op, "&&") == 0 || strcmp(op, "||") == 0) prec = 0;
            else if (strcmp(op, "=") == 0) prec = -1;
            
            if (prec < min_precedence) {
                min_precedence = prec;
                split_index = i;
            }
        }
    }
    
    if (split_index != -1) {
        SyntaxNode *left_expr = create_node("EXPRESSION", "");
        for (int i = 0; i < split_index; i++) {
            if (left_expr->num_descendant < 10) { 
                add_descendant(left_expr, node->descendants[i]);
            }
        }
        
        SyntaxNode *right_expr = create_node("EXPRESSION", "");
        for (int i = split_index + 1; i < node->num_descendant; i++) {
            if (right_expr->num_descendant < 10) { 
                add_descendant(right_expr, node->descendants[i]);
            }
        }
        
        char *left_type = infer_expr_type(table, left_expr);
        char *right_type = infer_expr_type(table, right_expr);
        char *op = node->descendants[split_index]->value;
        
        char *result_type = bin_op_result_type(left_type, op, right_type);
        
        free(left_expr);
        free(right_expr);
        
        return result_type;
    }
    
    return infer_expr_type(table, node->descendants[0]);
}