
#include "Middleware.h"


class MazdaLED : public Middleware
{
  
  private:
    QueueArray<Message>* mainQueue;
    unsigned long updateCounter;
    void pushNewMessage();
    int fastUpdateDelay;
    unsigned long stockOverrideTimer;
    unsigned long statusOverrideTimer;
  public:
    MazdaLED( QueueArray<Message> *q, byte enabled );
    Message process( Message );
    void tick();
    // void init( QueueArray<Message> *q, byte enabled );
    
    void showNewPageMessage();
    boolean enabled;
    void showStatusMessage(char* str, int time);
    char lcdString[13];
    char lcdStockString[13];
    char lcdStatusString[];
    byte lcdStatusStringLength;
    char lcdStatusStringSlice[];
    void setOverrideTime( int n );
    void setStatusTime( int n );
    unsigned int animationCounter;
    char* currentLcdString();
    void commandHandler(byte*, int);
};





MazdaLED::MazdaLED( QueueArray<Message> *q, byte enabl )
{
  mainQueue = q;
  enabled = (enabl == 1);
  
  // Register a serial command callback handler
  // SerialCommand::registerCommand(0x16, commandHandler);
  
  // Instance Properties 
  updateCounter = 0;
  fastUpdateDelay = 500;
  animationCounter = 0;
  stockOverrideTimer = 0;
  statusOverrideTimer = 0;
  
  strcpy(lcdString, "CANBusTriple");
  strcpy(lcdStockString, "            ");
  strcpy(lcdStatusString, "                                                                ");  
  lcdStatusStringLength = 12;
  strcpy(lcdStatusStringSlice, lcdStockString);
  
}


void MazdaLED::commandHandler(byte* bytes, int length)
{
  
  char chars[65];
  for(unsigned int i = 0; i < length; i++){
    chars[i] = (char)bytes[i];
  }
  
  if( length > 12 ){
    lcdStatusStringLength = animationCounter = length;
    MazdaLED::showStatusMessage(chars, 4000+(length*200));
  }else{
    animationCounter = 0;
    MazdaLED::showStatusMessage(chars, 7000);
  }
  
  
  
}


void MazdaLED::tick()
{
  
  if(!enabled) return;
  
  // New LED update CAN message for fast updates
  // check stockOverrideTimer, no need to do fast updates while stock override is active
  if( (updateCounter+fastUpdateDelay) < millis() && stockOverrideTimer < millis() ){
    pushNewMessage();
    updateCounter = millis();
  }
  
}


void MazdaLED::showStatusMessage(char* str, int time){
  sprintf( MazdaLED::lcdStatusString , str);
  MazdaLED::setStatusTime(time);
}


void MazdaLED::pushNewMessage(){
  
  char* lcd = currentLcdString();
  
  Message msg;
  msg.busId = 3;
  msg.frame_id = 0x290;
  msg.frame_data[0] = 0xC0;  // Look this up from log
  msg.frame_data[1] = lcd[0];
  msg.frame_data[2] = lcd[1];
  msg.frame_data[3] = lcd[2];
  msg.frame_data[4] = lcd[3];
  msg.frame_data[5] = lcd[4];
  msg.frame_data[6] = lcd[5];
  msg.frame_data[7] = lcd[6];
  msg.length = 8;
  msg.dispatch = true;
  mainQueue->push(msg);
  
  Message msg2;
  msg2.busId = 3;
  msg2.frame_id = 0x291;
  msg2.frame_data[0] = 0x87;  // Look this up from log
  msg2.frame_data[1] = lcd[7];
  msg2.frame_data[2] = lcd[8];
  msg2.frame_data[3] = lcd[9];
  msg2.frame_data[4] = lcd[10];
  msg2.frame_data[5] = lcd[11];
  msg2.length = 8;
  msg2.dispatch = true;
  mainQueue->push(msg2);
  
  // Turn off extras and periods on screen
  // 02 03 02 8F C0 00 00 00 01 27 10 40 08

  Message msg3;
  msg3.busId = 1;
  msg3.frame_id = 0x28F;
  msg3.frame_data[0] = 0xC0;
  msg3.frame_data[1] = 0x0;
  msg3.frame_data[2] = 0x0;
  msg3.frame_data[3] = 0x0;
  msg3.frame_data[4] = 0x1;
  msg3.frame_data[5] = 0x27;
  msg3.frame_data[6] = 0x10;
  msg3.frame_data[7] = 0x40;
  msg3.length = 8;
  msg3.dispatch = true;
  mainQueue->push(msg3);
  
  
}


char* MazdaLED::currentLcdString(){
  if( stockOverrideTimer > millis() )
    return lcdStockString;
  else if( statusOverrideTimer > millis() ){
    // Return a slice of the lcdStatusString (Animation)
    sprintf( lcdStatusStringSlice, "            " );
    
    // for(int i=0; i<12; i++)
      // if( (lcdStatusStringLength - animationCounter)+i < lcdStatusStringLength )
        // lcdStatusStringSlice[i] = lcdStatusString[(lcdStatusStringLength - animationCounter)+i];
      if( animationCounter > 0 )
        memcpy ( &lcdStatusStringSlice+(animationCounter-12), &lcdStatusString, 12 );
      else
        memcpy ( &lcdStatusStringSlice, &lcdStatusString, 12 );
    
    if(animationCounter > 0) animationCounter--;
    
    return lcdStatusStringSlice;
  }else
    return lcdString;
}


void MazdaLED::setOverrideTime( int n ){
  stockOverrideTimer = millis() + n;
}

