# Compiler and compiler flags
CC = gcc
CFLAGS = -ansi -Wall -g -O0 -Wwrite-strings -Wshadow -pedantic-errors -fstack-protector-all -Wextra

# Linker flags
LDFLAGS = -lreadline

# Source and object files
SOURCES = lexer.c parser.tab.c executor.c d8sh.c
OBJECTS = $(SOURCES:.c=.o)

# Executable name
EXECUTABLE = d8sh

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

d8sh.o: d8sh.c executor.h lexer.h
	$(CC) $(CFLAGS) -c d8sh.c

lexer.o: lexer.c 
	$(CC) $(CFLAGS) -c lexer.c

parser.tab.o: parser.tab.c command.h
	$(CC) $(CFLAGS) -c parser.tab.c

executor.o: executor.c executor.h command.h
	$(CC) $(CFLAGS) -c executor.c

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
