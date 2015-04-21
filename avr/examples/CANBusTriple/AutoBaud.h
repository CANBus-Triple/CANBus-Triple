#ifndef AutoBaud_H
#define AutoBaud_H


class AutoBaud
{
  public:
    static int baudDetect(byte, Stream*);
};

/*
*  Cycle through available baud rates and listen for successful packets
*  Save successful baud rate to cbt_settings structure
*/
int AutoBaud::baudDetect(byte busId, Stream* activeSerial){
  
  if( busId < 1 || busId > 3 ) return 0;
  
  CANBus channel = busses[busId-1];
  
  int io, reset;
  switch(busId){
    case 1:
      io = CAN1INT_D;
    break;
    case 2:
      io = CAN2INT_D;
    break;
    case 3:
      io = CAN3INT_D;
    break;
  }
  
  // Save settings
  byte canctrl = channel.readRegister( CANCTRL );
  byte caninte = channel.readRegister( CANINTE );

  
  int rates[9] = {10, 20, 50, 83, 100, 125, 250, 500, 1000};
  
  /*
  *  Watch for packet
  */
  int rateFound = 0;
  for( int rateIndex = 0; rateIndex < 9; rateIndex++ ){
    
    activeSerial->print(F("{\"event\":\"autobaudTest\", \"rate\":"));
    activeSerial->print(rates[rateIndex]);
    activeSerial->println("}");
    
    // reset
    // channel.reset();
    channel.setMode(CONFIGURATION);
    delay(5);
    channel.baudConfig(rates[rateIndex]);
    delay(5);
    channel.setMode(LISTEN);
    delay(5);
    channel.bitModify( CANINTF, 0x00, 0xFF ); // Clear Interrupts
    delay(5);
    
   
    delay(200); // Wait for errors
    
    byte canintf = channel.readRegister( CANINTF );   
    byte merrf = canintf & MERRF;

    if( merrf == 0 & (canintf && 0x03) )
      rateFound = rates[rateIndex];
    
  }
  
  
  // Restore Mode
  // channel.reset();
  channel.setMode(CONFIGURATION);
  delay(10);
  channel.baudConfig(rateFound);
  delay(1);
  channel.bitModify( CANCTRL, canctrl, 0xFF ); //0xE0
  delay(1);
  channel.bitModify( CANINTE, caninte, 0xFF );
  delay(1);
  channel.setMode(NORMAL);
  
  activeSerial->print(F("{\"event\":\"autobaudComplete\", \"rate\":"));
  activeSerial->print(rateFound);
  activeSerial->println("}");
  
  
  // Save new baud rate to eeprom settings
  if(rateFound)
    Settings::setBaudRate(busId, rateFound);
    
  return rateFound;

}

#endif
