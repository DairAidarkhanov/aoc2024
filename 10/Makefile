CC = clang
CFLAGS = -std=c89 -Wall -Wextra -Werror -Wpedantic

all: clean compile run

clean:
	@rm -f main.exe

compile: clean
	@$(CC) $(CFLAGS) -o main.exe main.c

run:
	@./main.exe < input.txt
