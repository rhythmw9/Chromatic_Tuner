#include <stdio.h>
#include "qpn_port.h"
#include "tuner.h"
#include "xil_printf.h"
#include "xil_cache.h"
#include "lcd.h"
#include "tuner_display.h"
#include "xparameters.h"
#include "xgpio.h"
#include "xspi.h"
#include "bsp.h"


#define CAL_IDLE_TICKS 50   // Used to go back to main state after no encoder input

/* BSP functions */
void BSP_display(char const *msg) {
    xil_printf("%s", msg);
}

void BSP_exit(void) {
    xil_printf("Exiting...\n");
}

/* dispatcher*/
static void dispatch(QSignal sig) {
    Q_SIG((QHsm *)&HSM_Tuner) = sig;
    QHsm_dispatch((QHsm *)&HSM_Tuner);
}

void Q_onAssert(char const Q_ROM * const Q_ROM_VAR file, int line) {
    xil_printf("ASSERT FAILED: %s:%d\n", file, line);
    while (1) {
        // wait to see error in terminal 
    }
}



/* Crude delay */
static void delay(volatile int loops) {
    for (volatile int i = 0; i < loops; ++i) { }
}



int main(void) {
    
    // flags
	static int enc_accum = 0;
	static int cal_idle = 0;

    // Enable caches
	Xil_ICacheInvalidate();
	Xil_ICacheEnable();
	Xil_DCacheInvalidate();
	Xil_DCacheEnable();


	xil_printf("Chromatic Tuner Project starting...\r\n");


	// SPI + LCD init
	XSpi_Config *spiConfig;
	XSpi spi;
	XGpio dc;


	// Initialize GPIO
	if (XGpio_Initialize(&dc, XPAR_SPI_DC_DEVICE_ID) != XST_SUCCESS)
		return -1;

	// Initialize SPI
	spiConfig = XSpi_LookupConfig(XPAR_SPI_DEVICE_ID);
	if (!spiConfig) return -1;

	if (XSpi_CfgInitialize(&spi, spiConfig, spiConfig->BaseAddress) != XST_SUCCESS)
		return -1;

	XSpi_Reset(&spi);

	// Enable master mode
	u32 ctrl = XSpi_GetControlReg(&spi);
	XSpi_SetControlReg(&spi, (ctrl | XSP_CR_ENABLE_MASK | XSP_CR_MASTER_MODE_MASK) &
							  (~XSP_CR_TRANS_INHIBIT_MASK));
	XSpi_SetSlaveSelectReg(&spi, ~0x01);

	// Initialize LCD driver
	initLCD();

	// Initialize the rest of the hardware
	BSP_init();
	BSP_buttons_irq_init();
	BSP_encoder_init();
	BSP_encoder_irq_init();


	// HSM construct + init
	Tuner_ctor();
	QHsm_init((QHsm *)&HSM_Tuner);


	xil_printf("=== Entering main loop ===\r\n");


    // ---- main event loop ----
    while (1) {
        // drive the state machine
        dispatch(TICK_SIG);

        // change modes with push buttons
        int m = BSP_take_last_press();
        if (m) {

            //xil_printf("Button %d pressed\r\n", m);

            switch (m) {
                case 1:   // BTNU -> MAIN
                    dispatch(BTN_MAIN_SIG);
                    break;
                case 2:   // BTNL -> DEBUG
                    dispatch(BTN_DEBUG_SIG);
                    break;
                case 3:   // BTNR -> CAL
                    dispatch(BTN_CAL_SIG);
                    break;
                default:
                    break;
            }
        }

        // encoder events
        int steps, pressed;
        BSP_encoder_get_events(&steps, &pressed);

        if (HSM_Tuner.mode == TUNER_MODE_CAL) {

            // apply rotation to ref A4 via ROT_* signals
            if (steps != 0) {
                enc_accum += steps;

                const int EDGES_PER_HZ = 3;   // tuned so 1 detent ≈ 1 Hz

                // CW
                while (enc_accum >= EDGES_PER_HZ) {
                    dispatch(ROT_CW_SIG);
                    enc_accum -= EDGES_PER_HZ;
                }

                // CCW
                while (enc_accum <= -EDGES_PER_HZ) {
                    dispatch(ROT_CCW_SIG);
                    enc_accum += EDGES_PER_HZ;
                }

                // knob moved -> reset idle timer
                cal_idle = 0;
            } else {
                // no rotation this loop -> count toward "stopped"
                if (cal_idle < CAL_IDLE_TICKS) {
                    cal_idle++;
                }
            }

            // encoder press = exit CAL immediately
            if (pressed) {
                cal_idle = CAL_IDLE_TICKS;
            }

            // once we've been idle long enough in CAL, transition back to MAIN
            if (cal_idle >= CAL_IDLE_TICKS) {
                dispatch(BTN_MAIN_SIG);   // remove overlay, keep new tuning
                cal_idle  = 0;
                enc_accum = 0;
            }

        } else {
            // not in CAL: ignore encoder and keep the timers reset
            enc_accum = 0;
            cal_idle  = 0;
        }

        // crude delay to keep UI readable
        delay(5000);

    /* End Main Event Loop*/
    }

    return 0;
}
