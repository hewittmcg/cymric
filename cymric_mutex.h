#pragma once

#include <inttypes.h>

// States
typedef enum {
    CYMRIC_MUT_STATE_RELEASED = 0,
    CYMRIC_MUT_STATE_TAKEN,
    NUM_CYMRIC_MUT_STATES,
} CymricMutState;

typedef struct {
    volatile CymricMutState state;
    uint8_t id;
} CymricMutex;

// Status codes
typedef enum {
    CYMRIC_MUT_STATUS_OK = 0,
    CYMRIC_MUT_STATUS_TIMEOUT,
    NUM_CYMRIC_MUT_STATUSES,
} CymricMutStatus;

// Initialize a mutex with the the initial state given.
CymricMutex cymric_mut_init(CymricMutState initial_state);

// Release a mutex, allowing it to be taken by future threads.
void cymric_mut_release(CymricMutex *mut);

// Attempt to take a mutex.  Will block for the timeout requested and then return.
// Pass in CYMRIC_TIMEOUT_FOREVER to block indefinitely.
CymricMutStatus cymric_mut_take(CymricMutex *mut, uint32_t timeout_ms);
