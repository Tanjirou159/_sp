CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -g
TARGETS = file_copy shell pipeline

.PHONY: all clean

all: $(TARGETS)

file_copy: file_copy.c
	$(CC) $(CFLAGS) -o $@ $<

shell: shell.c
	$(CC) $(CFLAGS) -o $@ $<

pipeline: pipeline.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)
