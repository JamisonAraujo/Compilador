#include "lexico.h"

int main (int argc, char *argv[]){


    FILE *codigo = fopen(argv[1], "r");
    if (!codigo) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    TokenArray tokens;
    init_token_array(&tokens, 100);
    
    analisador_lexico(codigo, &tokens);


    fclose(codigo);

    analisar_sintatico(&tokens);
    // Liberar memória
    free_token_array(&tokens);

    
    // Liberar memória
    free_token_array(&tokens);

    return 0;
}