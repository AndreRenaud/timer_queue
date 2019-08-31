/**
 * @file timer_queue.h
 * @description Implements a queue of callback functions that are triggered
 * at specific times.
 * Care is taken to ensure internal consistency, so this module should be 
 * able to be used from a multi-threaded context. However depending on the
 * underlying OS (or lack there of) the locking functions in timer_queue.c
 * may need to be revised.
 * Note: There is no specific unit assigned to the times here, this module does
 * not have any click related functions of its own, so as long as they are all
 * called consistently it does not matter what the units are
 */
#ifndef TIMER_QUEUE_H
#define TIMER_QUEUE_H

#include <stdint.h>

/**
 * The maximum number of outstanding queued timer functions that can be set
 * Once this limit has been reached, calls to timer_add will fail until the
 * queued functions are either removed, or executed via timer_update
 */
#define TIMER_QUEUE_COUNT 32

#define TIMER_INFINITY UINT64_MAX

struct timer_stats {
	uint64_t added;
	uint64_t add_failures;
	uint64_t removed;
	uint64_t executed;
	uint32_t max_outstanding;
	uint32_t current_outstanding;
};

/**
 * Retrieves the debugging statistics for the timer queue
 * @return Requires TIMER_QUEUE_STATS to be defined, otherwise 'NULL' is returned
 */
struct timer_stats *timer_get_stats(void);

/**
 * Prototype of the callback functions that will be called when timers
 * expire
 */
typedef void (*timer_callback)(uint64_t now, void *data);

/**
 * Adds a new callback to be executed at 'time'.
 * @return < 0 on failures, >= 0 on success
 */
int timer_add(uint64_t expires, timer_callback callback, void *data);

/**
 * Removes a callback that was previously added
 * Note: Both the callback and data function must match.
 * If a timer function has been added multiple times with the same data,
 * they will all be removed.
 * @return number of instances of callback that were removed
 */
int timer_remove(timer_callback callback, void *data);

/**
 * Checks the timer queue and executes any that have expired
 * Note: expired timers are removed from the system. For repeated
 * calls they should be manually reinstated
 * @param now Current time to compare against outstanding queued items
 */
void timer_update(uint64_t now);

/**
 * Returns the time before the next outstanding expiration
 * This will return 0 if there are timer callbacks that are already expired
 * but have not yet been executed, or TIMER_INFINITY if there are no timers
 */
uint64_t timer_next_expires(uint64_t now);

#endif