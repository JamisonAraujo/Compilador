#include "compilador.h"
#include "gerador.h"

SyntaxNode *create_node(const char *type, const char *value){
    SyntaxNode *node = malloc(sizeof(SyntaxNode));
    strcpy(node->type, type);
    strcpy(node->value, value);
    node->num_descendant = 0;
    return node;
}

void add_descendant(SyntaxNode *parent, SyntaxNode *descendant){
    if (parent->num_descendant < 10) {
        parent->descendants[parent->num_descendant++] = descendant;
    }
}

void add_predefined_functions(SymbolTable *table) {
    insert_symbol(table, "printf", FUNCAO, "int", 0);
    insert_symbol(table, "scanf", FUNCAO, "int", 0);
}

Token* current_token_obj(ParserState *state) {
    if (state->pos >= state->tokens->count) return NULL;
    return &state->tokens->token[state->pos];
}

void print_tree(SyntaxNode *node, int level, FILE *tree){
    
    for (int i = 0; i < level; i++)
        fprintf(tree, "  ");
    if (strlen(node->value) > 0)
        fprintf(tree, "%s: %s\n", node->type, node->value);
    else
        fprintf(tree, "%s\n", node->type);

    for (int i = 0; i < node->num_descendant; i++) {
        print_tree(node->descendants[i], level + 1, tree);
    }

}

int current_token(ParserState *state, const char *lexema, const char *fila){
    if (state->pos >= state->tokens->count) return 0;
    
    Token *token = &state->tokens->token[state->pos];
    
    return strcmp(token->lexema, lexema) == 0 && (fila == NULL || strcmp(token->fila, fila) == 0);
}

Token *consume_token(ParserState *state){
    return &state->tokens->token[state->pos++];
}

void syntax_error(const char *expected, ParserState *state){
    Token *token = &state->tokens->token[state->pos];

    if (state->pos < state->tokens->count){
        printf("%d %d ERRO SINTÁTICO: Esperado %s\n", token->row, token->col, expected);
    } else {
        printf("%d\n", state->pos);
        printf("ERRO SINTÁTICO: Esperado %s no final do arquivo\n", expected);
    }
    exit(1);
}

//  Funções de parsing

SyntaxNode *parse_block(ParserState *state);
SyntaxNode *parse_command(ParserState *state);
SyntaxNode *parse_expression(ParserState *state);

SyntaxNode *parse_if(ParserState *state){
    SyntaxNode *node = create_node("IF", "");

    consume_token(state); // 'if'

    if (!current_token(state, "SEPARATOR", "("))
        syntax_error("SEPARATOR '('", state);
    consume_token(state);


    add_descendant(node, parse_expression(state));

    if (!current_token(state, "SEPARATOR", ")"))
        syntax_error("SEPARATOR ')'", state);
    consume_token(state);

    add_descendant(node, parse_command(state));

    if (current_token(state, "KEYWORD", "else")) {
        consume_token(state);
        add_descendant(node, parse_command(state));
    }

    return node;
}

SyntaxNode *parse_while(ParserState *state){
    SyntaxNode *node = create_node("WHILE", "");
    consume_token(state); // 'while'

    if (!current_token(state, "SEPARATOR", "("))
        syntax_error("SEPARATOR '('", state);
    consume_token(state);

    add_descendant(node, parse_expression(state));

    if (!current_token(state, "SEPARATOR", ")"))
        syntax_error("SEPARATOR ')'", state);
    consume_token(state);

    add_descendant(node, parse_command(state));
    return node;
}

SyntaxNode *parse_for(ParserState *state){
    SyntaxNode *no = create_node("FOR", "");
    consume_token(state); // 'for'

    if (!current_token(state, "SEPARATOR", "("))
        syntax_error("SEPARATOR '('", state);
    consume_token(state);

    add_descendant(no, parse_expression(state));
    if (!current_token(state, "SEPARATOR", ";"))
        syntax_error("SEPARATOR ';'", state);
    consume_token(state);

    add_descendant(no, parse_expression(state));
    if (!current_token(state, "SEPARATOR", ";"))
        syntax_error("SEPARATOR ';'", state);
    consume_token(state);

    add_descendant(no, parse_expression(state));
    if (!current_token(state, "SEPARATOR", ")"))
        syntax_error("SEPARATOR ')'", state);
    consume_token(state);

    add_descendant(no, parse_command(state));
    return no;
}

