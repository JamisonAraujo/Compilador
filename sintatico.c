#include "compilador.h"

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
    
    printf("%d %d lexema: %s-%s, fila: %s-%s\n", token->row, token->col, lexema, token->lexema, fila, token->fila);
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

// === Funções de parsing ===

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
        else if (current_token(state, "KEYWORD", "printf")) {
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
    
    // Tipo da variável
    Token *type_token = consume_token(state);
    add_descendant(decl_node, create_node("TYPE", type_token->fila));
    
    // Nome da variável
    if (!current_token(state, "IDENTIFIER", NULL))
        syntax_error("identificador", state);
    Token *name_token = consume_token(state);
    add_descendant(decl_node, create_node("NAME", name_token->fila));
    
    // Inicialização opcional
    if (current_token(state, "OPERATOR", "=")) {
        consume_token(state); // Consome o '='
        add_descendant(decl_node, parse_expression(state));
    }
    
    // Ponto e vírgula obrigatório
    if (!current_token(state, "SEPARATOR", ";"))
        syntax_error(";", state);
    consume_token(state);
    
    return decl_node;
}

SyntaxNode *parse_assignment(ParserState *state) {
    SyntaxNode *assign_node = create_node("ASSIGNMENT", "");
    
    // Nome da variável
    Token *var_token = consume_token(state);
    add_descendant(assign_node, create_node("VARIABLE", var_token->fila));
    
    // Operador de atribuição
    if (!current_token(state, "OPERATOR", "="))
        syntax_error("=", state);
    consume_token(state);
    add_descendant(assign_node, create_node("OPERATOR", "="));
    
    // Expressão
    add_descendant(assign_node, parse_expression(state));
    
    // Ponto e vírgula obrigatório
    if (!current_token(state, "SEPARATOR", ";"))
        syntax_error(";", state);
    consume_token(state);
    
    return assign_node;
}

SyntaxNode *parse_function_call(ParserState *state) {
    SyntaxNode *func_node = create_node("FUNCTION_CALL", "");
    
    // Nome da função
    Token *func_token = consume_token(state);
    add_descendant(func_node, create_node("FUNCTION", func_token->fila));
    
    // Parêntese de abertura
    if (!current_token(state, "SEPARATOR", "("))
        syntax_error("(", state);
    consume_token(state);
    
    // Argumentos
    SyntaxNode *args_node = create_node("ARGUMENTS", "");
    add_descendant(func_node, args_node);
    
    // Primeiro argumento (literal)
    if (current_token(state, "LITERAL", NULL)) {
        Token *arg_token = consume_token(state);
        add_descendant(args_node, create_node("ARGUMENT", arg_token->fila));
    } else {
        syntax_error("string literal", state);
    }
    
    // Vírgula separadora
    if (!current_token(state, "SEPARATOR", ","))
        syntax_error(",", state);
    consume_token(state);
    
    // Segundo argumento (variável)
    if (current_token(state, "IDENTIFIER", NULL)) {
        Token *arg_token = consume_token(state);
        add_descendant(args_node, create_node("ARGUMENT", arg_token->fila));
    } else {
        syntax_error("identificador", state);
    }
    
    // Parêntese de fechamento
    if (!current_token(state, "SEPARATOR", ")"))
        syntax_error(")", state);
    consume_token(state);
    
    // Ponto e vírgula obrigatório
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
    // return type
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
    if (!current_token(state, "SEPARATOR", "("))
        syntax_error("(", state);
    consume_token(state);

    if (!current_token(state, "SEPARATOR", ")"))
        syntax_error(")", state);
    consume_token(state);
    add_descendant(node, parse_block(state));
    return node;
}

void analisar_sintatico(TokenArray *tokens){
    ParserState state;
    state.tokens = tokens;
    state.pos = 0;    
    FILE *tree = fopen("tree.txt", "w");
    SyntaxNode *root = create_node("Program", "");

    while (current_token(&state, "DIRECTIVE", NULL)) {
        add_descendant(root, create_node("DIRECTIVE", state.tokens->token[state.pos].fila));
        state.pos++;
    }
    
    while (state.pos < tokens->count) {
        add_descendant(root, parse_function(&state));
    }

    print_tree(root, 0, tree);
    fclose (tree);
}