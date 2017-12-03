/*****************************************************************************
 *   This example shows how to access an MMC/SD card
 *
 *   Copyright(C) 2009, Embedded Artists AB
 *   All rights reserved.
 *
 ******************************************************************************/



#include "mcu_regs.h"
#include "type.h"
//#include "uart.h"
#include "stdio.h"
//#include "timer32.h"

#include "gpio.h"
#include "ssp.h"

#include "diskio.h"
#include "ff.h"


static FILINFO Finfo;
static FATFS Fatfs[1];
static uint8_t buf[64];

FRESULT open_append (
    FIL* fp,            /* [OUT] File object to create */
    const char* path    /* [IN]  File name to be opened */
)
{
    FRESULT fr;

    /* Opens an existing file. If not exist, creates a new file. */
    fr = f_open(fp, path, FA_WRITE | FA_OPEN_ALWAYS);
    if (fr == FR_OK) {
        /* Seek to end of the file to append data */
        //fr = f_lseek(fp, f_size(fp));
        if (fr != FR_OK)
            f_close(fp);
    }
    return fr;
}


void SysTick_Handler(void) {
    disk_timerproc();
}


/* Main Program */

int main (void) {
    DSTATUS stat;
    DWORD p2;
    WORD w1;
    BYTE res, b1;
    DIR dir;

    int i = 0;

    GPIOInit();
    //init_timer32(0, 10);

    //UARTInit(115200);
    printf("MMC/SD example\r\n");

    SSPInit();

    SysTick_Config(SystemCoreClock / 100);

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

    //delay32Ms(0, 500);

    stat = disk_initialize(0);
    if (stat & STA_NOINIT) {
        //printf("MMC: not initialized\r\n");
        return 1;
    }

    if (stat & STA_NODISK) {
        //printf("MMC: No Disk\r\n");
        return 1;
    }

    if (stat != 0) {
        return 1;
    }

    printf("MMC: Initialized\r\n");
    
        FRESULT fr;
    FATFS fs;
    FIL fil;


/*
    if (disk_ioctl(0, GET_SECTOR_COUNT, &p2) == RES_OK) {
        //printf("Drive size: %d \r\n", p2);
        //UARTSend(buf, i);
    }

    if (disk_ioctl(0, GET_SECTOR_SIZE, &w1) == RES_OK) {
        //i = sprintf((char*)buf, "Sector size: %d \r\n", w1);
        //UARTSend(buf, i);
    }

    if (disk_ioctl(0, GET_BLOCK_SIZE, &p2) == RES_OK) {
        //i = sprintf((char*)buf, "Erase block size: %d \r\n", p2);
        //UARTSend(buf, i);
    }

    if (disk_ioctl(0, MMC_GET_TYPE, &b1) == RES_OK) {
        //i = sprintf((char*)buf, "Card type: %d \r\n", b1);
        //UARTSend(buf, i);
    }*/

    res = f_mount(0, &Fatfs[0]);
    if (res != FR_OK) {
        //i = sprintf((char*)buf, "Failed to mount 0: %d \r\n", res);
        //UARTSend(buf, i);
        return 1;
    }

    res = f_opendir(&dir, "/");
    if (res) {
        i = sprintf((char*)buf, "Failed to open /: %d \r\n", res);
        //UARTSend(buf, i);
        return 1;
    }

    /*for(;;) {
        res = f_readdir(&dir, &Finfo);
        if ((res != FR_OK) || !Finfo.fname[0]) break;

        //UARTSendString((uint8_t*)&(Finfo.fname[0]));
        //UARTSendString((uint8_t*)"\r\n");

    }*/
    
        /* Open or create a log file and ready to append */
    
    //f_mount(&fs, "", 0);
    fr = open_append(&fil, "spi.txt");
    if (fr != FR_OK) return 1;

    char line[100];
    
    f_write(&fil, "funcion\n", 8, NULL);
    res = f_lseek(&fil, 8);
    f_write(&fil, "funcion\n", 8, NULL);
    /* Close the file */
   /* while (f_gets(line, sizeof line, &fil)) {
        
    }
    f_write(&fil, "funcion\n", 16, NULL);
    res = f_lseek(&fil, 25);
    f_write(&fil, "funcion\n", 8, NULL);*/
    f_close(&fil);
    
    
    
    
    



    while(1);


}
