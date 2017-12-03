
#include "mcu_regs.h"
#include "gpio.h"
#include "nossoSSP.h"

/*****************************************************************************
* Funcao SSPInit: Inicializacao do ssp
* Sem parametros.
* Sem retorno.
*****************************************************************************/
void SSPInit( void )
{
  uint8_t i, Dummy=Dummy;
  // Reseta o SSP
  LPC_SYSCON->PRESETCTRL |= (0x1<<0);
  // Habilita o clock do sistema            
  LPC_SYSCON->SYSAHBCLKCTRL |= (1<<11); 
  // Divide o clock do sistema por 2
  LPC_SYSCON->SSP0CLKDIV = 0x02;	 
  // Configura o pino PIO0_8 como sendo MISO0
  LPC_IOCON->PIO0_8           &= ~0x07;	        
  LPC_IOCON->PIO0_8           |= 0x01;		
  // Configura o pino PIO0_9 como sendo MOSI0
  LPC_IOCON->PIO0_9           &= ~0x07;	                
  LPC_IOCON->PIO0_9           |= 0x01;		

  // P2.11 function 1 is SSP clock, need to combined with IOCONSCKLOC register setting
  LPC_IOCON->SCK_LOC = 0x01;
  LPC_IOCON->PIO2_11 = 0x01;

  //Cada frame com 8 bits
  LPC_SSP0->CR0 = 0x0707;  
  // Modo master: SSP_PCLK dividido por 2             
  LPC_SSP0->CPSR = 0x2;                

  for ( i = 0; i < FIFOSIZE; i++ )
  {
  // Limpa o buffer de para a recepçcoo
      Dummy = LPC_SSP0->DR;		
  }
  // Habilita interrupcoes
  NVIC_EnableIRQ(SSP0_IRQn);            
	
  // Habilita o SSP
  LPC_SSP0->CR1 = SSPCR1_SSE;                   

  //Habilita interrupcoes de erro
  LPC_SSP0->IMSC = SSPIMSC_RORIM | SSPIMSC_RTIM;        
  return;
}

/*****************************************************************************
* Funcao SSPSend: Manda um bloco de dados para o port SSP 
* Parametros: buf (ponteiro do buffer), Lenght (tamanho do bloco)
* Sem retorno.
*****************************************************************************/
void SSPSend( uint8_t *buf, uint32_t Length )
{
  uint32_t i;
  uint8_t Dummy = Dummy;
  
  // Loop de envio da mensagem, enquanto não enviar por completo  
  for ( i = 0; i < Length; i++ )        
  {
    // Enquanto o buffer de transmissao não estiver cheio e ocupado
    while ( (LPC_SSP0->SR & (SSPSR_TNF|SSPSR_BSY)) != SSPSR_TNF );  
	
	// Buffer de transmissao recebe o dado
    LPC_SSP0->DR = *buf;    
	// Pega o proximo dado           
    buf++;                              
    
	 // Se estiver em Loop Back Mode  
#if !LOOPBACK_MODE    
    // Enquanto o buffer de recepcao nao estiver vazio e ocupado                
	while ( (LPC_SSP0->SR & (SSPSR_BSY|SSPSR_RNE)) != SSPSR_RNE );          
    // Esvazia o buffer de recepcao
	Dummy = LPC_SSP0->DR;                                                                           
#else       // Se nao estiver em Loop Back Mode 
	// Espera o ocupado ser zerado                                           
	while ( LPC_SSP->SR & SSPSR_BSY );                               
#endif
  }
  return; 
}

/*****************************************************************************
* Funcao SSPReceiveS: Recebe um bloco de dados para do SSP 
* Parametros: buf (ponteiro do buffer), Lenght (tamanho do bloco)
* Sem retorno.
*****************************************************************************/
void SSPReceive( uint8_t *buf, uint32_t Length )
{
  uint32_t i;
  // Loop espera a mensage,
  for ( i = 0; i < Length; i++ )                                
  {
// Se estiver em Loop Back Mode
#if !LOOPBACK_MODE // Se modo Slave                                             
#if SSP_SLAVE   
    // Espera o buffer de recepcao estar vazio                                                  
	while ( !(LPC_SSP->SR & SSPSR_RNE) );                              
#else // Se modo Master  
    // Buffer recepcao com FF                                                                 
    LPC_SSP0->DR = 0xFF;                                                                                            
    // Espera bit ocupado ser zerado
	while ( (LPC_SSP0->SR & (SSPSR_BSY|SSPSR_RNE)) != SSPSR_RNE );  
#endif
#else   // Se nao estiver em Loop Back Mode 
    // Enquanto o buffer de recepcao estiver vazio                                                                    
	while ( !(LPC_SSP0->SR & SSPSR_RNE) );                  
#endif
    // Recebe o dado 
	*buf = LPC_SSP0->DR;
	// Incrementa para salvar o dado seguinte                                    
	buf++;                                                  
	
  }
  return; 
}

