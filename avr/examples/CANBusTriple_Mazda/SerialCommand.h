
/*
// Serial Commands

System info and EEPROM
----------------------
0x01 0x01            Print system debug to serial
0x01 0x02            Dump EEPROM value
0x01 0x03            Read and save EEPROM
0x01 0x04            Restore EEPROM to stock values
0x01 0x08 0x01       Auto detect baud rate for bus 1 (handled by AutoBaud.h)
0x01 0x08 0x02       Auto baud bus 2
0x01 0x08 0x03       Auto baud bus 3
0x01 0x09 0x01 N     Set baud rate on bus 1 to N (N is 16 bits)
0x01 0x0A BUS  MODE  Get CAN mode on bus BUS, or Set CAN mode MODE on bus BUS
                     (CONFIGURATION = 0, NORMAL = 1, SLEEP = 2, LISTEN = 3, LOOPBACK = 4)
0x01 0x10 0x01       Print bus 1 debug to serial
0x01 0x10 0x02       Print bus 2 debug to serial
0x01 0x10 0x03       Print bus 3 debug to serial
0x01 0x16            Reboot to bootloader


Send CAN Packet
---------------
Cmd    Bus id  PID  Data 0-7                 Length
0x02   01      290  00 00 00 00 00 00 00 00  8


Enable/disable logging over serial (filters are optional)
--------------------------------------------------
Cmd  Bus  On/Off MsgId1 MsgId2
0x03 0x01 0x01                    // Enable logging on bus 1 (do not touch current filter)
0x03 0x01 0x01   0x290  0x291     // Enable logging on bus 1 and filter only messages 0x290 and 0x291
0x03 0x01 0x00                    // Disable logging on bus 1

                 MsgId1 Mask1 MsgId2 Mask2
0x03 0x01 0x02   0x290  0xFFF 0x400  0xFF0  // Enable logging on Bus 1 filter messages 0x290 and 0x40* (0 in mask is a wildcard)
0x03 0x01 0x02   0x000  0x000               // Enable logging on Bus 1 for ALL messages


Set Bluetooth Message ID filter
----------------------------------------
Cmd  Bus  Message Id 1 Message Id 2
0x04 0x01 0x290        0x291            // Enable Message ID 290 output over BT
0x04 0x01 0x0000       0x0000           // Disable


Bluetooth Functions
-------------------
Cmd  Function
0x08 0x01 Reset BLE112
0x08 0x02 Enter pass through mode to talk to BLE112
0x08 0x03 Exit pass through mode
TODO: Implement this ^^^
*/


// #define JSON_OUT

#ifndef SerialCommand_H
#define SerialCommand_H


#define COMMAND_OK 0xFF
#define COMMAND_ERROR 0x80
#define NEWLINE "\r\n"
#define MAX_MW_CALLBACKS 8
#define BT_SEND_DELAY 20
#define COMMAND_TIMEOUT 100   // ms to wait before serial command timeout

#include <CANBus.h>

#include "Middleware.h"


struct middleware_command {
    byte command;
    // void (Middleware::*cb)(byte[], int);
    Middleware *cbInstance;
};


class SerialCommand : public Middleware
{
public:
    SerialCommand( QueueArray<Message> *q );
    void tick();
    Message process( Message msg );
    void commandHandler(byte* bytes, int length);
    Stream* activeSerial;
    void printMessageToSerial(Message msg);
    void registerCommand(byte commandId, Middleware *cbInstance);
    void resetToBootloader();

private:
    int freeRam();
    QueueArray<Message>* mainQueue;
    void printChannelDebug();
    void printChannelDebug(CANBus);
    void processCommand(byte command);
    int  getCommandBody( byte* cmd, int length );
    void clearBuffer();
    void getAndSend();
    void printSystemDebug();
    void settingsCall();
    void dumpEeprom();
    void getAndSaveEeprom();
    void bitRate();
    void canMode();
    void logCommand();
    void bluetooth();
    void setBluetoothFilter();
    char btMessageIdFilters[][2];
    boolean passthroughMode;
    byte busLogEnabled;
    void printEFLG(CANBus);
    int byteCount;
    void btDelay();
    bool btRateLimit();
    unsigned long lastBluetoothRX;
};


