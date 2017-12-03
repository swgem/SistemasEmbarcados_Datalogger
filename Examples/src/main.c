/******************************************************************************************
 LABORATORIO 3
 CARLO, ERIC, PAULO
******************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "type.h"

#include "stdio.h"

#include "cmsis_os.h"
#include "lpc13xx.h"

#include "timer.h"
#include "gpio.h"
#include "uart.h"
#include "spi.h"
#include "i2c.h"

#include "diskio.h"
#include "ff.h"

#include "light.h"
#include "oled.h"
#include "temp.h"
#include "acc.h"
#include "joystick.h"
#include "diskio.h"
#include "ff.h"
#include "rgb.h"
#include "pca9532.h"

/******************************************************************************************
 MACROS
******************************************************************************************/

#define SYSTEM_PAUSED           1
#define SYSTEM_RUNNING          2

#define BUFFER_SIZE             8

#define PRINT_PERIOD            1000

/******************************************************************************************
 ESTRUTURAS
******************************************************************************************/

typedef struct Sensors {
    uint32_t count;
    int8_t x;
    int8_t y;
    int8_t z;
    //uint32_t temp;
    uint32_t lux;
} ST_Sensors;

/******************************************************************************************
 DECLARACAO DE VARIAVEIS GLOBAIS
******************************************************************************************/

//// SISTEMA

static uint32_t msTicks = 0;
uint8_t system_state = SYSTEM_RUNNING; // SYSTEM_PAUSED ou SYSTEM_RUNNING

uint8_t flag_pause = 0;
uint32_t debounce_count = 0;

//// VARIAVEIS DE CONFIGURACAO

int sample_period = 0;
char display_header[15];

//// FATFS

FATFS fatFs;
FIL fil_data;
FIL fil_config;
FRESULT fr;
DSTATUS stat;
BYTE res;
DIR dir;

int file_offset;
int written_bytes = 0;

/******************************************************************************************
 MUTEXES, SEMAFOROS E MAIL QUEUES
******************************************************************************************/

osMailQId mail_DataProcess;
osMailQId mail_DataSD;
osMailQId mail_DataOLED;

osMutexId mutex_Spi;

osMailQDef(mail_DataProcess, BUFFER_SIZE, ST_Sensors);
osMailQDef(mail_DataSD, BUFFER_SIZE, ST_Sensors);
osMailQDef(mail_DataOLED, 1, ST_Sensors);

osMutexDef(mutex_Spi);

/******************************************************************************************
 DECLARACAO DE FUNCOES
******************************************************************************************/

static void intToString(int value, uint8_t* pBuf, uint32_t len, uint32_t base);
static void config_read(char* config, int* sample_period, char* display_header);

void task_ReadSensors(void const *argument);
void task_ProcessData(void const *argument);
void task_PrintSD(void const *argument);
void task_PrintOLED(void const *argument);
void task_PauseRoutine(void const *argument);


/******************************************************************************************
 THREADS
******************************************************************************************/

osThreadId t_PrintSD;
osThreadId t_ReadSensors;
osThreadId t_PrintOLED;
osThreadId t_ProcessData;
osThreadId t_PauseRoutine;

osThreadDef(task_PrintSD, osPriorityNormal, 1, 0);
osThreadDef(task_ReadSensors, osPriorityNormal, 1, 0);
osThreadDef(task_PrintOLED, osPriorityNormal, 1, 0);
osThreadDef(task_ProcessData, osPriorityNormal, 1, 0);
osThreadDef(task_PauseRoutine, osPriorityNormal, 1, 0);

/******************************************************************************************
 INTERRUP��ES
******************************************************************************************/

void SSP0_IRQHandler(void) {
  uint32_t regValue;

  regValue = LPC_SSP0->MIS;
  if (regValue & SSPMIS_RORMIS) { LPC_SSP0->ICR = SSPICR_RORIC; }
  if (regValue & SSPMIS_RTMIS) { LPC_SSP0->ICR = SSPICR_RTIC; }
  if (regValue & SSPMIS_RXMIS) {  }
  return;
}

