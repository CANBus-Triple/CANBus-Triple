/***
***  Basic read / send firmware sketch
***  https://github.com/etx/CANBus-Triple
***/


#include <SPI.h>
#include <CANBus.h>
#include <Message.h>
#include <QueueArray.h>
#include <EEPROM.h>

#include "Settings.h"
#include "WheelButton.h"
#include "ChannelSwap.h"
#include "SerialCommand.h"
#include "MazdaLED.h"
#include "ServiceCall.h"


// CANBus Triple Rev D
#define BOOT_LED 6

#define CAN1INT 0
#define CAN1SELECT 9
#define CAN1RESET 4

#define CAN2INT 1
#define CAN2SELECT 10
#define CAN2RESET 12

// #define CAN3INT 6
#define CAN3SELECT 5
#define CAN3RESET 11
#define ISC60 0 // Set INT6 Low
#define ISC61 0

#define BTRESET 8



CANBus CANBus1(CAN1SELECT, CAN1RESET, 1, "Bus 1");
CANBus CANBus2(CAN2SELECT, CAN2RESET, 2, "Bus 2");
CANBus CANBus3(CAN3SELECT, CAN3RESET, 3, "Bus 3");

boolean CAN1Recieved = false;
boolean CAN2Recieved = false;
boolean CAN3Recieved = false;


byte rx_status;
QueueArray<Message> messageQueue;

CANBus busses[3] = { CANBus1, CANBus2, CANBus3 };
CANBus SerialCommand::busses[3] = { CANBus1, CANBus2, CANBus3 }; // Maybe do this better

byte wheelButton = 0;
boolean debug = false;



void setup(){
  
  /*
  *  BLE112 Setup
  */
  pinMode(8, OUTPUT);
  digitalWrite(8, LOW);
  
  // Reset BT Module
  pinMode( 21, OUTPUT );
  digitalWrite( 21, HIGH );
  delay(50);
  digitalWrite( 21, LOW );
  
  /*
  *  Boot LED
  */
  pinMode( BOOT_LED, OUTPUT );
  
  
  Settings::init();
  
  digitalWrite( BOOT_LED, HIGH );
  delay(50);
  digitalWrite( BOOT_LED, LOW );
  delay(50);
  digitalWrite( BOOT_LED, HIGH );
  delay(50);
  digitalWrite( BOOT_LED, LOW );
  delay(50);
  digitalWrite( BOOT_LED, HIGH );
  delay(50);
  digitalWrite( BOOT_LED, LOW );
  
  
  // Setup CAN Busses 
  CANBus1.begin();
  CANBus1.baudConfig(125);
  CANBus1.setRxInt(true);
  CANBus1.setMode(NORMAL);
  attachInterrupt(CAN1INT, handleInterrupt1, LOW);
  
  CANBus2.begin();
  CANBus2.baudConfig(125);
  CANBus2.setRxInt(true);
  CANBus2.setMode(NORMAL);
  attachInterrupt(CAN2INT, handleInterrupt2, LOW);
  
  CANBus3.begin();
  CANBus3.baudConfig(125);
  CANBus3.setRxInt(true);
  CANBus3.setMode(NORMAL); 
  // Manually configure INT6 for Bus 3
  EICRB |= (1<<ISC60)|(1<<ISC61); // sets the interrupt type
  EIMSK |= (1<<INT6); // activates the interrupt
  
  
  digitalWrite( BOOT_LED, HIGH );
  delay(100);
  digitalWrite( BOOT_LED, LOW );
  delay(100);
  
  // Middleware setup
  SerialCommand::init( &messageQueue, busses, 0 );
  // ServiceCall::init( &messageQueue );
  // MazdaLED::init( &messageQueue, cbt_settings.displayEnabled );
  
}

/*
*  Interrupt Handlers
*/
void handleInterrupt1(){
  readBus(CANBus1);
}
void handleInterrupt2(){
  readBus(CANBus2);
}
ISR(INT6_vect) {
  readBus(CANBus3);
}