byte mwCommandIndex = 0;
int byteCount = 0;
struct middleware_command mw_cmds[MAX_MW_CALLBACKS];


SerialCommand::SerialCommand( QueueArray<Message> *q )
{
    mainQueue = q;

//  // TODO Finish this
//  btMessageIdFilters[][2] = {
//      {0x28F,0x290},
//      {0x0,0x0},
//      {0x28F,0x290},
//  };
//

    // Default Instance Properties
    busLogEnabled = 0;        // Start with all busses logging disabled
    passthroughMode = false;
    activeSerial = &Serial;
    lastBluetoothRX = 0;
}


void SerialCommand::tick()
{
    // Serial.println(passthroughMode, BIN);

    // Pass-through mode for bluetooth DFU mode
    if ( passthroughMode ){
        while(Serial.available()) Serial1.write(Serial.read());
        while(Serial1.available()) Serial.write(Serial1.read());
        return;
    }

    if( Serial1.available() > 0 ){
        activeSerial = &Serial1;
        processCommand( Serial1.read() );
    }

    if( Serial.available() > 0 ){
        activeSerial = &Serial;
        processCommand( Serial.read() );
    }
}


Message SerialCommand::process( Message msg )
{
    if (busLogEnabled & (0x1 << (msg.busId - 1))) printMessageToSerial(msg);
    return msg;
}


void SerialCommand::commandHandler(byte* bytes, int length){}


void SerialCommand::printMessageToSerial( Message msg )
{
  // Bus Filter
  byte flag = 0x1 << (msg.busId - 1);
  if( !(busLogEnabled & flag) ){
    return;
  }

    // Bluetooth rate limiting
    if ( activeSerial == &Serial1 && btRateLimit() ) return;

#ifdef JSON_OUT

    // Output to serial as json string
    activeSerial->print(F("{\"packet\": {\"status\":\""));
    activeSerial->print( msg.busStatus, HEX);
    activeSerial->print(F("\",\"channel\":\""));
    activeSerial->print( busses[msg.busId-1].name );
    activeSerial->print(F("\",\"length\":\""));
    activeSerial->print(msg.length, HEX);
    activeSerial->print(F("\",\"id\":\""));
    activeSerial->print(msg.frame_id, HEX);
    activeSerial->print(F("\",\"timestamp\":\""));
    activeSerial->print(millis(), DEC);
    activeSerial->print(F("\",\"payload\":[\""));
    for (int i = 0; i < 8; i++) {
        activeSerial->print(msg.frame_data[i], HEX);
        if(i < 7) activeSerial->print(F("\",\""));
    }
    activeSerial->print(F("\"]}}"));
    activeSerial->println();

#else

    activeSerial->write( 0x03 ); // Prefix with logging command
    activeSerial->write( msg.busId );
    activeSerial->write( msg.frame_id >> 8 );
    activeSerial->write( msg.frame_id );

    for (int i = 0; i < 8; i++) activeSerial->write(msg.frame_data[i]);

    activeSerial->write( msg.length );
    activeSerial->write( msg.busStatus );
    activeSerial->write( NEWLINE );

#endif
}


void SerialCommand::processCommand(byte command)
{
//  Commented out because causes corrupted data when sending serial to Android Bluetooth
//  The necessary delay is now moved into method getCommandBody() 
//  delay(32); // Delay to wait for the entire command from Serial

  switch( command ){
    case 0x01:
      settingsCall();
    break;
    case 0x02:
      getAndSend();
    break;
    case 0x03:
      logCommand();
    break;
    case 0x04:
      setBluetoothFilter();
    break;
    case 0x08:
      bluetooth();
    break;
    default:
      // Check for Middleware commands
      for(int i=0; i<mwCommandIndex; i++ ){
        if( mw_cmds[i].command == command ){

          byte cmd[64];
          int bytesRead = getCommandBody( cmd, 64 );
          delay(1);
          // (*mw_cmds[i].cb)( cmd, bytesRead );
          mw_cmds[i].cbInstance->commandHandler(cmd, bytesRead);
          break;
        }
      }
    break;
  }

  clearBuffer();
}


