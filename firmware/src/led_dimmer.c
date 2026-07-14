#include "led_dimmer.h"
#include "definitions.h"
#include "FreeRTOS.h"
#include "task.h"

#define LED_R_PIN    GPIO_PIN_RB0
#define LED_G_PIN    GPIO_PIN_RB3
#define LED_B_PIN    GPIO_PIN_RB5

static volatile uint8_t s_stateR = 0;
static volatile uint8_t s_stateG = 0;
static volatile uint8_t s_stateB = 0;
static volatile bool s_identifying = false;

static void LED_GPIO_SetRGB(uint8_t r, uint8_t g, uint8_t b)
{
    if (r)
        GPIO_PinSet(LED_R_PIN);
    else
        GPIO_PinClear(LED_R_PIN);

    if (g)
        GPIO_PinSet(LED_G_PIN);
    else
        GPIO_PinClear(LED_G_PIN);

    if (b)
        GPIO_PinSet(LED_B_PIN);
    else
        GPIO_PinClear(LED_B_PIN);
}

void LED_Dimmer_Init(void)
{
    GPIO_PinOutputEnable(LED_R_PIN);
    GPIO_PinOutputEnable(LED_G_PIN);
    GPIO_PinOutputEnable(LED_B_PIN);

    LED_GPIO_SetRGB(0, 0, 0);

    LED_GPIO_SetRGB(1, 1, 1);
    vTaskDelay(pdMS_TO_TICKS(300));
    LED_GPIO_SetRGB(0, 0, 0);
}

void LED_Dimmer_SetRGB(uint8_t r_pct, uint8_t g_pct, uint8_t b_pct)
{
    s_stateR = (r_pct > 0) ? 1 : 0;
    s_stateG = (g_pct > 0) ? 1 : 0;
    s_stateB = (b_pct > 0) ? 1 : 0;

    if (!s_identifying)
        LED_GPIO_SetRGB(s_stateR, s_stateG, s_stateB);
}

void LED_Dimmer_GetRGB(uint8_t *r_pct, uint8_t *g_pct, uint8_t *b_pct)
{
    if (r_pct) *r_pct = s_stateR ? 100 : 0;
    if (g_pct) *g_pct = s_stateG ? 100 : 0;
    if (b_pct) *b_pct = s_stateB ? 100 : 0;
}

void LED_Dimmer_Identify(void)
{
    s_identifying = true;

    for (uint8_t i = 0; i < 10; i++)
    {
        LED_GPIO_SetRGB(1, 1, 1);
        vTaskDelay(pdMS_TO_TICKS(150));
        LED_GPIO_SetRGB(0, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(150));
    }

    s_identifying = false;
    LED_GPIO_SetRGB(s_stateR, s_stateG, s_stateB);
}
