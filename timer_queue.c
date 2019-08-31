#include <stdbool.h>
#include <errno.h>

#include "timer_queue.h"

struct timer {
	uint64_t expires;
	timer_callback callback;
	void *data;
	bool inuse; // TODO: Replace with uint32_t array & use ffs to find empty slots
};

#ifdef TIMER_QUEUE_STATS
static struct timer_stats stats = {};
#endif

#if defined(__APPLE__) || defined(__linux__)
/* For posix-pthread compliant systems */
#include <pthread.h>
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#define disable_irq() pthread_mutex_lock(&mutex)
#define enable_irq() pthread_mutex_unlock(&mutex)
#else
/* For other systems, ie: embedded, these macros should be defined as appropriate */
#define disable_irq()
#define enable_irq()
#endif

static struct timer timers[TIMER_QUEUE_COUNT] = {};

struct timer_stats *timer_get_stats(void)
{
#ifdef TIMER_QUEUE_STATS
	return &stats;
#else
	return NULL;
#endif
}

int timer_add(uint64_t expires, timer_callback callback, void *data)
{
	int slot;
	disable_irq();
	for (slot = 0; slot < TIMER_QUEUE_COUNT; slot++)
		if (!timers[slot].inuse)
			break;
	if (slot == TIMER_QUEUE_COUNT) {
#ifdef TIMER_QUEUE_STATS
		stats.add_failures++;
#endif
		enable_irq();
		return -ENOENT;
	}
#ifdef TIMER_QUEUE_STATS
	stats.added++;
#endif

	timers[slot].expires = expires;
	timers[slot].callback = callback;
	timers[slot].data = data;
	timers[slot].inuse = 1;
#ifdef TIMER_QUEUE_STATS
	stats.current_outstanding++;
	if (stats.current_outstanding > stats.max_outstanding)
		stats.max_outstanding = stats.current_outstanding;
#endif

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
#ifdef TIMER_QUEUE_STATS
	stats.removed += removed;
	stats.current_outstanding -= removed;
#endif
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
#ifdef TIMER_QUEUE_STATS
			stats.current_outstanding--;
			stats.executed++;
#endif
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