CC=gcc-12
SOURCES=$(wildcard sources/*.c)
OBJECTS=$(SOURCES:.c=.o)
HEADERS=headers

EXECUTABLE=./server

all: build
	$(EXECUTABLE) $(EFLAGS)

build: $(OBJECTS)
	$(CC) -o $(EXECUTABLE) $(CFLAGS) $^

.c.o:
	$(CC) -c -o $@ -Wall -Wextra -I$(HEADERS) $(CFLAGS) $<

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)