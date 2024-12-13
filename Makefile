CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Werror -std=c99

SRCS = smallsh.c
TARGET = smallsh

# Default target
all: $(TARGET)

# Normal build
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $@

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

clean:
	rm -f $(TARGET)

re: clean $(TARGET)