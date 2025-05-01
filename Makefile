CC = gcc
CFLAGS += -flto -O3 -DDEBUG_LEVEL=4 -fprofile-arcs -ftest-coverage
LDFLAGS += -lgcov

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
	$(CC) $(CFLAGS) -o test_lexer lexer.c test_lexer.c

test_parser: parser.c parser.h test_parser.c lexer.c lexer.h ast.h minunit.h
	$(CC) $(CFLAGS) -o test_parser parser.c lexer.c test_parser.c

.PHONY: test coverage

test: test_lexer test_parser
	./test_lexer
	./test_parser

coverage: test
	lcov --capture --directory . --output-file coverage.info
	genhtml coverage.info --output-directory coverage
	@echo "Coverage report generated in ./coverage/index.html"

# To install lcov on macOS, use Homebrew:
#    brew install lcov

clean:
	rm -f $(OBJ) $(TEST_OBJ) compiler test_lexer test_parser cfg.png df.png cfg_with_phi.png *.gcda *.gcno coverage.info