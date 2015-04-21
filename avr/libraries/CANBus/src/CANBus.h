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

#define SCK 15  //spi clock line
#define MISO 14
#define MOSI 16

#define RESET_REG 0xc0
#define READ 0x03
#define WRITE 0x02 //read and write comands for SPI

#define READ_RX_BUF_0_ID 0x90
#define READ_RX_BUF_0_DATA 0x92
#define READ_RX_BUF_1_ID 0x94
#define READ_RX_BUF_1_DATA 0x96 //SPI commands for reading CAN RX buffers

#define LOAD_TX_BUF_0_ID 0x40
#define LOAD_TX_BUF_0_DATA 0x41
#define LOAD_TX_BUF_1_ID 0x42
#define LOAD_TX_BUF_1_DATA 0x43
#define LOAD_TX_BUF_2_ID 0x44
#define LOAD_TX_BUF_2_DATA 0x45 //SPI commands for loading CAN TX buffers

#define SEND_TX_BUF_0 0x81
#define SEND_TX_BUF_1 0x82
#define SEND_TX_BUF_2 0x83 //SPI commands for transmitting CAN TX buffers

#define READ_STATUS 0xA0
#define RX_STATUS 0xB0
#define BIT_MODIFY 0x05 //Other commands


//Registers
#define CNF1 0x2A
#define CNF2 0x29
#define CNF3 0x28
#define TXB0CTRL 0x30
#define TXB1CTRL 0x40
#define TXB2CTRL 0x50 //TRANSMIT BUFFER CONTROL REGISTER
#define TXB0DLC 0x35 //Data length code registers
#define TXB1DLC 0x45
#define TXB2DLC 0x55
#define CANCTRL 0x0F //Mode control register
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
#define RXM0EID8	0x22
#define RXM0EID0	0x23
#define RXM1SIDH	0x24
#define RXM1SIDL	0x25
#define RXM1EID8	0x26
#define RXM1EID0	0x27

#define MERRF       0x80

#define SUPPORTS_29BIT 0


#ifdef SUPPORTS_29BIT
    typedef unsigned long IDENTIFIER_INT;
#else
    typedef unsigned short IDENTIFIER_INT;
#endif

#include "Arduino.h"

struct MessageNew {
    byte length;
    IDENTIFIER_INT frame_id;
    byte frame_data[8];
    byte ide;
    unsigned int busStatus;
    unsigned int busId;
    bool dispatch;
};

enum CANMode {CONFIGURATION,NORMAL,SLEEP,LISTEN,LOOPBACK};

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

    void begin();                       //sets up MCP2515
    bool baudConfig(int bitRate);       //sets up baud

    void reset();                       // Send MCP2515 Reset command

    void bitModify( byte reg, byte value, byte mask  );

	//Method added to enable testing in loopback mode.(pcruce_at_igpp.ucla.edu)
	void setMode(CANMode mode) ;        //put CAN controller in one of five modes

    // Method to send CLKPRE (Clock output scaler) 1,2,4,8 available values.
    void setClkPre(int mode);

    // Set RX Filter registers
    void setFilter(byte ide, IDENTIFIER_INT filter0, IDENTIFIER_INT filter1);
    void clearFilters();

    int getNextTxBuffer();

    // Interrupt control register methods
    void setRxInt(bool b);


    byte readRegister( int addr );
    void writeRegister( int addr, byte value );
    void writeRegister( int addr, byte value, byte value2 );
    void writeRegister( int addr, byte value, byte value2, byte value3, byte value4 );

    // byte readTXBNCTRL(int bufferid);

	void send_0();//request to transmit buffer X
	void send_1();
	void send_2();

	char readID_0();//read ID/DATA of recieve buffer X
	char readID_1();

	char readDATA_0();
	char readDATA_1();

	//extending CAN data read to full frames(pcruce_at_igpp.ucla.edu)
	//data_out should be array of 8-bytes or frame length.
	void readDATA_ff_0(byte* length_out,byte *data_out,IDENTIFIER_INT *id_out,byte *ide);
	void readDATA_ff_1(byte* length_out,byte *data_out,IDENTIFIER_INT *id_out,byte *ide);

	//Adding can to read status register(pcruce_at_igpp.ucla.edu)
	//can be used to determine whether a frame was received.
	//(readStatus() & 0x80) == 0x80 means frame in buffer 0
	//(readStatus() & 0x40) == 0x40 means frame in buffer 1
    byte readStatus();

    // byte readControl();
    // byte readErrorRegister();

    void load_0(byte identifier, byte data);//load transmit buffer X
	void load_1(byte identifier, byte data);
	void load_2(byte identifier, byte data);

	//extending CAN write to full frame(pcruce_at_igpp.ucla.edu)
	//Identifier should be a value between 0 and 2^11-1 for 11-bit messages
    //Identifier should be a value between 0 and 2^29-1 for 29-bit messages
	void load_ff_0(byte length,IDENTIFIER_INT identifier,byte *data,byte ide);
    void load_ff_1(byte length,IDENTIFIER_INT identifier,byte *data,byte ide);
	void load_ff_2(byte length,IDENTIFIER_INT identifier,byte *data,byte ide);

};

#endif