void SerialCommand::settingsCall()
{
    byte cmd[1];
    if (getCommandBody( cmd, 1 ) != 1) return;

    // Debug Command
    switch( cmd[0] ) {
        case 0x01:
            printSystemDebug();
            break;
        case 0x02:
            dumpEeprom();
            break;
        case 0x03:
            getAndSaveEeprom();
            break;
        case 0x04:
            Settings::firstbootSetup();
            break;
        case 0x08:
            getCommandBody( cmd, 1 );
            AutoBaud::baudDetect(cmd[0], activeSerial);
            break;
        case 0x09:
            bitRate();
            break;
        case 0x0A:
            canMode();
            break;    
        case 0x10:
            printChannelDebug();
            break;
        case 0x16:
            resetToBootloader();
            break;
    }
}


void SerialCommand::setBluetoothFilter()
{
    byte cmd[5];
    getCommandBody( cmd, 5 );

    if( cmd[0] >= 0 && cmd[0] <= 3 ){
        btMessageIdFilters[cmd[0]][0] = (cmd[1] << 8)+cmd[2];
        btMessageIdFilters[cmd[0]][1] = (cmd[3] << 8)+cmd[4];
    }
}


void SerialCommand::bitRate()
{  
    byte cmd[3], bytesRead;

    bytesRead = getCommandBody( cmd, 3 );

    if (bytesRead == 3) Settings::setBaudRate( cmd[0], (cmd[1] << 8) + cmd[2] );
    
    activeSerial->print( F( "{\"event\":\"bitrate-bus" ) );
    activeSerial->print(cmd[0]);
    activeSerial->print( F( "\", \"rate\":" ) );
    activeSerial->print( Settings::getBaudRate( cmd[0] ), DEC );
    activeSerial->println( F( "}" ) ); 
}


void SerialCommand::canMode()
{  
    byte cmd[2], bytesRead;

    bytesRead = getCommandBody( cmd, 2 );

    if (bytesRead == 2) Settings::setCanMode( cmd[0], cmd[1] );
    
    CANMode mode = Settings::getCanMode( cmd[0] );

    activeSerial->print( F( "{\"event\":\"can-mode-bus" ) );
    activeSerial->print(cmd[0]);
    activeSerial->print( F( "\", \"mode\":\"" ) );
    switch(mode) {
        case CONFIGURATION:
            activeSerial->print( F("CONFIGURATION") );
            break;
        case NORMAL:
            activeSerial->print( F("NORMAL") );
            break;
        case SLEEP:
            activeSerial->print( F("SLEEP") );
            break;
        case LISTEN:
            activeSerial->print( F("LISTEN") );
            break;
        case LOOPBACK:
            activeSerial->print( F("LOOPBACK") );
            break;
        default:
            activeSerial->print( F("UNKNOWN") );
            break;                
    }
    activeSerial->println( F( "\"}" ) ); 
}


void SerialCommand::logCommand()
{
    byte cmd[8] = {0};
    if (getCommandBody( cmd, 2 ) < 2) {
        activeSerial->write(COMMAND_ERROR);
        return;
    }
    int busId = cmd[0];

    if( busId < 1 || busId > 3 ){
        activeSerial->write(COMMAND_ERROR);
        return;
    }
    CANBus bus = busses[busId - 1];

    if( cmd[1] > 0 )
        busLogEnabled |= 1 << (busId-1);
    else
        busLogEnabled &= ~(1 << (busId-1));

    // Set optional filter    
    bus.setMode(CONFIGURATION);
    switch(cmd[1]) {
        case 1:
            if (getCommandBody(cmd, 4) > 0)
                bus.setFilter((cmd[0] << 8) + cmd[1], (cmd[2] << 8) + cmd[3]);
            break;
        case 2:
        {
            int bytesRead = getCommandBody( cmd, 8 );
            int filter1 = (cmd[0] << 8) + cmd[1];
            int mask1 = (cmd[2] << 8) + cmd[3];
            int filter2 = (bytesRead > 4)? (cmd[4] << 8) + cmd[5] : filter1;
            int mask2 = (bytesRead > 6)? (cmd[6] << 8) + cmd[7] : mask1;
            bus.setFilterMask(filter1, mask1, filter2, mask2);
            break;
        }
    }
    bus.setMode(Settings::getCanMode(busId));

    activeSerial->write(COMMAND_OK);
    activeSerial->write(NEWLINE);
}


