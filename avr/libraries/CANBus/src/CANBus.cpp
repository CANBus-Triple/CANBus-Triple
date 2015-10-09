
/*
6/17/2012:
Modified by Derek Kuschel for use with multiple MCP2515
*/


#include <SPI.h>
#include "Arduino.h"
#include <CANBus.h>

//define OLD_BAUD
#define FREQ 16


CANBus::CANBus( int ss, int reset, unsigned int bid, String nameString )
{
    _ss = ss;
    _reset = reset;
    busId = bid;
    name = nameString;
}

CANBus::CANBus( int ss, int reset )
{
    CANBus( ss, reset, 0, "Default" );
}

void CANBus::setName( String s )
{
    name = s;
}

void CANBus::setBusId( unsigned int n )
{
    busId = n;
}

// Constructor for initializing can module.
void CANBus::begin()
{
    // Set the slaveSelectPin as an output
    pinMode (SCK,OUTPUT);
    pinMode (MISO,INPUT);
    pinMode (MOSI, OUTPUT);
    pinMode (_ss, OUTPUT);
    pinMode (_reset,OUTPUT);

    // Initialize SPI:
    SPI.begin();
    SPI.setDataMode(SPI_MODE0);
    SPI.setClockDivider(SPI_CLOCK_DIV2);
    SPI.setBitOrder(MSBFIRST);

    digitalWrite(_reset, LOW); /* RESET CAN CONTROLLER*/
    delay(50);
    digitalWrite(_reset, HIGH);
    delay(50);
}

// Constructor for initializing can module.
void CANBus::reset()
{
    digitalWrite(_ss, LOW);
    delay(1);
    SPI.transfer(RESET_REG);
    delay(1);
    digitalWrite(_ss, HIGH);
}

#ifdef OLD_BAUD
// Sets bitrate for CAN node
bool CANBus::baudConfig(int bitRate)
{
    byte config1, config2, config3;

    switch (bitRate) {
        case 10:
            config1 = 0x31;
            config2 = 0xB8;
            config3 = 0x05;
            break;
        case 20:
            config1 = 0x18;
            config2 = 0xB8;
            config3 = 0x05;
            break;
        case 50:
            config1 = 0x09;
            config2 = 0xB8;
            config3 = 0x05;
            break;
        case 83:
            config1 = 0x03;
            config2 = 0xBE;
            config3 = 0x07;
            break;
        case 100:
            config1 = 0x04;
            config2 = 0xB8;
            config3 = 0x05;
            break;
        case 125:
            config1 = 0x03;
            config2 = 0xB8;
            config3 = 0x05;
            break;
        case 250:
            config1 = 0x01;
            config2 = 0xB8;
            config3 = 0x05;
            break;
        case 500:
            config1 = 0x00;
            config2 = 0xB8;
            config3 = 0x05;
            break;
        case 1000:
            // 1 megabit mode added by Patrick Cruce(pcruce_at_igpp.ucla.edu)
            // Faster communications enabled by shortening bit timing phases(3 Tq. PS1 & 3 Tq. PS2) 
            // Note that this may exacerbate errors due to synchronization or arbitration.
            config1 = 0x80;
            config2 = 0x90;
            config3 = 0x02;
            break;
    }

    this->writeRegister(CNF1, config1);
    this->writeRegister(CNF2, config2);
    this->writeRegister(CNF3, config3);

    return true;
}
#else
/*
*   New Baud rate calculation
*/
bool CANBus::baudConfig(int bitRate)//sets bitrate for CAN node
{
    // Calculate bit timing registers
    byte BRP;
    float TQ;
    byte BT;
    float tempBT;

    float NBT = 1.0 / (float)bitRate * 1000.0; // Nominal Bit Time

    for( BRP=0; BRP<8 ;BRP++ ) {
        TQ = 2.0 * (float)(BRP + 1) / (float)FREQ;
        tempBT = NBT / TQ;

        if(tempBT<=25) {
            BT = (int)tempBT;
            if(tempBT-BT==0) break;
        }
    }

    byte SPT = (0.8 * BT); // Sample point 80%
    byte PRSEG = (SPT - 1) / 2;
    byte PHSEG1 = SPT - PRSEG - 1;
    byte PHSEG2 = BT - PHSEG1 - PRSEG - 1;

    byte SJW = 1;

    // Programming requirements
    if(PRSEG + PHSEG1 < PHSEG2) return false;
    if(PHSEG2 <= SJW) return false;

    byte BTLMODE = 1;
    byte SAM = 0;

    // Set registers
    byte config1 = (((SJW-1) << 6) | BRP);
    byte config2 = ((BTLMODE << 7) | (SAM << 6) | ((PHSEG1-1) << 3) | (PRSEG-1));
    byte config3 = (B00000000 | (PHSEG2-1));
    
    // Hard coded configs
    switch(bitRate){
        case 83:
            config1 = 0x03;
            config2 = 0xBE;
            config3 = 0x07;
        break;
    }

    this->writeRegister(CNF1, config1);
    this->writeRegister(CNF2, config2);
    this->writeRegister(CNF3, config3);

    /*
    Serial.print("Bit rate = ");
    Serial.println(bitRate);
    Serial.print("TQ = ");
    Serial.println(TQ);
    Serial.print("BT = ");
    Serial.println(BT);
    Serial.print("BRP = ");
    Serial.println(BRP);
    Serial.print("Prop = ");
    Serial.println(PRSEG);
    Serial.print("Phase 1 = ");
    Serial.println(PHSEG1);
    Serial.print("Phase 2 = ");
    Serial.println(PHSEG2);

    Serial.print("CNT1 ");
    Serial.println(config1, BIN);
    Serial.print("CNT2 ");
    Serial.println(config2, BIN);
    Serial.print("CNT3 ");
    Serial.println(config3, BIN);
    */
    return true;
}
#endif


