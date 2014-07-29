
#include "Middleware.h"

#define NUM_PID_TO_PROCESS 2


class ServiceCall : Middleware
{
  
  private:
    static QueueArray<Message>* mainQueue;
    static void saveSettings();
    static byte* index;
  public:
    static void init( QueueArray<Message> *q );
    static void tick();
    static Message process(Message msg);
    static unsigned long lastServiceCallSent;
    static void sendNextServiceCall( struct pid pid[] );
    static void setServiceIndex(byte i);
    static byte getServiceIndex();
    static byte incServiceIndex();
    static byte decServiceIndex();
};


QueueArray<Message>* ServiceCall::mainQueue;
unsigned long ServiceCall::lastServiceCallSent = millis();
byte * ServiceCall::index = &cbt_settings.displayIndex;

void ServiceCall::init( QueueArray<Message> *q )
{
  mainQueue = q;
}


void ServiceCall::tick()
{
  // Send service calls every ~100ms
  if( millis() > ServiceCall::lastServiceCallSent + 30 ){
    ServiceCall::lastServiceCallSent = millis();
    ServiceCall::sendNextServiceCall( cbt_settings.pids );
  }
  
}


Message ServiceCall::process(Message msg){
  
  // Process service call responses 
  for( int i=*index; i<*index+NUM_PID_TO_PROCESS; i++ ){
    
    struct pid *pid = &cbt_settings.pids[i];
    if( pid->txd[0] == 0 && pid->txd[1] == 0 )
      continue;
    
    // If the PID returned is 8 higher than the request pid we've recieved a response
    if(msg.frame_id == ((pid->txd[0]<<8)+pid->txd[1]+0x08) ){
      
      // Match RXD
      int rxfi = 0;
      boolean match = true;
      while( rxfi < 6 && pid->rxf[rxfi] ){
        if( msg.frame_data[ pid->rxf[rxfi]-3 ] != pid->rxf[rxfi+1]){ // use rxf -3 as scangauge is 1 indexed and assumes 2 byes of pid data 
          match = false;
          break;
        };
        rxfi += 2;
      }
      
      if(match){
        
        Serial.print("Got response packet for PID ");
        Serial.println( pid->name );
        SerialCommand::printMessageToSerial( msg );
        
        
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
        divResult = (float)base / (float)div;
        divResult = divResult * mult;
        base = divResult + add;
        
        pid->value = base;
        
      }
      
    }
    
  }
  
  
  return msg;
}



byte ServiceCall::getServiceIndex()
{
  return *index;
}

void ServiceCall::setServiceIndex(byte i)
{
  *index = i;
  saveSettings();
}

byte ServiceCall::incServiceIndex()
{
  
  if( (*index+1) > Settings::pidLength-1 )
   *index = 0;
  else
   *index = *index + 1;
  
  ServiceCall::saveSettings();
  return *index;
}

byte ServiceCall::decServiceIndex()
{
  
  if( (*index-1) < 0 )
   *index = Settings::pidLength-1;
  else
   *index = *index-1;
  
  ServiceCall::saveSettings();
  return *index;
}

void ServiceCall::saveSettings()
{
  EEPROM.write( offsetof(struct cbt_settings, displayIndex), cbt_settings.displayIndex);
}


void ServiceCall::sendNextServiceCall( struct pid pid[] ){
  
  
  /* TODO
   * Make this look down to 0 when the index is 7, right now it just increments up one which could try to access a non-existant index of 9.
   * Same with the read response function.
  */
  for( int i=*index; i<*index+NUM_PID_TO_PROCESS; i++ ){
    
    // if( pid[i].txd[0] == 0 && pid[i].txd[1] == 0 ) // Aborts if we have no PID
    if( pid[i].txd[2] == 0 || pid[i].busId < 1 )      // Aborts if we have no service call data. Allows us to match on passive PIDs
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
    Serial.print("Service call sent pid settings storage location: ");
    Serial.print(i);
    Serial.print(" ");
    Serial.println( pid[i].name );
    // SerialCommand::printMessageToSerial( msg );
    */
    
  }
  
}
