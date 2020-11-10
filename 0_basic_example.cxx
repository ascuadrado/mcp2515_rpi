/*
 * 0_basic_example.cxx
 * Alberto Sánchez Cuadrado
 *
 * This example uses interrupts to read messages
 * It also sends a message periodically
 *
 * Connexions:
 * If using SPI bus 0 in Raspberry Pi:
 * MOSI (GPIO10);
 * MISO (GPIO9);
 * SCLK (GPIO11);
 * CE0 (GPIO8);
 * INT (GPIO25)
 *
 */


// Presinstalled libraries
#include <iostream>
#include <unistd.h>

// CAN library (with mcp2515)
#include "src/mcp_can_rpi.h"

// Shows more info on the console
#define DEBUG_MODE    1

// CAN setup
#define IntPIN        25
#define SPIBus        0
#define CANSpeed      CAN_500KBPS
#define MCPClock      MCP_8MHZ
#define MCPMode       MCP_NORMAL

// Message to be sent
#define N             8       // Max is 8
#define EXT           1       // 1=extended, 0=normal
#define DELAY         1000000 // Delay in microseconds
uint32_t id      = 0x12C;
uint8_t  data[N] = { 3, 14, 15, 2, 1, 2, 3, 4 };

// Auxiliary functions
void printCANMsg();

// New MCP_CAN instance
// MCP_CAN(int spi_channel, int spi_baudrate, INT8U gpio_can_interrupt);
MCP_CAN CAN(SPIBus, 10000000, IntPIN);

int main()
{
    /* -----------------------------------------------------------------
     * SETUP LOOP
     * -----------------------------------------------------------------
     */

    printf("Hello World! Program 0_basic_example.cxx is running!\n\n");

    // Initialize GPIO pins and SPI bus of the Raspberry Pi
    wiringPiSetup();
    CAN.setupInterruptGpio();
    CAN.setupSpi();
    printf("GPIO Pins initialized & SPI started\n");

    // Attach interrupt to read incoming messages
    wiringPiISR(IntPIN, INT_EDGE_FALLING, printCANMsg);

    /* Start CAN bus
     * INT8U begin(INT8U idmodeset, INT8U speedset, INT8U clockset);
     */

    while (CAN_OK != CAN.begin(MCP_ANY, CANSpeed, MCPClock))
    {
        printf("CAN BUS Shield init fail\n");
        printf("Trying to init CAN BUS Shield again\n\n");
        usleep(1000000);
    }
    printf("CAN BUS Shield init ok!\n");
    CAN.setMode(MCPMode);

    while (1)
    {
        /* -----------------------------------------------------------------
         * MAIN LOOP
         * -----------------------------------------------------------------
         */

        data[1] = data[1] + 1; // Just to show some change in message

        printf("\n\nMessage sent: %d\n", CAN.sendMsgBuf(id, EXT, N, data));

        usleep(DELAY);
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
        printf("Received data from ID: %lu | len:%d\n", canId, len);

        for (int i = 0; i < len; i++) // print the data
        {
            printf("(%d)", buf[i]);
            printf("\t");
        }
    }
}


// ---------------------------------------------------------------------------
