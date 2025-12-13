#ifndef BSP_H
#define BSP_H

#include "xil_types.h"   // for u32, etc.

/* Button API */
void BSP_init(void);                 // init INTC + button GPIO + connect BtnIsr
void BSP_buttons_irq_init(void);     // start INTC, enable button IRQ
int  BSP_take_last_press(void);      // returns 1..4, or 0 if no new press

/* Timer API (used for timebase / debounce / smoothing) */
void BSP_timer_irq_init(u32 reset_value);  // start AXI timer in periodic IRQ mode
u32  BSP_take_timer_ticks(void);           // number of ticks since last call
u32  BSP_time_now(void);                   // running tick counter

/* Encoder API */
int  BSP_encoder_init(void);
int  BSP_encoder_irq_init(void);
void BSP_encoder_get_events(int* steps, int* pressed);

#endif
