#include "compilador.h"

SintaxNode *create_node(const char *type, const char *value){
    SintaxNode *node = malloc(sizeof(SintaxNode));
    strcpy(node->type, type);
    strcpy(node->value, value);
    node->num_descendant = 0;
    return node;
}

void add_descendant(SintaxNode *parent, SintaxNode *descendant){
    if (parent->num_descendant < 10)
    {
        parent->descendants[parent->num_descendant++] = descendant;
    }
}

void imprimir_arvore(SintaxNode *node, int nivel){
    for (int i = 0; i < nivel; i++)
        printf("  ");
    if (strlen(node->value) > 0)
        printf("%s: %s\n", node->type, node->value);
    else
        printf("%s\n", node->type);

    for (int i = 0; i < node->num_descendant; i++)
    {
        imprimir_arvore(node->descendants[i], nivel + 1);
    }
}

int token_atual_e(const char *lexema, const char *fila, TokenArray *tokens, int pos){

    return pos < tokens->count &&
           strcmp(tokens->token[pos].lexema, lexema) == 0 && (fila == NULL || strcmp(tokens->token[pos].fila, fila) == 0);
}

Token *consumir_token(TokenArray *tokens, int pos){
    return &tokens->token[pos];
}

void erro_sintatico(const char *expected, TokenArray *tokens, int pos){
    if (pos < tokens->count){
        printf("%d %d ERRO SINTÁTICO: Esperado %s\n", tokens->token[pos].row, tokens->token[pos].col, expected);
    }
    else {
        printf("ERRO SINTÁTICO: Esperado %s no final do arquivo\n", expected);
    }
    exit(1);
}

// === Funções de parsing ===

SintaxNode *parse_bloco(TokenArray *tokens, int pos);
SintaxNode *parse_comando(TokenArray *tokens, int pos);
SintaxNode *parse_expressao(TokenArray *tokens, int pos);

SintaxNode *parse_if(TokenArray *tokens, int pos){
    SintaxNode *no = create_node("IF", "");

    consumir_token(tokens, pos); // 'if'

    if (!token_atual_e("SEPARATOR", "(", tokens, pos))
        erro_sintatico("SEPARATOR '('", tokens, pos);
    consumir_token(tokens, pos);

    add_descendant(no, parse_expressao(tokens, pos));

    if (!token_atual_e("SEPARATOR", ")", tokens, pos))
        erro_sintatico("SEPARATOR ')'", tokens, pos);
    consumir_token(tokens, pos);

    add_descendant(no, parse_comando(tokens, pos));

    if (token_atual_e("KEYWORD", "else", tokens, pos))
    {
        consumir_token(tokens, pos);
        add_descendant(no, parse_comando(tokens, pos));
    }

    return no;
}

SintaxNode *parse_while(TokenArray *tokens, int pos){
    SintaxNode *node = create_node("WHILE", "");
    consumir_token(tokens, pos); // 'while'

    if (!token_atual_e("SEPARATOR", "(", tokens, pos))
        erro_sintatico("SEPARATOR '('", tokens, pos);
    consumir_token(tokens, pos);

    add_descendant(node, parse_expressao(tokens, pos));

    if (!token_atual_e("SEPARATOR", ")", tokens, pos))
        erro_sintatico("SEPARATOR ')'", tokens, pos);
    consumir_token(tokens, pos);

    add_descendant(node, parse_comando(tokens, pos));
    return node;
}

SintaxNode *parse_for(TokenArray *tokens, int pos){
    SintaxNode *no = create_node("FOR", "");
    consumir_token(tokens, pos); // 'for'

    if (!token_atual_e("SEPARATOR", "(", tokens, pos))
        erro_sintatico("SEPARATOR '('", tokens, pos);
    consumir_token(tokens, pos);

    add_descendant(no, parse_expressao(tokens, pos));
    if (!token_atual_e("SEPARATOR", ";", tokens, pos))
        erro_sintatico("SEPARATOR ';'", tokens, pos);
    consumir_token(tokens, pos);

    add_descendant(no, parse_expressao(tokens, pos));
    if (!token_atual_e("SEPARATOR", ";", tokens, pos))
        erro_sintatico("SEPARATOR ';'", tokens, pos);
    consumir_token(tokens, pos);

    add_descendant(no, parse_expressao(tokens, pos));
    if (!token_atual_e("SEPARATOR", ")", tokens, pos))
        erro_sintatico("SEPARATOR ')'", tokens, pos);
    consumir_token(tokens, pos);

    add_descendant(no, parse_comando(tokens, pos));
    return no;
}

