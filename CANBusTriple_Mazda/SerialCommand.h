
/*
// Serial Commands

0x01 Print Debug to Serial

Send CAN frame
Cmd    Bus id  PID    data 0-7                  length
0x02   01      290    00 00 00 00 00 00 00 00   8

Set log mode and filter
Cmd    Mode   Filer PID
0x03   1      0290
  
  Modes 1 Log all
        2 Show only filter PID
        3 Show lower than filter PID
        4 Show higher than filter PID


Enable / Disable logging of busses
Cmd    Bus Enabled Flags
0x04   0x7 (B111)         // Enable all 
0x04   0x1 (B001)         // Enable only Bus 1
0x04   0x2 (B010)         // Enable only Bus 2
0x04   0x4 (B100)         // Enable only Bus 3



*/

#include "Middleware.h"


class SerialCommand : Middleware
{
  public:
    static byte logOutputMode;
    static unsigned int logOutputFilter;
    static CANBus busses[3];
    static void init( QueueArray<Message> *q, CANBus b[], int logOutput );
    static void tick();
    static Message process( Message msg );
    static void printMessageToSerial(Message msg);
  private:
    static QueueArray<Message>* mainQueue;
    static void printChannelDebug(CANBus channel);
    static void processCommand(int command);
    static int  getCommandBody( byte* cmd, int length );
    static void getAndSend();
    static void printChannelDebug();
    static void logCommand();
    static void setBusEnableFlags();
    static byte busEnabled;
    static Message newMessage;
};

// Defaults
byte SerialCommand::logOutputMode = 0x0;
unsigned int SerialCommand::logOutputFilter = 0;
QueueArray<Message> *SerialCommand::mainQueue;
//CANBus SerialCommand::busses[3];
byte SerialCommand::busEnabled = 0x7; // Start with all busses enabled


void SerialCommand::init( QueueArray<Message> *q, CANBus b[], int logOutput )
{
  // busses = { b[0], b[1], b[2] };
  SerialCommand::logOutputMode = logOutput;
  mainQueue = q;
}

void SerialCommand::tick()
{
  
  if (Serial.available() > 0) {
    processCommand( Serial.read() );
  }
  
}

Message SerialCommand::process( Message msg )
{
  if(logOutputMode > 0) printMessageToSerial(msg);
  return msg;
}


void SerialCommand::printMessageToSerial( Message msg )
{
  
  // PID Filter
  if( logOutputMode == 2 && msg.frame_id != logOutputFilter ) return;
  if( logOutputMode == 3 && msg.frame_id < logOutputFilter ) return;
  if( logOutputMode == 4 && msg.frame_id > logOutputFilter ) return;
  
  // Bus Filter
  byte flag;
  flag = 0x1 << (msg.busId-1);
  if( !(SerialCommand::busEnabled & flag) ){
    Serial.println("Bus disabled");
    return;
  }
  
  
  // Output to serial as json string
  Serial.print("{\"packet\": {\"status\":\"");
  Serial.print( msg.busStatus,HEX);
  Serial.print("\",\"channel\":\"");
  Serial.print( busses[msg.busId-1].name );
  Serial.print("\",\"length\":\"");
  Serial.print(msg.length,HEX);
  Serial.print("\",\"id\":\"");
  Serial.print(msg.frame_id,HEX);
  Serial.print("\",\"timestamp\":\"");
  Serial.print(millis(),DEC);
  Serial.print("\",\"payload\":[\"");
  
  
  for (int i=0; i<8; i++) {
    Serial.print(msg.frame_data[i],HEX);
    if( i<7 ) Serial.print("\",\"");
  }
  
  Serial.print("\"]}}");
  Serial.println();
  
}


void SerialCommand::printChannelDebug(CANBus channel){
  
  Serial.print( "Debugging channel " );
  Serial.println( channel.name );
  Serial.print(" CANCTRL: ");
  Serial.print( channel.readControl(), BIN );
  Serial.print(" STATUS: ");
  Serial.print( channel.readStatus(), BIN );
  Serial.print(" ERROR: ");
  Serial.println( channel.readRegister(EFLG), BIN );
  Serial.print(" IntE: ");
  Serial.println( channel.readIntE(), BIN );
  Serial.print(" Next Available TX buffer: ");
  Serial.println( channel.getNextTxBuffer(), DEC );
  
}



void SerialCommand::processCommand(int command)
{
  
  switch( command ){
    case 1:
      // Debug Command
      printChannelDebug();
    break;
    case 2:
      // Send command
      // Ex: 02 03 02 92 66 66 66 66 66 66 66 66 08 
      getAndSend();
    break;
    case 3:
      // Log output toggle
      // Ex: 03 02 02 90
      logCommand();
    break;
    case 4:
      setBusEnableFlags();
    break;
  }
}


void SerialCommand::logCommand()
{
  byte cmd[3];
  int bytesRead = getCommandBody( cmd, 3 );
  
  logOutputMode = cmd[0];
  logOutputFilter = (cmd[1]<<8) + cmd[2];
  Serial.print("{\"event\":\"logMode\", \"mode\": ");
  Serial.print(logOutputMode);
  Serial.print(", \"filter\": ");
  Serial.print(logOutputFilter, HEX);
  Serial.println("}");
  
  // Change this system to set RXB0CTRL and RXB1CTRL on each CAN Bus for efficiency
  // This may ot may not work, as the MCP2515 has to be in Configuration mode to set filters
  
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


void SerialCommand::setBusEnableFlags(){
  
  byte cmd[1];
  int bytesRead = getCommandBody( cmd, 1 );
  
  SerialCommand::busEnabled = cmd[0];
  Serial.print("{\"event\":\"logBusFilter\", \"mode\": ");
  Serial.print(SerialCommand::busEnabled, HEX);
  Serial.println("}");
  
}


int SerialCommand::getCommandBody( byte* cmd, int length )
{
  unsigned int i = 0;
  
  while( Serial.available() ){
    if( i <= length ){
      cmd[i] = Serial.read();
      // Serial.print( cmd[i], HEX );
    }else{
      Serial.read();
    }
    i++;
  }
  
  return i;
}


void SerialCommand::printChannelDebug()
{
  Serial.println("DEBUG INFO:: CANBus Triple Mazda 0.1.1");
  
  printChannelDebug( busses[0] );
  printChannelDebug( busses[1] );
  printChannelDebug( busses[2] );
  
}



