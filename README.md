# cymric

This is a basic RTOS that I wrote, loosely based on a project that used to be assigned in MTE 241, a class I took in my second year of university.  The prof provided us with the project outline, and I decided to undertake it to learn more about the inner workings of a real-time operating system by writing it for a Cortex-M4-based microcontroller I had.

Cymric supports basic scheduling of tasks using fixed-priority pre-emptive scheduling.  It includes a basic blocking delay function, as well as threadsafe mutex and semaphore implementations.

# Basic use
 
A brief example of the use of the RTOS can be found in main.c.  cymric.h also has some basic documentation for function signatures.  However, some general instructions on its use are as follows:

## Setup
Basic setup instructions are as follows:
- Include "cymric.h" in your source file.
- Define any task functions.
  - Task functions have the following signature: `typedef void (*CymricTaskFunction)(void *args);` 
- To initialize the RTOS (prior to performing any related operations/scheduling tasks), call `cymric_init();`

## Scheduling tasks
Tasks can be scheduled once the RTOS is initialized by calling: 
`cymric_task_new(func, args, pri);`

Argument | Type | Description
--- | --- | --- 
func | CymricTaskFunction | A pointer to the task function to be scheduled.
args | void* | A pointer to argument(s) to be passed into the task.
pri | CymricPriority | The priority of the task.

## Starting
To run the RTOS, call:
`cymric_start();`

This will start the RTOS. Note that this is an infinitely blocking call.

# Porting to your platform

Cymric was written for a STM32F446RE that I had lying around, so some things will need to be adapted for your use:

- The STM32F4xx-specific includes in cymric.c will need to be changed to ones that match your processor architecture, along with any functions that depend on these if there is a naming convention difference.
- PendSV_Handler() in cymric.c may need to be changed based on the registers that your processor needs to push/pop when executing a context switch.

# TODO (non-exhaustive)
- Test more exhaustively and cleanly instead of having one main.c for everything.
- Clean up function documentation into one part of the README.
- Include mutex + semaphore documentation in README.
- Remove logging files from IDE to clean up repository.
- Include cymric_delay documentation in README.