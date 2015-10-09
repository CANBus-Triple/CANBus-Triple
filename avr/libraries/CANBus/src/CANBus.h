#include <SPI.h>

/*
Created by: Kyle Crockett
For canduino with 16MHz oscillator.

6/17/2012:
Modified by Derek Kuschel for use with multiple MCP2515

CNFx register values.
use preprocessor command "_XXkbps"
"XX" is the baud rate.

10 kbps
CNF1/BRGCON1    b'00110001'     0x31
CNF2/BRGCON2    b'10111000'     0xB8
CNF3/BRGCON3    b'00000101'     0x05

20 kbps
CNF1/BRGCON1    b'00011000'     0x18
CNF2/BRGCON2    b'10111000'     0xB8
CNF3/BRGCON3    b'00000101'     0x05

50 kbps
CNF1/BRGCON1    b'00001001'     0x09
CNF2/BRGCON2    b'10111000'     0xB8
CNF3/BRGCON3    b'00000101'     0x05

100 kbps
CNF1/BRGCON1    b'00000100'     0x04
CNF2/BRGCON2    b'10111000'     0xB8
CNF3/BRGCON3    b'00000101'     0x05

125 kbps
CNF1/BRGCON1    b'00000011'     0x03
CNF2/BRGCON2    b'10111000'     0xB8
CNF3/BRGCON3    b'00000101'     0x05

250 kbps
CNF1/BRGCON1    b'00000001'     0x01
CNF2/BRGCON2    b'10111000'     0xB8
CNF3/BRGCON3    b'00000101'     0x05

500 kbps
CNF1/BRGCON1    b'00000000'     0x00
CNF2/BRGCON2    b'10111000'     0xB8
CNF3/BRGCON3    b'00000101'     0x05

800 kbps
Not yet supported

1000 kbps
Settings added by Patrick Cruce(pcruce_at_igpp.ucla.edu)
CNF1=b'10000000'=0x80 = SJW = 3 Tq. & BRP = 0
CNF2=b'10010000'=0x90 = BLTMode = 1 & SAM = 0 & PS1 = 3 & PR = 1
CNF3=b'00000010'=0x02 = SOF = 0  & WAKFIL = 0 & PS2 = 3

*/
#ifndef can_h
#define can_h

#define SCK 15  // SPI clock line
#define MISO 14
#define MOSI 16

#define RESET_REG 0xc0
#define READ 0x03
#define WRITE 0x02 // Read and write comands for SPI

#define READ_RX_BUF_0_ID 0x90
#define READ_RX_BUF_0_DATA 0x92
#define READ_RX_BUF_1_ID 0x94
#define READ_RX_BUF_1_DATA 0x96 // SPI commands for reading CAN RX buffers

#define LOAD_TX_BUF_0_ID 0x40
#define LOAD_TX_BUF_0_DATA 0x41
#define LOAD_TX_BUF_1_ID 0x42
#define LOAD_TX_BUF_1_DATA 0x43
#define LOAD_TX_BUF_2_ID 0x44
#define LOAD_TX_BUF_2_DATA 0x45 // SPI commands for loading CAN TX buffers

#define SEND_TX_BUF_0 0x81
#define SEND_TX_BUF_1 0x82
#define SEND_TX_BUF_2 0x83 // SPI commands for transmitting CAN TX buffers

#define READ_STATUS 0xA0
#define RX_STATUS 0xB0
#define BIT_MODIFY 0x05 // Other commands


// Registers
#define CNF1 0x2A  // Configuration registers (bit timing)
#define CNF2 0x29  
#define CNF3 0x28  
#define TXB0CTRL 0x30 // Transmit buffer control registers
#define TXB1CTRL 0x40
#define TXB2CTRL 0x50 
#define TXB0DLC 0x35 // Data length code registers
#define TXB1DLC 0x45
#define TXB2DLC 0x55
#define CANCTRL 0x0F // Mode control register
#define CANSTAT 0x0E // Status register
#define CANINTE 0x2B // Interrupt Enable
#define CANINTF 0x2C // Interrupt Flag
#define EFLG 0x2D // Error Register address


