SOURCES=$(wildcard sources/*.c)
OBJECTS=$(SOURCES:.c=.o)
HEADERS=headers

EXECUTABLE=./server

all: build
	$(EXECUTABLE)

build: $(OBJECTS)
	gcc -o $(EXECUTABLE) $^

.c.o:
	gcc -c -o $@ -Wall -Wextra -I$(HEADERS) $<

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)