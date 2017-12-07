/*****************************************************************************
 modbus.c:  Implementation file for modbus protocol middleware
******************************************************************************/

/*****************************************************************************
 Includes
******************************************************************************/

#include "modbus.h"
#include <inttypes.h>
#include "type.h"
#include "uart.h"
#include "stdio.h"
#include "gpio.h"
#include "i2c.h"
#include "pca9532.h"

/*****************************************************************************
 Variaveis locais
******************************************************************************/

static uint8_t uart_buffer[20];


// DEVEM SER ALTERADOS:
static uint8_t * modbus_coil_map;
static int16_t * modbus_register_map;

/*****************************************************************************
 Declaracao de funcoes locais
******************************************************************************/

static uint16_t usMBCRC16(unsigned char * pucFrame, unsigned char usLen);
static uint16_t switch_lsbmsb(uint16_t data);
static Modbus_Response_ST process_master_request();
static void respond_RM_Coil(uint8_t * response_type);
static void respond_RM_Reg(uint8_t * response_type);
static void respond_WS_Coil(uint8_t * response_type, uint16_t * data);
static void respond_WM_Reg(uint8_t * response_type, uint16_t * data);

/*****************************************************************************
 Definicao de funcoes locais
******************************************************************************/

// Codigo para cálculo de CRC
// parametro1: pucFrame, vetor de dados para calcular CRC
// parametro2: uLen, tamanho desse vetor
static uint16_t usMBCRC16(unsigned char * pucFrame, unsigned char usLen) {
    unsigned char ucCRchi = 0xFF;
    unsigned char ucCRCLo = 0xFF;
    int iIndex;
    while( usLen-- ) {
		iIndex = ucCRCLo ^ *( pucFrame++ );
		ucCRCLo = ucCRchi ^ aucCRchi[iIndex];
		ucCRchi = aucCRCLo[iIndex];
	}
	return (uint16_t)(ucCRchi << 8 | ucCRCLo);
}

static uint16_t switch_lsbmsb(uint16_t data) {
    return ((data << 0x8) & 0xFF00) | ((data >> 0x8) & 0x00FF);
}


static Modbus_Response_ST process_master_request() {
    Modbus_Response_ST response;

    switch(uart_buffer[1]) {
        case COMMAND_RM_COIL://montar resposta para comando tipo 2
            respond_RM_Coil(&response.response_type);
            break;
        case COMMAND_RM_REG:
            respond_RM_Reg(&response.response_type);
            break;
        case COMMAND_WS_COIL:
            respond_WS_Coil(&response.response_type, &response.data);
            break;
        case COMMAND_WM_REG:
            respond_WM_Reg(&response.response_type, &response.data);
            break;
    }

    return response;
}

static void respond_RM_Coil(uint8_t * response_type) {
    uint8_t data = 0;
    
    Master_R_ST master_data;

    master_data.id = uart_buffer[0];
    master_data.command_type = uart_buffer[1];
    master_data.initial_address = switch_lsbmsb(*((uint16_t *)&uart_buffer[2]));
    master_data.points = switch_lsbmsb(*((uint16_t *)&uart_buffer[4]));
    master_data.CRC = *((uint16_t *)&uart_buffer[6]);

    // verifica codigo de erro
    if(usMBCRC16(uart_buffer, 6) != master_data.CRC) {
        *response_type = MODBUS_FAILED;
        return;
    }

    for (uint16_t i = 0; i < master_data.points; i++) {
        data |= (modbus_coil_map[master_data.initial_address + i] << i);
    }

    Slave_RMC_ST slave_buffer = { .id = 0x01,
                                .command_type = 0x02,
                                .size = 0x01,
                                .data = data,
                                .CRC = 0x0000 };

    slave_buffer.CRC = usMBCRC16((uint8_t *)&slave_buffer, 4);

    UARTSend((uint8_t *)&slave_buffer, 6);

    *response_type = MODBUS_WITH_NO_RETURN;
}

