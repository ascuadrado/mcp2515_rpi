/*
 *  ejemplo.cxx
 *  Alberto Sánchez Cuadrado
 *
 *  Ejemplo de utilización de la librería CAN del ISC:
 *  Es una daptación de la librería de NiryoRobotics
 *  (https://github.com/NiryoRobotics/niryo_one_ros/tree/master/mcp_can_rpi),
 *  la cual es a su vez una adaptación para Raspberry Pi de la librería de coryjfowler para Arduino
 *  (https://github.com/coryjfowler/MCP_CAN_lib).
 *
 *  Conexiones:
 *  Usaremos el bus SPI nº 0 de la Raspberry Pi para comunicarnos con el módulo mcp2515
 *      MOSI (GPIO10);
 *      MISO (GPIO9);
 *      SCLK (GPIO11);
 *      CE0 (GPIO8);
 *      INT (GPIO16)
 *
 */


// Librerías preinstaladas
#include <iostream>
#include <unistd.h>

#include "src/mcp_can_rpi.h"                  // Librería CAN (mcp2515)

#define DEBUG_MODE                1           // Muestra en la consola más información

#define IntPIN                    25          // Pin de interrupciones es GPIO 25
#define nBMS                      0           // Número de celdas
#define chargerID                 0x1806E7F4  // ID del cargador
#define voltage                   90          // Tensión máxima
#define intensidad                5           // Intensidad de carga
#define shuntVoltageMillivolts    3600        // Tensión en la que se empieza a balancear

// Inicializamos una variable de clase MCP_CAN
// MCP_CAN(int spi_channel, int spi_baudrate, INT8U gpio_can_interrupt);
MCP_CAN CAN(0, 10000000, IntPIN);  // (No hay que tocar nada aqui)

// Funciones lectura datos
void printCANMsg();
void readIncomingCANMsg();
void printData();

// data -> [voltajes, temperaturas, tension cargador, corriente cargador]
int     data[nBMS * 14 + 3];                    // Variables a leer
uint8_t messageBMS[2] = { 0, 0 };               // Los 2 bytes que vamos a enviar al BMS

int main()
{
    printf("Welcome\n\n");

    // Inicialización de los pines GPIO y del bus SPI en la Raspberry Pi
    CAN.setupInterruptGpio();
    CAN.setupSpi();
    printf("GPIO Pins initialized & SPI started\n");

    // Inicialización wiringPi e inicializamos interrupciones
    wiringPiSetup();
    wiringPiISR(IntPIN, INT_EDGE_FALLING, readIncomingCANMsg);

    /* Inicializamos el bus CAN:
     * INT8U begin(INT8U idmodeset, INT8U speedset, INT8U clockset);
     * Permitimos cualquier tipo de mensaje (standard o extended)
     * Velocidad del bus CAN es 250 KBPS (Fijado por el BMS)
     * Nuestro MCP2515 tiene un reloj de cuarzo de 8 MHz
     */

    while (CAN_OK != CAN.begin(MCP_ANY, CAN_250KBPS, MCP_8MHZ))
    {
        printf("CAN BUS Shield init fail\n");
        printf("Init CAN BUS Shield again\n\n");
        usleep(1000000);
    }
    printf("CAN BUS Shield init ok!\n");  // El bus ya está funcionando

    while (1)
    {
        for (int i = 0; i < nBMS; i++)
        {
            CAN.queryBMS(i, shuntVoltageMillivolts);
        }
        CAN.startCharging(voltage, intensidad, chargerID);

        usleep(1000000);
    }
    return 0;
}


void readIncomingCANMsg()
{
    INT8U  len    = 0;
    INT8U  buf[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    INT32U canId  = 0;

    if (CAN_MSGAVAIL == CAN.checkReceive())  // check if data coming
    {
        // INT8U MCP_CAN::readMsgBuf(INT32U *id, INT8U *len, INT8U buf[])
        // Read data rellena canId, len (longitud de lo recibido) y guarda los datos en buf
        CAN.readMsgBuf(&canId, &len, &buf[0]);

        if (canId == chargerID)
        {
            data[nBMS * 14]     = ((buf[0] << 8) + buf[1]) * 100;
            data[nBMS * 14 + 1] = ((buf[2] << 8) + buf[3]) * 100;
            data[nBMS * 14 + 2] = buf[4];
        }

        canId = canId & 0xFFFFFFF;

        if ((canId > 300) && (canId < 300 + 16 * 10))  // BMS
        {
            int n = (canId - 300) / 10;                // Numero del BMS (de 0 a 16)
            int m = (canId - 300 - n * 10 - 1);        // N paquete dentro del BMS (0-3)
            if (m < 3)
            {
                for (int i = 0; i < 4; i++)
                {
                    data[n * 12 + m * 4 + i] = (buf[2 * i] << 8) + buf[2 * i + 1];
                }
            }
            else if (m == 3)
            {
                for (int i = 0; i < 2; i++)
                {
                    data[n * 12 + m * 4 + i] = buf[i] - 40;
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

    if (CAN_MSGAVAIL == CAN.checkReceive())   // check if data coming
    {
        // INT8U MCP_CAN::readMsgBuf(INT32U *id, INT8U *len, INT8U buf[])
        // Read data rellena canId, len (longitud de lo recibido) y guarda los datos en buf
        CAN.readMsgBuf(&canId, &len, &buf[0]);

        for (int i = 0; i < len; i++)  // print the data
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

    printf("\nCharger: V = %d \t I = %d \t Flag = %d", data[3 * nBMS], data[3 * nBMS + 1], data[3 * nBMS + 2]);
}


// ---------------------------------------------------------------------
