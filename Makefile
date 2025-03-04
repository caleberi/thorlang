# Compiler settings
CC = gcc
CFLAGS = -fshort-enums -Wall -Wextra -I./include

# Source files
SRC_DIR = include
SOURCES = main.c \
          $(SRC_DIR)/chunk.c \
          $(SRC_DIR)/memory.c \
          $(SRC_DIR)/debug.c \
          $(SRC_DIR)/value.c \
          $(SRC_DIR)/vm.c \
          $(SRC_DIR)/compiler.c \
          $(SRC_DIR)/alloc.c \
          $(SRC_DIR)/trie.c \
          $(SRC_DIR)/scanner.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Output executable name
TARGET = thor

# Analyser executable name
ANALYSER = thor-analyser

# Phony targets
.PHONY: all clean

# Default target
all: $(TARGET)

analyser: $(ANALYSER)

# Linking rule
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

$(ANALYSER):  ./include/trie.c ./include/memory.c ./include/analyser.c
	$(CC) $(CFLAGS) -Iinclude $(SRC_DIR)/analyser.c $(SRC_DIR)/trie.c $(SRC_DIR)/memory.c -o $(ANALYSER)

# Compilation rule
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(OBJECTS) $(TARGET) $(ANALYSER)
