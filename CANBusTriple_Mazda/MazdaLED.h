
#include "Middleware.h"


class MazdaLED : Middleware
{
  
  private:
    static QueueArray<Message>* mainQueue;
    static unsigned long updateCounter;
    static void pushNewMessage();
    static int fastUpdateDelay;
  public:
    static void init( QueueArray<Message> *q, byte enabled );
    static void tick();
    static boolean enabled;
    static void showStatusMessage(char* str, int time);
    static char lcdString[13];
    static char lcdStockString[13];
    static char lcdStatusString[13];
    static unsigned long stockOverrideTimer;
    static unsigned long statusOverrideTimer;
    static void setOverrideTime( int n );
    static void setStatusTime( int n );
    static unsigned long animationCounter;
    static Message process( Message msg );
    static char* currentLcdString();
    static void sendServiceCalls( struct pid pid[] );
    
};

boolean MazdaLED::enabled = cbt_settings.displayEnabled;
unsigned long MazdaLED::updateCounter = 0;
int MazdaLED::fastUpdateDelay = 500;
QueueArray<Message>* MazdaLED::mainQueue;
char MazdaLED::lcdString[13] = "CANBusTriple";
char MazdaLED::lcdStockString[13] = "            ";
char MazdaLED::lcdStatusString[13] = "            ";

int afr;
int afrWhole;
int afrRemainder;
int knockRetard;
int egt;
int sparkAdvance;
int pWeight;

unsigned long MazdaLED::animationCounter = 0;
unsigned long MazdaLED::stockOverrideTimer = 4000;
unsigned long MazdaLED::statusOverrideTimer = 0;

byte egtSettings[2][3];



void MazdaLED::init( QueueArray<Message> *q, byte enabled )
{
  mainQueue = q;
  MazdaLED::enabled = (enabled == 1);
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
  
  // Send service calls every ~100ms
  if( (millis() % 100) < 1 ) MazdaLED::sendServiceCalls( cbt_settings.pids );
  
}


void MazdaLED::showStatusMessage(char* str, int time){
  sprintf( MazdaLED::lcdStatusString , str);
  MazdaLED::setStatusTime(time);
}



void MazdaLED::sendServiceCalls( struct pid pid[] ){
  
  for( int i=0; i<Settings::pidLength; i++ ){
    
    // if( pid[i].txd[0] == 0 && pid[i].txd[1] == 0 ) // Aborts if we have no PID
    if( pid[i].txd[2] == 0 )                          // Aborts if we have no service call data. Allows us to match on passive PIDs
      continue;
    
    Message msg;
    msg.busId = pid[i].busId;
    msg.frame_id = (pid[i].txd[0] << 8) + pid[i].txd[1];
    
    int ii = 0;
    while( ii <= 5 && pid[i].txd[ii+2] != 0 ){
      msg.frame_data[ii+1] = pid[i].txd[ii+2];
      ii++;
    }
      
    msg.frame_data[0] = ii;
    
    msg.length = 8;
    msg.dispatch = true;
    mainQueue->push(msg);
    
    /*
    Serial.print("Service call sent pid settings storage location:");
    Serial.println(i);
    SerialCommand::printMessageToSerial( msg );
    */
    
  }
  
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
  else if( statusOverrideTimer > millis() )
    return lcdStatusString;
  else
    return lcdString;
}


void MazdaLED::setOverrideTime( int n ){
  stockOverrideTimer = millis() + n;
}

void MazdaLED::setStatusTime( int n ){
  statusOverrideTimer = millis() + n;
}


Message MazdaLED::process(Message msg)
{
  if(!enabled) return msg;
  
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
  
  // Process service call responses
  for( int i=0; i<Settings::pidLength; i++ ){
    
    struct pid *pid = &cbt_settings.pids[i];
    if( pid->txd[0] == 0 && pid->txd[1] == 0 )
      continue;
    
    // If the PID returned is 8 higher than the request pid we've recieved a response
    if(msg.frame_id == ((pid->txd[0]<<8)+pid->txd[1]+0x08) ){
      
      // Match RXD
      int rxfi = 0;
      boolean match = true;
      while( rxfi <= 6 && pid->rxf[rxfi] ){
        if( msg.frame_data[ pid->rxf[rxfi]-3 ] != pid->rxf[rxfi+1]){ // use rxf -3 as scangauge is 1 indexed and assumes 2 byes of pid data 
          match = false;
          break;
        };
        rxfi += 2;
      }
      
      if(match){
        
        /*
        Serial.print("Got response packet for PID ");
        Serial.println( pid->name );
        SerialCommand::printMessageToSerial( msg );
        */
        
        byte start = (pid->rxd[0]/8) - 2; // Remove two bytes of length to compensate for the fact PID is not in this array. For ScanGauge compat
        byte A = msg.frame_data[start];
        byte B;
        
        if( pid->rxd[1] == 16 )
          B = msg.frame_data[start+1];
        else
          B = 0;
        
        // Do math
        unsigned int base;
        base = 0;
        if( pid->rxd[1] == 16 )
          base = (A << 8) + B;
        else
          base = A;
        
        /*
        Serial.println(A);
        Serial.println(B);
        */
        
        unsigned int mult = (pid->mth[0] << 8) + pid->mth[1];
        unsigned int div  = (pid->mth[2] << 8) + pid->mth[3];
        unsigned int add  = (pid->mth[4] << 8) + pid->mth[5];
        
        if(div == 0) div = 1;
        
        float divResult;
        divResult = (float)base / (float)div;\
        divResult = divResult * mult;
        base = divResult + add;
        
        pid->value = base;
        
      }
      
      
      
      
    }
    
  }
  
  
  sprintf(lcdString, "A:%d E:%d",  cbt_settings.pids[1].value, cbt_settings.pids[0].value );
  
  // Turn off extras like decimal point. Needs verification!
  if( msg.frame_id == 0x201 ){
    msg.dispatch = false;
  }
  
  
  // Animation
  // strcpy( MazdaLED::lcdString, "CANBusTriple" );
  // animationCounter++;
  
   
  return msg;
}


