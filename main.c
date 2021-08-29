#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_rcc.h"

#include "cymric.h"

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

#define LED_ON() (HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET))
#define LED_OFF() (HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET))

// Test tasks
static void prv_task1(void *args) {
	while(1) {
		//asm("nop");
		LED_ON();
	}
}

static void prv_task2(void *args) {
	while(1) {
		//asm("nop");
		LED_OFF();
	}
}

int main(void) {
	HAL_Init();
	
	prv_led_gpio_init();

	cymric_init();
	
	// Different func, same args
	cymric_task_new(&prv_task1, &prv_task1, CYMRIC_PRI_HIGH);
	cymric_task_new(&prv_task2, &prv_task1, CYMRIC_PRI_HIGH);
	
	cymric_start();
}
