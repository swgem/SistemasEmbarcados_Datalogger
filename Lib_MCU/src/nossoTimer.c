#include "mcu_regs.h"
#include "nossoTimer.h"


/*****************************************************************************
* Funcao delayMs: loop de delay em ms
* Parametros: timer_num (escolha do timer), delayInMs (tempo em milisegundos)
* Sem retorno.
*****************************************************************************/

void delayMs(uint8_t timer_num, uint32_t delayInMs)
{
  // Se escolher o timer zero
  if (timer_num == 0)                   
  {
    // Reseta o timer
    LPC_TMR32B0->TCR = 0x02;
	// O divisor do timer é zerado		
    LPC_TMR32B0->PR  = 0x00;		
    // Carrega o timer com o valor desejado, utilizando uma proporção em relação ao clock do sistema                                    
    LPC_TMR32B0->MR0 = delayInMs * ((SystemFrequency/LPC_SYSCON->SYSAHBCLKDIV) / 1000); 
	// Para o timer quando ocorre o match
    LPC_TMR32B0->MCR = 0x04;		
	// Habilita o timer
      LPC_TMR32B0->TCR = 0x01;		
    // Enquanto não zera o contador fica no loop
    while (LPC_TMR32B0->TCR & 0x01);    
  }
  // Se escolher o timer um
  else if (timer_num == 1)                    
  {
    // Reseta o timer
    LPC_TMR32B1->TCR = 0x02;	
	// O divisor do timer é zerado	
    LPC_TMR32B1->PR  = 0x00;		
	// Carrega o timer com o valor desejado, utilizando uma proporção em relação ao clock do sistema 
    LPC_TMR32B1->MR0 = delayInMs * ((SystemFrequency/LPC_SYSCON->SYSAHBCLKDIV) / 1000);
	// Para o timer quando ocorre o match
    LPC_TMR32B1->MCR = 0x04;
	// Habilita o timer
    LPC_TMR32B1->TCR = 0x01;		
    // Enquanto não zera o contador fica no loop
    while (LPC_TMR32B1->TCR & 0x01);            
  }
  return;
}


/*****************************************************************************
* Funcao init_myTimer: funcao para inicializar o timer
* Parametros: timer_num (escolha do timer)
* Sem retorno.
*****************************************************************************/
void init_myTimer(uint8_t timer_num) 
{
  // Se escolher o timer zero
  if (timer_num == 0)
  {
    // Habilita o clock do sistema 
    LPC_SYSCON->SYSAHBCLKCTRL |= (1<<9);        
  }
  // Se escolher o timer um
  else if (timer_num == 1)
  {
    // Habilita o clock do sistema 
    LPC_SYSCON->SYSAHBCLKCTRL |= (1<<10);      
  }
  return;
}