void PIOINT2_IRQHandler(void) {
    static uint32_t debounce_count = 0;
    
    if (msTicks + 300 > debounce_count) {
        // caso tenha sido pressionado por 300ms,
        // alterna entre os estados running/paused, caso off permanece
        system_state = (system_state == SYSTEM_RUNNING)? SYSTEM_PAUSED :
                       (system_state == SYSTEM_PAUSED)? SYSTEM_RUNNING :
                        0;

        if (system_state == SYSTEM_PAUSED) {
            osSignalSet(t_PauseRoutine, 0x1);
        }
        debounce_count = msTicks;
    }
    GPIOIntClear(PORT2, 9);
    return;
}

/******************************************************************************************
 MAIN
******************************************************************************************/

int main (void) {
    
    //// INICIALIZACOES

    osKernelInitialize();
    SystemInit();
    SystemCoreClockUpdate();

    GPIOInit();
    SSPInit(); 
    joystick_init();
    I2CInit((uint32_t)I2CMASTER, 0x3c);
    oled_init();
    light_init();
    acc_init();
    rgb_init(); 
    light_enable();
    light_setRange(LIGHT_RANGE_4000);

    GPIOIntEnable(PORT2, 9);
    GPIOSetInterrupt(PORT2, 9, 0, 0, 1);

    //// INICIALIZACAO DO FATFS

    stat = disk_initialize(0);
    if (stat != FR_OK) return 1;

    res = f_mount(0, &fatFs);
    if (res != FR_OK) return 1;

    res = f_opendir(&dir, "/");
    if (res != FR_OK) return 1;

    //// LEITURA DO ARQUIVO DE CONFIGURACAO

    fr = f_open(&fil_config, "config.txt", FA_READ);
    if (fr != FR_OK) return 1;

    uint8_t buffer[100];
    f_read(&fil_config, buffer, 100, NULL);
    char* config = (char *) buffer;

    config_read(config, &sample_period, display_header);

    f_close(&fil_config);

    //// IMPRESSAO DA BASE NA TELA

    oled_clearScreen(OLED_COLOR_WHITE);

    oled_putString(1, 1, (uint8_t*)display_header, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

    //oled_putString(1, 9,  (uint8_t*)"Temp   : ", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    oled_putString(1, 17, (uint8_t*)"Acc x  : ", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    oled_putString(1, 25, (uint8_t*)"Acc y  : ", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    oled_putString(1, 33, (uint8_t*)"Acc z  : ", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    oled_putString(1, 41, (uint8_t*)"Light  : ", OLED_COLOR_BLACK, OLED_COLOR_WHITE);

    //// INICIALIZACAO DOS MUTEXES, SEMAFOROS E MAIL QUEUES

    mail_DataProcess = osMailCreate(osMailQ(mail_DataProcess), NULL);
    mail_DataSD = osMailCreate(osMailQ(mail_DataSD), NULL);
    mail_DataOLED = osMailCreate(osMailQ(mail_DataOLED), NULL);

    mutex_Spi = osMutexCreate(osMutex(mutex_Spi));

    //// INICIALIZACAO DAS THREADS E TIMERS

    osDelay(500);

    t_PrintSD = osThreadCreate(osThread(task_PrintSD), NULL);
    t_ReadSensors = osThreadCreate(osThread(task_ReadSensors), NULL);
    t_PrintOLED = osThreadCreate(osThread(task_PrintOLED), NULL);
    t_ProcessData = osThreadCreate(osThread(task_ProcessData), NULL);
    t_PauseRoutine = osThreadCreate(osThread(task_PauseRoutine), NULL);
    
    osKernelStart();
    
    osDelay(osWaitForever);
    
    return 0;
}

/******************************************************************************************
 DEFINI��O DE FUNCOES
******************************************************************************************/

static void intToString(int value, uint8_t* pBuf, uint32_t len, uint32_t base) {
    static const char* pAscii = "0123456789abcdefghijklmnopqrstuvwxyz";
    int pos = 0;
    int tmpValue = value;

    // the buffer must not be null and at least have a length of 2 to handle one
    // digit and null-terminator
    if (pBuf == NULL || len < 2) {
        return;
    }

    // a valid base cannot be less than 2 or larger than 36
    // a base value of 2 means binary representation. A value of 1 would mean only zeros
    // a base larger than 36 can only be used if a larger alphabet were used.
    if (base < 2 || base > 36) {
        return;
    }

    // negative value
    if (value < 0) {
        tmpValue = -tmpValue;
        value    = -value;
        pBuf[pos++] = '-';
    }

    // calculate the required length of the buffer
    do {
        pos++;
        tmpValue /= base;
    } while(tmpValue > 0);


    if (pos > len) {
        // the len parameter is invalid.
        return;
    }

    pBuf[pos] = '\0';

    do {
        pBuf[--pos] = pAscii[value % base];
        value /= base;
    } while(value > 0);

    return;
}

static void config_read(char* config, int* sample_period, char* display_header) {
    char vcSample_period[5];

    int i = 0;
    do {
        vcSample_period[i] = config[i];
    } while (config[++i] != '\r');
    i++; // pra pular '\r'
    i++; // pra pular '\n'
    int k = 0;
    do {
        display_header[k++] = config[i];
    } while (config[++i] != '\0');

    *sample_period = atoi(vcSample_period);
}

void task_ReadSensors(void const *argument) {
    int32_t xoff;
    int32_t yoff;
    int32_t zoff;
    int8_t xaux;
    int8_t yaux;
    int8_t zaux;
    
    acc_read(&xaux, &yaux, &zaux);
    xoff = 0 - xaux;
    yoff = 0 - yaux;
    zoff = 64 - zaux;

    while (TRUE) {
        if (system_state == SYSTEM_RUNNING) {        
            ST_Sensors *data = (ST_Sensors*)osMailAlloc(mail_DataProcess, osWaitForever);
            acc_read(&data->x, &data->y, &data->z);
            data->x = data->x + xoff;
            data->y = data->y + yoff;
            data->z = data->z + zoff;
            //data->temp = temp_read();
            data->lux = light_read();

            osMailPut(mail_DataProcess, data);
            osDelay(sample_period);
        }
        else if (system_state == SYSTEM_PAUSED) {
            osSignalWait(0x1, osWaitForever); // entra em estado vegetativo ate receber sinal
        }
    }
}

void task_ProcessData(void const *argument) {
    ST_Sensors sensors_buf[4];
    ST_Sensors processed_sensors;
    uint8_t index = 0;
    uint32_t count = 0;

    while (TRUE) {
        if (system_state == SYSTEM_RUNNING) {
            osEvent evt = osMailGet(mail_DataProcess, osWaitForever);
            ST_Sensors *data = (ST_Sensors*)evt.value.p;
            sensors_buf[index++ % 4] = *data;
            osMailFree(mail_DataProcess, data);
            
            count++;
            
            processed_sensors.x = (uint32_t)(sensors_buf[0].x + sensors_buf[1].x +
                                   sensors_buf[2].x + sensors_buf[3].x)  / 4;
            processed_sensors.y = (uint32_t)(sensors_buf[0].y + sensors_buf[1].y +
                                   sensors_buf[2].y + sensors_buf[3].y) / 4;
            processed_sensors.z = (uint32_t)(sensors_buf[0].z + sensors_buf[1].z +
                                   sensors_buf[2].z + sensors_buf[3].z) / 4;
            processed_sensors.lux = (uint32_t)(sensors_buf[0].lux + sensors_buf[1].lux +
                                   sensors_buf[2].lux + sensors_buf[3].lux) / 4;
            processed_sensors.count = count;
            
            
            ST_Sensors *data_OLED = (ST_Sensors*)osMailAlloc(mail_DataOLED, 0);
            *data_OLED = processed_sensors;
            
            ST_Sensors *data_SD = (ST_Sensors*)osMailAlloc(mail_DataSD, osWaitForever);
            *data_SD = processed_sensors;
            
            if (data_OLED != NULL) osMailPut(mail_DataOLED, data_OLED);
            osMailPut(mail_DataSD, data_SD);
        }
        else if (system_state == SYSTEM_PAUSED) {
            osSignalWait(0x1, osWaitForever); // entra em estado vegetativo ate receber sinal
        }
    }
}

void task_PrintOLED(void const *argument) {
    uint8_t buffer[10];
    ST_Sensors processed_sensors;

    while (TRUE) {
        if (system_state == SYSTEM_RUNNING) {
            osEvent evt = osMailGet(mail_DataOLED, osWaitForever);
            ST_Sensors *data = (ST_Sensors*)evt.value.p;
            processed_sensors = *data;
            osMailFree(mail_DataOLED, data);
            
            
            osMutexWait(mutex_Spi, osWaitForever);

            pca9532_setLeds(0x0080,0xFFFF);

            intToString(sample_period, buffer,10,10);
            oled_fillRect((1+9*7),55, 95, 62, OLED_COLOR_WHITE);
            oled_putString((1+9*7),55, buffer, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

            intToString(PRINT_PERIOD, buffer,10,10);
            oled_fillRect((1),55, 48, 62, OLED_COLOR_WHITE);
            oled_putString((1),55, buffer, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

            intToString(processed_sensors.x, buffer, 10, 10);
            oled_fillRect((1+9*6),17, 90, 24, OLED_COLOR_WHITE);
            oled_putString((1+9*6),17, buffer, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

            intToString(processed_sensors.y, buffer, 10, 10);
            oled_fillRect((1+9*6),25, 90, 32, OLED_COLOR_WHITE);
            oled_putString((1+9*6),25, buffer, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

            intToString(processed_sensors.z, buffer, 10, 10);
            oled_fillRect((1+9*6),33, 90, 40, OLED_COLOR_WHITE);
            oled_putString((1+9*6),33, buffer, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

            intToString(processed_sensors.lux, buffer, 10, 10);
            oled_fillRect((1+9*6),41, 90, 48, OLED_COLOR_WHITE);
            oled_putString((1+9*6),41, buffer, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

            pca9532_setLeds(0x0000,0xFFFF);

            osMutexRelease(mutex_Spi);
            osDelay(PRINT_PERIOD);
        }
        else if (system_state == SYSTEM_PAUSED) {
            osSignalWait(0x1, osWaitForever); // entra em estado vegetativo ate receber sinal
        }
    }
}

void task_PrintSD(void const *argument) {
    osMutexWait(mutex_Spi, osWaitForever);

    fr = f_open(&fil_data, "data.txt", FA_CREATE_ALWAYS | FA_WRITE);
    file_offset = 0;
    
    written_bytes = f_printf(&fil_data, "<%dmseg>\r\n", sample_period);
    file_offset = file_offset + written_bytes;
    f_close(&fil_data);

    osMutexRelease(mutex_Spi);
    
    ST_Sensors processed_sensors;
    
    while(1) {
        if (system_state == SYSTEM_RUNNING) {
               
            osEvent evt = osMailGet(mail_DataSD, osWaitForever);

            ST_Sensors *data = (ST_Sensors*)evt.value.p;
            processed_sensors = *data;
            osMailFree(mail_DataSD, data);
            
            osMutexWait(mutex_Spi, osWaitForever);

            pca9532_setLeds(0x0100,0xFFFF);

            fr = f_open(&fil_data, "data.txt", FA_WRITE);
            res = f_lseek(&fil_data, fil_data.fsize);
            if(fr == FR_OK && res == FR_OK) 
            {
              written_bytes = f_printf(&fil_data, "%4d;%4d;%4d;%4d;%4d;\r\n", processed_sensors.count,
                                       processed_sensors.x, processed_sensors.y, processed_sensors.z,
                                       processed_sensors.lux);
            }
            f_close(&fil_data);
            pca9532_setLeds(0x0000,0xFFFF);

            osMutexRelease(mutex_Spi);
        }
        else if (system_state == SYSTEM_PAUSED) {
            osSignalWait(0x1, osWaitForever); // entra em estado vegetativo ate receber sinal
        }
    }
}

void task_PauseRoutine(void const *argument) {
    
    while (TRUE) {
        if (system_state == SYSTEM_PAUSED) {
            pca9532_setLeds(0x0001,0xFFFF);
        }
        else if (system_state == SYSTEM_RUNNING) {
            // acorda todas threads de modo normal antes de dormir
            osSignalSet(t_ReadSensors, 0x1);
            osSignalSet(t_ProcessData, 0x1);
            osSignalSet(t_PrintOLED, 0x1);
            osSignalSet(t_PrintSD, 0x1);

            osSignalWait(0x1, osWaitForever);
        }
        debounce_count = 0;
    }
}