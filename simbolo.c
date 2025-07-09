#include "compilador.h"

SymbolTable* create_symbol_table() {
    SymbolTable *table = malloc(sizeof(SymbolTable));
    table->head = NULL;
    table->scope_level = 0;
    return table;
}

void enter_scope(SymbolTable *table) {
    table->scope_level++;
}

void exit_scope(SymbolTable *table) {
    Symbol *prev = NULL;
    Symbol *current = table->head;
    
    while (current != NULL) {
        if (current->scope_level == table->scope_level) {
            if (prev == NULL) {
                table->head = current->next;
                free(current->name);
                free(current->data_type);
                free(current);                
                current = table->head;
            } else {
                prev->next = current->next;
                free(current->name);
                free(current->data_type);
                free(current); 
                current = prev->next;
            }
        } else {
            prev = current;
            current = current->next;
        }
    }
    table->scope_level--;
}

Symbol* find_symbol(SymbolTable *table, const char *name) {
    Symbol *current = table->head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

Symbol* insert_symbol(SymbolTable *table, const char *name, SymbolType type, const char *data_type, int line) {
    Symbol *existing = find_symbol(table, name);
    if (existing != NULL) {
        if (existing->scope_level == table->scope_level) {
            fprintf(stderr, "Erro semântico: '%s' redeclarado (linha %d)\n", name, line);
            return NULL;
        }
    }
    
    Symbol *new_symbol = malloc(sizeof(Symbol));
    new_symbol->name = strdup(name);
    new_symbol->type = type;
    new_symbol->data_type = strdup(data_type);
    new_symbol->scope_level = table->scope_level;
    new_symbol->declared = 1;
    new_symbol->first_occurrence_line = line;
    new_symbol->next = table->head;
    table->head = new_symbol;
    return new_symbol;
}

void print_symbol_table(SymbolTable *table, FILE *out) {
    fprintf(out, "%-20s %-20s %-10s %-10s %s\n", "Nome", "Tipo", "Tipo Dado", "Escopo", "Linha");
    
    int count = 0;
    Symbol *current = table->head;
    while (current) {
        count++;
        current = current->next;
    }
    
    Symbol **symbols = malloc(count * sizeof(Symbol*));
    current = table->head;
    for (int i = 0; i < count; i++) {
        symbols[i] = current;
        current = current->next;
    }
    
    for (int i = count - 1; i >= 0; i--) {
        const char *type_str = (symbols[i]->type == VARIAVEL) ? "Variável" : 
                              (symbols[i]->type == FUNCAO) ? "Função" : "Tipo";
        fprintf(out, "%-15s %-10s %-10s %-10d %d\n", 
                symbols[i]->name, 
                type_str, 
                symbols[i]->data_type,
                symbols[i]->scope_level,
                symbols[i]->first_occurrence_line);
    }
}

void check_undeclared_symbols(SymbolTable *table) {
    Symbol *current = table->head;
    while (current != NULL) {
        if (!current->declared) {
            fprintf(stderr, "Erro semântico: '%s' não declarado (linha %d)\n", 
                    current->name, current->first_occurrence_line);
        }
        current = current->next;
    }
}

void free_symbol_table(SymbolTable *table) {
    if (!table) return;

    Symbol *current = table->head;
    while (current != NULL) {
        Symbol *next = current->next;
        free(current->name);
        free(current->data_type);
        free(current);
        current = next;
    }
    free(table);
}