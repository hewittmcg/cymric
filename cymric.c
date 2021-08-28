#include "cymric.h"

#include "cmsis_armcc.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx.h"

// Globals for use in context switches
// These should be updated just prior to the context switch
typedef struct {
	uint8_t cur_task; // Current task
	uint8_t next_task; // Task to switch to
	
	// These need to be double pointers so that the asm in PendSV_Handler has 
	// a consistent memory address to access when getting them from the s_tcbs array.
	uint32_t **cur_top_addr; // Pointer to current top address
	uint32_t **next_top_addr; // Pointer to next top address
} ContextSwitchInfo;

// Task control blocks
static Cymric_TCB s_tcbs[CYMRIC_MAX_TASKS];

// Current task ID to be used for a new task
static uint8_t s_cur_alloc_id;

static uint32_t s_ticks_ms;

ContextSwitchInfo switch_info;

// Flag to allow context switches and schedling to occur.
static bool s_started_flag;

// TODO: Run the task scheduler.  Should be called in SysTick_Handler().
// static inline void prv_schedule(void) {}

// Handler for SysTick interrupts (allows delays to work)
void SysTick_Handler(void) {
	s_ticks_ms++;
	HAL_IncTick(); // to be removed once unnecessary
	
	// Testing just context switching between cur_task and next_task (TODO)
	if(s_started_flag) {
		if(s_ticks_ms % 1000 == 0) {
			if(switch_info.cur_task == 0) {
				switch_info.next_task = 1;
			} else if(switch_info.cur_task == 2) {
				switch_info.next_task = 1;
			} else if(switch_info.cur_task == 1)  {
				switch_info.next_task = 2;
			} else {
				// Infinite loop, shouldn't get here
				while(1) {
					asm("nop");
				}
			}
			
			switch_info.cur_top_addr = &s_tcbs[switch_info.cur_task].top_addr;
			switch_info.next_top_addr = &s_tcbs[switch_info.next_task].top_addr;
			
			// Task being switched to is now the current task
			switch_info.cur_task = switch_info.next_task;
			
			// Context switch by setting PendSV to pending
			SCB->ICSR |= 1 << SCB_ICSR_PENDSVSET_Pos;
		}
	}
}

// Handler for context switches
__asm void PendSV_Handler(void) {
	// Since this is an exception and as such occurs in handler mode,
	// need to get the PSP into a register to access it.
	MRS R2,PSP 
	
	// Push R4-R11 onto the process stack
	STMFD R2!,{R4-R11} 
	
	// Copy current top of stack into the TCB for the current task
	LDR R4,=__cpp(&switch_info.cur_top_addr)
	LDR R4,[R4] // Dereference
	STR R2,[R4]
	
	// Set the stack pointer to the top of stack of the new task
	LDR R4,=__cpp(&switch_info.next_top_addr)
	LDR R4,[R4] // Dereference
	LDR R2,[R4]
	
	// Pop R4-R11 from the stack
	LDMFD R2!,{R4-R11}
	
	// Update PSP
	MSR PSP,R2
	
	// Padding, to remove a warning
	NOP 
	
	// Return from handler
	BX LR
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
	uint32_t *cur_stack_addr = (uint32_t*)*main_stack_base_addr - CYMRIC_MAIN_STACK_SIZE / 4; // 4 bytes in uint32

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
	NVIC_SetPriority(SysTick_IRQn, CYMRIC_SYSTICK_PRIORITY);
	NVIC_SetPriority(PendSV_IRQn, CYMRIC_PENDSV_PRIORITY);
	
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
	addr--;
	
	// PC - address of function
	*addr = (uint32_t)func;
	
	// R0 - address of args (located 6 indices below PC)
	addr -= 6;
	*addr = (uint32_t)args;
	
	// Top of stack is 8 indices below R0 for the other 8 registers stored
	s_tcbs[s_cur_alloc_id].top_addr = addr - 8;
	
	s_cur_alloc_id++;
	return true;
}
