#include "compilador.h"

const char *palavras_chave[] = {
    "if", "else", "do", "while", "for", "return", "struct",
    "break", "continue", "switch", "case", "printf", "scanf","default"
};
const char *types[] = {
    "int", "float", "char", "void"
};
const char separadores[] = {' ', ',', ';', '\n', '\r', '(', ')','{', '}'};
const char operadores[] = {'=', '+', '-', '/','*'};
const char *operadores_duplos[] = {"++", "--", "==", "||", };

void analisador_lexico(FILE *codigo, TokenArray *tokens){
    int pos = 0;
    int col = 0;
    int row = 0;

    char c = fgetc(codigo);
    FILE *saida = fopen("saida.txt", "w");
    while (c != EOF){
        int tipo;
        char fila[FILA_MAX] = {0};
        int cont = 0;
        
        while (isspace(c)) {
            if (c == '\n') {
                row++;
                col = 0;
            } if (c == '\t'){
                col +=4;
            } else {
                col++;
            }
            c = fgetc(codigo);
        }

        if (isalpha(c)){
            tipo = 1; //identificador
        } else if(isdigit(c)){
            tipo = 2; //numero
        } else if (isOperador(c)) {
            tipo = 3; //operador (se for - pode  ser número negativo)
        } else if(c == '\'' || c == '\"'){
            tipo = 4; //identifica inicio de um literal
        } else if (c == '#'){
            tipo = 5; //identifica diretivas
        } else if (isSeparador(c)){
            tipo = 6; // separadores
        } else {
            tipo = 11;
        }
        
        
        
        fila[cont++] = c;
        if(tipo != 6){

            while ((c = fgetc(codigo)) != EOF){
                if (tipo == 4) { 
                    fila[cont++] = c;
                    if (c == fila[0] && fila[0] != '\\') {
                        break;
                    } 
                    continue;
                }
    
                if (tipo == 5) { // Diretivas
                    if (c == '\n' || c == '\r') {
                        break;
                    }
                    fila[cont++] = c;
                    continue;
                }
    
                if (!isSeparador(c)) {
                    fila[cont++] = c;
                } else {
                    ungetc(c, codigo);
                    break;
                }
                
            }
        }

        int tipoLexema = DefineLexema(fila, tipo);
        if (tipoLexema != 10) { 
            CriaTabela(saida, pos++, fila, tipoLexema, col, row, tokens);
        }

        if (c == '\n') {
            row++;
            col = 0;
        } else if (c == '\t') {
            col += 4;
        } else {
            col++;
        }
        c = fgetc(codigo);
        
    }
    fclose(saida);
}

int DefineLexema(char *fila, int tipo){
    int tamanho = strlen (fila);
    //return 0: palavra-chave
    //return 1: identificador
    //return 2: numero
    //return 3: operador
    //return 4: tipo
    //return 7: literal
    //return 8: diretiva
    //return 9: separador
    //return 10: default
    //return 11: erro 
    if (tipo == 1){

        if(isReservada(fila)){
            return 0;
        }
        if(isType(fila)){
            return 4;
        }

        for (int i = 0; i < tamanho; i++){
            if (i != 0){
                if (!isalnum(fila[i]) && fila[i] != '_'){
                    return 11;
                }
            } 
        }

        return 1;
    }

    if (tipo == 2){
        for (int i = 0; i < tamanho; i++){
            if (!isdigit(fila[i])){
                return 11;
            }
        }
        return 2;
    }

    if (tipo == 3){
        if (isOperadorDuplo(fila)){
            return 3;
        }

        if (strlen (fila) == 1 && isOperador(fila[0])){
            return 3;
        }
        if (fila[0] == '-'){
            if (isdigit(fila[1])){
                return 2;
            }
        }
        return 11;
    }

    if (tipo == 4) { 
        if (tamanho >= 2 && fila[0] == fila[tamanho-1]) {
            return 7; 
        }
        return 11; 
    }

    if (tipo == 5) { 
        return 8; 
    }

    if (tipo == 6 && (fila[0] != '\n' || fila[0] != '\r' || fila[0] != '\t' || fila[0] != ' ')) { 
        return 9; 
    }

    return 10;
}

