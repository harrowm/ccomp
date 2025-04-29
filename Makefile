CC = gcc
CFLAGS = -flto -O3 -DDEBUG_LEVEL=3

SRC = main.c lexer.c parser.c cfg.c dominance.c
OBJ = $(SRC:.c=.o)

TEST_SRC = test.c lexer.c parser.c cfg.c dominance.c
TEST_OBJ = $(TEST_SRC:.c=.o)

all: compiler test_runner

compiler: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

test_runner: $(TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $(TEST_OBJ)

run: compiler
	./compiler $(SRC_FILE)
	dot -Tpng cfg.dot -o cfg.png
	dot -Tpng df.dot -o df.png
	dot -Tpng cfg_with_phi.dot -o cfg_with_phi.png

clean:
	rm -f $(OBJ) $(TEST_OBJ) compiler test_runner cfg.png df.png cfg_with_phi.png