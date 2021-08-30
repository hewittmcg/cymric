#include "cymric.h"

#include "cmsis_armcc.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx.h"

// Globals for use in context switches
// These should be updated just prior to the context switch
typedef struct {
	uint8_t cur_task; // Current task
	
	// These need to be double pointers so that the asm in PendSV_Handler has 
	// a consistent memory address to access when getting them from the s_tcbs array.
	uint32_t **cur_top_addr; // Pointer to current top address
	uint32_t **next_top_addr; // Pointer to next top address
} ContextSwitchInfo;

// Task control block definition
typedef struct CymricTCB {
	uint8_t id;
	uint32_t *addr; // Base address of task stack
	uint32_t *top_addr; // Address of top of task stack
	CymricPriority pri;
	struct CymricTCB *next; // For use in linked-list implementation
} CymricTCB;

// Task control blocks
static CymricTCB s_tcbs[CYMRIC_MAX_TASKS];

// Current task ID to be used for a new task
static uint8_t s_cur_alloc_id;

static uint32_t s_ticks_ms;

ContextSwitchInfo switch_info;

// Flag to allow context switches and schedling to occur.
static bool s_started_flag;

// List to contain all TCBs to run at a given priority
// Need access to head for removal and tail for insertion
typedef struct {
	CymricTCB *head;
	CymricTCB *tail;
} List;

// One list for each priority
static List s_ready[NUM_CYMRIC_PRIORITIES];

// Bitset of whether priority list has TCBs in it
static uint32_t s_pri_mask;
#define PRI_MASK_CLZ_MAX 31 // max number of leading zeros

// Insert a TCB into the list corresponding to its priority.
static void prv_insert(CymricTCB *tcb, CymricPriority pri) {
	if(s_ready[pri].tail) {
		s_ready[pri].tail->next = tcb;
	} else {
		// List was empty
		s_ready[pri].head = tcb;
		s_pri_mask |= 1 << pri;
	}
	
	// TCB is now the last in the list
	s_ready[pri].tail = tcb;
	tcb->next = NULL;
}

// Remove the TCB at the head of the priority list.  Returns a reference to this TCB.
static CymricTCB *prv_remove(CymricPriority pri) {
	if(!s_ready[pri].head) {
		return NULL;
	} else {
		CymricTCB *ret = s_ready[pri].head;
		s_ready[pri].head = s_ready[pri].head->next;
		if(!s_ready[pri].head) {
			// List is now empty
			s_ready[pri].tail = NULL;
			s_pri_mask &= ~(1 << pri);
		}
		return ret;
	}
}
	

// Schedule tasks using fixed-priority pre-emptive scheduling.  
// Should be called in SysTick_Handler().
static inline void prv_schedule(void) {
	// If the only task is the idle task, continue running it
	if(s_pri_mask == 1 << CYMRIC_PRI_IDLE) {
		return;
	}
	
	// Get the highest task scheduled to run
	uint8_t highest_sched = PRI_MASK_CLZ_MAX - __clz(s_pri_mask);
	
	// If priority of current task is lower than or equal to highest available, insert it back, remove the 
	// next available task, and set the current running task to the next available
	if(s_tcbs[switch_info.cur_task].pri <= highest_sched) {
		prv_insert(&s_tcbs[switch_info.cur_task], s_tcbs[switch_info.cur_task].pri);
		CymricTCB *next = prv_remove((CymricPriority)highest_sched);
		
		// Update switch info for the context switch
		switch_info.next_top_addr = &next->top_addr;
		
		switch_info.cur_top_addr = &s_tcbs[switch_info.cur_task].top_addr;
		switch_info.cur_task = next->id; // now the next task
		
		// Initiate a context switch
		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	}
}

// Handler for SysTick interrupts (allows delays to work)
void SysTick_Handler(void) {
	s_ticks_ms++;
	HAL_IncTick(); // to be removed once unnecessary
	
	// Perform scheduling if necessary
	if(s_started_flag && (s_ticks_ms % CYMRIC_SCHED_INT_MS == 0)) {
			prv_schedule();
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
	control |= CONTROL_SPSEL_Msk;
	__set_CONTROL(control);
	
	// Change PSP to the address of the idle task
	__set_PSP((uint32_t)s_tcbs[CYMRIC_IDLE_ID].addr);
	
	// Configure systick
	s_ticks_ms = 0;
	s_started_flag = true;
	
	// Invoke idle task function
	//s_cur_alloc_id++;
	prv_insert(&s_tcbs[CYMRIC_IDLE_ID], CYMRIC_PRI_IDLE);
	prv_idle(0);
}

bool cymric_task_new(CymricTaskFunction func, void *args, CymricPriority pri) {
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
	
	// Update ID
	s_tcbs[s_cur_alloc_id].id = s_cur_alloc_id;
	
	// Update priority and insert
	s_tcbs[s_cur_alloc_id].pri = pri;
	prv_insert(&s_tcbs[s_cur_alloc_id], pri);
	
	s_cur_alloc_id++;
	return true;
}

void cymric_delay(uint32_t delay_ms) {
	uint32_t want = s_ticks_ms + delay_ms;
	while(s_ticks_ms < want) {}
}

uint32_t cymric_get_ticks(void) {
	return s_ticks_ms;
}

void cymric_thread_yield(void) {
	// Just run the scheduler early to push the thread back to the end of the line
	prv_schedule();
}
