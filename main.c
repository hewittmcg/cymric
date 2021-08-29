#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_rcc.h"

#include "cymric.h"
#include "cymric_semaphore.h"

// Initialize the GPIO pin corresponding to LED2 on the nucleo board
static void prv_led_gpio_init(void) {
	GPIO_InitTypeDef s_init_struct;
	__GPIOA_CLK_ENABLE();
	
	// Configure LED
	s_init_struct.Pin = GPIO_PIN_5;
	s_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
	s_init_struct.Speed = GPIO_SPEED_FAST;
	HAL_GPIO_Init(GPIOA, &s_init_struct);
}

// Initialize the GPIO pin corresponding to B1 on the nucleo board
static void prv_btn_gpio_init(void) {
	GPIO_InitTypeDef init_struct;
	__GPIOC_CLK_ENABLE();
	
	// Configure button
	init_struct.Pin = GPIO_PIN_13;
	init_struct.Mode = GPIO_MODE_INPUT;
	init_struct.Speed = GPIO_SPEED_FAST;
	HAL_GPIO_Init(GPIOC, &init_struct);
}

#define LED_ON() (HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET))
#define LED_OFF() (HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET))
#define LED_TOGGLE() (HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5))

// Tasks for testing the RTOS.

// LED blink interval, controlled by button state
static uint32_t s_blink_delay_ms = 500;

// Delays for LED when button pressed/unpressed
static uint32_t s_blink_interval_ms[2] = {100, 500};

// Blink the LED.
static void prv_led_blink(void *args) {
	while(1) {
		//asm("nop");
		LED_ON();
		cymric_delay(s_blink_delay_ms);
		LED_OFF();
		cymric_delay(s_blink_delay_ms);
	}
}

// Read the button and adjust the blink LED depending on its state.
static void prv_btn_read(void *args) {
	uint32_t *blink_delays_ms = (uint32_t*)args;
	while(1) {
		// Read from the button and update the blink delay correspondingly.
		if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET) {
			// Button pressed
			s_blink_delay_ms = blink_delays_ms[0];
		} else {
			// Button released
			s_blink_delay_ms = blink_delays_ms[1];
		}
	}
}


// Toggle the LED when a semaphore is released.
static void prv_led_toggle_sem(void *args) {
	CymricSemaphore *sem = args;
	while(1) {
		LED_TOGGLE();
		cymric_sem_wait(sem, CYMRIC_TIMEOUT_FOREVER);
	}
}

// Signal the semaphore each time the button is pressed and released
static void prv_btn_read_sem(void *args) {
	CymricSemaphore *sem = args;
	GPIO_PinState prev_state = GPIO_PIN_SET;
	GPIO_PinState state = GPIO_PIN_SET;
	while(1) {
		state = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);
		if(state != prev_state) {
			prev_state = state;
			if(state == GPIO_PIN_RESET) {
				cymric_sem_signal(sem);
			}
			cymric_delay(10); // Debouncing
		}
	}
}

static CymricSemaphore s_sem;

int main(void) {
	HAL_Init();
	
	prv_led_gpio_init();
	prv_btn_gpio_init();
	
	cymric_init();
	
	// Tasks to turn LED on/off using semaphores and button press
	s_sem = cymric_sem_init(0);
	cymric_task_new(&prv_led_toggle_sem, &s_sem, CYMRIC_PRI_HIGH);
	cymric_task_new(&prv_btn_read_sem, &s_sem, CYMRIC_PRI_HIGH);
	
	cymric_start();
		
	// Tasks to vary LED blink rate based on button press (unreached, left uncommented to remove warnings)
	cymric_task_new(&prv_led_blink, NULL, CYMRIC_PRI_HIGH);
	cymric_task_new(&prv_btn_read, s_blink_interval_ms, CYMRIC_PRI_HIGH);
}
