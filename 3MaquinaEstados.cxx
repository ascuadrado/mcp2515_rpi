/*
 *  3MaquinaEstados.cxx
 *  Alberto Sánchez Cuadrado
 *
 *  Es una daptación de la librería de NiryoRobotics
 *  (https://github.com/NiryoRobotics/niryo_one_ros/tree/master/mcp_can_rpi),
 *  la cual es a su vez una adaptación para Raspberry Pi de la librería de
 *  coryjfowler para Arduino
 *  (https://github.com/coryjfowler/MCP_CAN_lib).
 *
 *  Conexiones:
 *  Usaremos el bus SPI nº 0 de la Raspberry Pi para comunicarnos con el módulo mcp2515
 *      MOSI (GPIO10) - Pin 19;
 *      MISO (GPIO9) - Pin 21;
 *      SCLK (GPIO11) - Pin 23;
 *      CE0 (GPIO8) - Pin 24;
 *      INT (GPIO16) - Pin 22;
 *
 *			Protecciones (GPIO24) - Pin 22 (0=STOP / 1=OK);
 */


// Librerías I/O
#include <iostream>
#include <unistd.h>
#include <ctime>

#include "src/mcp_can_rpi.h"           // Librería CAN (mcp2515)

#define DEBUG_MODE                1    // Muestra en la consola más información

// Setup Variables
#define IntPIN                    25          // Pin interrupciones - GPIO 25
#define proteccionesPIN           24          // Pin de control interrupciones
#define nBMS                      3           // Número de celdas
#define chargerID                 0x1806E7F4  // ID del cargador

// Operation Variables
#define tensionMaxCarga           90      // Tensión máxima
#define intensidadMaxCarga        5       // Intensidad de carga
#define shuntVoltageMillivolts    100     // Tensión activación shunts
#define maxCellVoltage            4200    // Max tensión de cada celda (mV)
#define minCellVoltage            2800    // Min tensión de cada celda (mV)
#define maxTemp                   70      // Temp max en grados

// Data variables
#define nDatos                    nBMS * 14 + 9;
int  data[nDatos];                    // Variables a leer y guardar
char fileName[15] = "datos.txt";      // Fichero donde guardar datos

enum
{
    standby, charge, run
}
estado;

// Inicializamos una variable de clase MCP_CAN
MCP_CAN CAN(0, 10000000, IntPIN);             // (No hay que tocar nada aqui)

// Funciones lectura datos del bus CAN
void readIncomingCANMsg();
void saveData();
void getTime(char *dateString);
void queryAll(int charge);

// Funciones manejo datos
int checkCellsOK(); // Return 0 if all cells ok


int main()
{
    /* -----------------------------------------------------------------
     * SETUP
     * -----------------------------------------------------------------
     */

    printf("Welcome\n\n");
    estado = standby;

    // Inicialización de los pines GPIO y del bus SPI en la Raspberry Pi
    CAN.setupInterruptGpio();
    CAN.setupSpi();
    printf("GPIO Pins initialized & SPI started\n");

    // Inicialización wiringPi e inicializamos interrupciones
    wiringPiSetup();
    wiringPiISR(IntPIN, INT_EDGE_FALLING, readIncomingCANMsg);

    // Inicializar todos los datos a 0
    for (int i = 0; i < nDatos; i++)
    {
        data[i] = 0;
    }

    // Inicializamos CAN BUS
    while (CAN_OK != CAN.begin(MCP_ANY, CAN_250KBPS, MCP_8MHZ))
    {
        printf("CAN BUS Shield init fail\n");
        printf("Init CAN BUS Shield again\n\n");
        usleep(1000000);
    }
    printf("CAN BUS Shield init ok!\n"); // El bus ya está funcionando

    while (1)                            // Bucle de funcionamiento
    {
        /* -----------------------------------------------------------------
         * LOOP
         * -----------------------------------------------------------------
         */

        printf("----------------------------------\n\n");

        switch (estado)
        {
        case standby:
            printf("Standby mode on\n");
            queryAll(0);
            break;

        case charge:
            printf("Charge mode on\n");
            queryAll(1);
            break;

        case run:
            printf("Run mode on\n");
            queryAll(0);
            break;
        }

        saveData();
        usleep(1000000);
    }
    return 0;
}


