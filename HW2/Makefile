SOURCES=$(wildcard *.c)
OBJECTS=$(SOURCES:.c=.o)
DEPS=$(SOURCES:.c=.d)
BINS=$(SOURCES:.c=)

CFLAGS+=-Wall -O1 -Wpedantic -Werror -pthread


all: $(BINS)

.PHONY: clean

clean:
	$(RM) $(OBJECTS) $(BINS)

-include $(DEPS)
