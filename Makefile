CFLAGS=-g -Wall -pipe
LFLAGS=

default: timer_queue_test

timer_queue_test: timer_queue_test.o timer_queue.o
	$(CC) -o $@ timer_queue_test.o timer_queue.o

test: timer_queue_test
	./timer_queue_test

%.o: %.c timer_queue.h Makefile
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm *.o timer_queue_test