#define WAKIE 0x40
#define WAKIF 0x40
#define BFPCTRL 0x0C
#define B1BFS 0x20
#define B0BFS 0x10
#define B1BFE 0x08
#define B0BFE 0x04
#define B1BFM 0x02
#define B0BFM 0x01


#define RXB0CTRL 0x60
#define RXB1CTRL 0x70

#define RXF0SIDH	0x00
#define RXF0SIDL	0x01
#define RXF0EID8	0x02
#define RXF0EID0	0x03
#define RXF1SIDH	0x04
#define RXF1SIDL	0x05
#define RXF1EID8	0x06
#define RXF1EID0	0x07
#define RXF2SIDH	0x08
#define RXF2SIDL	0x09
#define RXF2EID8	0x0A
#define RXF2EID0	0x0B
#define RXF3SIDH	0x10
#define RXF3SIDL	0x11
#define RXF3EID8	0x12
#define RXF3EID0	0x13
#define RXF4SIDH	0x14
#define RXF4SIDL	0x15
#define RXF4EID8	0x16
#define RXF4EID0	0x17
#define RXF5SIDH	0x18
#define RXF5SIDL	0x19
#define RXF5EID8	0x1A
#define RXF5EID0	0x1B

#define RXM0SIDH	0x20
#define RXM0SIDL	0x21
#define RXM1SIDH	0x24
#define RXM1SIDL	0x25

#define MERRF       0x80


#include "Arduino.h"


struct MessageNew {
    byte length;
    unsigned short frame_id;
    byte frame_data[8];
    unsigned int busStatus;
    unsigned int busId;
    bool dispatch;
};

enum CANMode {CONFIGURATION,NORMAL,SLEEP,LISTEN,LOOPBACK,UNKNOWN};


class CANBus
{
private:
    int _ss;
    int _reset;

public:

    String name;
    unsigned int busId;

    CANBus( int ss, int reset, unsigned int bid, String nameString );
    CANBus( int ss, int reset );

    void setName(String s);
    void setBusId(unsigned int n);

    void begin();                       // Sets up MCP2515
    bool baudConfig(int bitRate);       // Sets up baud

    void reset();                       // Send MCP2515 Reset command

    void bitModify( byte reg, byte value, byte mask );

	// Method added to enable testing in loopback mode (pcruce_at_igpp.ucla.edu)
	void setMode(CANMode mode);         // Put CAN controller in one of five modes

    // Method to send CLKPRE (Clock output scaler) 1,2,4,8 available values.
    void setClkPre(int mode);

    // Set RX Filter registers
    void setFilter(int, int);
    void clearFilters();

    int getNextTxBuffer();

    // Interrupt control register methods
    void setRxInt(bool enable);

    byte readRegister( int addr );
    void writeRegister( int addr, byte value );
    void writeRegister( int addr, byte value, byte value2 );

	void transmitBuffer(int bufferId); // Request to transmit buffer X

	// Extending CAN data read to full frames (pcruce_at_igpp.ucla.edu)
	// Data_out should be array of 8-bytes or frame length.
    void readFullFrame(byte buffer_id, byte* length_out, byte *data_out, unsigned short *id_out);

	// Adding can to read status register (pcruce_at_igpp.ucla.edu)
	// Can be used to determine whether a frame was received.
	// (readStatus() & 0x80) == 0x80 means frame in buffer 0
	// (readStatus() & 0x40) == 0x40 means frame in buffer 1
    byte readStatus();

    // byte readControl();
    // byte readErrorRegister();

	// Extending CAN write to full frame (pcruce_at_igpp.ucla.edu)
	// Identifier should be a value between 0 and 2^11-1, longer identifiers will be truncated
    // (i.e. does not support extended frames)
	void loadFullFrame(byte bufferId, byte length, unsigned short identifier, byte *data);
};

#endif
