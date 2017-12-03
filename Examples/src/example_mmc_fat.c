
/*============================================================================
 *             Exemplos de utilização do RTOS CMSISc com MMC/SD
 *           LPCXpresso 1343 + Embedded Artists Development Board 
 *---------------------------------------------------------------------------*
 *                             Acesso ao cartão SD
 *---------------------------------------------------------------------------*
 *               
 *                http://elm-chan.org/fsw/ff/00index_e.html
 *               
 *---------------------------------------------------------------------------*
 *                    Prof. César Ofuchi
 *            Universidade Tecnológica Federal do Paraná (UTFPR)
 * 
 *===========================================================================*/


#include "libdemo.h"
#include "mcu_regs.h"
#include "type.h"
#include "uart.h"
#include "stdio.h"
#include "timer32.h"

#include "gpio.h"
#include "ssp.h"

#include "diskio.h"
#include "ff.h"


static FILINFO Finfo;
static FATFS Fatfs[1];
static uint8_t buf_mmc[64];

void osDiskTimer(void const *param) {
  //chamada a cada 10ms para verificar se cartão esta conectado
    disk_timerproc();
}
//name, function
osTimerDef(timer10,osDiskTimer);

/* Main Program */

int main (void) {
    osKernelInitialize();
    
  
    DSTATUS stat;
    DWORD p2;
    WORD w1;
    BYTE res, b1;
    DIR dir;

    int i = 0;

    GPIOInit();
    init_timer32(0, 10);

    UARTInit(115200);
    UARTSendString((uint8_t*)"MMC/SD example\r\n");

    SSPInit();
    osTimerId timer10_handle = osTimerCreate(osTimer(timer10), osTimerPeriodic, (void *)0);    
    osTimerStart(timer10_handle, 10);//função deve ser chamada a cada 10ms
    osKernelStart();
   

    delay32Ms(0, 500);

    stat = disk_initialize(0);
    if (stat & STA_NOINIT) {
        UARTSendString((uint8_t*)"MMC: not initialized\r\n");
    }

    if (stat & STA_NODISK) {
        UARTSendString((uint8_t*)"MMC: No Disk\r\n");
    }

    if (stat != 0) {
        return 1;
    }

    UARTSendString((uint8_t*)"MMC: Initialized\r\n");

    if (disk_ioctl(0, GET_SECTOR_COUNT, &p2) == RES_OK) {
        i = sprintf((char*)buf_mmc, "Drive size: %d \r\n", p2);
        UARTSend(buf_mmc, i);
    }

    if (disk_ioctl(0, GET_SECTOR_SIZE, &w1) == RES_OK) {
        i = sprintf((char*)buf_mmc, "Sector size: %d \r\n", w1);
        UARTSend(buf_mmc, i);
    }

    if (disk_ioctl(0, GET_BLOCK_SIZE, &p2) == RES_OK) {
        i = sprintf((char*)buf_mmc, "Erase block size: %d \r\n", p2);
        UARTSend(buf_mmc, i);
    }

    if (disk_ioctl(0, MMC_GET_TYPE, &b1) == RES_OK) {
        i = sprintf((char*)buf_mmc, "Card type: %d \r\n", b1);
        UARTSend(buf_mmc, i);
    }

    res = f_mount(0, &Fatfs[0]);
    if (res != FR_OK) {
        i = sprintf((char*)buf_mmc, "Failed to mount 0: %d \r\n", res);
        UARTSend(buf_mmc, i);
        return 1;
    }

    res = f_opendir(&dir, "/");
    if (res) {
        i = sprintf((char*)buf_mmc, "Failed to open /: %d \r\n", res);
        UARTSend(buf_mmc, i);
        return 1;
    }

    for(;;) {
        res = f_readdir(&dir, &Finfo);
        if ((res != FR_OK) || !Finfo.fname[0]) break;

        UARTSendString((uint8_t*)&(Finfo.fname[0]));
        UARTSendString((uint8_t*)"\r\n");

    }


    while(1);


}
