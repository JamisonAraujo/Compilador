#ifndef COMPILADOR_H
#define COMPILADOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h> 
#include <ctype.h>

#define FILA_MAX 256

typedef struct Token {
    int id;
    char *fila;
    char *lexema;
    int row;
    int col;
} Token;

typedef struct {
    Token *token;     
    int count;       
    int capacity;    
} TokenArray;

typedef struct SyntaxNode {
    char type[50];
    char value[100];
    struct SyntaxNode *descendants[10];
    int num_descendant;
} SyntaxNode;

typedef enum {
    VARIAVEL,
    FUNCAO,
    TIPO
} SymbolType;

typedef struct Symbol {
    char *name;
    SymbolType type;
    char *data_type;    
    int scope_level;     
    int declared;        // 1 = declarado, 0 = não declarado
    int first_occurrence_line; 
    struct Symbol *next; 
} Symbol;

typedef struct SymbolTable {
    Symbol *head;
    int scope_level;
} SymbolTable;

typedef struct {
    TokenArray *tokens;
    int pos;
    SymbolTable *symbol_table;
    const char *current_function_type; 
} ParserState;


//lexico.c

void analisador_lexico(FILE *codigo, TokenArray *tokens);
int DefineLexema(char *fila, int tipo);
void CriaTabela(FILE *saida, int pos, char *fila, int tipoLexema, int col, int row, TokenArray *tokens);
int isSeparador(char c);
int isReservada(char *fila);
int isOperador(char c);
int isType(char *fila);
int isOperadorDuplo(const char *token);
char *uppercase(char *string);
void init_token_array(TokenArray *arr, int initial_capacity);
void add_token(TokenArray *arr, Token token);
void free_token_array(TokenArray *arr);

//sintatico.c

void analisador_sintatico(TokenArray *tokens);
SyntaxNode *create_node(const char *type, const char *value);
void add_descendant(SyntaxNode *parent, SyntaxNode *descendant);
SyntaxNode *parse_declaration(ParserState *state);
SyntaxNode *parse_assignment(ParserState *state);
SyntaxNode *parse_function_call(ParserState *state);
SyntaxNode *parse_expression(ParserState *state);

//semantico.c
void check_variable_declaration(SymbolTable *table, Token *type_token, Token *name_token);
void check_function_declaration(SymbolTable *table, Token *type_token, Token *name_token);
void check_variable_usage(SymbolTable *table, Token *var_token);
void check_assignment(SymbolTable *table, Token *var_token, SyntaxNode *expr_node, int line);
void check_function_call(SymbolTable *table, Token *func_token, int line);
void check_return_type(SymbolTable *table, SyntaxNode *return_node, const char *expected_type, int line);
char* infer_expr_type(SymbolTable *table, SyntaxNode *node);

//simbolo.c
SymbolTable* create_symbol_table();
void enter_scope(SymbolTable *table);
void exit_scope(SymbolTable *table);
Symbol* find_symbol(SymbolTable *table, const char *name);
Symbol* insert_symbol(SymbolTable *table, const char *name, SymbolType type, const char *data_type, int line);
void print_symbol_table(SymbolTable *table, FILE *out);
void check_undeclared_symbols(SymbolTable *table);
void free_symbol_table(SymbolTable *table);

#endif