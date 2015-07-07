
#ifndef Naptime_H
#define Naptime_H

#include <avr/power.h>
#include <avr/sleep.h>

#include <CANBus.h>

class Naptime : public Middleware
{
  public:
    void tick();
    Message process( Message msg );
    void reset();
    static void handleInterrupt();
    boolean enabled;
    unsigned long timer;
    unsigned short resetId;
    Naptime(SerialCommand *serialCommand);
    Naptime(int, SerialCommand *serialCommand);
    void commandHandler(byte* bytes, int length);
  private:


};


Naptime::Naptime(SerialCommand *serialCommand)
{
  timer = 0;
  resetId = 0;
  enabled = true;

  // Set a command callback to enable disable sleep
  // 4E01 on 4E00 off
  serialCommand->registerCommand(0x4E, this);
}

Naptime::Naptime( int resetMessageId, SerialCommand *serialCommand )
{
  timer = 0;
  resetId = resetMessageId;
  enabled = true;

  serialCommand->registerCommand(0x4E, this);
}

void Naptime::commandHandler(byte* bytes, int length){
  enabled = bytes[0];
}

void Naptime::tick()
{

  if(enabled) timer++;

  if( timer > 20000 ){
    timer = 0;

    // Sleep if USB CDC is not connected
    if(Serial) return;

    // Let BLE112 Sleep
    digitalWrite( BT_SLEEP, LOW );

    // disable the dawg
    wdt_disable();


    for( byte i=0; i<8; i++ ){
      PORTE |= B00000100;
      delay(100);
      PORTE &= B00000000;
      delay(100);
    }


    for(byte fade=220; fade>0; fade--){
      analogWrite(BOOT_LED, fade);
      delay(1);
    }

    // TODO: Make this optional by checking resetId
    // Leave Bus1 awake and set a filter to wake the system up
    busses[0].setMode(CONFIGURATION);
    busses[0].setFilter( resetId, resetId );
    busses[0].bitModify( CANINTF, 0, 0x03 );
    busses[0].setMode(NORMAL);
    delay(2);


    byte controllerModes[3];

    // Put MCP2515s down for a nap
    for(int i=2; i>=0; i--){

      controllerModes[i] = busses[i].readRegister( CANCTRL );

      busses[i].bitModify( BFPCTRL, B1BFE, B1BFE );
      busses[i].bitModify( BFPCTRL, 0, B1BFM );
      busses[i].bitModify( BFPCTRL, B1BFS, B1BFS );

      if( i != 0 ){
        // busses[i].bitModify( CANINTE, WAKIE, WAKIE );
        busses[i].setMode(SLEEP);
      }

      // Clear RX Buffers
      busses[i].bitModify( CANINTF, 0, 0x03 );

    }


    attachInterrupt(0, Naptime::handleInterrupt, LOW);
    attachInterrupt(1, Naptime::handleInterrupt, LOW);
    attachInterrupt(4, Naptime::handleInterrupt, LOW);


    // SLEEP_MODE_IDLE         -the least power savings
    // SLEEP_MODE_ADC
    // SLEEP_MODE_PWR_SAVE
    // SLEEP_MODE_STANDBY
    // SLEEP_MODE_PWR_DOWN     -the most power savings

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    cli();
    sleep_enable();
    sei();

    power_adc_disable();
    sleep_cpu();

    // CPU will wakeup here

    delay(4);
    sleep_disable();
    power_adc_enable();
    delay(2);


    for(int i=0; i<=2; i++){

      busses[i].bitModify( CANINTF, WAKIF, WAKIF );
      busses[i].bitModify( CANINTE, WAKIE, WAKIE );
      delay(2);
      busses[i].bitModify( CANINTE, 0, WAKIE );
      busses[i].bitModify( CANINTF, 0, WAKIF );
      busses[i].bitModify( CANINTF, 0, 0xFF  );
      busses[i].bitModify( BFPCTRL, 0, B1BFS );
      delay(1);


      if( i == 0 ){
        delay(2);
        // Remove Bus 1 filter
        busses[0].setMode(CONFIGURATION);
        busses[0].clearFilters();
      }

      busses[i].bitModify( CANCTRL, controllerModes[i], 0xE0 );
      delay(2);

    }



    PORTE |= B00000100;
    digitalWrite( BOOT_LED, LOW );

    digitalWrite( BT_SLEEP, HIGH ); // Keep BLE112 Awake

    // Re-enable the watchdog
    // wdt_enable(WDTO_1S);

    // Sweep Gauges
    delay(500);
    MazdaLED::sweepGauges = 8000;

    Naptime::reset();
  }

}

void Naptime::reset()
{
  timer = 0;
}

Message Naptime::process( Message msg ){

  if( msg.frame_id == Naptime::resetId )
    Naptime::reset();

  return msg;
}

void Naptime::handleInterrupt(){
  // Disable all interrupts
  detachInterrupt(0);
  detachInterrupt(1);
  detachInterrupt(4);
}


#endif