SintaxNode *parse_return(TokenArray *tokens, int pos){
    SintaxNode *no = create_node("RETURN", "");
    consumir_token(tokens, pos); // 'return'

    add_descendant(no, parse_expressao(tokens, pos));

    if (!token_atual_e("SEPARATOR", ";", tokens, pos))
        erro_sintatico("SEPARATOR ';'", tokens, pos);
    consumir_token(tokens, pos);
    return no;
}

SintaxNode *parse_bloco(TokenArray *tokens, int pos){
    if (!token_atual_e("SEPARATOR", "{", tokens, pos))
        erro_sintatico("SEPARATOR '{'", tokens, pos);
    consumir_token(tokens, pos);

    SintaxNode *no = create_node("BLOCO", "");
    while (!token_atual_e("SEPARATOR", "}", tokens, pos))
    {
        add_descendant(no, parse_comando(tokens, pos));
    }

    consumir_token(tokens, pos); // '}'
    return no;
}

SintaxNode *parse_expressao(TokenArray *tokens, int pos){
    SintaxNode *no = create_node("EXPR", "");
    int parenteses_abertos = 0;
    int num_tokens = tokens->count;
    while (pos < num_tokens) {
        Token *token = tokens[pos].token;
        if (token_atual_e("SEPARATOR", "(", tokens, pos))
            parenteses_abertos++;

        if (token_atual_e("SEPARATOR", ")", tokens, pos))
        {
            if (parenteses_abertos == 0)
                break;
            parenteses_abertos--;
        }

        if (parenteses_abertos == 0 && token_atual_e("SEPARATOR", ";", tokens, pos))
            break;

        add_descendant(no, create_node(token->lexema, token->fila));
        consumir_token(tokens, pos);
    }

    return no;
}

SintaxNode *parse_comando(TokenArray *tokens, int pos){
    if (token_atual_e("KEYWORD", "if", tokens, pos))
        return parse_if(tokens, pos);
    if (token_atual_e("KEYWORD", "while", tokens, pos))
        return parse_while(tokens, pos);
    if (token_atual_e("KEYWORD", "for", tokens, pos))
        return parse_for(tokens, pos);
    if (token_atual_e("KEYWORD", "return", tokens, pos))
        return parse_return(tokens, pos);
    if (token_atual_e("SEPARATOR", "{", tokens, pos))
        return parse_bloco(tokens, pos);

    // Expressão simples
    SintaxNode *no = create_node("COMANDO", "");
    add_descendant(no, parse_expressao(tokens, pos));
    if (!token_atual_e("SEPARATOR", ";", tokens, pos))
        erro_sintatico("SEPARATOR ';'", tokens, pos);
    consumir_token(tokens, pos);
    return no;
}

SintaxNode *parse_funcao(TokenArray *tokens, int pos){
    SintaxNode *node = create_node("FUNCAO", "");
    Token *token = tokens[pos].token;
    // Espera: tipo retorno
    if (!token_atual_e("KEYWORD", NULL, tokens, pos))
        erro_sintatico("INT", tokens, pos);
    add_descendant(node, create_node("Tipo", token->fila));
    consumir_token(tokens, pos);

    // Espera: nome
    if (!token_atual_e("IDENTIFICADOR", NULL, tokens, pos))
        erro_sintatico("IDENTIFICADOR", tokens, pos);
    add_descendant(node, create_node("Nome", token->fila));
    consumir_token(tokens, pos);

    if (!token_atual_e("SEPARATOR", "(", tokens, pos))
        erro_sintatico("SEPARATOR '('", tokens, pos);
    consumir_token(tokens, pos);

    if (!token_atual_e("SEPARATOR", ")", tokens, pos))
        erro_sintatico("SEPARATOR ')'", tokens, pos);
    consumir_token(tokens, pos);

    add_descendant(node, parse_bloco(tokens, pos));
    return node;
}

void analisar_sintatico(TokenArray *tokens){
    int pos = 0;
    SintaxNode *raiz = create_node("Programa", "");

     // Ignora diretivas no início
    while (pos < tokens->count && token_atual_e("DIRECTIVE", NULL, tokens, pos)) {
        add_descendant(raiz, create_node("DIRECTIVE", tokens->token[pos].fila));
        pos++;
    }
    
    // Parse das funções
    while (pos < tokens->count) {
        SintaxNode *funcao = parse_funcao(tokens, pos);
        add_descendant(raiz, funcao);
    }

    imprimir_arvore(raiz, 0);
}