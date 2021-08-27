// WIP RTOS
#include <inttypes.h>
#include <stdbool.h>

// Can be changed based on overall application stack size
#define CYMRIC_MAX_TASKS 6

// ID of the idle task
#define CYMRIC_IDLE_ID 0

// Size of task threads (in bytes)
#define CYMRIC_THREAD_STACK_SIZE 1024
#define CYMRIC_MAIN_STACK_SIZE 2048

// Location of reset value of the main stack pointer (see p.g. 17 of Cortex-M4 Generic User Guide)
#define CORTEX_M4_MSP_RST_ADDR 0x00000000ul

// Offset of CONTROL register bits (see p.g. 22 of Cortex-M4 Generic User Guide)
#define CORTEX_M4_CONTROL_SPSEL 1 // Currently active stack pointer
#define CORTEX_M4_CONTROL_nPRIV 0 // Thread mode privilege level

// CONTROL register bit storage values
#define CORTEX_M4_CONTROL_SPSEL_MSP 0
#define CORTEX_M4_CONTROL_SPSEL_PSP 1

#define CORTEX_M4_CONTROL_nPRIV_PRIV 0
#define CORTEX_M4_CONTROL_nPRIV_UNPRIV 1

// Task control block definition
typedef struct {
	uint32_t addr; // Base address of task stack
	uint32_t top_addr; // Address of top of task stack
} Cymric_TCB;

//void SysTick_Handler(void);

// Function pointer for thread functions.
typedef void (*CymricTaskFunction)(void *args);

// Initialize the OS.  Returns true if successful, false otherwise.
bool cymric_init(void);

// Start the RTOS.  This function transforms into the idle task and, as such, is blocking.
void cymric_start(void);

// Create a new task with the function pointer specified.  Returns true if successful, false otherwise.
bool cymric_task_new(CymricTaskFunction func, void *args);

// Run the scheduler.
void cymric_run(void);
