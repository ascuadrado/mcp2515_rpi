/*
 *  1_charger_and_bms.cxx
 *  Alberto Sánchez Cuadrado
 *
 * We will use the Library's built-in functions to query the BMS and the charger.
 *
 */

// Presinstalled libraries
#include <iostream>
#include <unistd.h>

// CAN library (with mcp2515)
#include "src/mcp_can_rpi.h"

// Shows more info on the console
#define DEBUG_MODE                1

// CAN setup
#define IntPIN                    25
#define SPIBus                    0
#define CANSpeed                  CAN_250KBPS
#define MCPClock                  MCP_8MHZ
#define MCPMode                   MCP_NORMAL

// BMS and charger setup
#define StartCharge               0    // 0 = don't charge
#define nBMS                      3    // Number of BMS modules
#define chargerID                 0x1806E7F4
#define maxChargingVolts          90   // Max charging voltage (V)
#define maxChargingAmps           5    // Max charging current (A)
#define shuntVoltageMillivolts    3600 // Cell balancing voltage (mV)

// New MCP_CAN instance
MCP_CAN CAN(SPIBus, 10000000, IntPIN);

// Auxiliary functions
void printCANMsg();
void readIncomingCANMsg();
void printData();
void saveData();
INT8U queryCharger(float voltage, float current, int address, int charge);
INT8U queryBMS(int moduleID, int shuntVoltageMillivolts);

// data -> [voltajes, temperaturas, tension cargador, corriente cargador]
int  data[nBMS * 14 + 7];
char fileName[15] = "datos.txt";
bool updateNeeded = false;

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
    wiringPiISR(IntPIN, INT_EDGE_FALLING, readIncomingCANMsg());

    // Inicializar todos los datos a 0
    for (int i = 0; i < nBMS * 14 + 7; i++)
    {
        data[i] = 0;
    }

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

        for (int i = 0; i < nBMS; i++)
        {
            queryBMS(i, shuntVoltageMillivolts);
            usleep(1000000);
        }

        startCharging(maxChargingVolts, maxChargingAmps, chargerID, StartCharge);

        if (updateNeeded)
        {
            saveData();
            printData();
        }
    }
    return 0;
}


void readIncomingCANMsg()
{
    INT8U  len    = 0;
    INT8U  buf[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    INT32U canId  = 0;

    if (CAN_MSGAVAIL == CAN.checkReceive())
    {
        updateNeeded = true;
        CAN.readMsgBuf(&canId, &len, &buf[0]);
        canId = canId & 0x1FFFFFFF;

        if (canId == chargerID)
        {
            // Vc
            data[nBMS * 14] = ((buf[0] << 8) + buf[1]) * 100;
            // Ic
            data[nBMS * 14 + 1] = ((buf[2] << 8) + buf[3]) * 100;
            // Flags
            int flags = buf[4];
            data[nBMS * 14 + 2] = (flags >> 7) & 0x1; // Flag 0
            data[nBMS * 14 + 3] = (flags >> 6) & 0x1; // Flag 1
            data[nBMS * 14 + 4] = (flags >> 5) & 0x1; // Flag 2
            data[nBMS * 14 + 5] = (flags >> 4) & 0x1; // Flag 3
            data[nBMS * 14 + 6] = (flags >> 3) & 0x1; // Flag 4
        }

        // BMS modules
        if ((canId > 300) && (canId < 300 + 16 * 10))
        {
            // BMS number (1-16)
            int n = (canId - 300) / 10;
            // Message number (0-3)
            int m = (canId - 300 - n * 10 - 1);

            // Voltage frame
            if (m < 3)
            {
                for (int i = 0; i < 4; i++)
                {
                    data[n * 14 + m * 4 + i] = (buf[2 * i] << 8) + buf[2 * i + 1];
                }
            }


            // Temperature frame
            else if (m == 3)
            {
                for (int i = 0; i < 2; i++)
                {
                    data[n * 14 + m * 4 + i] = buf[i] - 40;
                }
            }
        }
    }
}


void printCANMsg()
{
    INT8U  len    = 0;
    INT8U  buf[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    INT32U canId  = 0;

    if (CAN_MSGAVAIL == CAN.checkReceive())                             // check if data coming
    {
        // INT8U MCP_CAN::readMsgBuf(INT32U *id, INT8U *len, INT8U buf[])
        // Read data rellena canId, len (longitud de lo recibido) y guarda los datos en buf
        CAN.readMsgBuf(&canId, &len, &buf[0]);
        canId = canId & 0x1FFFFFFF;

        printf("-----------------------------\n");
        printf("Received data from ID: %lu | len:%d\n", canId, len);

        for (int i = 0; i < len; i++)                                                         // print the data
        {
            printf("(%d)", buf[i]);
            printf("\t");
        }
    }
}


void printData()
{
    for (int i = 0; i < nBMS; i++)
    {
        printf("\nBMS %d: \n", i);
        for (int j = 0; j < 12; j++)
        {
            printf("V%d: %d\t", j, data[14 * i + j]);
        }
    }

    printf("\nCharger: V = %d \t I = %d \t Flag = %d %d %d %d %d",
           data[3 * nBMS], data[3 * nBMS + 1], data[3 * nBMS + 2],
           data[3 * nBMS + 3], data[3 * nBMS + 4], data[3 * nBMS + 5],
           data[3 * nBMS + 6]);
}


void saveData()
{
    FILE *file;

    file = fopen(fileName, "w+");
    fprintf(file, "[");

    for (int i = 0; i < nBMS; i++)
    {
        for (int j = 0; j < 14; j++)
        {
            fprintf(file, " %d ,", data[14 * i + j]);
        }
    }

    fprintf(file, " %d , %d , %d ]", data[14 * nBMS], data[14 * nBMS + 1], data[14 * nBMS + 2]);
    fclose(file);
}


INT8U queryCharger(float voltage, float current, int address, int charge)
{
    uint8_t v = (uint8_t)(voltage * 10);
    uint8_t i = (uint8_t)(current * 10);
    uint8_t messageCharger[5] = { (uint8_t)(v >> 8) & 0xFF, v & 0xFF, (i >> 8) & 0xFF, i & 0xFF, 1 - charge };

    int res = sendMsgBuf((uint8_t)address, 1, 5, messageCharger);

    return res;
}


INT8U queryBMS(int moduleID, int shuntVoltageMillivolts)
{
    uint8_t messageBMS[2] = { (shuntVoltageMillivolts >> 8) & 0xFF, shuntVoltageMillivolts & 0xFF };

    int res = sendMsgBuf(300 + 10 * moduleID, 1, 2, messageBMS);

    return res;
}


// ---------------------------------------------------------------------
