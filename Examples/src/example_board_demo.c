/*============================================================================
 *           LPCXpresso 1343 + Embedded Artists Development Board 
 *---------------------------------------------------------------------------*
 *            Universidade Tecnol�gica Federal do Paran� (UTFPR)
 *===========================================================================*/
#include "mcu_regs.h"
#include "libdemo.h"
#include "type.h"
#include "uart.h"
#include "stdio.h"
#include "timer32.h"
#include "i2c.h"
#include "gpio.h"
#include "ssp.h"
#include "adc.h"
#include "light.h"
#include "oled.h"
#include "rgb.h"
#include "temp.h"
#include "acc.h"
//#include "led7seg.h"
//#include "pca9532.h"



osTimerId conta;

void contaHandler(void const *n){
    msTicks++;
}

osTimerDef(conta_0, contaHandler);
int main (void)
{
    SystemCoreClockUpdate();
    osKernelInitialize();
    conta = osTimerCreate(osTimer(conta_0), osTimerPeriodic,(void*)0);
    osTimerStart(conta,  10);
    osKernelStart();

    int32_t xoff = 0;
    int32_t yoff = 0;
    int32_t zoff = 0;

    int8_t x = 0;
    int8_t y = 0;
    int8_t z = 0;

    int32_t t = 0;
    uint32_t lux = 0;
    uint32_t trim = 0;

    uint16_t leds = 0x0000;
    uint8_t seg = '1';
    
    GPIOInit();
    init_timer32(0, 10);
    
    UARTInit(115200);
    UARTSendString((uint8_t*)"OLED - Peripherals\r\n");

    I2CInit( (uint32_t)I2CMASTER, 0 );
    SSPInit();
    ADCInit( ADC_CLK );

    oled_init();
    light_init();
    acc_init();
    led7seg_init();
    rgb_init();
    temp_init (&getTicks);

    /* setup sys Tick. Elapsed time is e.g. needed by temperature sensor */
    SysTick_Config(SystemCoreClock / 1000);
    if ( !(SysTick->CTRL & (1<<SysTick_CTRL_CLKSOURCE_Msk)) )
    {
      /* When external reference clock is used(CLKSOURCE in
      Systick Control and register bit 2 is set to 0), the
      SYSTICKCLKDIV must be a non-zero value and 2.5 times
      faster than the reference clock.
      When core clock, or system AHB clock, is used(CLKSOURCE
      in Systick Control and register bit 2 is set to 1), the
      SYSTICKCLKDIV has no effect to the SYSTICK frequency. See
      more on Systick clock and status register in Cortex-M3
      technical Reference Manual. */
      LPC_SYSCON->SYSTICKCLKDIV = 0x08;
    }

    /*
     * Assume base board in zero-g position when reading first value.
     */
    acc_read(&x, &y, &z);
    xoff = 0-x;
    yoff = 0-y;
    zoff = 64-z;
    
    // Example of OLED display
    light_enable();
    light_setRange(LIGHT_RANGE_4000);

    oled_clearScreen(OLED_COLOR_WHITE);

    oled_putString(1,1,  (uint8_t*)"Temp   : ", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    oled_putString(1,9,  (uint8_t*)"Light  : ", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    oled_putString(1,17, (uint8_t*)"Trimpot: ", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    oled_putString(1,25, (uint8_t*)"Acc x  : ", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    oled_putString(1,33, (uint8_t*)"Acc y  : ", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    oled_putString(1,41, (uint8_t*)"Acc z  : ", OLED_COLOR_BLACK, OLED_COLOR_WHITE);

    while(1) {
      
        /* RGB OLED - conflict with OLED power (green led)  */   
        //rgb_setLeds(rgb);
        //rgb++;
      
        /* PCA IC2 16-bit leds *
        if (leds == 0x0000) {
          leds = 0x0001;
          pca9532_setLeds(leds,0x00ff);
        }      
        else {  
          leds=leds*2;
          if (leds == 0x100) {
            leds = 0x0000;
            pca9532_setLeds(leds,0x00ff);
          }        
          else
            pca9532_setLeds(leds,leds/2);
        }
      
        /* 7-seg *
        led7seg_setChar(seg,FALSE);
        seg++; 
        if (seg=='9') seg='1';

        /* Accelerometer */
        acc_read(&x, &y, &z);
        x = x+xoff;
        y = y+yoff;
        z = z+zoff;

        /* Temperature */
        t = temp_read();

        /* light */
        lux = light_read();

        /* trimpot */
        trim = ADCRead(0);

        /* output values to OLED display */
        intToString(t, buf, 10, 10);
        oled_fillRect((1+9*6),1, 80, 8, OLED_COLOR_WHITE);
        oled_putString((1+9*6),1, buf, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

        intToString(lux, buf, 10, 10);
        oled_fillRect((1+9*6),9, 80, 16, OLED_COLOR_WHITE);
        oled_putString((1+9*6),9, buf, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

        intToString(trim, buf, 10, 10);
        oled_fillRect((1+9*6),17, 80, 24, OLED_COLOR_WHITE);
        oled_putString((1+9*6),17, buf, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

        intToString(x, buf, 10, 10);
        oled_fillRect((1+9*6),25, 80, 32, OLED_COLOR_WHITE);
        oled_putString((1+9*6),25, buf, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

        intToString(y, buf, 10, 10);
        oled_fillRect((1+9*6),33, 80, 40, OLED_COLOR_WHITE);
        oled_putString((1+9*6),33, buf, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

        intToString(z, buf, 10, 10);
        oled_fillRect((1+9*6),41, 80, 48, OLED_COLOR_WHITE);
        oled_putString((1+9*6),41, buf, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

        /* delay */
        delay32Ms(0, 200);
    }
}