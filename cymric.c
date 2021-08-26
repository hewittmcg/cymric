#include "cymric.h"

// Task control blocks
static Cymric_TCB s_tcbs[CYMRIC_MAX_TASKS];

bool cymric_init(void) {
	// Get the base address of the main stack 
	uint32_t *main_stack_base_addr = 0x00000000;
	uint32_t cur_stack_addr = *main_stack_base_addr + CYMRIC_MAIN_STACK_SIZE;
	
	// Initialize each TCB
	for(uint8_t i = 0; i < CYMRIC_MAX_TASKS; i++) {
		// Increment addresses by thread stack size
		s_tcbs[i].addr = cur_stack_addr;
		cur_stack_addr += CYMRIC_THREAD_STACK_SIZE;
	}
	
	return true;
}
