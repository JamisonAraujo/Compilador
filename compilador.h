#ifndef COMPILADOR_H

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
    Token *token;     // Array de tokens
    int count;       // Número de tokens usados
    int capacity;    // Capacidade total do array
} TokenArray;

typedef struct SintaxNode {
    char type[50];
    char value[100];
    struct SintaxNode *descendants[10];
    int num_descendant;
} SintaxNode;

void analisar_sintatico(TokenArray *tokens);

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

#endif