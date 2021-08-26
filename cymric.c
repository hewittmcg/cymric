#include "cymric.h"

#include "cmsis_armcc.h"

// Task control blocks
static Cymric_TCB s_tcbs[CYMRIC_MAX_TASKS];

// Current task ID to be used for a new task
static uint8_t s_cur_alloc_id;

// Idle task
static void prv_idle(void *args) {
	while(1) {
		asm("nop");
	}
}

bool cymric_init(void) {
	// Get the base address of the main stack 
	uint32_t *main_stack_base_addr = CORTEX_M4_MSP_RST_ADDR;
	uint32_t cur_stack_addr = *main_stack_base_addr + CYMRIC_MAIN_STACK_SIZE;
	
	// Initialize each TCB
	for(uint8_t i = 0; i < CYMRIC_MAX_TASKS; i++) {
		// Increment addresses by thread stack size
		s_tcbs[i].addr = cur_stack_addr;
		cur_stack_addr += CYMRIC_THREAD_STACK_SIZE;
	}
	
	return true;
}

void cymric_start(void) {
	// Reset MSP to main stack base address
	uint32_t *main_stack_base_addr = CORTEX_M4_MSP_RST_ADDR;
	__set_MSP(*main_stack_base_addr);
	
	// Switch from MSP to PSP
	uint32_t control = __get_CONTROL();
	control |= 1 << CORTEX_M4_CONTROL_SPSEL;
	__set_CONTROL(control);
	
	// Change PSP to the address of the idle task
	__set_PSP(s_tcbs[CYMRIC_IDLE_ID].addr);
	
	// Configure systick (TODO)
	
	// Invoke idle task function
	s_cur_alloc_id++;
	prv_idle(0);
}

bool cymric_task_new(CymricTaskFunction func, void *args) {
	if(s_cur_alloc_id >= CYMRIC_MAX_TASKS) return false;
	
	// TODO
	return true;
}


