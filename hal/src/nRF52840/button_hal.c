#include "button_hal.h"
#include <nrf_rtc.h>
#include <nrf_nvic.h>
#include "platform_config.h"
#include "interrupts_hal.h"
#include "pinmap_impl.h"
#include "nrfx_gpiote.h"
#include "gpio_hal.h"

button_config_t HAL_Buttons[] = {
    {
        .interrupt_mode = BUTTON1_GPIOTE_INTERRUPT_MODE,
        .active         = 0,
        .pin            = BUTTON1_GPIO_PIN,
        .debounce_time  = 0,
    },
    {
        .active         = 0,
        .debounce_time  = 0
    }
};

void BUTTON_Interrupt_Handler(void *data)
{
    BUTTON_Check_Irq((uint32_t)data);
}

void BUTTON_Check_Irq(uint16_t button)
{
    HAL_Buttons[button].debounce_time = 0x00;
    HAL_Buttons[button].active = 1;

    /* Disable button Interrupt */
    BUTTON_EXTI_Config(button, DISABLE);

    /* Enable RTC1 tick interrupt */
    nrf_rtc_int_enable(NRF_RTC1, NRF_RTC_INT_TICK_MASK);
}

/**
 * @brief  Configures Button GPIO, EXTI Line and DEBOUNCE Timer.
 * @param  Button: Specifies the Button to be configured.
 *   This parameter can be one of following parameters:
 *     @arg BUTTON1: Button1
 * @param  Button_Mode: Specifies Button mode.
 *   This parameter can be one of following parameters:
 *     @arg BUTTON_MODE_GPIO: Button will be used as simple IO
 *     @arg BUTTON_MODE_EXTI: Button will be connected to EXTI line with interrupt
 *                     generation capability
 * @retval None
 */
void BUTTON_Init(Button_TypeDef Button, ButtonMode_TypeDef Button_Mode)
{
    if (Button_Mode == BUTTON_MODE_EXTI)
    {
        /* Disable RTC0 tick Interrupt */
        nrf_rtc_int_disable(NRF_RTC1, NRF_RTC_INT_TICK_MASK);

        BUTTON_EXTI_Config(Button, ENABLE);

        /* Enable the RTC0 NVIC Interrupt */
        sd_nvic_SetPriority(RTC1_IRQn, RTC1_IRQ_PRIORITY);
        sd_nvic_ClearPendingIRQ(RTC1_IRQn);
        sd_nvic_EnableIRQ(RTC1_IRQn);
    }
}

void BUTTON_EXTI_Config(Button_TypeDef Button, FunctionalState NewState)
{
    HAL_InterruptExtraConfiguration config = {0};
    config.version = HAL_INTERRUPT_EXTRA_CONFIGURATION_VERSION;
    config.keepHandler = false;
    config.flags = HAL_DIRECT_INTERRUPT_FLAG_NONE;

    if (NewState == ENABLE)
    {
        HAL_Interrupts_Attach(HAL_Buttons[Button].pin, BUTTON_Interrupt_Handler, (void *)((int)Button), FALLING, &config); 
    }
    else
    {
        HAL_Interrupts_Detach(HAL_Buttons[Button].pin);
    }
}

/**
 * @brief  Returns the selected Button non-filtered state.
 * @param  Button: Specifies the Button to be checked.
 *   This parameter can be one of following parameters:
 *     @arg BUTTON1: Button1
 * @retval Actual Button Pressed state.
 */
uint8_t BUTTON_GetState(Button_TypeDef Button)
{
    HAL_Pin_Mode(HAL_Buttons[Button].pin, INPUT_PULLUP);

    return HAL_GPIO_Read(HAL_Buttons[Button].pin);
}

/**
 * @brief  Returns the selected Button Debounced Time.
 * @param  Button: Specifies the Button to be checked.
 *   This parameter can be one of following parameters:
 *     @arg BUTTON1: Button1
 * @retval Button Debounced time in millisec.
 */
uint16_t BUTTON_GetDebouncedTime(Button_TypeDef Button)
{
    return HAL_Buttons[Button].debounce_time;
}

void BUTTON_ResetDebouncedState(Button_TypeDef Button)
{
    HAL_Buttons[Button].debounce_time = 0;
}

void BUTTON_Check_State(uint16_t button, uint8_t pressed)
{
    if (BUTTON_GetState(button) == pressed)
    {
        if (!HAL_Buttons[button].active)
            HAL_Buttons[button].active = 1;
        HAL_Buttons[button].debounce_time += BUTTON_DEBOUNCE_INTERVAL;
    }
    else if (HAL_Buttons[button].active)
    {
        HAL_Buttons[button].active = 0;
        /* Enable button Interrupt */
        BUTTON_EXTI_Config(button, ENABLE);
    }
}

int BUTTON_Debounce()
{
    BUTTON_Check_State(BUTTON1, BUTTON1_PRESSED);

    int pressed = HAL_Buttons[BUTTON1].active;
    if (pressed == 0)
    {
        /* Disable RTC1 tick Interrupt */
        nrf_rtc_int_disable(NRF_RTC1, NRF_RTC_INT_TICK_MASK);
    }

    return pressed;
}

void BUTTON_Init_Ext()
{
    if (BUTTON_Debounce())
        nrf_rtc_int_enable(NRF_RTC1, NRF_RTC_INT_TICK_MASK);
}

uint8_t BUTTON_Is_Pressed(Button_TypeDef button)
{
    return HAL_Buttons[button].active;
}

uint16_t BUTTON_Pressed_Time(Button_TypeDef button)
{
    return HAL_Buttons[button].debounce_time;
}

void BUTTON_Uninit()
{
    for (int i = 0; i < BUTTONn; i++)
    {
        HAL_Interrupts_Detach(HAL_Buttons[i].pin);
    }
}
