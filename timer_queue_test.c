#include <pthread.h>
#include <stdatomic.h>

#include "acutest.h"
#include "timer_queue.h"

static void counter_inc_cb(uint64_t now, void *data)
{
	(void)now;
	atomic_int *i = data;
	(*i)++;
}

static void recursive_cb(uint64_t now, void *data)
{
	atomic_int *i = data;
	(*i)++;
	TEST_CHECK(timer_add(now + 1, recursive_cb, data) >= 0);
}

static void recursive_del_cb(uint64_t now, void *data)
{
	atomic_int *i = data;
	(*i)++;
	TEST_CHECK(timer_add(now + 1, recursive_cb, data) >= 0);
	TEST_CHECK(timer_remove(recursive_cb, data) > 0);
}

static void simple_test(void)
{
	atomic_int counter = 0;
	TEST_CHECK(timer_add(1, counter_inc_cb, &counter) >= 0);
	timer_update(0);
	TEST_CHECK(counter == 0);
	timer_update(1);
	TEST_CHECK(counter == 1);
	timer_update(2);
	TEST_CHECK(counter == 1);
}

static void recursive_test(void)
{
	atomic_int counter = 0;
	TEST_CHECK(timer_add(1, recursive_cb, &counter) >= 0);
	timer_update(0);
	TEST_CHECK(counter == 0);
	timer_update(1);
	TEST_CHECK(counter == 1);
	timer_update(2);
	TEST_CHECK(counter == 2);
}

static void *timer_thread(void *data)
{
	for (int i = 0; i < 5; i++) {
		TEST_CHECK(timer_add(i, counter_inc_cb, data) >= 0);
	}
	return NULL;
}

static void threaded_test(void)
{
	pthread_t threads[5];
	atomic_int counter = 0;
	for (int i = 0; i < 5; i++)
		pthread_create(&threads[i], NULL, timer_thread, &counter);

	for (int i = 0; i < 5; i++) {
		usleep(1); // yield
		timer_update(i);
	}
	for (int i = 0; i < 5; i++)
		pthread_join(threads[i], NULL);
	timer_update(10);
	TEST_CHECK(counter == 5 * 5);
}

static void remove_test(void)
{
	atomic_int counter = 0;
	TEST_CHECK(timer_add(1, counter_inc_cb, &counter) >= 0);
	TEST_CHECK(timer_remove(counter_inc_cb, &counter) == 1);
	timer_update(1);
	TEST_CHECK(counter == 0);

	TEST_CHECK(timer_add(1, recursive_del_cb, &counter) >= 0);
	timer_update(1);
	TEST_CHECK(counter == 1);
	timer_update(2);
	TEST_CHECK(counter == 1);
}

static void expire_test(void)
{
	atomic_int counter = 0;
	TEST_CHECK(timer_next_expires(0) == TIMER_INFINITY);
	TEST_CHECK(timer_add(1, counter_inc_cb, &counter) >= 0);
	TEST_CHECK(timer_next_expires(0) == 1);
	TEST_CHECK(timer_next_expires(1) == 0);
	TEST_CHECK(timer_next_expires(2) == 0);
	timer_update(1);
	TEST_CHECK(timer_next_expires(1) == TIMER_INFINITY);
}

TEST_LIST = {
	{"simple", simple_test},
	{"recursive", recursive_test},
	{"threaded", threaded_test},
	{"remove", remove_test},
	{"expire", expire_test},
	{0, 0},
};