SyntaxNode *parse_return(ParserState *state){
    SyntaxNode *no = create_node("RETURN", "");
    consume_token(state); // 'return'

    add_descendant(no, parse_expression(state));

    // Verificação semântica
    if (state->symbol_table && state->current_function_type) {
        check_return_type(state->symbol_table, no->descendants[0], 
                         state->current_function_type, current_token_obj(state)->row);
    }

    if (!current_token(state, "SEPARATOR", ";"))
        syntax_error("SEPARATOR ';'", state);
    consume_token(state);
    return no;
}

SyntaxNode *parse_block(ParserState *state){
    if (!current_token(state, "SEPARATOR", "{"))
        syntax_error("SEPARATOR '{'", state);
    consume_token(state);
    SyntaxNode *block_node = create_node("BLOCK", "");
    while (!current_token(state, "SEPARATOR", "}")) {
        if (current_token(state, "INT", NULL) || 
            current_token(state, "FLOAT", NULL) || 
            current_token(state, "CHAR", NULL)) {
            add_descendant(block_node, parse_declaration(state));
        } 
        else if (current_token(state, "IDENTIFIER", NULL)) {
            add_descendant(block_node, parse_assignment(state));
        }
        else if (current_token(state, "KEYWORD", "printf") || current_token(state, "KEYWORD", "scanf")) {
            
            add_descendant(block_node, parse_function_call(state));
        }
        else {
            syntax_error("declaração, atribuição ou chamada de função", state);
        }
    }

    if (!current_token(state, "SEPARATOR", "}"))
        syntax_error("}", state);
    consume_token(state);
    
    return block_node;
}

SyntaxNode *parse_declaration(ParserState *state) {
    SyntaxNode *decl_node = create_node("DECLARATION", "");
    
    Token *type_token = consume_token(state);
    add_descendant(decl_node, create_node("TYPE", type_token->fila));
    
    if (!current_token(state, "IDENTIFIER", NULL))
        syntax_error("identificador", state);
    Token *name_token = consume_token(state);
    add_descendant(decl_node, create_node("NAME", name_token->fila));
        
    // Verificação semântica
    if (state->symbol_table) {
        insert_symbol(state->symbol_table, name_token->fila, VARIAVEL, type_token->fila, name_token->row);
    }

    if (current_token(state, "OPERATOR", "=")) {
        consume_token(state); 
        add_descendant(decl_node, parse_expression(state));
    }
    
    if (!current_token(state, "SEPARATOR", ";"))
        syntax_error(";", state);
    consume_token(state);
    
    return decl_node;
}

SyntaxNode *parse_assignment(ParserState *state) {
    SyntaxNode *assign_node = create_node("ASSIGNMENT", "");
    
    Token *var_token = consume_token(state);
    add_descendant(assign_node, create_node("VARIABLE", var_token->fila));
    
    // Verificação semântica
    if (state->symbol_table) {
        check_variable_usage(state->symbol_table, var_token);
    }

    if (!current_token(state, "OPERATOR", "="))
        syntax_error("=", state);
    consume_token(state);

    add_descendant(assign_node, create_node("OPERATOR", "="));
    
    SyntaxNode *expr_node = parse_expression(state); 
    add_descendant(assign_node, expr_node); 

    if (state->symbol_table) {
        check_assignment(state->symbol_table, var_token, expr_node, var_token->row);
    }
    
    if (!current_token(state, "SEPARATOR", ";"))
        syntax_error(";", state);
    consume_token(state);

    return assign_node;
}

SyntaxNode *parse_function_call(ParserState *state) {
    SyntaxNode *func_node = create_node("FUNCTION_CALL", "");
    
    Token *func_token = consume_token(state);
    add_descendant(func_node, create_node("FUNCTION", func_token->fila));

    // Verificação semântica
    if (state->symbol_table) {
        check_function_call(state->symbol_table, func_token, func_token->row);
    }
    if (!current_token(state, "SEPARATOR", "("))
        syntax_error("(", state);
    consume_token(state);
    
    SyntaxNode *args_node = create_node("ARGUMENTS", "");
    add_descendant(func_node, args_node);
    
    while (!current_token(state, "SEPARATOR", ")")) {
        if (current_token(state, "LITERAL", NULL) || current_token(state, "IDENTIFIER", NULL)) {
            Token *arg_token = consume_token(state);
            add_descendant(args_node, create_node("ARGUMENT", arg_token->fila));
        }
        
        if (current_token(state, "SEPARATOR", ",")) {
            consume_token(state);
        } else if (!current_token(state, "SEPARATOR", ")")) {
            syntax_error(", or )", state);
        }
    }

    if (!current_token(state, "SEPARATOR", ")"))
        syntax_error(")", state);
    consume_token(state);
    
    if (!current_token(state, "SEPARATOR", ";"))
        syntax_error(";", state);
    consume_token(state);
    
    return func_node;
}



