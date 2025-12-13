#include "bsp.h"
#include "xparameters.h"
#include "xintc.h"
#include "xgpio.h"
#include "xtmrctr.h"
#include "xtmrctr_l.h"
#include "xil_exception.h"
#include "xil_printf.h"

/* ---------- Static HW instances ---------- */

static XIntc   intc;
static XGpio   btngpio;
static XTmrCtr tmr;
static XGpio   ENC;

/* ---------- Button state ---------- */

static volatile int last_press = 0;

/* ---------- Timer / timebase ---------- */

static volatile u32 s_timer_ticks = 0;
static volatile u32 s_time_now    = 0;

/* ---------- Encoder state ---------- */

#define ENC_CH          1
#define ENC_A_BIT       0
#define ENC_B_BIT       1
#define ENC_BTN_BIT     2

#define ENC_GPIO_INTR_ID XPAR_MICROBLAZE_0_AXI_INTC_ENCODER_IP2INTC_IRPT_INTR

#define ENC_BTN_ACTIVE_LOW  1   // 1 if encoder button pulls line low when pressed
#define DEBOUNCE_TICKS     10u  // ticks to debounce button

/* ---------- Static globals ---------- */

static volatile uint8_t prev_ab        = 3;  // last A/B quadrature state
static volatile uint8_t flag_cw        = 0;
static volatile uint8_t flag_ccw       = 0;
static volatile uint8_t btn_irq_seen   = 0;
static volatile u32     btn_last_time  = 0;

/* Quadrature lookup table */
static const int8_t QTAB[16] = {
/* prev->curr: 00,01,10,11 */
    0, +1, -1,  0,
   -1,  0,  0, +1,
   +1,  0,  0, -1,
    0, -1, +1,  0
};

#ifndef ENC_DIR_FLIP
#define ENC_DIR_FLIP 0
#endif

/* ---------- Local ISR prototypes ---------- */

static void BtnIsr   (void *CallbackRef);
static void TimerIsr (void *CallbackRef);
static void EncIsr   (void *CallbackRef);

/* Helper to read encoder A/B as 2-bit number */
static inline uint8_t read_ab(void) {
    u32 v = XGpio_DiscreteRead(&ENC, ENC_CH);
    uint8_t A = (v >> ENC_A_BIT) & 1u;
    uint8_t B = (v >> ENC_B_BIT) & 1u;
    return (uint8_t)((A << 1) | B);   // 11=3 idle, 01=1, 00=0, 10=2
}


/* ========================================================================== */
/*                            BUTTON / INTC INIT                              */
/* ========================================================================== */

void BSP_init(void) {
    int status;

    /* Initialize button GPIO  */
    status = XGpio_Initialize(&btngpio, XPAR_AXI_GPIO_BTN_DEVICE_ID);
    if (status != XST_SUCCESS) {
        xil_printf("BSP_init: XGpio_Initialize(btngpio) failed\r\n");
        return;
    }
    XGpio_SetDataDirection(&btngpio, 1, 0xFFFFFFFF); // inputs

    /* Initialize interrupt controller and connect button ISR */
    status = XIntc_Initialize(&intc, XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);
    if (status != XST_SUCCESS) {
        xil_printf("BSP_init: XIntc_Initialize failed\r\n");
        return;
    }

    status = XIntc_Connect(&intc,
                           XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_BTN_IP2INTC_IRPT_INTR,
                           (XInterruptHandler)BtnIsr,
                           &btngpio);
    if (status != XST_SUCCESS) {
        xil_printf("BSP_init: XIntc_Connect(BtnIsr) failed\r\n");
        return;
    }
}

/* Start INTC, enable button IRQ, and arm GPIO interrupts */
void BSP_buttons_irq_init(void) {
    /* Start interrupt controller in real mode */
    XIntc_Start(&intc, XIN_REAL_MODE);

    /* Enable button IRQ line in INTC */
    XIntc_Enable(&intc,
                 XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_BTN_IP2INTC_IRPT_INTR);

    /* Clear any stale button interrupt and enable GPIO IRQs */
    XGpio_InterruptClear(&btngpio, 1);
    XGpio_InterruptEnable(&btngpio, 1);
    XGpio_InterruptGlobalEnable(&btngpio);

    /* Enable CPU-level interrupts */
    microblaze_enable_interrupts();
}

/* Atomically read and clear last_press. 0 means no new press. */
int BSP_take_last_press(void) {
    int v = last_press;
    if (v != 0) {
        last_press = 0;
    }
    return v;
}

/* Button ISR: decode which button and stash it in last_press */
static void BtnIsr(void *CallbackRef) {
    XGpio *g = (XGpio *)CallbackRef;
    u32 b = XGpio_DiscreteRead(g, 1);

    if      (b & 0x00000001u) last_press = 1;  // BTN0
    else if (b & 0x00000002u) last_press = 2;  // BTN1
    else if (b & 0x00000004u) last_press = 3;  // BTN2
    else if (b & 0x00000008u) last_press = 4;  // BTN3

    /* ACK GPIO interrupt */
    XGpio_InterruptClear(g, 1);
}


/* ========================================================================== */
/*                           TIMER / TIMEBASE                                 */
/* ========================================================================== */

