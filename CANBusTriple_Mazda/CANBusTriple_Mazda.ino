/***
***  Basic read / send firmware sketch
***  https://github.com/etx/CANBus-Triple
***/


#include <SPI.h>
#include <CANBus.h>
#include <Message.h>
#include <QueueArray.h>
#include <EEPROM.h>

// #define DEBUG_BUILD
#define USE_MIDDLEWARE


// CANBus Triple Rev E
#define BOOT_LED 6

#define CAN1INT 0
#define CAN1INT_D 3
#define CAN1SELECT 9
#define CAN1RESET 4

#define CAN2INT 1
#define CAN2INT_D 2
#define CAN2SELECT 10
#define CAN2RESET 12

// #define CAN3INT 6
#define CAN3INT_D 7
#define CAN3SELECT 5
#define CAN3RESET 11
#define ISC60 0 // Set INT6 Low
#define ISC61 0

#define BT_RESET 8


#include "Settings.h"
#include "WheelButton.h"
#include "ChannelSwap.h"
#include "SerialCommand.h"
#include "MazdaLED.h"
#include "ServiceCall.h"




CANBus CANBus1(CAN1SELECT, CAN1RESET, 1, "Bus 1");
CANBus CANBus2(CAN2SELECT, CAN2RESET, 2, "Bus 2");
CANBus CANBus3(CAN3SELECT, CAN3RESET, 3, "Bus 3");


byte rx_status;
QueueArray<Message> readQueue;
QueueArray<Message> writeQueue;

CANBus busses[] = { CANBus1, CANBus2, CANBus3 };
CANBus SerialCommand::busses[] = { CANBus1, CANBus2, CANBus3 }; // TODO do this better, pass into via init method

byte wheelButton = 0;


// TODO Create a list of middleware for init and processing later
// SerialCommand* middleware = &SerialCommand;


void setup(){
  
  /*
  *  BLE112 Setup
  */
  pinMode(BT_RESET, OUTPUT);
  digitalWrite(BT_RESET, LOW);
  
  /*
  *  Boot LED
  */
  pinMode( BOOT_LED, OUTPUT );
  
  
  Settings::init();
  
  for (int b = 0; b<2; b++) {
    digitalWrite( BOOT_LED, HIGH );
    delay(50);
    digitalWrite( BOOT_LED, LOW );
    delay(50);
  }
  
  
  // Setup CAN Busses 
  CANBus1.begin();
  CANBus1.baudConfig(125);
  CANBus1.setRxInt(true);
  CANBus1.setMode(NORMAL);
  // attachInterrupt(CAN1INT, handleInterrupt1, LOW);
  
  CANBus2.begin();
  CANBus2.baudConfig(500);
  CANBus2.setRxInt(true);
  CANBus2.setMode(NORMAL);
  // attachInterrupt(CAN2INT, handleInterrupt2, LOW);
  
  CANBus3.begin();
  CANBus3.baudConfig(125);
  CANBus3.setRxInt(true);
  CANBus3.setMode(NORMAL);
  // Manually configure INT6 for Bus 3
  // EICRB |= (1<<ISC60)|(1<<ISC61); // sets the interrupt type
  // EIMSK |= (1<<INT6); // activates the interrupt
  
  
  digitalWrite( BOOT_LED, HIGH );
  delay(100);
  digitalWrite( BOOT_LED, LOW );
  delay(100);
  
  // Middleware setup
  SerialCommand::init( &writeQueue, busses );
  
  #ifdef USE_MIDDLEWARE
    ServiceCall::init( &writeQueue );
    MazdaLED::init( &writeQueue, cbt_settings.displayEnabled );
  #endif
  
}

/*
*  Interrupt Handlers
*/
void handleInterrupt1(){
  
}
void handleInterrupt2(){
  
}
ISR(INT6_vect) {
  
}




