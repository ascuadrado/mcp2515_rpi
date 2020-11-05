/*
 * 0Ejemplo_basico.cxx
 * Alberto Sánchez Cuadrado
 *
 * Ejemplo con interrupciones:
 *    Muestra cualquier mensaje que llegue
 *
 * Conexiones:
 * Usaremos el bus SPI nº 0 de la Raspberry Pi
 * para comunicarnos con el módulo mcp2515
 * MOSI (GPIO10);
 * MISO (GPIO9);
 * SCLK (GPIO11);
 * CE0 (GPIO8);
 * INT (GPIO25)
 *
 */


// Librerías preinstaladas
#include <iostream>
#include <unistd.h>

// Librería CAN (mcp2515)
#include "src/mcp_can_rpi.h"

// Muestra en la consola más información
#define DEBUG_MODE    1

// Pin de interrupciones es GPIO 25
#define IntPIN        24

void printCANMsg();

// Inicializamos una variable de clase MCP_CAN
// MCP_CAN(int spi_channel, int spi_baudrate, INT8U gpio_can_interrupt);
MCP_CAN CAN(1, 10000000, IntPIN); // (No hay que tocar nada aqui)

int main()
{
    /* -----------------------------------------------------------------
     * SETUP
     * -----------------------------------------------------------------
     */

    printf("Welcome\n\n");
    wiringPiSetup();

    // Inicialización de los pines GPIO y del bus SPI en la Raspberry Pi
    CAN.setupInterruptGpio();
    CAN.setupSpi();
    printf("GPIO Pins initialized & SPI started\n");

    // Inicialización wiringPi e inicializamos interrupciones
    
    wiringPiISR(IntPIN, INT_EDGE_FALLING, printCANMsg);

    /* Inicializamos el bus CAN:
     * INT8U begin(INT8U idmodeset, INT8U speedset, INT8U clockset);
     * Permitimos cualquier tipo de mensaje (standard o extended)
     * Velocidad del bus CAN es 250 KBPS (Fijado por el BMS)
     * Nuestro MCP2515 tiene un reloj de cuarzo de 8 MHz
     */

    while (CAN_OK != CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ))
    {
        printf("CAN BUS Shield init fail\n");
        printf("Init CAN BUS Shield again\n\n");
        usleep(1000000);
    }
    printf("CAN BUS Shield init ok!\n");   // El bus ya está funcionando
    CAN.setMode(MCP_NORMAL);

    // Los 2 bytes que vamos a enviar por el bus CAN
    uint8_t data[8] = { 3, 14, 15, 2, 1, 2, 3, 4};

    while (1)
    {
        /* -----------------------------------------------------------------
         * LOOP
         * -----------------------------------------------------------------
         */
         data[3] = data[3]+1;

        int result = CAN.sendMsgBuf(0x12C, 1, 8, data);
        printf("\n\nMessage sent: %d\n", result);

        usleep(2000000);
        //printCANMsg();
    }
    return 0;
}


void printCANMsg()
{
    INT8U  len    = 0;
    INT8U  buf[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    INT32U canId  = 0;

    if (CAN_MSGAVAIL == CAN.checkReceive())  // check if data coming
    {
        // INT8U MCP_CAN::readMsgBuf(INT32U *id, INT8U *len, INT8U buf[])
        // Read data rellena canId, len y guarda los datos en buf
        CAN.readMsgBuf(&canId, &len, &buf[0]);
        
        canId = canId & 0x1FFFFFFF;
        printf("-----------------------------\n");
        printf("get data from ID: %lu | len:%d\n", canId, len);

        for (int i = 0; i < len; i++) // print the data
        {
            printf("(%d)", buf[i]);
            printf("\t");
        }
    }
}


// ---------------------------------------------------------------------------
