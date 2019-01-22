.PHONY: memtest clean

CC = clang
CFLAGS = -g

SRCS = linkedlist.c talloc.c tokenizer.c parser.c interpreter.c main.c
HDRS = linkedlist.h value.h talloc.h tokenizer.h parser.h interpreter.h
OBJS = $(SRCS:.c=.o)

interpreter: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

memtest: interpreter
	valgrind --leak-check=full --show-leak-kinds=all ./$<

%.o : %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o
	rm -f interpreter
