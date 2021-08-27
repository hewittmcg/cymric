#include "cymric.h"

#include "cmsis_armcc.h"
#include "stm32f4xx_hal.h"

// Globals for use in context switches
// These should be updated just prior to the context switch
typedef struct {
	uint8_t cur_task; // Current task
	uint8_t next_task; // Task to switch to
} ContextSwitchInfo;

// Task control blocks
static Cymric_TCB s_tcbs[CYMRIC_MAX_TASKS];

// Current task ID to be used for a new task
static uint8_t s_cur_alloc_id;

static uint32_t s_ticks_ms;

static ContextSwitchInfo s_switch_info;

// Handler for SysTick interrupts (allows delays to work)
void SysTick_Handler(void) {
	s_ticks_ms++;
	HAL_IncTick(); // to be removed once unnecessary
	
	// Currently we just context switch here.  Not sure if there's 
	// a better way of implementing the switch than this...
	
	// Testing just context switching between cur_task and next_task (TODO)
}

// Handler for context switches (TODO)
__asm void PendSV_Handler(void) {
   
	// Push R4-R11 onto the stack
	
	// Copy current top of stack into the TCP for the current task
   	// return from handler
	BX		LR
}

// Idle task
static void prv_idle(void *args) {
	while(1) {
		asm("nop");
	}
}

bool cymric_init(void) {
	// Get the base address of the main stack and subtract 
	uint32_t *main_stack_base_addr = CORTEX_M4_MSP_RST_ADDR;
	uint32_t *test = (uint32_t*)*main_stack_base_addr;
	uint32_t *cur_stack_addr =  test - CYMRIC_MAIN_STACK_SIZE / 4; // 4 bytes in uint32

	// Initialize each TCB
	for(uint8_t i = 0; i < CYMRIC_MAX_TASKS; i++) {
		
		// Increment addresses by thread stack size
		s_tcbs[i].addr = cur_stack_addr;
		
		// Top of stack initializes to the same address as base
		s_tcbs[i].top_addr = cur_stack_addr;
		
		// Decrement for next TCB
		cur_stack_addr -= CYMRIC_THREAD_STACK_SIZE / 4; // 4 bytes in uint32
	}
	
	// First ID which application tasks can be allocated to
	s_cur_alloc_id = CYMRIC_IDLE_ID + 1;
	
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
	__set_PSP((uint32_t)s_tcbs[CYMRIC_IDLE_ID].addr);
	
	// Configure systick (TODO)
	s_ticks_ms = 0;
	
	// Invoke idle task function
	s_cur_alloc_id++;
	prv_idle(0);
}

bool cymric_task_new(CymricTaskFunction func, void *args) {
	if(s_cur_alloc_id >= CYMRIC_MAX_TASKS) return false;
	
	// TODO: Configure initial registers for future context switching
	uint32_t *addr = s_tcbs[s_cur_alloc_id].addr;
	
	// PSR
	*addr = PSR_DEFAULT;
	addr++;
	
	// PC - address of function
	*addr = (uint32_t)func;
	
	// R0 - address of args (located 6 indices above PC)
	addr += 6;
	*addr = (uint32_t)args;
	
	// Top of stack is 8 indices above R0 for the other 8 registers stored
	s_tcbs[s_cur_alloc_id].top_addr = addr + 8;
	
	s_cur_alloc_id++;
	return true;
}

void cymric_run(void) {
	// TODO: perform scheduling here
	asm("nop");
}
