// Basic semaphore implementation.
#pragma once

#include <inttypes.h>

typedef struct {
	volatile uint32_t count;
	uint8_t id;
} CymricSemaphore;

// Status codes
typedef enum {
	CYMRIC_SEM_STATUS_OK = 0,
	CYMRIC_SEM_STATUS_TIMEOUT,
	NUM_CYMRIC_SEM_STATUSES,
} CymricSemStatus;

// Initialize a semaphore with the initial count given.
CymricSemaphore cymric_sem_init(uint32_t initial_count);

// Increase the count of the semaphore.
void cymric_sem_signal(CymricSemaphore *sem);

// Attempt to decrease the count of the semaphore if its count is > 0.  
// Will until either the decrement is successful or the timeout fires.
// Call with CYMRIC_TIMEOUT_FOREVER to wait indefinitely.
// Returns a status code corresponding to the result of the wait attempt.
CymricSemStatus cymric_sem_wait(CymricSemaphore *sem, uint32_t timeout_ms);
