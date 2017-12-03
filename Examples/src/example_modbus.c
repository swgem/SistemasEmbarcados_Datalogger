//#include "cmsis_os.h"                      // CMSIS-RTOS header file
#include "libdemo.h"
 /*============================================================================
 *                  Exemplos de utilização do RTOS CMSIS
 *           LPCXpresso 1343 + Embedded Artists Development Board 
 *---------------------------------------------------------------------------*
 *                     Estrutura exemplo para processamento serial
 *---------------------------------------------------------------------------*
   
 *---------------------------------------------------------------------------*
 *                    Prof. César Yutaka Ofuchi
 *            Universidade Tecnológica Federal do Paraná (UTFPR)
 *===========================================================================*/

#include <inttypes.h>
#include "type.h"
#include "uart.h"
#include "stdio.h"
#include "gpio.h"
#include "i2c.h"
#include "modbus_protocol.h"
#include "pca9532.h"

uint8_t debug_crc_msb = 0;
uint8_t debug_crc_lsb = 0;
uint16_t crc;
uint8_t uart_buffer[20];//vetor de dados
//uint8_t data_buffer[20];

uint8_t bufferRsp[10];   //buffer de respoqsta (global)
uint8_t iterBuffer=0;
uint8_t g_iMsgIndex=0; //iterador
//Master_Req1 Master_Recebido;
//Master_Req1 * pMsg = (Master_Req1*)&uart_buffer;
//Master_Req2_3 * pMsg2_3 = (Master_Req2_3*)&data_buffer;
EState state=WAITING_START_MSG;

uint16_t addr2led[4] = {0x1, 0x2, 0x8000, 0x4000};

