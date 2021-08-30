#include "cymric_mutex.h"
#include "cymric.h"

CymricMutex cymric_mut_init(CymricMutState initial_state) {
    CymricMutex mut = {
        .state = initial_state,
        .id = 0, // Unused currently
    };
    return mut;
}

void cymric_mut_release(CymricMutex *mut) {
    // Don't need to worry about conflicting accesses since writes <= word size
    // should be atomic on ARM
    mut->state = CYMRIC_MUT_STATE_RELEASED;
}

CymricMutStatus cymric_mut_take(CymricMutex *mut, uint32_t timeout_ms) {
    // This can overflow if timeout_ms == CYMRIC_TIMEOUT_FOREVER,
    // but we don't use the timeout anyways in that case
    uint32_t timeout_end_ms = cymric_get_ticks() + timeout_ms;

    bool finished = false;
    while(!finished) {
        // Check for timeout
        if(timeout_ms != CYMRIC_TIMEOUT_FOREVER) {
            if(timeout_end_ms < cymric_get_ticks()) {
                return CYMRIC_MUT_STATUS_TIMEOUT;
            }
        }

        // Check mutex state and take it if released
        __disable_irq();
        if(mut->state == CYMRIC_MUT_STATE_RELEASED) {
            finished = true;
            mut->state = CYMRIC_MUT_STATE_TAKEN;
        }
        __enable_irq();
    }

    return CYMRIC_MUT_STATUS_OK;
}
