gcc -std=c23 -O3 -flto main.c parser.c lexer.c cfg.c -o compiler
./compiler test.c > output.s
as -o output.o output.s
ld -o output output.o -lSystem -syslibroot `xcrun -sdk macosx --show-sdk-path` -arch arm64
