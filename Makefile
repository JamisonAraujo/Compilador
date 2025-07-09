CC=gcc
CFLAGS=-I.
DEPS = compilador.h gerador.h
OBJ = lexico.o sintatico.o semantico.o simbolo.o gerador.o main.o
EXEC = main

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
.PHONY: clean
clean:
	rm -f main lexico sintatico semantico simbolo gerador *.o *~ core saida.txt arvore.txt tabela_de_simbolos.txt codigo_final.txt