static void TimerIsr(void *CallbackRef) {
    XTmrCtr *t = (XTmrCtr *)CallbackRef;

    u32 csr = XTmrCtr_ReadReg(t->BaseAddress, 0, XTC_TCSR_OFFSET);

    /* count ticks and accumulate into time_now */
    s_timer_ticks++;
    XTmrCtr_WriteReg(t->BaseAddress, 0, XTC_TCSR_OFFSET,
                     csr | XTC_CSR_INT_OCCURED_MASK);
}

/* Configure AXI Timer 0 in periodic + IRQ mode with given reset value */
void BSP_timer_irq_init(u32 reset_value) {
    int status;

    status = XTmrCtr_Initialize(&tmr, XPAR_AXI_TIMER_0_DEVICE_ID);
    if (status != XST_SUCCESS) {
        xil_printf("BSP_timer_irq_init: XTmrCtr_Initialize failed\r\n");
        return;
    }

    /* Connect timer ISR into the shared INTC */
    status = XIntc_Connect(&intc,
                            XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR,
                            (XInterruptHandler)TimerIsr,
                            &tmr);
    if (status != XST_SUCCESS) {
        xil_printf("BSP_timer_irq_init: XIntc_Connect(TimerIsr) failed\r\n");
        return;
    }

    XIntc_Enable(&intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR);

    /* Periodic + interrupt mode */
    XTmrCtr_SetOptions(&tmr, 0, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);

    /* Set reset value to control tick period */
    XTmrCtr_SetResetValue(&tmr, 0, reset_value);

    /* Start timer */
    XTmrCtr_Start(&tmr, 0);
}

/* Return and clear number of timer ticks since last call. Also advances time_now. */
u32 BSP_take_timer_ticks(void) {
    u32 n = s_timer_ticks;
    if (n != 0u) {
        s_timer_ticks = 0u;
        s_time_now   += n;
    }
    return n;
}

/* Time in ticks since start. */
u32 BSP_time_now(void) {
    return s_time_now + s_timer_ticks;
}


/* ========================================================================== */
/*                             ENCODER                                        */
/* ========================================================================== */

int BSP_encoder_init(void) {
    int status;

    status = XGpio_Initialize(&ENC, XPAR_ENCODER_DEVICE_ID);
    if (status != XST_SUCCESS) {
        xil_printf("BSP_encoder_init: XGpio_Initialize(ENC) failed\r\n");
        return XST_FAILURE;
    }

    XGpio_SetDataDirection(&ENC, ENC_CH, 0xFFFFFFFF); 

    /* seed prev_ab from real hardware state */
    uint8_t ab = read_ab();
    prev_ab = ab;

    return XST_SUCCESS;
}

int BSP_encoder_irq_init(void) {
    int status;

    status = XIntc_Connect(&intc,
                           ENC_GPIO_INTR_ID,
                           (XInterruptHandler)EncIsr,
                           &ENC);
    if (status != XST_SUCCESS) {
        xil_printf("BSP_encoder_irq_init: XIntc_Connect(EncIsr) failed\r\n");
        return XST_FAILURE;
    }

    XIntc_Enable(&intc, ENC_GPIO_INTR_ID);

    /* clear & enable GPIO interrupts on encoder GPIO */
    XGpio_InterruptClear(&ENC, XGPIO_IR_CH1_MASK);
    XGpio_InterruptEnable(&ENC, XGPIO_IR_CH1_MASK);
    XGpio_InterruptGlobalEnable(&ENC);

    microblaze_enable_interrupts();

    return XST_SUCCESS;
}

/* Return net steps and press flag */
void BSP_encoder_get_events(int* steps, int* pressed) {
    /* steps = cw - ccw */
    *steps   = (int)flag_cw - (int)flag_ccw;
    flag_cw  = 0;
    flag_ccw = 0;

    if (btn_irq_seen) {
		*pressed = 1;
		btn_irq_seen = 0;   // consume the click
	} else {
		*pressed = 0;
	}
}

/* Encoder ISR: quadrature decode + button edge detection */
static void EncIsr(void *CallbackRef) {
    XGpio *g = (XGpio *)CallbackRef;

    /* ---- Quadrature ---- */
    uint8_t curr = read_ab();
    uint8_t idx  = (uint8_t)((prev_ab << 2) | curr);
    int8_t d     = QTAB[idx];

#if ENC_DIR_FLIP
    d = (int8_t)(-d);
#endif

    if (d > 0) {
        flag_cw++;
    } else if (d < 0) {
        flag_ccw++;
    }

    prev_ab = curr;

    /* ---- Button ---- */
    u32 v = XGpio_DiscreteRead(g, ENC_CH);
    uint8_t raw_btn = (uint8_t)((v >> ENC_BTN_BIT) & 1u);
    uint8_t btn     = ENC_BTN_ACTIVE_LOW ? (uint8_t)!raw_btn : raw_btn;

    static uint8_t last_btn = 0xFF;
    if (last_btn == 0xFF) {
        last_btn = btn;
    } else if (btn != last_btn) {
        last_btn = btn;
        if (btn == 1u) {
            btn_irq_seen  = 1;
            btn_last_time = BSP_time_now();
        }
    }

    /* ACK encoder GPIO IRQ */
    XGpio_InterruptClear(g, XGPIO_IR_CH1_MASK);
}
