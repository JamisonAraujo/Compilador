#ifndef GERADOR_H
#define GERADOR_H

#include "compilador.h"

typedef struct {
    char *name;
    int offset;
} VariableOffset;

typedef struct {
    VariableOffset *variables;
    int count;
    int capacity;
    int next_offset;
} OffsetTable;

void init_offset_table(OffsetTable *table);
void add_variable_offset(OffsetTable *table, const char *name, int offset);
int get_variable_offset(OffsetTable *table, const char *name);
void generate_code(SyntaxNode *root, SymbolTable *sym_table, FILE *out);

#endif