void CANBus::bitModify( byte reg, byte value, byte mask )
{
    digitalWrite(_ss, LOW);
    SPI.transfer(BIT_MODIFY);
    SPI.transfer(reg);
    SPI.transfer(mask);
    SPI.transfer(value);
    digitalWrite(_ss, HIGH);
}


int CANBus::getNextTxBuffer()
{
    byte stat = this->readStatus();

    if( (stat & 0x4) != 0x4 )   return 0;
    if( (stat & 0x10) != 0x10 ) return 1;
    if( (stat & 0x40) != 0x40 ) return 2;
    return -1;
}


void CANBus::setFilter( int filter0, int filter1 ){
    
    byte SIDH = filter0 >> 3;
    byte SIDL = filter0 << 5;
    this->writeRegister(RXF0SIDH, SIDH, SIDL ); // RXB0
    this->writeRegister(RXF2SIDH, SIDH, SIDL ); // RXB1

    SIDH = filter1 >> 3;
    SIDL = filter1 << 5;
    this->writeRegister(RXF1SIDH, SIDH, SIDL ); // RXB0
    this->writeRegister(RXF3SIDH, SIDH, SIDL ); // RXB1
    this->writeRegister(RXF4SIDH, SIDH, SIDL ); // RXB1
    this->writeRegister(RXF5SIDH, SIDH, SIDL ); // RXB1

    // Set mask to match everything
    /*
    int combined = filter0 | filter1;
    SIDH = combined >> 3;
    SIDL = combined << 5;
    */
    SIDH = 0xFF >> 3;
    SIDL = 0xFF << 5;
    this->writeRegister(RXM0SIDH, SIDH, SIDL );
    this->writeRegister(RXM1SIDH, SIDH, SIDL );

}


void CANBus::clearFilters(){
    this->writeRegister(RXM0SIDH, 0, 0 );
    this->writeRegister(RXM1SIDH, 0, 0 );
}


// Enable / Disable interrupt pin on message Rx
void CANBus::setRxInt(bool enable)
{
    this->bitModify(CANINTE, enable? 0x03 : 0x00, 0x03);
}


// Clear interrupts
/*
void CANBus::clearInterrupt(){

    byte mask,writeVal;

    writeVal = 0x00;

    mask = 0x03;

    digitalWrite(_ss, LOW);
    SPI.transfer(BIT_MODIFY);
    SPI.transfer(CANINTF);
    SPI.transfer(mask);
    SPI.transfer(writeVal);
    digitalWrite(_ss, HIGH);
}
*/


// Set clock output scaler
void CANBus::setClkPre(int mode)
{
    byte writeVal;

    switch(mode) {
        case 1:
            writeVal = 0x00;
            break;
        case 2:
            writeVal = 0x01;
            break;
        case 4:
            writeVal = 0x02;
            break;
        case 8:
            writeVal = 0x03;
            break;  
    }

    this->bitModify(CANCTRL, writeVal, 0x03);
}


// Method added to enable testing in loopback mode.(pcruce_at_igpp.ucla.edu)
// Put CAN controller in one of five modes
void CANBus::setMode(CANMode mode)  
{
    byte writeVal;

    switch(mode) {
      case CONFIGURATION:
          writeVal = 0x80;
          break;
      case NORMAL:
          writeVal = 0x00;
          break;
      case SLEEP:
          writeVal = 0x20;
          break;
      case LISTEN:
          writeVal = 0x60;
          break;
      case LOOPBACK:
          writeVal = 0x40;
          break;
    }

    this->bitModify(CANCTRL, writeVal, 0xE0);
}


void CANBus::transmitBuffer(int bufferId) // Transmits buffer 
{
    byte bufAddr;
    switch(bufferId) {
        case 0:
            bufAddr = SEND_TX_BUF_0;
            break;
        case 1:
            bufAddr = SEND_TX_BUF_1;
            break;
        case 2:
            bufAddr = SEND_TX_BUF_2;
            break;
        default:
            return;
            break;
    }

    // Delays removed from SEND command(pcruce_at_igpp.ucla.edu)
    // In testing we found that any lost data was from PC<->Serial Delays,
    // not CAN Controller/AVR delays. Thus removing the delays at this level
    // allows maximum flexibility and performance.
    digitalWrite(_ss, LOW);
    SPI.transfer(bufAddr);
    digitalWrite(_ss, HIGH);
}


