CFLAGS=-g -Wall -pipe -Wextra -DTIMER_QUEUE_STATS
LFLAGS=-lpthread

default: timer_queue_test

timer_queue_test: timer_queue_test.o timer_queue.o
	$(CC) -o $@ timer_queue_test.o timer_queue.o $(LFLAGS)

test: timer_queue_test
	./timer_queue_test

%.o: %.c timer_queue.h Makefile
	cppcheck --quiet $<
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm *.o timer_queue_test