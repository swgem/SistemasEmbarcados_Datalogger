/*****************************************************************************
 *   ssp.c:  SSP C file for NXP LPC13xx Family Microprocessors
 *
 *   Copyright(C) 2008, NXP Semiconductor
 *   All rights reserved.
 *
 *   History
 *   2008.07.20  ver 1.00    Preliminary version, first Release
 *
*****************************************************************************/
#include "mcu_regs.h"
#include "gpio.h"
#include "spi.h"

/*****************************************************************************
** Function name:   SSP_IRQHandler
**
** Descriptions:    SSP port is used for SPI communication.
**            SSP interrupt handler
**            The algorithm is, if RXFIFO is at least half full, 
**            start receive until it's empty; if TXFIFO is at least
**            half empty, start transmit until it's full.
**            This will maximize the use of both FIFOs and performance.
**
** parameters:      None
** Returned value:    None
** 
*****************************************************************************/
//DR:
void SSP0_IRQHandler(void) 
{
  uint32_t regValue;

  regValue = LPC_SSP0->MIS;
  if ( regValue & SSPMIS_RORMIS ) /* Receive overrun interrupt */
  {
  //interruptOverRunStat++;
  LPC_SSP0->ICR = SSPICR_RORIC;   /* clear interrupt */
  }
  if ( regValue & SSPMIS_RTMIS )  /* Receive timeout interrupt */
  {
  //interruptRxTimeoutStat++;
  LPC_SSP0->ICR = SSPICR_RTIC;    /* clear interrupt */
  }

  /* please be aware that, in main and ISR, CurrentRxIndex and CurrentTxIndex
  are shared as global variables. It may create some race condition that main
  and ISR manipulate these variables at the same time. SSPSR_BSY checking (polling)
  in both main and ISR could prevent this kind of race condition */
  if ( regValue & SSPMIS_RXMIS )  /* Rx at least half full */
  {
  //interruptRxStat++;    /* receive until it's empty */    
  }
  return;
}

/*****************************************************************************
** Function name:   SSPInit
**
** Descriptions:    SSP port initialization routine
**        
** parameters:      None
** Returned value:    None
** 
*****************************************************************************/
void SSPInit( void )
{
  uint8_t i, Dummy=Dummy;

  LPC_SYSCON->PRESETCTRL |= (0x1<<0);
  LPC_SYSCON->SYSAHBCLKCTRL |= (1<<11);
  //DR:
  LPC_SYSCON->SSP0CLKDIV = 0x02;      /* Divided by 2 */
  LPC_IOCON->PIO0_8           &= ~0x07; /*  SSP I/O config */
  LPC_IOCON->PIO0_8           |= 0x01;    /* SSP MISO */
  LPC_IOCON->PIO0_9           &= ~0x07; 
  LPC_IOCON->PIO0_9           |= 0x01;    /* SSP MOSI */
//  LPC_IOCON->SCKLOC = 0x00;
//  LPC_IOCON->JTAG_TCK_PIO0_10 &= ~0x07;
//  LPC_IOCON->JTAG_TCK_PIO0_10 |= 0x02;    /* SSP CLK */
  LPC_IOCON->SCK_LOC = 0x01;
  LPC_IOCON->PIO2_11 = 0x01;/* P2.11 function 1 is SSP clock, need to combined
                with IOCONSCKLOC register setting */

  LPC_IOCON->PIO0_2 &= ~0x07;   /* SSP SSEL is a GPIO pin */
  /* port0, bit 2 is set to GPIO output and high */
  GPIOSetDir( PORT0, 2, 1 );
  GPIOSetValue( PORT0, 2, 1 );

  /* Set DSS data to 8-bit, Frame format SPI, CPOL = 0, CPHA = 0, and SCR is 15 */
  //DR:
  LPC_SSP0->CR0 = 0x0707;

  /* SSPCPSR clock prescale register, master mode, minimum divisor is 0x02 */
  LPC_SSP0->CPSR = 0x2;

  for ( i = 0; i < FIFOSIZE; i++ )
  {
      //DR:
      Dummy = LPC_SSP0->DR;   /* clear the RxFIFO */
  }

  /* Enable the SSP Interrupt */
  NVIC_EnableIRQ(SSP0_IRQn);

  LPC_SSP0->CR1 = SSPCR1_SSE;
  /* Set SSPINMS registers to enable interrupts */
  /* enable all error related interrupts */
  //DR:
  LPC_SSP0->IMSC = SSPIMSC_RORIM | SSPIMSC_RTIM;
  return;
}

/*****************************************************************************
** Function name:   SSPSend
**
** Descriptions:    Send a block of data to the SSP port, the 
**            first parameter is the buffer pointer, the 2nd 
**            parameter is the block length.
**
** parameters:      buffer pointer, and the block length
** Returned value:    None
** 
*****************************************************************************/
void SSPSend( uint8_t *buf, uint32_t Length )
{
  uint32_t i;
  uint8_t Dummy = Dummy;
    
  for ( i = 0; i < Length; i++ )
  {
  /* Move on only if NOT busy and TX FIFO not full. */
    //DR:
    while ( (LPC_SSP0->SR & (SSPSR_TNF|SSPSR_BSY)) != SSPSR_TNF );
  //DR:
    LPC_SSP0->DR = *buf;
  buf++;

  while ( (LPC_SSP0->SR & (SSPSR_BSY|SSPSR_RNE)) != SSPSR_RNE );
  /* Whenever a byte is written, MISO FIFO counter increments, Clear FIFO 
  on MISO. Otherwise, when SSP0Receive() is called, previous data byte
  is left in the FIFO. */
  Dummy = LPC_SSP0->DR;
  }
  return; 
}

/*****************************************************************************
** Function name:   SSPReceive
** Descriptions:    the module will receive a block of data from 
**            the SSP, the 2nd parameter is the block 
**            length.
** parameters:      buffer pointer, and block length
** Returned value:    None
** 
*****************************************************************************/
void SSPReceive( uint8_t *buf, uint32_t Length )
{
  uint32_t i;
 
  for ( i = 0; i < Length; i++ )
  {
  /* As long as Receive FIFO is not empty, I can always receive. */
  /* If it's a loopback test, clock is shared for both TX and RX,
  no need to write dummy byte to get clock to get the data */
  /* if it's a peer-to-peer communication, SSPDR needs to be written
  before a read can take place. */

  //DR:
    LPC_SSP0->DR = 0xFF;
  /* Wait until the Busy bit is cleared */
    //DR:
  while ( (LPC_SSP0->SR & (SSPSR_BSY|SSPSR_RNE)) != SSPSR_RNE );

  *buf = LPC_SSP0->DR;
  buf++;
  
  }
  return; 
}

/******************************************************************************
**                            End Of File
******************************************************************************/