void MazdaLED::setStatusTime( int n ){
  statusOverrideTimer = millis() + n;
  // animationCounter = 0;
}


Message MazdaLED::process(Message msg)
{
  if(!enabled){
    return msg;
  }
  
  if( msg.frame_id == 0x28F && stockOverrideTimer < millis() ){
    // Block extras
    msg.frame_data[0] = 0xC0;
    msg.frame_data[1] = 0x0;
    msg.frame_data[2] = 0x0;
    msg.frame_data[3] = 0x0;
    // msg.frame_data[4] = 0x01; // Looks like phone light
    msg.frame_data[5] = 0x27;
    msg.frame_data[6] = 0x10;
    // msg.frame_data[7] = 0x40; looks like music note light
    msg.dispatch = true;
  }
  
  if( msg.frame_id == 0x290 ){
    
    if( msg.frame_data[1] != lcdStockString[0] ||
        msg.frame_data[2] != lcdStockString[1] || 
        msg.frame_data[3] != lcdStockString[2] || 
        msg.frame_data[4] != lcdStockString[3] || 
        msg.frame_data[5] != lcdStockString[4] || 
        msg.frame_data[6] != lcdStockString[5] || 
        msg.frame_data[7] != lcdStockString[6] ){
          
        lcdStockString[0] = msg.frame_data[1];
        lcdStockString[1] = msg.frame_data[2];
        lcdStockString[2] = msg.frame_data[3];
        lcdStockString[3] = msg.frame_data[4];
        lcdStockString[4] = msg.frame_data[5];
        lcdStockString[5] = msg.frame_data[6];
        lcdStockString[6] = msg.frame_data[7];
        
    }
    
    char* lcd = currentLcdString();
    
    msg.frame_data[1] = lcd[0];
    msg.frame_data[2] = lcd[1];
    msg.frame_data[3] = lcd[2];
    msg.frame_data[4] = lcd[3];
    msg.frame_data[5] = lcd[4];
    msg.frame_data[6] = lcd[5];
    msg.frame_data[7] = lcd[6];
    msg.dispatch = true;
    
  }
  
  if( msg.frame_id == 0x291 ){
    
    if( msg.frame_data[1] != lcdStockString[7] ||
        msg.frame_data[2] != lcdStockString[8] || 
        msg.frame_data[3] != lcdStockString[9] || 
        msg.frame_data[4] != lcdStockString[10] || 
        msg.frame_data[5] != lcdStockString[11] ){
      
      lcdStockString[7] = msg.frame_data[1];
      lcdStockString[8] = msg.frame_data[2];
      lcdStockString[9] = msg.frame_data[3];
      lcdStockString[10] = msg.frame_data[4];
      lcdStockString[11] = msg.frame_data[5];
      
      setOverrideTime(1500);
      
    }
    
    char* lcd = currentLcdString();
    
    msg.frame_data[1] = lcd[7];
    msg.frame_data[2] = lcd[8];
    msg.frame_data[3] = lcd[9];
    msg.frame_data[4] = lcd[10];
    msg.frame_data[5] = lcd[11];
    msg.dispatch = true;
    
  }
  
  // TODO: Make float compat
  byte incIndex = ( cbt_settings.displayIndex+1 > Settings::pidLength-1 ) ? 0 : cbt_settings.displayIndex+1;
  
  char buffer[6] = "     ";
  
  
  
  sprintf(lcdString, "            ");
  
  // TODO: Tidy up with a loop
  if( cbt_settings.pids[cbt_settings.displayIndex].settings & B00000001 == B00000001 ){ // add decimal flag
  sprintf(lcdString, "%c:%d.%d", cbt_settings.pids[cbt_settings.displayIndex].name[0], 
                                   cbt_settings.pids[cbt_settings.displayIndex].value/10,
                                   cbt_settings.pids[cbt_settings.displayIndex].value%10);
  }else{
    sprintf(lcdString, "%c:%d", cbt_settings.pids[cbt_settings.displayIndex].name[0], 
                                   cbt_settings.pids[cbt_settings.displayIndex].value);
  }
  
  if( cbt_settings.pids[incIndex].settings & B00000001 == B00000001 ){ // add decimal flag
  sprintf(lcdString+6, "%c:%d.%d", cbt_settings.pids[incIndex].name[0], 
                                    cbt_settings.pids[incIndex].value/10,
                                    cbt_settings.pids[incIndex].value%10);
  }else{
    sprintf(lcdString+6, "%c:%d", cbt_settings.pids[incIndex].name[0], 
                                   cbt_settings.pids[incIndex].value);
  }
  
  
  // Turn off extras like decimal point. Needs verification!
  if( msg.frame_id == 0x201 ){
    msg.dispatch = false;
  }
  
   
  return msg;
}


void MazdaLED::showNewPageMessage()
{
  char msgBuffer[13] = "            ";
  sprintf( msgBuffer, " %c%c%c%c  %c%c%c%c ", cbt_settings.pids[cbt_settings.displayIndex].name[0], 
                                              cbt_settings.pids[cbt_settings.displayIndex].name[1], 
                                              cbt_settings.pids[cbt_settings.displayIndex].name[2],
                                              cbt_settings.pids[cbt_settings.displayIndex].name[3],
                                              
                                              cbt_settings.pids[cbt_settings.displayIndex+1].name[0],
                                              cbt_settings.pids[cbt_settings.displayIndex+1].name[1],
                                              cbt_settings.pids[cbt_settings.displayIndex+1].name[2],
                                              cbt_settings.pids[cbt_settings.displayIndex+1].name[3]);
  MazdaLED::showStatusMessage(msgBuffer, 2000);
}



