
#include <MessageQueue.h>
#include "Middleware.h"

#define NUM_PID_TO_PROCESS 2
#define BLUETOOTH_SENSORS

class ServiceCall : public Middleware
{

  private:
    MessageQueue* mainQueue;
    void saveSettings();
    byte* index;
  public:
    void tick();
    Message process(Message msg);
    ServiceCall( MessageQueue *q );
    void commandHandler(byte* bytes, int length);
    unsigned long lastServiceCallSent;
    void sendNextServiceCall( struct pid pid[] );
    void setServiceIndex(byte i);
    byte getServiceIndex();
    byte incServiceIndex();
    byte decServiceIndex();
    void setFilterPids();
    int  filterPids[ NUM_PID_TO_PROCESS ];
    void updateBTSensors( pid *pid );
};


ServiceCall::ServiceCall( MessageQueue *q )
{

  lastServiceCallSent = millis();
  index = &cbt_settings.displayIndex;

  mainQueue = q;
  setFilterPids();
}


void ServiceCall::tick()
{
  // Send service calls every ~100ms
  if( millis() > lastServiceCallSent + 30 ){
    lastServiceCallSent = millis();
    sendNextServiceCall( cbt_settings.pids );
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

      // Match RXF
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

        // Serial.print("Got response packet for PID ");
        // Serial.println( pid->name );
        // printMessageToSerial( msg );


        // Remove two bytes of length to compensate for the fact PID is not in this array. For ScanGauge compat
        byte start = (pid->rxd[0]/8) - 2;
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

        unsigned int mult = (pid->mth[0] << 8) + pid->mth[1];
        unsigned int div  = (pid->mth[2] << 8) + pid->mth[3];
        unsigned int add  = (pid->mth[4] << 8) + pid->mth[5];

        if(div == 0) div = 1;

        float divResult;
        divResult = (float)base / (float)div;
        divResult = divResult * mult;
        base = divResult + add;

        if( pid->value != base ){
          pid->value = base;

          #ifdef BLUETOOTH_SENSORS
          // Send over UART to Bluetooth
          char out[7];
          out[0] = 0xe7;
          out[1] = 0x83;
          out[2] = i+1;
          out[3] = base >> 8;
          out[4] = base & 0xFF;
          out[5] = 0x0D;
          out[6] = 0x0A;
          Serial1.print(out);
          #endif

        }



      }

    }

  }


  return msg;
}

void ServiceCall::commandHandler(byte* bytes, int length){}


byte ServiceCall::getServiceIndex()
{
  return *index;
}

void ServiceCall::setServiceIndex(byte i)
{
  *index = i;
  saveSettings();
  setFilterPids();
}

byte ServiceCall::incServiceIndex()
{

  if( (*index+1) > Settings::pidLength-1 )
   *index = 0;
  else
   *index = *index + 1;

  saveSettings();
  setFilterPids();

  return *index;
}

byte ServiceCall::decServiceIndex()
{

  if( (*index-1) < 0 )
   *index = Settings::pidLength-1;
  else
   *index = *index-1;

  saveSettings();
  setFilterPids();

  return *index;
}

void ServiceCall::setFilterPids()
{

  byte ii = 0;
  for( int i=*index; i<*index+NUM_PID_TO_PROCESS; i++ ){
    // Add 8 to TXD message ID, this is the ID that the response will use
    filterPids[ii] = int((cbt_settings.pids[i].txd[0] << 8) + cbt_settings.pids[i].txd[1]) + 0x08;
    ii++;
  }

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

  }

}
