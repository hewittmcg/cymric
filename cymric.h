// WIP RTOS
#include <inttypes.h>
#include <stdbool.h>

// Can be changed based on overall application stack size
#define CYMRIC_MAX_TASKS 6

// ID of the idle task
#define CYMRIC_IDLE_ID 0

// Interval to perform scheduling on
#define CYMRIC_SCHED_INT_MS 1000

// Size of task threads (in bytes)
#define CYMRIC_THREAD_STACK_SIZE 1024
#define CYMRIC_MAIN_STACK_SIZE 2048

// Location of reset value of the main stack pointer (see p.g. 17 of Cortex-M4 Generic User Guide)
#define CORTEX_M4_MSP_RST_ADDR 0x00000000ul

// PSR default address
#define PSR_DEFAULT 0x01000000

// Lowest priority
#define CYMRIC_SYSTICK_PRIORITY 0x00

// Highest priority, to avoid nested interrupts affecting the stack
#define CYMRIC_PENDSV_PRIORITY 0xFF

typedef enum {
	CYMRIC_PRI_IDLE = 0,
	CYMRIC_PRI_LOW,
	CYMRIC_PRI_MED,
	CYMRIC_PRI_HIGH,
	NUM_CYMRIC_PRIORITIES,
} CymricPriority;

// Thread function definition.
typedef void (*CymricTaskFunction)(void *args);

// Initialize the RTOS.  Returns true if successful, false otherwise.
bool cymric_init(void);

// Start the RTOS.  This function transforms into the idle task and, as such, is blocking.
void cymric_start(void);

// Create a new task with the function pointer, arguments, and priority specified.  Returns true if successful, false otherwise.
bool cymric_task_new(CymricTaskFunction func, void *args, CymricPriority pri);