void SerialCommand::getAndSaveEeprom()
{
    #define CHUNK_SIZE 32

    byte* settings = (byte *) &cbt_settings;
    byte cmd[CHUNK_SIZE + 2];
    int bytesRead = getCommandBody( cmd, CHUNK_SIZE + 2 );

    if ( bytesRead == CHUNK_SIZE + 2 && cmd[CHUNK_SIZE+1] == 0xA1 ) {
        memcpy( settings+(cmd[0]*CHUNK_SIZE), &cmd[1], CHUNK_SIZE );

        activeSerial->print( F("{\"event\":\"eepromData\", \"result\":\"success\", \"chunk\":\"") );
        activeSerial->print(cmd[0]);
        activeSerial->println(F("\"}"));

        if( cmd[0]+1 == 512/CHUNK_SIZE ){ // At last chunk
            Settings::save(&cbt_settings);
            activeSerial->println(F("{\"event\":\"eepromSave\", \"result\":\"success\"}"));
        }
    } 
    else {
        activeSerial->print( F("{\"event\":\"eepromData\", \"result\":\"failure\", \"chunk\":\"") );
        activeSerial->print(cmd[0]);
        activeSerial->println(F("\"}"));
    }
}


void SerialCommand::getAndSend()
{
    byte cmd[12];
    int bytesRead = getCommandBody( cmd, 12 );

    Message msg;
    msg.busId = cmd[0];
    msg.frame_id = (cmd[1]<<8) + cmd[2];
    msg.frame_data[0] = cmd[3];
    msg.frame_data[1] = cmd[4];
    msg.frame_data[2] = cmd[5];
    msg.frame_data[3] = cmd[6];
    msg.frame_data[4] = cmd[7];
    msg.frame_data[5] = cmd[8];
    msg.frame_data[6] = cmd[9];
    msg.frame_data[7] = cmd[10];
    msg.length = cmd[11];
    msg.dispatch = true;

    mainQueue->push( msg );
}


void SerialCommand::bluetooth()
{
    byte cmd[1];
    int bytesRead = getCommandBody( cmd, 1 );

    switch( cmd[0] ){
        case 1:
            // digitalWrite(BT_RESET, HIGH);
            // delay(100);
            // digitalWrite(BT_RESET, LOW);
            break;
        case 2:
            passthroughMode = true;
            digitalWrite(BOOT_LED, HIGH);
            delay(100);
            digitalWrite(BOOT_LED, LOW);
            delay(100);
            digitalWrite(BOOT_LED, HIGH);
            break;
        case 3:
            passthroughMode = false;
            break;
    }
}


int SerialCommand::getCommandBody( byte* cmd, int length )
{
    // Loop until requested amount of bytes are received. Needed for BT latency
    int i = 0;
    int timeout = COMMAND_TIMEOUT;
    while( i < length ) {
        // Cannot simply use delay() because Android Bluetooth gets corrupted data
        while(activeSerial->available() == 0 && timeout > 0) {
            delay(1);
            timeout--;
        }
        if (timeout < 1) return i;
        cmd[i] = activeSerial->read();
        i++;
    }
    return i;
}


void SerialCommand::clearBuffer()
{
    while(activeSerial->available()) activeSerial->read();
}


void SerialCommand::printSystemDebug()
{
    activeSerial->print("{\"event\":\"version\", \"name\":\""+String(BUILDNAME)+"\", "+
#ifdef BUILD_VERSION
     "\"version\":\""+String(BUILD_VERSION)+"\", "+
#endif
     "\"memory\":\"");
    activeSerial->print(freeRam());
    activeSerial->println(F("\"}"));
}


void SerialCommand::printChannelDebug()
{
    byte cmd[1];
    getCommandBody( cmd, 1 );
    if( cmd[0] > 0 && cmd[0] <= 3 ) printChannelDebug( busses[cmd[0]-1] );
}


