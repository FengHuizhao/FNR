CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
LDLIBS = -lm

TARGET = Compare_ALL
SRCS = Compare_ALL.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

clean:
	rm -f $(TARGET)

.PHONY: all clean
