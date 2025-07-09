#include "gerador.h"
#include <stdlib.h>
#include <string.h>

void init_offset_table(OffsetTable *table) {
    table->capacity = 10;
    table->variables = malloc(table->capacity * sizeof(VariableOffset));
    table->count = 0;
    table->next_offset = -4;  
}

void add_variable_offset(OffsetTable *table, const char *name, int offset) {
    if (table->count >= table->capacity) {
        table->capacity *= 2;
        table->variables = realloc(table->variables, 
                                  table->capacity * sizeof(VariableOffset));
    }
    
    table->variables[table->count].name = strdup(name);
    table->variables[table->count].offset = offset;
    table->count++;
}

int get_variable_offset(OffsetTable *table, const char *name) {
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->variables[i].name, name) == 0) {
            return table->variables[i].offset;
        }
    }
    return 0;   
}

void generate_expression(SyntaxNode *node, OffsetTable *offset_table, FILE *out) {
    if (strcmp(node->type, "NUMBER") == 0) {
        fprintf(out, "    movl $%s, %%eax\n", node->value);
        fprintf(out, "    push %%rax\n");
    } 
    else if (strcmp(node->type, "IDENTIFIER") == 0) {
        int offset = get_variable_offset(offset_table, node->value);
        fprintf(out, "    movl %d(%%rbp), %%eax\n", offset);
        fprintf(out, "    push %%rax\n");
    }
    else if (strcmp(node->type, "OPERATOR") == 0) {
        if (node->num_descendant >= 2) {
            generate_expression(node->descendants[0], offset_table, out);
            generate_expression(node->descendants[1], offset_table, out);
            
            fprintf(out, "    pop %%rdi\n");
            fprintf(out, "    pop %%rax\n");
            
            if (strcmp(node->value, "+") == 0) {
                fprintf(out, "    addl %%edi, %%eax\n");
            } 
            else if (strcmp(node->value, "-") == 0) {
                fprintf(out, "    subl %%edi, %%eax\n");
            }
            else if (strcmp(node->value, "*") == 0) {
                fprintf(out, "    imull %%edi, %%eax\n");
            }
            else if (strcmp(node->value, "/") == 0) {
                fprintf(out, "    cltd\n");
                fprintf(out, "    idivl %%edi\n");
            }
            
            fprintf(out, "    push %%rax\n");
        }
    }
}

void generate_code(SyntaxNode *root, SymbolTable *sym_table, FILE *out) {
    // Cabeçalho do assembly
    // fprintf(out, ".section .data\n");
    // fprintf(out, "format: .string \"%%d\\n\"\n\n");
    // fprintf(out, ".section .text\n");
    // fprintf(out, ".globl main\n");
    fprintf(out, "main:\n");
    fprintf(out, "    pushq %%rbp\n");
    fprintf(out, "    movq %%rsp, %%rbp\n");
    
    // Offset de variáveis locais
    OffsetTable offset_table;
    init_offset_table(&offset_table);
    
    int var_count = 0;
    Symbol *current = sym_table->head;
    while (current != NULL) {
        if (current->type == VARIAVEL && current->scope_level == 1) {
            var_count++;
        }
        current = current->next;
    }
    
    int stack_space = (var_count * 4 + 15) & ~15; 
    fprintf(out, "    subq $%d, %%rsp\n\n", stack_space);
    
    // Atribuição de offsets às variáveis
    current = sym_table->head;
    while (current != NULL) {
        if (current->type == VARIAVEL && current->scope_level == 1) {
            add_variable_offset(&offset_table, current->name, offset_table.next_offset);
            offset_table.next_offset -= 4;
        }
        current = current->next;
    }
    
    for (int i = 0; i < root->num_descendant; i++) {
        SyntaxNode *node = root->descendants[i];
        
        if (strcmp(node->type, "FUNCTION") == 0) {
            for (int j = 0; j < node->num_descendant; j++) {
                SyntaxNode *child = node->descendants[j];
                
                if (strcmp(child->type, "BLOCK") == 0) {
                    for (int k = 0; k < child->num_descendant; k++) {
                        SyntaxNode *stmt = child->descendants[k];
                        
                        if (strcmp(stmt->type, "DECLARATION") == 0) {
                            if (stmt->num_descendant > 2) {
                                char *var_name = stmt->descendants[1]->value;
                                int offset = get_variable_offset(&offset_table, var_name);
                                
                                if (strcmp(stmt->descendants[2]->type, "EXPRESSION") == 0 &&
                                    strcmp(stmt->descendants[2]->descendants[0]->type, "NUMBER") == 0) {
                                    
                                    fprintf(out, "    movl $%s, %d(%%rbp)\n",
                                            stmt->descendants[2]->descendants[0]->value,
                                            offset);
                                }
                            }
                        }
                        else if (strcmp(stmt->type, "ASSIGNMENT") == 0) {
                            char *var_name = stmt->descendants[0]->value;
                            int offset = get_variable_offset(&offset_table, var_name);
                            
                            generate_expression(stmt->descendants[1], &offset_table, out);
                            
                            fprintf(out, "    pop %%rax\n");
                            fprintf(out, "    movl %%eax, %d(%%rbp)\n\n", offset);
                        }
                        else if (strcmp(stmt->type, "FUNCTION_CALL") == 0) {
                            if (strcmp(stmt->descendants[0]->value, "printf") == 0) {
                                fprintf(out, "    leaq format(%%rip), %%rdi\n");
                                
                                SyntaxNode *arg = stmt->descendants[1]->descendants[1];
                                
                                if (strcmp(arg->type, "IDENTIFIER") == 0) {
                                    int offset = get_variable_offset(&offset_table, arg->value);
                                    fprintf(out, "    movl %d(%%rbp), %%esi\n", offset);
                                }
                                else if (strcmp(arg->type, "NUMBER") == 0) {
                                    fprintf(out, "    movl $%s, %%esi\n", arg->value);
                                }
                                
                                fprintf(out, "    xorl %%eax, %%eax\n");
                                fprintf(out, "    call printf\n\n");
                            }
                        }
                    }
                }
            }
        }
    }
    
    fprintf(out, "    movl $0, %%eax\n");
    fprintf(out, "    leave\n");
    fprintf(out, "    ret\n");
    
    for (int i = 0; i < offset_table.count; i++) {
        free(offset_table.variables[i].name);
    }
    free(offset_table.variables);
}