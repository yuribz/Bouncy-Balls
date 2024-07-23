# Compiler and flags
CC = gcc
CFLAGS = `sdl2-config --cflags` -Isrc/include -Wall -g
LDFLAGS = `sdl2-config --libs` -lm

# Source files
SRCS = src/balls.c
OBJS = $(SRCS:.c=.o)

# Output executable
TARGET = balls

# Default target
all: $(TARGET)

# Link the object files to create the executable
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up object files and the executable
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean
