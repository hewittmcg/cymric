#include "cymric_semaphore.h"
#include "cymric.h"

CymricSemaphore cymric_sem_init(uint32_t initial_count) {
	CymricSemaphore sem = {
        .id = 0, // ID is unused currently
        .count = initial_count,
    };
    return sem;
}

void cymric_sem_signal(CymricSemaphore *sem) {
    __disable_irq();
    sem->count++;
    __enable_irq();
}

CymricSemStatus cymric_sem_wait(CymricSemaphore *sem, uint32_t timeout_ms) {
    if(timeout_ms == CYMRIC_TIMEOUT_FOREVER) {
        while(sem->count == 0) {}
    } else {
        uint32_t timeout_end_ms = cymric_get_ticks() + timeout_ms;
        while(sem->count == 0) {
            if(cymric_get_ticks() <= timeout_end_ms) {
                // Timeout reached, return
                return CYMRIC_SEM_STATUS_TIMEOUT;
            }
        }
    }
    __disable_irq();
    sem->count--;
    __enable_irq();

    return CYMRIC_SEM_STATUS_OK;
}