SyntaxNode *parse_expression(ParserState *state){
    SyntaxNode *no = create_node("EXPRESSION", "");
    int parenteses_abertos = 0;
    while (state->pos < state->tokens->count) {
        Token *token = &state->tokens->token[state->pos];
        if (current_token(state, "SEPARATOR", "("))
            parenteses_abertos++;

        if (current_token(state, "SEPARATOR", ")"))
        {
            if (parenteses_abertos == 0)
                break;
            parenteses_abertos--;
        }

        if (parenteses_abertos == 0 && current_token(state, "SEPARATOR", ";"))
            break;

        add_descendant(no, create_node(token->lexema, token->fila));
        consume_token(state);
    }

    return no;
}

SyntaxNode *parse_command(ParserState *state){
    if (current_token(state, "KEYWORD", "if"))
        return parse_if(state);
    if (current_token(state, "KEYWORD", "while"))
        return parse_while(state);
    if (current_token(state, "KEYWORD", "for"))
        return parse_for(state);
    if (current_token(state, "KEYWORD", "return"))
        return parse_return(state);
    if (current_token(state, "SEPARATOR", "{"))
        return parse_block(state);

    SyntaxNode *node = create_node("COMMAND", "");
    add_descendant(node, parse_expression(state));
    if (!current_token(state, "SEPARATOR", ";"))
        syntax_error("SEPARATOR ';'", state);
    consume_token(state);
    return node;
}

SyntaxNode *parse_function(ParserState *state){
    SyntaxNode *node = create_node("FUNCTION", "");
    Token *token = &state->tokens->token[state->pos];

    if (!current_token(state, "INT", NULL) &&
        !current_token(state, "FLOAT", NULL) &&
        !current_token(state, "CHAR", NULL) &&
        !current_token(state, "VOID", NULL)) {
        syntax_error("type (INT, FLOAT, CHAR, VOID)", state);
    }

    Token *tipo = consume_token(state);
    add_descendant(node, create_node("RETURN_TYPE", tipo->fila));

    if (!current_token(state, "IDENTIFIER", NULL) && !current_token(state, "KEYWORD", "main"))
        syntax_error("identifier", state);
    
    Token *name = consume_token(state);
    add_descendant(node, create_node("NAME", name->fila));
    // Verificação semântica
    if (state->symbol_table) {
        int current_scope = state->symbol_table->scope_level;
        state->symbol_table->scope_level = 0;

        check_function_declaration(state->symbol_table, tipo, name);

        state->symbol_table->scope_level = current_scope;
        state->current_function_type = tipo->fila; 
    }
    
    if (!current_token(state, "SEPARATOR", "("))
    syntax_error("(", state);
    consume_token(state);
    
    if (!current_token(state, "SEPARATOR", ")"))
    syntax_error(")", state);
    consume_token(state);
    enter_scope(state->symbol_table); 
    add_descendant(node, parse_block(state));

    if (state->symbol_table) {
        state->current_function_type = NULL;
    }

    return node;
}

void analisador_sintatico(TokenArray *tokens){
    ParserState state;
    state.tokens = tokens;
    state.pos = 0;    
    state.symbol_table = create_symbol_table();
    state.current_function_type = NULL;
    add_predefined_functions(state.symbol_table);

    FILE *tree = fopen("arvore.txt", "w");
    SyntaxNode *root = create_node("Program", "");

    while (current_token(&state, "DIRECTIVE", NULL)) {
        add_descendant(root, create_node("DIRECTIVE", state.tokens->token[state.pos].fila));
        state.pos++;
    }
    
    while (state.pos < tokens->count) {
        add_descendant(root, parse_function(&state));
    }

    check_undeclared_symbols(state.symbol_table);
    
    // Tabela de símbolos
    FILE *sym_table_file = fopen("tabela_de_simbolos.txt", "w");
    print_symbol_table(state.symbol_table, sym_table_file);

    fclose(sym_table_file);

    print_tree(root, 0, tree);
    fclose (tree);
    FILE *output = fopen("codigo_final.txt", "w");
    generate_code(root, state.symbol_table, output);
    fclose(output);
    free_symbol_table(state.symbol_table);

}