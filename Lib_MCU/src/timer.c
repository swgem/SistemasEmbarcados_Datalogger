#include "mcu_regs.h"


/*****************************************************************************
** Noma da função:    delayMs
**
** Descrição:           Gera um delay de X milisegundos.
**            
**
** Parâmetros:      timer_num - Número do timer;     
**        delayInMs - delay desejado.     
**
** Retorno:           Nenhum
** 
*****************************************************************************/
void delayMs(uint8_t timer_num, uint32_t delayInMs)
{
  if (timer_num == 0)                   // Se for o timer 0
  {
    
    LPC_TMR32B0->TCR = 0x02;    // Reseta o timer
    LPC_TMR32B0->PR  = 0x00;    // Prescaler (divisor do clock) é zerado
                                        
    LPC_TMR32B0->MR0 = delayInMs * ((SystemCoreClock/LPC_SYSCON->SYSAHBCLKDIV) / 1000); 
                                        // (SystemFrequency/LPC_SYSCON->SYSAHBCLKDIV) = Frequência utilizada pelo timer, configurado na inicialização
                                        // Logo, o Match Register será carregado com número de contagem = freq. utilizada pelo timer divididos por 1000 e mult. pelo delay desejado
                                        // número de contagem = freq. utilizada equivale a 1 segundo e dividindo por 1000 teremos 1ms.
    
    LPC_TMR32B0->MCR = 0x04;    // Para o timer quando ocorre o Match (também zera o TCR)
    LPC_TMR32B0->TCR = 0x01;    // Liga o timer
  

    while (LPC_TMR32B0->TCR & 0x01);    // Fica em loop até o TCR ser zerado
  }
  else if (timer_num == 1)              // Se for o timer 1       
  {
    
    LPC_TMR32B1->TCR = 0x02;    // Reseta o timer
    LPC_TMR32B1->PR  = 0x00;    // Prescaler (divisor do clock) é zerado
    
    LPC_TMR32B1->MR0 = delayInMs * ((SystemCoreClock/LPC_SYSCON->SYSAHBCLKDIV) / 1000);
                                        // (SystemFrequency/LPC_SYSCON->SYSAHBCLKDIV) = Frequência utilizada pelo timer, configurado na inicialização
                                        // Logo, o Match Register será carregado com número de contagem = freq. utilizada pelo timer divididos por 1000 e mult. pelo delay desejado
                                        // número de contagem = freq. utilizada equivale a 1 segundo e dividindo por 1000 teremos 1ms.
    
    LPC_TMR32B1->MCR = 0x04;    // Para o timer quando ocorre o Match (também zera o TCR)
    LPC_TMR32B1->TCR = 0x01;    // Liga o timer
  

    while (LPC_TMR32B1->TCR & 0x01);    // Fica em loop até o TCR ser zerado        
  }
  return;
}





/*****************************************************************************
** Noma da função:    init_myTimer
**
** Descrição:           Inicializa o timer.
**              Configura o clock a ser utilizado por ele.      
**
** Parâmetros:      timer_num - Número do timer;     
**    
**
** Retorno:           Nenhum
** 
*****************************************************************************/
void init_myTimer(uint8_t timer_num) 
{
  if (timer_num == 0)
  {

    LPC_SYSCON->SYSAHBCLKCTRL |= (1<<9);        // Habilita o clock do sistema para o timer0 de 32 bits  
  }
  else if (timer_num == 1)
  {

    LPC_SYSCON->SYSAHBCLKCTRL |= (1<<10);       // Habilita o clock do sistema para o timer1 de 32 bits       

  }
  return;
}

/******************************************************************************
**                            End Of File
******************************************************************************/
