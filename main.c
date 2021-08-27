#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_rcc.h"

#include "cymric.h"

// Initialize the GPIO pin corresponding to LED2
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

int main(void) {
	HAL_Init();
	
	prv_led_gpio_init();

	cymric_init();
	
	// cymric_start();
	
	// Blink LED2
	while(1) {
		LED_ON();
		HAL_Delay(500);
		LED_OFF();
		HAL_Delay(500);
	}
}
