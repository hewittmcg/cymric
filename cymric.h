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

// Reset value of the main stack pointer (see p.g. 17 of Cortex-M4 Generic User Guide)
#define CORTEX_M4_MSP_RST_ADDR 0x00000000ul

// Task control block definition
typedef struct {
	uint32_t addr; // Base address of task stack
} Cymric_TCB;

// Function pointer for thread functions.
typedef void (*CymricTaskFunction)(void *args);

// Initialize the OS.  Returns true if successful, false otherwise.
bool cymric_init(void);

// Start the RTOS.  This function transforms into the idle task and, as such, is blocking.
void cymric_start(void);

// Create a new task with the function pointer specified.  Returns true if successful, false otherwise.
bool cymric_task_new(CymricTaskFunction);