void readIncomingCANMsg()
{
    INT8U  len    = 0;
    INT8U  buf[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    INT32U canId  = 0;

    if (CAN_MSGAVAIL == CAN.checkReceive()) // check if data coming
    {
        // Leemos el mensaje del bus
        CAN.readMsgBuf(&canId, &len, &buf[0]);

        if (canId == chargerID) // Mensaje del cargador
        {
            // Tensión detectada carga
            data[nBMS * 14] = ((buf[0] << 8) + buf[1]) * 100;

            // Intensidad actual de carga
            data[nBMS * 14 + 1] = ((buf[2] << 8) + buf[3]) * 100;

            // Flags
            int flags = buf[4];

            data[nBMS * 14 + 2] = flags & 0xFF; // Flag0
            data[nBMS * 14 + 3] = flags & 0xFF; // Flag1
            data[nBMS * 14 + 4] = flags & 0xFF; // Flag2
            data[nBMS * 14 + 5] = flags & 0xFF; // Flag3
            data[nBMS * 14 + 6] = flags & 0xFF; // Flag4
        }

        // Corregimos dirección para poder leer las direcciones de los BMS
        canId = canId & 0xFFFFFFF;

        if ((canId > 300) && (canId < 300 + 16 * 10)) // BMS
        {
            int n = (canId - 300) / 10;               // Numero del BMS (de 0 a 16)
            int m = (canId - 300 - n * 10 - 1);       // N paquete dentro del BMS (0-3)

            if (m < 3)                                // Paquete de tensiones
            {
                for (int i = 0; i < 4; i++)
                {
                    data[n * 14 + m * 4 + i] = (buf[2 * i] << 8) + buf[2 * i + 1];
                }
            }
            else if (m == 3) // Paquete de temperaturas
            {
                for (int i = 0; i < 2; i++)
                {
                    data[n * 14 + m * 4 + i] = buf[i] - 40;
                }
            }
        }
    }
}


void saveData() // Saves all data into file
{
    FILE *file;
    char date[50];

    getTime(date);

    file = fopen("data/" + date + ".txt", "w+");
    fprintf(file, "[");

    for (int i = 0; i < nDatos - 1; i++)
    {
        fprintf(file, " %d ,", data[i]);
    }

    fprintf(file, " %d ]", data[nDatos - 1]);
    fclose(file);
}


void queryAll(bool requestCharge)
{
    for (int i = 0; i < nBMS; i++)
    {
        CAN.queryBMS(i, shuntVoltageMillivolts);
    }

    if (requestCharge)
    {
        CAN.queryCharger(tensionMaxCarga, intensidadMaxCarga, chargerID, 1);
    }
    else
    {
        CAN.queryCharger(tensionMaxCarga, intensidadMaxCarga, chargerID, 0);
    }
}


int checkCellsOK()                   // Checks cells V and Temp
{
    for (int i = 0; i < nBMS; i++)   // Num BMS
    {
        for (int j = 0; j < 12; j++) // Num CelL
        {
            int v = data[i * 14 + j];

            if ((v < minCellVoltage) || (v > maxCellVoltage))
            {
                return 1; // Some cell is above or below
            }
        }
        for (int j = 0; j < 2; j++)
        {
            int temp = data[i * 14 + 12 + j];

            if (temp > maxTemp)
            {
                return 1;
            }
        }
    }

    return 0;
}


void getTime(char *dateString)
{
    time_t curr_time;
    tm     *curr_tm;
    char   date_string[100];
    char   time_string[100];

    time(&curr_time);
    curr_tm = localtime(&curr_time);

    strftime(dateString, 50, "20%y-%m-%d-%H-%M-%S", curr_tm);
}


// ---------------------------------------------------------------------
