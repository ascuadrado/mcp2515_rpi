# mcp2515_rpi
Raspberry Pi 3 library for MCP2515 module (CAN bus interface) through SPI GPIOs.

Forked from [mcp_can_rpi](https://github.com/NiryoRobotics/niryo_one_ros/tree/master/mcp_can_rpi) library.

The MCP2515 module is a SPI-CAN interface. The MCP_CAN library is using the SPI protocol on Arduino to program and use this module. It has been adapted here to work with the Raspberry Pi 3 GPIOs, using the SPI functions of the wiringPi library. On top of that a few functions are added for ease of use with ZEVA's BMS modules and SEVCON's GEN 4 motor controller. 


## Usage
The best way is to look into 0_basic_example.cxx. The functions used are described below. 

1. Create CAN instance

```c
MCP_CAN(int spi_channel, int spi_baudrate, INT8U gpio_can_interrupt);
// spi_channel: 0 or 1 
// spi_baudrate: Usually 1000000 (1Mbit/s)
// gpio_can_interrupt: Interrupt pin that will be used (GPIO numbering as in WiringPi)
// ex. 
MCP_CAN CAN(0, 10000000, 25); // (No hay que tocar nada aqui)
```

2. Setup GPIO & SPI

```c
// Starts wiringPi
wiringPiSetup();

// Equivalent to doing pinMode(interruptPIN, INPUT)
CAN.setupInterruptGpio(); 

// Uses WiringPi to initialize SPI channel at the speed defined earlier
CAN.setupSpi();
```

3. Attach interrupt

```c
// Using WiringPi library attach interrupt to a function that reads the message
// ex.
wiringPiISR(IntPIN, INT_EDGE_FALLING, printCANMsg);
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

```

4. Initialize CANBus

```c
INT8U begin(INT8U idmodeset, INT8U speedset, INT8U clockset);
// idmodeset: MCP_ANY: Masks & Filters diabled / MCP_STD: Only standard IDs / MCP_EXT: Only extended IDs / MCP_STDEXT: Standard and extended
// speedset: CAN_250KBPS/ CAN_500KBPS/ ... (for a complete list of speeds search in mcp_can_dfs_rpi.h)
// clockset: MCP_8MHZ / MCP_16MHZ
// ex. 
CAN.begin(MCP_ANY, CAN_250KBPS, MCP_8MHZ);
```

5. Set CAN mode

```c
setMode(const INT8U opMode);
// opMode: MCP_NORMAL / MCP_SLEEP / MCP_LOOPBACK (receives messages sent too) / MCP_LISTENONLY
//ex. 
CAN.setMode(MCP_NORMAL);
```

6. Send message

```c
INT8U MCP_CAN::sendMsgBuf(INT32U id, INT8U ext, INT8U len, INT8U *buf);
// id: Sending ID address (11 bits std or 29 bits ext)
// ext: 0=standard, 1=extended
// len: number of bytes to send (1 to 8)
// *buf: array where data is stored
// ex. 
uint8_t data = {1,2,3,4,5,6,7,8};
CAN.sendMsgBuf(0x12C, 1, 8, data);
```