// Extending CAN data read to full frames(pcruce_at_igpp.ucla.edu)
// It is the responsibility of the user to allocate memory for output.
// If you don't know what length the bus frames will be, data_out should be 8-bytes
void CANBus::readFullFrame(byte buffer_id, byte* length_out, byte *data_out, unsigned short *id_out)
{
    byte len, i;
    unsigned short id_h, id_l;

    digitalWrite(_ss, LOW);
    SPI.transfer((buffer_id == 0)? READ_RX_BUF_0_ID : READ_RX_BUF_1_ID);
    id_h = (unsigned short) SPI.transfer(0xFF); //id high
    id_l = (unsigned short) SPI.transfer(0xFF); //id low
    SPI.transfer(0xFF); // Extended id high(unused)
    SPI.transfer(0xFF); // Extended id low(unused)
    len = (SPI.transfer(0xFF) & 0x0F); //data length code
    for (i = 0; i < len; i++) {
        data_out[i] = SPI.transfer(0xFF);
    }
    digitalWrite(_ss, HIGH);
    (*length_out) = len;
    (*id_out) = ((id_h << 3) + ((id_l & 0xE0) >> 5)); // Repack identifier
}


byte CANBus::readStatus()
{
    byte retVal;
    digitalWrite(_ss, LOW);
    SPI.transfer(READ_STATUS);
    retVal = SPI.transfer(0xFF);
    digitalWrite(_ss, HIGH);
    return retVal;
}


byte CANBus::readRegister( int addr )
{
    byte retVal;
    digitalWrite(_ss, LOW);
    SPI.transfer(READ);
    SPI.transfer(addr);
    retVal = SPI.transfer(0xFF);
    digitalWrite(_ss, HIGH);
    return retVal;
}

void CANBus::writeRegister( int addr, byte value )
{
    digitalWrite(_ss, LOW);
    delay(1);
    SPI.transfer(WRITE);
    SPI.transfer(addr);
    SPI.transfer(value);
    delay(1);
    digitalWrite(_ss, HIGH);
    delay(1);
}


void CANBus::writeRegister( int addr, byte value, byte value2 )
{
    digitalWrite(_ss, LOW);
    delay(1);
    SPI.transfer(WRITE);
    SPI.transfer(addr);
    SPI.transfer(value);
    SPI.transfer(value2);
    delay(1);
    digitalWrite(_ss, HIGH);
    delay(1);
}


/*
byte CANBus::readControl()
{
    byte retVal;
    digitalWrite(_ss, LOW);
    SPI.transfer(READ);
    SPI.transfer(CANCTRL);
    retVal = SPI.transfer(0xFF);
    digitalWrite(_ss, HIGH);
    return retVal;
}
*/

/*
// Read Error Register
byte CANBus::readErrorRegister()
{
    byte retVal;
    digitalWrite(_ss, LOW);
    SPI.transfer(READ);
    SPI.transfer(EFLG);
    retVal = SPI.transfer(0xFF);
    digitalWrite(_ss, HIGH);
    return retVal;
}
*/

/*
// Check Transmit Buffer Controls.
byte CANBus::readTXBNCTRL(int bufferid)
{
    byte retVal;
    digitalWrite(_ss, LOW);
    SPI.transfer(READ);

    switch(bufferid){
        case 0:
            SPI.transfer(TXB0CTRL);
            break;
        case 1:
            SPI.transfer(TXB1CTRL);
            break;
        case 2:
            SPI.transfer(TXB2CTRL);
            break;
    }

    retVal = SPI.transfer(0xFF);
    digitalWrite(_ss, HIGH);
    return retVal;
}
*/


void CANBus::loadFullFrame(byte bufferId, byte length, unsigned short identifier, byte *data)
{
    byte i, id_high, id_low, bufAddr;

    switch(bufferId) {
        case 0:
            bufAddr = LOAD_TX_BUF_0_ID;
            break;
        case 1:
            bufAddr = LOAD_TX_BUF_1_ID;
            break;
        case 2:
            bufAddr = LOAD_TX_BUF_2_ID;
            break;
        default:
            return;
            break;
    }

    // Generate id bytes before SPI write
    id_high = (byte) (identifier >> 3);
    id_low = (byte) ((identifier << 5) & 0x00E0);

    digitalWrite(_ss, LOW);
    SPI.transfer(bufAddr);
    SPI.transfer(id_high); // Identifier high bits
    SPI.transfer(id_low); // Identifier low bits
    SPI.transfer(0x00); // Extended identifier registers (unused)
    SPI.transfer(0x00);
    SPI.transfer(length);
    for (i = 0; i < length; i++) { // Load data buffer
        SPI.transfer(data[i]);
    }
    digitalWrite(_ss, HIGH);
}
