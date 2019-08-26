#ifndef TIMER_QUEUE_H
#define TIMER_QUEUE_H

#include <stdint.h>

#define TIMER_QUEUE_COUNT 32

#define TIMER_INFINITY UINT64_MAX

typedef void (*timer_callback)(uint64_t now, void *data);

/**
 * Adds a new callback to be executed at 'time'.
 * @return < 0 on failures, >= 0 on success
 */
int timer_add(uint64_t expires, timer_callback callback, void *data);

/**
 * Removes a callback that was previously added
 * @return < 0 on failure, >= 0 on success
 */
int timer_remove(timer_callback callback, void *data);

/**
 * Checks the timer queue and executes any that have passed
 */
void timer_update(uint64_t now);

/**
 * Returns the time before the next outstanding expiration
 * This will return 0 if there are pending timers that are already expired,
 * or TIMER_INFINITY if there are no timers
 */
uint64_t timer_next_expires(uint64_t now);

#endif