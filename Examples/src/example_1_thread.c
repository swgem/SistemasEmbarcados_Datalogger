#include "libdemo.h"
/*============================================================================
 *                  Exemplos de utiliza��o do RTOS CMSIS
 *           LPCXpresso 1343 + Embedded Artists Development Board 
 *---------------------------------------------------------------------------*
 *                   Gerenciamento de m�ltiplas Threads
 *---------------------------------------------------------------------------*
 *                    Prof. Andr� Schneider de Oliveira
 *            Universidade Tecnol�gica Federal do Paran� (UTFPR)
 *===========================================================================*/
 
void led2_thread(void const *args) {
    while (1) {
        pca_toggle(15);//led verde
        osDelay(1000);
    }
}
osThreadDef(led2_thread, osPriorityNormal, 1, 0);
 
int main() {
  
    osKernelInitialize();

    I2CInit( (uint32_t)I2CMASTER, 0 );
  
    osThreadCreate(osThread(led2_thread), NULL);
      
    osKernelStart();
    
    while (1) {
        pca_toggle(1);//led vermelho
        osDelay(500);
    }
}
