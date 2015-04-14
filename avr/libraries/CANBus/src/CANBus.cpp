
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

CANBus::CANBus( int ss, int reset ){
    CANBus( ss, reset, 0, "Default" );
}

void CANBus::setName( String s ){
    name = s;
}

void CANBus::setBusId( unsigned int n ){
    busId = n;
}

void CANBus::begin()//constructor for initializing can module.
{
	// set the slaveSelectPin as an output
	pinMode (SCK,OUTPUT);
	pinMode (MISO,INPUT);
	pinMode (MOSI, OUTPUT);
	pinMode (_ss, OUTPUT);
	pinMode (_reset,OUTPUT);

	// initialize SPI:
	SPI.begin();
	SPI.setDataMode(SPI_MODE0);
	SPI.setClockDivider(SPI_CLOCK_DIV2);
	SPI.setBitOrder(MSBFIRST);

	digitalWrite(_reset,LOW); /* RESET CAN CONTROLLER*/
	delay(50);
	digitalWrite(_reset,HIGH);
	delay(50);
}

void CANBus::reset()//constructor for initializing can module.
{
    digitalWrite(_ss, LOW);
    delay(1);
    SPI.transfer(RESET_REG);
    delay(1);
    digitalWrite(_ss, HIGH);
}

#ifdef OLD_BAUD
bool CANBus::baudConfig(int bitRate)//sets bitrate for CAN node
{
	byte config0, config1, config2;

	switch (bitRate)
	{
case 10:
		config0 = 0x31;
		config1 = 0xB8;
		config2 = 0x05;
		break;

case 20:
		config0 = 0x18;
		config1 = 0xB8;
		config2 = 0x05;
		break;

case 50:
		config0 = 0x09;
		config1 = 0xB8;
		config2 = 0x05;
		break;

case 83:
        // TODO
        config0 = 0x00;
        config1 = 0x00;
        config2 = 0x00;
        break;

case 100:
		config0 = 0x04;
		config1 = 0xB8;
		config2 = 0x05;
		break;

case 125:
		config0 = 0x03;
		config1 = 0xB8;
		config2 = 0x05;
		break;

case 250:
		config0 = 0x01;
		config1 = 0xB8;
		config2 = 0x05;
		break;

case 500:
		config0 = 0x00;
		config1 = 0xB8;
		config2 = 0x05;
		break;
case 1000:
	//1 megabit mode added by Patrick Cruce(pcruce_at_igpp.ucla.edu)
	//Faster communications enabled by shortening bit timing phases(3 Tq. PS1 & 3 Tq. PS2) Note that this may exacerbate errors due to synchronization or arbitration.
	config0 = 0x80;
	config1 = 0x90;
	config2 = 0x02;
	}


	digitalWrite(_ss, LOW);
	delay(1);
	SPI.transfer(WRITE);
	SPI.transfer(CNF1);
	SPI.transfer(config0);
	delay(1);
	digitalWrite(_ss, HIGH);
	delay(1);

	digitalWrite(_ss, LOW);
	delay(1);
	SPI.transfer(WRITE);
	SPI.transfer(CNF2);
	SPI.transfer(config1);
	delay(1);
	digitalWrite(_ss, HIGH);
	delay(1);

	digitalWrite(_ss, LOW);
	delay(1);
	SPI.transfer(WRITE);
	SPI.transfer(CNF3);
	SPI.transfer(config2);
	delay(1);
	digitalWrite(_ss, HIGH);
	delay(1);

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

    digitalWrite(_ss, LOW);
    delay(1);
    SPI.transfer(WRITE);
    SPI.transfer(CNF1);
    SPI.transfer(config1);
    delay(1);
    digitalWrite(_ss, HIGH);
    delay(1);

    digitalWrite(_ss, LOW);
    delay(1);
    SPI.transfer(WRITE);
    SPI.transfer(CNF2);
    SPI.transfer(config2);
    delay(1);
    digitalWrite(_ss, HIGH);
    delay(1);

    digitalWrite(_ss, LOW);
    delay(1);
    SPI.transfer(WRITE);
    SPI.transfer(CNF3);
    SPI.transfer(config3);
    delay(1);
    digitalWrite(_ss, HIGH);
    delay(1);

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


void CANBus::bitModify( byte reg, byte value, byte mask  ){
    digitalWrite(_ss, LOW);
    SPI.transfer(BIT_MODIFY);
    SPI.transfer(reg);
    SPI.transfer(mask);
    SPI.transfer(value);
    digitalWrite(_ss, HIGH);
}


int CANBus::getNextTxBuffer(){

    byte stat;
    stat = this->readStatus();

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
void CANBus::setRxInt(bool b){

    byte mask,writeVal;

    if (b) {
        writeVal = 0x03;
    }else{
        writeVal = 0x00;
    }

    mask = 0x03;

	digitalWrite(_ss, LOW);
	SPI.transfer(BIT_MODIFY);
	SPI.transfer(CANINTE);
	SPI.transfer(mask);
	SPI.transfer(writeVal);
	digitalWrite(_ss, HIGH);

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
void CANBus::setClkPre(int mode){

	byte writeVal,mask;

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

	mask = 0x03;

	digitalWrite(_ss, LOW);
	SPI.transfer(BIT_MODIFY);
	SPI.transfer(CANCTRL);
	SPI.transfer(mask);
	SPI.transfer(writeVal);
	digitalWrite(_ss, HIGH);

}


//Method added to enable testing in loopback mode.(pcruce_at_igpp.ucla.edu)
void CANBus::setMode(CANMode mode) { //put CAN controller in one of five modes

	byte writeVal,mask,readVal;

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

	mask = 0xE0;

	digitalWrite(_ss, LOW);
	SPI.transfer(BIT_MODIFY);
	SPI.transfer(CANCTRL);
	SPI.transfer(mask);
	SPI.transfer(writeVal);
	digitalWrite(_ss, HIGH);

}


void CANBus::send_0()//transmits buffer 0
{

	//delays removed from SEND command(pcruce_at_igpp.ucla.edu)
	//In testing we found that any lost data was from PC<->Serial Delays,
	//Not CAN Controller/AVR delays.  Thus removing the delays at this level
	//allows maximum flexibility and performance.
	digitalWrite(_ss, LOW);
	SPI.transfer(SEND_TX_BUF_0);
	digitalWrite(_ss, HIGH);
}


void CANBus::send_1()//transmits buffer 1
{
	digitalWrite(_ss, LOW);
	SPI.transfer(SEND_TX_BUF_1);
	digitalWrite(_ss, HIGH);
}


void CANBus::send_2()//transmits buffer 2
{
	digitalWrite(_ss, LOW);
	SPI.transfer(SEND_TX_BUF_2);
	digitalWrite(_ss, HIGH);
}


char CANBus::readID_0()//reads ID in recieve buffer 0
{
	char retVal;
	digitalWrite(_ss, LOW);
	delay(1);
	SPI.transfer(READ_RX_BUF_0_ID);
	retVal = SPI.transfer(0xFF);
	delay(1);
	digitalWrite(_ss, HIGH);
	delay(1);
	return retVal;
}


char CANBus::readID_1()//reads ID in reciever buffer 1
{
	char retVal;
	digitalWrite(_ss, LOW);
	delay(1);
	SPI.transfer(READ_RX_BUF_1_ID);
	retVal = SPI.transfer(0xFF);
	delay(1);
	digitalWrite(_ss, HIGH);
	delay(1);
	return retVal;
}


char CANBus::readDATA_0()//reads DATA in recieve buffer 0
{
	char retVal;
	digitalWrite(_ss, LOW);
	delay(1);
	SPI.transfer( READ_RX_BUF_0_DATA);
	retVal = SPI.transfer(0xFF);
	delay(1);
	digitalWrite(_ss, HIGH);
	delay(1);
	return retVal;
}

char CANBus::readDATA_1()//reads data in recieve buffer 1
{
	char retVal;
	digitalWrite(_ss, LOW);
	delay(1);
	SPI.transfer( READ_RX_BUF_1_DATA);
	retVal = SPI.transfer(0xFF);
	delay(1);
	digitalWrite(_ss, HIGH);
	delay(1);
	return retVal;
}


//extending CAN data read to full frames(pcruce_at_igpp.ucla.edu)
//It is the responsibility of the user to allocate memory for output.
//If you don't know what length the bus frames will be, data_out should be 8-bytes
void CANBus::readDATA_ff_0(byte* length_out,byte *data_out,unsigned short *id_out){

	byte len,i;
	unsigned short id_h,id_l;

	digitalWrite(_ss, LOW);
	SPI.transfer(READ_RX_BUF_0_ID);
	id_h = (unsigned short) SPI.transfer(0xFF); //id high
	id_l = (unsigned short) SPI.transfer(0xFF); //id low
	SPI.transfer(0xFF); //extended id high(unused)
	SPI.transfer(0xFF); //extended id low(unused)
	len = (SPI.transfer(0xFF) & 0x0F); //data length code
	for (i = 0;i<len;i++) {
		data_out[i] = SPI.transfer(0xFF);
	}
	digitalWrite(_ss, HIGH);
	(*length_out) = len;
	(*id_out) = ((id_h << 3) + ((id_l & 0xE0) >> 5)); //repack identifier

}


void CANBus::readDATA_ff_1(byte* length_out,byte *data_out,unsigned short *id_out){

	byte id_h,id_l,len,i;

	digitalWrite(_ss, LOW);
	SPI.transfer(READ_RX_BUF_1_ID);
	id_h = SPI.transfer(0xFF); //id high
	id_l = SPI.transfer(0xFF); //id low
	SPI.transfer(0xFF); //extended id high(unused)
	SPI.transfer(0xFF); //extended id low(unused)
	len = (SPI.transfer(0xFF) & 0x0F); //data length code
	for (i = 0;i<len;i++) {
		data_out[i] = SPI.transfer(0xFF);
	}
	digitalWrite(_ss, HIGH);

	(*length_out) = len;
	(*id_out) = ((((unsigned short) id_h) << 3) + ((id_l & 0xE0) >> 5)); //repack identifier
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


void CANBus::load_0(byte identifier, byte data)//loads ID and DATA into transmit buffer 0
{
	digitalWrite(_ss, LOW);
	delay(1);
	SPI.transfer(LOAD_TX_BUF_0_ID);
	SPI.transfer(identifier);
	delay(1);
	digitalWrite(_ss, HIGH);
	delay(1);

	digitalWrite(_ss, LOW);
	delay(1);
	SPI.transfer(LOAD_TX_BUF_0_DATA);
	SPI.transfer(data);
	delay(1);
	digitalWrite(_ss, HIGH);
	delay(1);
}


void CANBus::load_1(byte identifier, byte data)//loads ID and DATA into transmit buffer 1
{
	digitalWrite(_ss, LOW);
	delay(1);
	SPI.transfer(LOAD_TX_BUF_1_ID);
	SPI.transfer(identifier);
	delay(1);
	digitalWrite(_ss, HIGH);
	delay(1);

	digitalWrite(_ss, LOW);
	delay(1);
	SPI.transfer(LOAD_TX_BUF_1_DATA);
	SPI.transfer(data);
	delay(1);
	digitalWrite(_ss, HIGH);
	delay(1);
}


void CANBus::load_2(byte identifier, byte data)//loads ID and DATA into transmit buffer 2
{
	digitalWrite(_ss, LOW);
	delay(1);
	SPI.transfer(LOAD_TX_BUF_2_ID);
	SPI.transfer(identifier);
	delay(1);
	digitalWrite(_ss, HIGH);
	delay(1);

	digitalWrite(_ss, LOW);
	delay(1);
	SPI.transfer(LOAD_TX_BUF_2_DATA);
	SPI.transfer(data);
	delay(1);
	digitalWrite(_ss, HIGH);
	delay(1);
}


void CANBus::load_ff_0(byte length,unsigned short identifier,byte *data)
{

	byte i,id_high,id_low;

	//generate id bytes before SPI write
	id_high = (byte) (identifier >> 3);
	id_low = (byte) ((identifier << 5) & 0x00E0);

	digitalWrite(_ss, LOW);
	SPI.transfer(LOAD_TX_BUF_0_ID);
	SPI.transfer(id_high); //identifier high bits
	SPI.transfer(id_low); //identifier low bits
	SPI.transfer(0x00); //extended identifier registers(unused)
	SPI.transfer(0x00);
	SPI.transfer(length);
	for (i=0;i<length;i++) { //load data buffer
		SPI.transfer(data[i]);
	}

	digitalWrite(_ss, HIGH);

}


void CANBus::load_ff_1(byte length,unsigned short identifier,byte *data)
{

	byte i,id_high,id_low;

	//generate id bytes before SPI write
	id_high = (byte) (identifier >> 3);
	id_low = (byte) ((identifier << 5) & 0x00E0);

	digitalWrite(_ss, LOW);
	SPI.transfer(LOAD_TX_BUF_1_ID);
	SPI.transfer(id_high); //identifier high bits
	SPI.transfer(id_low); //identifier low bits
	SPI.transfer(0x00); //extended identifier registers(unused)
	SPI.transfer(0x00);
	SPI.transfer(length);
	for (i=0;i<length;i++) { //load data buffer
		SPI.transfer(data[i]);
	}

	digitalWrite(_ss, HIGH);

}


void CANBus::load_ff_2(byte length,unsigned short identifier,byte *data)
{

	byte i,id_high,id_low;

	//generate id bytes before SPI write
	id_high = (byte) (identifier >> 3);
	id_low = (byte) ((identifier << 5) & 0x00E0);

	digitalWrite(_ss, LOW);

	SPI.transfer(LOAD_TX_BUF_2_ID);
	SPI.transfer(id_high); //identifier high bits
	SPI.transfer(id_low); //identifier low bits
	SPI.transfer(0x00); //extended identifier registers(unused)
	SPI.transfer(0x00);
	SPI.transfer(length); //data length code
	for (i=0;i<length;i++) { //load data buffer
		SPI.transfer(data[i]);
	}

	digitalWrite(_ss, HIGH);

}
