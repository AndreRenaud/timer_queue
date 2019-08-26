#include <stdbool.h>
#include <pthread.h>
#include <errno.h>

#include "timer_queue.h"

struct timer {
	uint64_t expires;
	timer_callback callback;
	void *data;
	bool inuse; // TODO: Replace with uint32_t array & use ffs to find empty slots
};

#if 1
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#define disable_irq() pthread_mutex_lock(&mutex)
#define enable_irq() pthread_mutex_unlock(&mutex)
#else
#define disable_irq()
#define enable_irq()
#endif

static struct timer timers[TIMER_QUEUE_COUNT] = {};

int timer_add(uint64_t expires, timer_callback callback, void *data)
{
	int slot;
	disable_irq();
	for (slot = 0; slot < TIMER_QUEUE_COUNT; slot++)
		if (!timers[slot].inuse)
			break;
	if (slot == TIMER_QUEUE_COUNT) {
		enable_irq();
		return -ENOENT;
	}

	timers[slot].expires = expires;
	timers[slot].callback = callback;
	timers[slot].data = data;
	timers[slot].inuse = 1;

	enable_irq();

	return slot;
}

int timer_remove(timer_callback callback, void *data)
{
	int removed = 0;
	disable_irq();
	for (int i = 0; i < TIMER_QUEUE_COUNT; i++) {
		if (timers[i].inuse && timers[i].callback == callback && timers[i].data == data) {
			removed++;
			timers[i].inuse = 0;
		}
	}
	enable_irq();
	return removed;
}

void timer_update(uint64_t now)
{
	disable_irq();
	for (int i = 0; i < TIMER_QUEUE_COUNT; i++) {
		if (timers[i].inuse && timers[i].expires <= now) {
			timer_callback callback = timers[i].callback;
			void*data = timers[i].data;
			timers[i].inuse = 0;
			enable_irq();

			callback(now, data);

			disable_irq();
		}
	}
	enable_irq();
}

uint64_t timer_next_expires(uint64_t now)
{
	uint64_t expires = TIMER_INFINITY;
	disable_irq();
	for (int i = 0; i < TIMER_QUEUE_COUNT; i++) {
		if (!timers[i].inuse)
			continue;
		if (timers[i].expires < now) {
			expires = 0;
			break;
		}
		uint64_t diff = timers[i].expires - now;
		if (diff < expires)
			expires = diff;
	}
	enable_irq();

	return expires;
}