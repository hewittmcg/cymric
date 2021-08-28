#include "cymric.h"

#include "cmsis_armcc.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx.h"

// Globals for use in context switches
// These should be updated just prior to the context switch
typedef struct {
	uint8_t cur_task; // Current task
	uint8_t next_task; // Task to switch to
	
	// These need to be double pointers so that the inlined asm in PendSV_Handler has 
	// a consistent memory address to access when getting them from the s_tcbs array so that
	// it can be updated directly from the asm function.
	uint32_t **cur_top_addr; // Pointer to current top address
	uint32_t **next_top_addr; // Pointer to next top address
} ContextSwitchInfo;

// Task control blocks
static Cymric_TCB s_tcbs[CYMRIC_MAX_TASKS];

// Current task ID to be used for a new task
static uint8_t s_cur_alloc_id;

static uint32_t s_ticks_ms;

ContextSwitchInfo s_switch_info;

static uint8_t s_test_next_task = 1; // for testing context switches only

// Flag to allow context switches etc
static bool s_started_flag;

// Handler for SysTick interrupts (allows delays to work)
void SysTick_Handler(void) {
	s_ticks_ms++;
	HAL_IncTick(); // to be removed once unnecessary
	
	// Currently we just context switch here.  Not sure if there's 
	// a better way of implementing the switch than this...
	
	// Testing just context switching between cur_task and next_task (TODO)
	if(s_started_flag) {
		if(s_ticks_ms % 1000 == 0) {
			if(s_switch_info.cur_task == 0) {
				s_switch_info.next_task = 1;
			} else if(s_switch_info.cur_task == 2) {
				//s_switch_info.cur_task = 2;
				s_switch_info.next_task = 1;
				// s_test_next_task = 1;
			} else if(s_switch_info.cur_task == 1)  {
				// s_switch_info.cur_task = 1;
				s_switch_info.next_task = 2;
				// s_test_next_task = 2;
			} else {
				// Infinite loop, shouldn't get here
				while(1) {
					asm("nop");
				}
			}
			
			s_switch_info.cur_top_addr = &s_tcbs[s_switch_info.cur_task].top_addr;
			s_switch_info.next_top_addr = &s_tcbs[s_switch_info.next_task].top_addr;
			// Context switch by setting PendSV to pending
			SCB->ICSR |= 1 << SCB_ICSR_PENDSVSET_Pos;
		}
	}
}

// Handler for context switches (TODO)
__asm void PendSV_Handler(void) {
	// Push R4-R11 onto the stack
	PUSH {R11}
	PUSH {R10}
	PUSH {R9}
	PUSH {R8}
	PUSH {R7}
	PUSH {R6}
	PUSH {R5}
	PUSH {R4}
	
	// TODO MAKE SURE THIS WORKS OK!!!! AND TEST CONTEXT SWITCHING!!!!!
	// Copy current top of stack into the TCB for the current task
	// TODO: need to fix this by using a double pointer or something so it updates the s_tcbs address
	LDR R4,=__cpp(&s_switch_info.cur_top_addr)
	LDR R4,[R4] // dereference? idk, TODO make sure this works!!!
	STR SP,[R4]
	
	// Set the stack pointer to the top of stack of the new task
	LDR R4,=__cpp(&s_switch_info.next_top_addr)
	LDR R4,[R4] // TODO make sure works
	LDR SP,[R4] // Idk if these two can be done in one line
	
	// Pop R4-R11 from the stack
	POP {R4}
	POP {R5}
	POP {R6}
	POP {R7}
	POP {R8}
	POP {R9}
	POP {R10}
	POP {R11}
	
   	// return from handler
	NOP // padding, to remove a warning
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
	
	// Configure IRQ priorities
	NVIC_SetPriority(SysTick_IRQn, 0x00); // highest priority
	NVIC_SetPriority(PendSV_IRQn, 0xFF); // lowest priority, to avoid nested interrupts
	
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
	
	// Configure systick
	s_ticks_ms = 0;
	s_started_flag = true;
	
	// Invoke idle task function
	s_cur_alloc_id++;
	prv_idle(0);
}

bool cymric_task_new(CymricTaskFunction func, void *args) {
	if(s_cur_alloc_id >= CYMRIC_MAX_TASKS) return false;
	
	// Configure initial registers for future context switching
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
