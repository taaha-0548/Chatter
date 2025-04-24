# Compiler and flags
CC = gcc
CFLAGS = -Wall -O2 -pthread -Iinclude

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN = chatter

# Source and object files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Default target
all: $(BIN)

# Link object files into final binary
$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile each .c file into .o in obj/
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(BIN)

.PHONY: all clean