uint8_t coil_map[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
int16_t register_map[5] = {1, 2, -1, 3, -4};

uint16_t sample_period;

uint16_t switch_lsbmsb(uint16_t data);

void respond_RM_Coil();

void respond_RM_Reg();

void respond_WS_Coil();

void respond_WM_Reg();

void ProcessaMensagemRecebida() {

    switch(uart_buffer[1]){
        case COMMAND_RM_COIL://montar resposta para comando tipo 2
            respond_RM_Coil();
            break;
        case COMMAND_RM_REG:
            respond_RM_Reg();
            break;
        case COMMAND_WS_COIL:
            respond_WS_Coil();
            break;
        case COMMAND_WM_REG:
            respond_WM_Reg();
            break;
    }
}

void respond_RM_Coil() {
    uint8_t data = 0;
    
    Master_R_ST master_data;

    master_data.id = uart_buffer[0];
    master_data.command_type = uart_buffer[1];
    master_data.initial_address = switch_lsbmsb(*((uint16_t *)&uart_buffer[2]));
    master_data.points = switch_lsbmsb(*((uint16_t *)&uart_buffer[4]));
    master_data.CRC = *((uint16_t *)&uart_buffer[6]);

    // verifica codigo de erro
    if(usMBCRC16(uart_buffer, 6) != master_data.CRC) return;

    for (uint16_t i = 0; i < master_data.points; i++) {
        data |= (coil_map[master_data.initial_address + i] << i);
    }

    Slave_RMC_ST slave_buffer = { .id = 0x01,
                                .command_type = 0x02,
                                .size = 0x01,
                                .data = data,
                                .CRC = 0x0000 };

    slave_buffer.CRC = usMBCRC16((uint8_t *)&slave_buffer, 4);

    UARTSend((uint8_t *)&slave_buffer, 6);
}

void respond_RM_Reg() {
    int16_t data[5];
    uint8_t buffer[15];
    Master_R_ST master_data;

    master_data.id = uart_buffer[0];
    master_data.command_type = uart_buffer[1];
    master_data.initial_address = switch_lsbmsb(*((uint16_t *)&uart_buffer[2]));
    master_data.points = switch_lsbmsb(*((uint16_t *)&uart_buffer[4]));
    master_data.CRC = *((uint16_t *)&uart_buffer[6]);
    
    // verifica codigo de erro
    if(usMBCRC16(uart_buffer, 6) != master_data.CRC) return;

    for (uint16_t i = 0; i < master_data.points; i++) {
        data[i] = register_map[master_data.initial_address + i];
        buffer[3 + 2*i] = *((uint8_t *)&data[i] + 1);
        buffer[3 + 2*i + 1] = *((uint8_t *)&data[i]);
    }

    buffer[0] = 0x01; // id
    buffer[1] = 0x03; // comando
    buffer[2] = 2*master_data.points; // tamanho

    crc = usMBCRC16(buffer, 2*master_data.points + 3); // calc do crc

    buffer[2*master_data.points + 3] = *((uint8_t *)&crc); // msb do crc
    buffer[2*master_data.points + 4] = *((uint8_t *)&crc + 1); // lsb do crc

    UARTSend(buffer, 2*master_data.points + 5);
}

void respond_WS_Coil() {
    Master_WSC_ST master_data;

    master_data.id = uart_buffer[0];
    master_data.command_type = uart_buffer[1];
    master_data.initial_address = switch_lsbmsb(*((uint16_t *)&uart_buffer[2]));
    master_data.data = switch_lsbmsb(*((uint16_t *)&uart_buffer[4]));
    master_data.CRC = *((uint16_t *)&uart_buffer[6]);

    // verifica codigo de erro
    if(usMBCRC16(uart_buffer, 6) != master_data.CRC) return;
    
    if(master_data.data == 0x0000){
        pca9532_setLeds(0x0000, addr2led[(master_data.initial_address)]);
        coil_map[master_data.initial_address] = 0;
    }
    else {
        pca9532_setLeds(addr2led[(master_data.initial_address)], 0x0000);
        coil_map[master_data.initial_address] = 1;
    }

    UARTSend(uart_buffer, 8);
}

void respond_WM_Reg() {
    Master_WMR_ST master_data;

    master_data.id = uart_buffer[0];
    master_data.command_type = uart_buffer[1];
    master_data.initial_address = switch_lsbmsb(*((uint16_t *)&uart_buffer[2]));
    master_data.points = switch_lsbmsb(*((uint16_t *)&uart_buffer[4]));
    master_data.size = uart_buffer[6];
    master_data.data = switch_lsbmsb(*((uint16_t *)&uart_buffer[7]));
    master_data.CRC = *((uint16_t *)&uart_buffer[9]);

    // verifica codigo de erro
    if(usMBCRC16(uart_buffer, 9) != master_data.CRC) return;

    Slave_WMR_ST slave_buffer;

    sample_period = master_data.data;

    slave_buffer.id = uart_buffer[0];
    slave_buffer.command_type = uart_buffer[1];
    slave_buffer.initial_address = *((uint16_t *)&uart_buffer[2]);
    slave_buffer.points = *((uint16_t *)&uart_buffer[4]);
    slave_buffer.CRC = usMBCRC16((uint8_t *)&slave_buffer, 6);

    UARTSend((uint8_t *)&slave_buffer, 8);
}

// uint8_t switch_index_lsbmsb(int it) {
//     return (it % 2 == 0)? ++it : --it;
// }

uint16_t switch_lsbmsb(uint16_t data) {
    return ((data << 0x8) & 0xFF00) | ((data >> 0x8) & 0x00FF);
}

int main (void) {
    uint8_t iter = 0;
    GPIOInit();  
    I2CInit(I2CMASTER, 0);
    UARTInit(9600);

    state = WAITING_START_MSG;
    while(1){
        switch(state){
            case WAITING_START_MSG:
                UARTReceive(&uart_buffer[iter++], 1, 1);
                //data_buffer[switch_index_lsbmsb(iter - 1)] = uart_buffer[iter - 1];

                if (uart_buffer[1] == COMMAND_WM_REG && iter == 11) {
                    iter = 0;
                    state=WAITING_END_MSG;
                }
                if (uart_buffer[1] != COMMAND_WM_REG && iter == 8) {
                    iter = 0;
                    state=WAITING_END_MSG;
                }
                break;
            case WAITING_END_MSG:
                // if (uart_buffer[1] == COMMAND_WM_REG)
                //   crc = usMBCRC16(uart_buffer, 9);
                // else
                  
                // if(crc == pMsg->CRC){            
                    
                // }
                ProcessaMensagemRecebida();
                state = WAITING_START_MSG;              
                break;  

                // state=WAITING_START_MSG;//aguarda nova msg
                // g_iMsgIndex=0;          //zera index para nova msg
                // senão
                // recebe dados seriais
                // uart_buffer[g_iMsgIndex++] = //preenche o buffer
        }
    }
}