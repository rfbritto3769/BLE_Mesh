#include "led_dimmer.h"
#include "definitions.h"
#include "FreeRTOS.h"
#include "task.h"

#define LED_PWM_R_CHANNEL    TCC1_CHANNEL0  /* PB0 -> TCC1/WO0 */
#define LED_PWM_G_CHANNEL    TCC1_CHANNEL1  /* PB3 -> TCC1/WO1 */
#define LED_PWM_B_CHANNEL    TCC1_CHANNEL2  /* PB5 -> TCC1/WO2 */

static volatile uint8_t s_dutyR = 0;
static volatile uint8_t s_dutyG = 0;
static volatile uint8_t s_dutyB = 0;
static volatile bool s_identifying = false;

static uint32_t LED_PWM_DutyFromPercent(uint8_t percent)
{
    uint32_t periodCounts = TCC1_PWM24bitPeriodGet() + 1U;

    return (periodCounts * percent) / 100U;
}

static void LED_PWM_SetRGB(uint8_t r_pct, uint8_t g_pct, uint8_t b_pct)
{
    (void)TCC1_PWM24bitDutySet(LED_PWM_R_CHANNEL, LED_PWM_DutyFromPercent(r_pct));
    (void)TCC1_PWM24bitDutySet(LED_PWM_G_CHANNEL, LED_PWM_DutyFromPercent(g_pct));
    (void)TCC1_PWM24bitDutySet(LED_PWM_B_CHANNEL, LED_PWM_DutyFromPercent(b_pct));
    TCC1_PWMForceUpdate();
}

void LED_Dimmer_Init(void)
{
    LED_PWM_SetRGB(0U, 0U, 0U);
    TCC1_PWMStart();

    LED_PWM_SetRGB(100U, 100U, 100U);
    vTaskDelay(pdMS_TO_TICKS(300));
    LED_PWM_SetRGB(0U, 0U, 0U);
}

void LED_Dimmer_SetRGB(uint8_t r_pct, uint8_t g_pct, uint8_t b_pct)
{
    if (r_pct > 100) r_pct = 100;
    if (g_pct > 100) g_pct = 100;
    if (b_pct > 100) b_pct = 100;
    s_dutyR = r_pct;
    s_dutyG = g_pct;
    s_dutyB = b_pct;

    if (!s_identifying)
        LED_PWM_SetRGB(r_pct, g_pct, b_pct);
}

void LED_Dimmer_GetRGB(uint8_t *r_pct, uint8_t *g_pct, uint8_t *b_pct)
{
    if (r_pct) *r_pct = s_dutyR;
    if (g_pct) *g_pct = s_dutyG;
    if (b_pct) *b_pct = s_dutyB;
}

void LED_Dimmer_Identify(void)
{
    s_identifying = true;

    for (uint8_t i = 0; i < 10; i++)
    {
        LED_PWM_SetRGB(100U, 100U, 100U);
        vTaskDelay(pdMS_TO_TICKS(150));
        LED_PWM_SetRGB(0U, 0U, 0U);
        vTaskDelay(pdMS_TO_TICKS(150));
    }

    s_identifying = false;
    LED_PWM_SetRGB(s_dutyR, s_dutyG, s_dutyB);
}