void loop() {
  
  byte button = WheelButton::getButtonDown();
  
  if( wheelButton != button ){
    wheelButton = button;
    
     switch(wheelButton){
       case B10000000:
         // MazdaLED::showStatusMessage("    LEFT    ", 2000);
       break;
       case B01000000:
         // MazdaLED::showStatusMessage("  !DANGER!  ", 2000);
       break;
       case B00100000:
         // MazdaLED::showStatusMessage("     UP     ", 2000);
       break;
       case B00010000:
         // MazdaLED::showStatusMessage("    DOWN    ", 2000);
       break;
       case B10000001:
         // Decrement service pid
         ServiceCall::decServiceIndex();
         MazdaLED::showNewPageMessage();
       break;
       case B01000001:
         // Increment service pid 
         ServiceCall::incServiceIndex();
         MazdaLED::showNewPageMessage();
       break;
       case B01000010:
         toggleMazdaLed();
       break;
     }
 
 }
 
  // All Middleware ticks (Like loop() for middleware)
  // ServiceCall::tick();
  // MazdaLED::tick();
  SerialCommand::tick();
  
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Remove
  readBus(CANBus2);
  
  
  // Process message stack
  if( debug && !messageQueue.isEmpty() ){ 
    SerialCommand::activeSerial->print(F("{queueCount:")); 
    SerialCommand::activeSerial->print( messageQueue.count(), DEC ); 
    SerialCommand::activeSerial->println(F("}"));
  }
  
  boolean success = true;
  while( !messageQueue.isEmpty() && success ){
    
    Message msg = messageQueue.pop();
    CANBus channel = busses[msg.busId-1];
    
    //SerialCommand::printMessageToSerial(msg);
    success = sendMessage( msg, channel );
    
    if( !success ){
      // TX Failure, add back to queue
      messageQueue.push(msg);
      if(debug) SerialCommand::activeSerial->println("ALL TX BUFFERS FULL ON " + busses[msg.busId-1].name );
    }
    
  }
  
}


void toggleMazdaLed()
{
  cbt_settings.displayEnabled = MazdaLED::enabled = !MazdaLED::enabled;
  EEPROM.write( offsetof(struct cbt_settings, displayEnabled), cbt_settings.displayEnabled);
  if(MazdaLED::enabled)
    MazdaLED::showStatusMessage("MazdaLED ON ", 2000);
}


boolean sendMessage( Message msg, CANBus channel ){
  
  if( msg.dispatch == false ) return true;
  
  digitalWrite( BOOT_LED, HIGH );
  
  int ch = channel.getNextTxBuffer();
  
  switch( ch ){
    case 0:
      channel.load_ff_0( msg.length, msg.frame_id, msg.frame_data );
      channel.send_0();
      break;
    case 1:
      channel.load_ff_1( msg.length, msg.frame_id, msg.frame_data );
      channel.send_1();
      break;
    case 2:
      channel.load_ff_2( msg.length, msg.frame_id, msg.frame_data );
      channel.send_2();
      break;
    default:
      // All TX buffers full
      return false;
      break;
  }
  
  if(debug){
    SerialCommand::activeSerial->print(F("Sent a message on TXB"));
    SerialCommand::activeSerial->print( ch, DEC );
    SerialCommand::activeSerial->print(F(" via bus "));
    SerialCommand::activeSerial->println( channel.name );
  }
  
  digitalWrite( BOOT_LED, LOW );
  
  return true;
  
}





void readBus( CANBus channel ){
  
  // byte bus_status;
  // bus_status = channel.readRegister(RX_STATUS);
  
  rx_status = channel.readStatus();
  
  // Check buffer 0
  if( (rx_status & 0x1) == 0x1 ){
    Message msg;
    msg.busStatus = rx_status;
    msg.busId = channel.busId;
    channel.readDATA_ff_0( &msg.length, msg.frame_data, &msg.frame_id );
    processMessage( msg );
  }
  
  
  // Check buffer 1
  if( (rx_status & 0x2) == 0x2 ) {
    Message msg;
    msg.busStatus = rx_status;
    msg.busId = channel.busId;
    channel.readDATA_ff_1( &msg.length, msg.frame_data, &msg.frame_id );
    processMessage( msg );
  }
    
}


void processMessage( Message msg ){
  
  // All Middleware process calls (Augment incoming CAN packets)
  msg = SerialCommand::process( msg );
  // msg = ServiceCall::process( msg );
  // msg = MazdaLED::process( msg );
  // msg = ChannelSwap::process( msg );
  
  if( msg.dispatch == true ){
    messageQueue.push( msg );
  }
  
}




