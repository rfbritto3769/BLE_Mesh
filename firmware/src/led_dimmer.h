#ifndef LED_DIMMER_H
#define LED_DIMMER_H

#include <stdint.h>

void LED_Dimmer_Init(void);
void LED_Dimmer_SetRGB(uint8_t r_pct, uint8_t g_pct, uint8_t b_pct);
void LED_Dimmer_GetRGB(uint8_t *r_pct, uint8_t *g_pct, uint8_t *b_pct);
void LED_Dimmer_Identify(void);

#endif
