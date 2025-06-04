CC=gcc
CFLAGS=-I.
DEPS = lexico.h
OBJ = lexico.o sintatico.o main.o
EXEC = main

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
.PHONY: clean
clean:
	rm -f $(EXEC)/*.o *~ core