void CriaTabela(FILE *saida, int pos, char *fila, int tipoLexema, int col, int row, TokenArray *tokens){
    char *lexema;
   
    switch(tipoLexema) {
        case 0: lexema = "KEYWORD"; break;
        case 1: lexema = "IDENTIFIER"; break;
        case 2: lexema = "NUMBER"; break;
        case 3: lexema = "OPERATOR"; break;
        case 4: lexema = uppercase(fila); break;
        case 7: lexema = "LITERAL"; break;
        case 8: lexema = "DIRECTIVE"; break;
        case 9: lexema = "SEPARATOR"; break;
        default: lexema = "ERROR"; break;
    }
    Token new_token;
    
    new_token.id = pos+1;
    new_token.fila = strdup(fila);
    new_token.lexema = strdup(lexema);
    new_token.col = col+1;
    new_token.row = row+1;
            
    add_token(tokens, new_token);
    fprintf(saida, "%3d | %-40s | %-10s | Linha:%3d Col:%3d\n", 
        new_token.id, new_token.fila, new_token.lexema, new_token.row, new_token.col);   

}


int isSeparador(char c) {
    int tamanho = sizeof(separadores)/sizeof(separadores[0]);
    for (size_t i = 0; i < tamanho; i++) {
        if (c == separadores[i]) {
            return 1;
        }
    }
    return 0;
}


int isReservada(char *fila){
    int tamanho = sizeof(palavras_chave) / sizeof(palavras_chave[0]);

    for (int i = 0; i < tamanho; i++){
        if (!strcmp(fila, palavras_chave[i])){
            return 1;
        }
    }
    return 0;
}

int isOperador(char c) {
    int tamanho = sizeof(operadores)/sizeof(operadores[0]);
    for (size_t i = 0; i < tamanho; i++) {
        if (c == operadores[i]){
            return 1;
        }
    }
    return 0;
}

int isType(char *fila){
    int tamanho = sizeof(types) / sizeof(types[0]);

    for (int i = 0; i < tamanho; i++){
        if (!strcmp(fila, types[i])){
            return 1;
        }
    }
    return 0;
}

int isOperadorDuplo(const char *token) {
    if (strlen(token) < 2) {
        return 0;
    }

    int num_operadores = sizeof(operadores_duplos) / sizeof(operadores_duplos[0]);
    
    for (int i = 0; i < num_operadores; i++) {
        if (strncmp(token, operadores_duplos[i], 2) == 0) {
            return 1; 
        }
    }
    return 0;
}

char *uppercase(char *string){

    if (string == NULL) return NULL;
    char *upperString = malloc(strlen(string) + 1);
    int i = 0;
    while(string[i]){
        upperString[i] = toupper((unsigned char)string[i]);
        i++;    
    }
    upperString[i] = '\0'; 
    return upperString;
}


void init_token_array(TokenArray *arr, int initial_capacity) {
    arr->token = malloc(initial_capacity * sizeof(Token));
    arr->count = 0;
    arr->capacity = initial_capacity;
}


void add_token(TokenArray *arr, Token token) {
    if (arr->count >= arr->capacity) {
        arr->capacity *= 2;
        Token *temp = realloc(arr->token, arr->capacity * sizeof(Token));
        if (!temp) {
            fprintf(stderr, "Erro ao realocar memória!\n");
            exit(EXIT_FAILURE);
        }
        arr->token = temp;
    }
    
    arr->token[arr->count] = token;
    arr->count++;
}


void free_token_array(TokenArray *arr) {
    free(arr->token);
    arr->token = NULL;
    arr->count = arr->capacity = 0;
}