static void respond_RM_Reg(uint8_t * response_type) {
    int16_t data[5];
    uint8_t buffer[15];
    uint16_t crc;
    Master_R_ST master_data;

    master_data.id = uart_buffer[0];
    master_data.command_type = uart_buffer[1];
    master_data.initial_address = switch_lsbmsb(*((uint16_t *)&uart_buffer[2]));
    master_data.points = switch_lsbmsb(*((uint16_t *)&uart_buffer[4]));
    master_data.CRC = *((uint16_t *)&uart_buffer[6]);
    
    // verifica codigo de erro
    if(usMBCRC16(uart_buffer, 6) != master_data.CRC) {
        *response_type = MODBUS_FAILED;
        return;
    }

    for (uint16_t i = 0; i < master_data.points; i++) {
        data[i] = modbus_register_map[master_data.initial_address + i];
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

    *response_type = MODBUS_WITH_NO_RETURN;
}

static void respond_WS_Coil(uint8_t * response_type, uint16_t * data) {
    Master_WSC_ST master_data;

    master_data.id = uart_buffer[0];
    master_data.command_type = uart_buffer[1];
    master_data.address = switch_lsbmsb(*((uint16_t *)&uart_buffer[2]));
    master_data.data = *((uint16_t *)&uart_buffer[4]);
    master_data.CRC = *((uint16_t *)&uart_buffer[6]);

    // verifica codigo de erro
    if(usMBCRC16(uart_buffer, 6) != master_data.CRC) {
        *response_type = MODBUS_FAILED;
        return;
    }

    UARTSend(uart_buffer, 8);

    *data = (((uint16_t)master_data.data) & 0x00FF) |
            (((uint16_t)master_data.address << 0x8) & 0xFF00);
    *response_type = MODBUS_WITH_RETURN;
}

// apesar do nome responde somente um dado
static void respond_WM_Reg(uint8_t * response_type, uint16_t * data) {
    Master_WMR_ST master_data;

    master_data.id = uart_buffer[0];
    master_data.command_type = uart_buffer[1];
    master_data.initial_address = switch_lsbmsb(*((uint16_t *)&uart_buffer[2]));
    master_data.points = switch_lsbmsb(*((uint16_t *)&uart_buffer[4]));
    master_data.size = uart_buffer[6];
    master_data.data = switch_lsbmsb(*((uint16_t *)&uart_buffer[7]));
    master_data.CRC = *((uint16_t *)&uart_buffer[9]);

    // verifica codigo de erro
    if(usMBCRC16(uart_buffer, 9) != master_data.CRC) {
        *response_type = MODBUS_FAILED;
        return;
    }

    Slave_WMR_ST slave_buffer;

    slave_buffer.id = uart_buffer[0];
    slave_buffer.command_type = uart_buffer[1];
    slave_buffer.initial_address = *((uint16_t *)&uart_buffer[2]);
    slave_buffer.points = *((uint16_t *)&uart_buffer[4]);
    slave_buffer.CRC = usMBCRC16((uint8_t *)&slave_buffer, 6);

    UARTSend((uint8_t *)&slave_buffer, 8);

    *data = master_data.data;
    *response_type = MODBUS_WITH_RETURN;
}

/*****************************************************************************
 Definicao de funcoes externaveis
******************************************************************************/

uint8_t modbus_waitMasterRequest() {
    uint8_t iter = 0;

    while (1) {
        UARTReceive(&uart_buffer[iter++], 1, 1);
        if (uart_buffer[1] == COMMAND_WM_REG && iter == 11) {
            iter = 0;
            break;
        }
        if (uart_buffer[1] != COMMAND_WM_REG && iter == 8) {
            iter = 0;
            break;
        }
    }

    return uart_buffer[1];
}

Modbus_Response_ST modbus_respondMaster(void * data) {
    switch (uart_buffer[1]) {
        case COMMAND_RM_COIL:
            modbus_coil_map = (uint8_t *)data;
            break;
        case COMMAND_RM_REG:
            modbus_register_map = (int16_t *)data;
            break;
        default:
            break;
    }

    return process_master_request();
}