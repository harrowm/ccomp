CC = gcc
CFLAGS += -flto -O3 -DDEBUG_LEVEL=4 -fprofile-arcs -ftest-coverage -g
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

test_cfg: cfg.c cfg.h test_cfg.c lexer.c lexer.h parser.c parser.h ast.h minunit.h
	$(CC) $(CFLAGS) -o test_cfg cfg.c lexer.c parser.c test_cfg.c

test_dominance: dominance.c cfg.c cfg.h test_dominance.c lexer.c lexer.h parser.c parser.h ast.h minunit.h
	$(CC) $(CFLAGS) -o test_dominance dominance.c cfg.c lexer.c parser.c test_dominance.c

test_tac: tac.c tac.h test_tac.c cfg.c cfg.h lexer.c lexer.h parser.c parser.h dominance.c minunit.h
	$(CC) $(CFLAGS) -o test_tac tac.c cfg.c lexer.c parser.c dominance.c test_tac.c

.PHONY: test coverage

test: test_lexer test_parser test_cfg test_dominance test_tac
	./test_lexer
	./test_parser
	./test_cfg
	./test_dominance
	./test_tac

coverage: test
	lcov --capture --directory . --output-file coverage.info
	genhtml coverage.info --output-directory coverage
	@echo "Coverage report generated in ./coverage/index.html"

# To install lcov on macOS, use Homebrew:
#    brew install lcov

clean:
	rm -f $(OBJ) $(TEST_OBJ) compiler test_lexer test_parser test_cfg test_dominance test_tac cfg.png df.png cfg_with_phi.png *.gcda *.gcno coverage.info