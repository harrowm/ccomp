CC = gcc
CFLAGS = -flto -O3 -DDEBUG_LEVEL=3

SRC = main.c lexer.c parser.c cfg.c dominance.c
OBJ = $(SRC:.c=.o)

all: compiler test

compiler: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

run: compiler
	./compiler $(SRC_FILE)
	dot -Tpng cfg.dot -o cfg.png
	dot -Tpng df.dot -o df.png
	dot -Tpng cfg_with_phi.dot -o cfg_with_phi.png

test_lexer: lexer.c lexer.h test_lexer.c minunit.h
	$(CC) $(CFLAGS) -o test_lexer lexer.c test_lexer.c -DDEBUG_LEVEL=3

.PHONY: test

test: test_lexer
	./test_lexer

clean:
	rm -f $(OBJ) $(TEST_OBJ) compiler test_lexer cfg.png df.png cfg_with_phi.png