void SerialCommand::printEFLG(CANBus channel) 
{
    if (channel.readRegister(EFLG) & 0b00000001)        // EWARN
        activeSerial->print( F("Receive Error Warning - TEC or REC >= 96, ") );
    if (channel.readRegister(EFLG) & 0b00000010)        // RXWAR
        activeSerial->print( F("Receive Error Warning - REC >= 96, ") );
    if (channel.readRegister(EFLG) & 0b00000100)        // TXWAR
        activeSerial->print( F("Transmit Error Warning - TEX >= 96, ") );
    if (channel.readRegister(EFLG) & 0b00001000)        // RXEP
        activeSerial->print( F("Receive Error Warning - REC >= 128, ") );
    if (channel.readRegister(EFLG) & 0b00010000)        // TXEP
        activeSerial->print( F("Transmit Error Warning - TEC >= 128, ") );
    if (channel.readRegister(EFLG) & 0b00100000)        // TXBO
        activeSerial->print( F("Bus Off - TEC exceeded 255, ") );
    if (channel.readRegister(EFLG) & 0b01000000)        // RX0OVR
        activeSerial->print( F("Receive Buffer 0 Overflow, ") );
    if (channel.readRegister(EFLG) & 0b10000000)        // RX1OVR
        activeSerial->print( F("Receive Buffer 1 Overflow, ") );
    if (channel.readRegister(EFLG) == 0)                // No errors
        activeSerial->print( F("No Errors") ); 
}


void SerialCommand::printChannelDebug(CANBus channel)
{
    activeSerial->print( F("{\"event\":\"busdbg\", \"name\":\"") );
    activeSerial->print( channel.name );
    activeSerial->print( F("\", \"canctrl\":\""));
    activeSerial->print( channel.readRegister(CANCTRL), HEX );
    activeSerial->print( F("\", \"status\":\""));
    activeSerial->print( channel.readStatus(), HEX );
    activeSerial->print( F("\", \"error\":\""));
    activeSerial->print( channel.readRegister(EFLG), HEX );
    /*if ( activeSerial == &Serial ) {
        activeSerial->print( F("\", \"errorText\":\""));
        printEFLG(channel);
    }*/
    activeSerial->print( F("\", \"nextTxBuffer\":\""));
    activeSerial->print( channel.getNextTxBuffer(), DEC );
    activeSerial->println(F("\"}"));
}


void SerialCommand::registerCommand(byte commandId, Middleware *cbInstance)
{
    // About if we've reached the max number of registered callbacks
    if( mwCommandIndex >= MAX_MW_CALLBACKS ) return;

    mw_cmds[mwCommandIndex].command = commandId;
    mw_cmds[mwCommandIndex].cbInstance = cbInstance;
    mwCommandIndex++;
}


void SerialCommand::resetToBootloader()
{
    cli();
    UDCON = 1;
    USBCON = (1<<FRZCLK);  // disable USB
    UCSR1B = 0;
    delay(5);
    EIMSK = 0; PCICR = 0; SPCR = 0; ACSR = 0; EECR = 0; ADCSRA = 0;
    TIMSK0 = 0; TIMSK1 = 0; TIMSK3 = 0;
    asm volatile("jmp 0x7000");
}


void SerialCommand::dumpEeprom()
{
    activeSerial->print( F("{\"event\":\"eeprom\", \"data\":\"") );
    
    // Dump EEPROM
    for(int i = 0; i < 512; i++){
        uint8_t v = EEPROM.read(i);
        if (v < 0x10) activeSerial->print( "0" );

        activeSerial->print( v, HEX );
        
        // Bluetooth buffer delay
        if( activeSerial == &Serial1 ) btDelay();
    }

    activeSerial->println(F("\"}"));
}


void SerialCommand::btDelay()
{  
    byteCount++;  
    if( byteCount >= 8 ) {
        delay(BT_SEND_DELAY);
        byteCount = 0;
    }
}


bool SerialCommand::btRateLimit()
{
    if ( millis() > lastBluetoothRX + 50 ) {
        lastBluetoothRX = millis();
        return false;
    } else
        return true;
}


int SerialCommand::freeRam ()
{
    extern int __heap_start, *__brkval;
    int v;
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}


#endif

