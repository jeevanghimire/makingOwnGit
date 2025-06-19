# Name of compiler
CC = gcc

# Source and output
SRC = main.c
OUT = mygit

# Flags
CFLAGS = -Wall -Wextra -std=c2x
LDFLAGS = -lz

# Default target
all: run

# Build target
$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC) $(LDFLAGS)

# Run with custom args: make run ARGS="init"
run: $(OUT)
	./$(OUT) $(ARGS)

# Clean build
clean:
	rm -f $(OUT)

