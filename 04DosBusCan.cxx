/*
 * 4DosBusCan.cxx
 * Alberto Sánchez Cuadrado
 *
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
#define IntPIN0       25
#define IntPIN1       24

void printCANMsg0();
void printCANMsg1();

// Inicializamos una variable de clase MCP_CAN
// MCP_CAN(int spi_channel, int spi_baudrate, INT8U gpio_can_interrupt);
MCP_CAN CAN0(0, 10000000, IntPIN0); // (No hay que tocar nada aqui)
MCP_CAN CAN1(1, 10000000, IntPIN1); // (No hay que tocar nada aqui)

int main()
{
    /* -----------------------------------------------------------------
     * SETUP
     * -----------------------------------------------------------------
     */

    printf("Welcome\n\n");
    wiringPiSetup();


    // Inicialización de los pines GPIO y del bus SPI en la Raspberry Pi
    CAN0.setupInterruptGpio();
    CAN0.setupSpi();
    CAN1.setupInterruptGpio();
    CAN1.setupSpi();
    printf("GPIO Pins initialized & SPI started\n");

    // Inicialización wiringPi e inicializamos interrupciones
    wiringPiISR(IntPIN0, INT_EDGE_FALLING, printCANMsg0);
    wiringPiISR(IntPIN1, INT_EDGE_FALLING, printCANMsg1);

    /* Inicializamos el bus CAN:
     * INT8U begin(INT8U idmodeset, INT8U speedset, INT8U clockset);
     * Permitimos cualquier tipo de mensaje (standard o extended)
     * Velocidad del bus CAN es 250 KBPS (Fijado por el BMS)
     * Nuestro MCP2515 tiene un reloj de cuarzo de 8 MHz
     */
    while (CAN_OK != CAN1.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ))
    {
        printf("CAN BUS Shield 1 init fail\n");
        printf("Init CAN BUS Shield again\n\n");
        usleep(1000000);
    }
    printf("CAN BUS Shield 1 init ok!\n");   // El bus ya está funcionando
    CAN1.setMode(MCP_NORMAL);

    while (CAN_OK != CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ))
    {
        printf("CAN BUS Shield 0 init fail\n");
        printf("Init CAN BUS Shield again\n\n");
        usleep(1000000);
    }
    printf("CAN BUS Shield 0 init ok!\n");   // El bus ya está funcionando
    CAN0.setMode(MCP_NORMAL);



    // Los 2 bytes que vamos a enviar por el bus CAN
    uint8_t data0[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    uint8_t data1[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

    while (1)
    {
        /* -----------------------------------------------------------------
         * LOOP
         * -----------------------------------------------------------------
         */
        data0[1] = data0[1] + 1;
        data1[1] = data1[1] + 1;

        int result0 = CAN0.sendMsgBuf(0x12C, 1, 8, data0);
        printf("\n\nMessage sent from CAN 0: %d\n", result0);

        usleep(2000000);


        int result1 = CAN1.sendMsgBuf(0x12D, 1, 8, data1);
        printf("\n\nMessage sent from CAN 1: %d\n", result1);

        usleep(2000000);
    }
    return 0;
}


void printCANMsg0()
{
    INT8U  len    = 0;
    INT8U  buf[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    INT32U canId  = 0;

    if (CAN_MSGAVAIL == CAN0.checkReceive())  // check if data coming
    {
        // INT8U MCP_CAN::readMsgBuf(INT32U *id, INT8U *len, INT8U buf[])
        // Read data rellena canId, len y guarda los datos en buf
        CAN0.readMsgBuf(&canId, &len, &buf[0]);

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


void printCANMsg1()
{
    INT8U  len    = 0;
    INT8U  buf[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    INT32U canId  = 0;

    if (CAN_MSGAVAIL == CAN1.checkReceive())  // check if data coming
    {
        // INT8U MCP_CAN::readMsgBuf(INT32U *id, INT8U *len, INT8U buf[])
        // Read data rellena canId, len y guarda los datos en buf
        CAN1.readMsgBuf(&canId, &len, &buf[0]);

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