void loop() {
  
  // All Middleware ticks (Like loop() for middleware)
  SerialCommand::tick();
  
  #ifdef USE_MIDDLEWARE
    ServiceCall::tick();
    MazdaLED::tick();
  #endif
  
  
  if( digitalRead(CAN1INT_D) == 0 ) readBus(CANBus1);
  if( digitalRead(CAN2INT_D) == 0 ) readBus(CANBus2);
  if( digitalRead(CAN3INT_D) == 0 ) readBus(CANBus3);
  
  
  // Process message stack
  
  if( !readQueue.isEmpty() && !writeQueue.isFull() ){
    processMessage( readQueue.pop() );
  }
  
  #ifdef DEBUG_BUILD
  if( !writeQueue.isEmpty() ){ 
    SerialCommand::activeSerial->print(F("{queueCount:")); 
    SerialCommand::activeSerial->print( writeQueue.count(), DEC ); 
    SerialCommand::activeSerial->print( readQueue.count(), DEC ); 
    SerialCommand::activeSerial->println(F("}"));
  }
  #endif
  
  boolean success = true;
  while( !writeQueue.isEmpty() && success ){
    
    Message msg = writeQueue.pop();
    CANBus channel = busses[msg.busId-1];
    
    //SerialCommand::printMessageToSerial(msg);
    success = sendMessage( msg, channel );
    
    if( !success ){
      // TX Failure, add back to queue
      writeQueue.push(msg);
      
      #ifdef DEBUG_BUILD
        SerialCommand::activeSerial->println("ALL TX BUFFERS FULL ON " + busses[msg.busId-1].name );
      #endif
    }
    
  }
  
  //* MOVE TO MORE LOGICAL PLACE
    
  byte button = WheelButton::getButtonDown();
  
  if( wheelButton != button ){
    wheelButton = button;
    
     switch(wheelButton){
       /*
       case B10000000:
         MazdaLED::showStatusMessage("    LEFT    ", 2000);
       break;
       case B01000000:
         MazdaLED::showStatusMessage("    RIGHT   ", 2000);
       break;
       case B00100000:
         MazdaLED::showStatusMessage("     UP     ", 2000);
       break;
       case B00010000:
         MazdaLED::showStatusMessage("    DOWN    ", 2000);
       break;
       case B00001000:
         MazdaLED::showStatusMessage("    WHAT    ", 2000);
       break;
       case B00000100:
         MazdaLED::showStatusMessage("    WHAT    ", 2000);
       break;
       case B00000010:
         MazdaLED::showStatusMessage("    WHAT    ", 2000);
       break;
       case B00000001:
         MazdaLED::showStatusMessage("    WHAT    ", 2000);
       break;
       */
       
       case B10000001:
         // Decrement service pid
         ServiceCall::decServiceIndex();
         MazdaLED::showNewPageMessage();
         CANBus2.setFilter( ServiceCall::filterPids[0], ServiceCall::filterPids[1] );
       break;
       case B01000001:
         // Increment service pid 
         ServiceCall::incServiceIndex();
         MazdaLED::showNewPageMessage();
         CANBus2.setFilter( ServiceCall::filterPids[0], ServiceCall::filterPids[1] );
       break;
       case B01000010:
         toggleMazdaLed();
       break;
     }
 
  }

  
} // End loop()



void toggleMazdaLed()
{
  cbt_settings.displayEnabled = MazdaLED::enabled = !MazdaLED::enabled;
  EEPROM.write( offsetof(struct cbt_settings, displayEnabled), cbt_settings.displayEnabled);
  if(MazdaLED::enabled)
    MazdaLED::showStatusMessage("MazdaLED ON ", 2000);
}


boolean sendMessage( Message msg, CANBus bus ){
  
  if( msg.dispatch == false ) return true;
  
  digitalWrite( BOOT_LED, HIGH );
  
  int ch = bus.getNextTxBuffer();
  
  switch( ch ){
    case 0:
      bus.load_ff_0( msg.length, msg.frame_id, msg.frame_data );
      bus.send_0();
      break;
    case 1:
      bus.load_ff_1( msg.length, msg.frame_id, msg.frame_data );
      bus.send_1();
      break;
    case 2:
      bus.load_ff_2( msg.length, msg.frame_id, msg.frame_data );
      bus.send_2();
      break;
    default:
      // All TX buffers full
      return false;
      break;
  }
  
  #ifdef DEBUG_BUILD
    SerialCommand::activeSerial->print(F("Sent a message on TXB"));
    SerialCommand::activeSerial->print( ch, DEC );
    SerialCommand::activeSerial->print(F(" via bus "));
    SerialCommand::activeSerial->println( bus.name );
  #endif
  
  digitalWrite( BOOT_LED, LOW );
  
  return true;
  
}





void readBus( CANBus bus ){
  
  // TODO Cleanup and optimize
  
  // Abort if readQueue is full
  if( readQueue.isFull() ) return;
  
  rx_status = bus.readStatus();
  
  // Check buffer RX0
  if( (rx_status & 0x1) == 0x1 ){
    Message msg;
    msg.busStatus = rx_status;
    msg.busId = bus.busId;
    bus.readDATA_ff_0( &msg.length, msg.frame_data, &msg.frame_id );
    readQueue.push(msg);
  }
  
  // Abort if readQueue is full
  if( readQueue.isFull() ) return;
  
  // Check buffer RX1
  if( (rx_status & 0x2) == 0x2 ) {
    Message msg;
    msg.busStatus = rx_status;
    msg.busId = bus.busId;
    bus.readDATA_ff_1( &msg.length, msg.frame_data, &msg.frame_id );
    readQueue.push(msg);
  }
  
}


void processMessage( Message msg ){
  
  // All Middleware process calls (Augment incoming CAN packets)
  msg = SerialCommand::process( msg );
  
  #ifdef USE_MIDDLEWARE
    msg = ServiceCall::process( msg );
    msg = MazdaLED::process( msg );
    msg = ChannelSwap::process( msg );
  #endif
  
  if( msg.dispatch == true ){
    writeQueue.push( msg );
  }
  
}




