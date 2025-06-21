# Name of compiler
CC = gcc

# Source and output
SRC = main.c cat.c blob.c
OUT = mygit

# Flags
CFLAGS = -Wall -Wextra -std=c2x
LDFLAGS = -lz

# Default target
all: build

# Build target
build: $(SRC) 
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

# Run with custom args: make run ARGS="init"
run: $(OUT)
	./$(OUT) $(ARGS)

# Clean build
clean:
	rm -f $(OUT)

