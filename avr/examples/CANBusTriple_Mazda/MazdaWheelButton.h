#ifndef MazdaWheelButton_H
#define MazdaWheelButton_H

#import "Middleware.h"
#import "MazdaLED.h"
#import "ServiceCall.h"

class MazdaWheelButton : public Middleware
{
  public:
    void tick();
    Message process( Message msg );
    MazdaWheelButton(MazdaLED *mled, ServiceCall *srv);
    void commandHandler(byte* bytes, int length);
    void toggleMazdaLed();
  private:
    byte wheelButton;
    MazdaLED *mazdaLed;
    ServiceCall *serviceCall;
};

MazdaWheelButton::MazdaWheelButton( MazdaLED *mled, ServiceCall *srv ){
  wheelButton = 0;
  mazdaLed = mled;
  serviceCall = srv;
}

Message MazdaWheelButton::process(Message msg)
{
  return msg;
}

void MazdaWheelButton::tick()
{

  byte button = WheelButton::getButtonDown();

  if( wheelButton != button ){
    wheelButton = button;

     switch(wheelButton){
       /*
       case B10000000:
         mazdaLed->showStatusMessage("    LEFT    ", 2000);
       break;
       case B01000000:
         mazdaLed->showStatusMessage("    RIGHT   ", 2000);
       break;
       case B00100000:
         mazdaLed->showStatusMessage("     UP     ", 2000);
       break;
       case B00010000:
         mazdaLed->showStatusMessage("    DOWN    ", 2000);
       break;
       case B00001000:
         mazdaLed->showStatusMessage("    WHAT    ", 2000);
       break;
       case B00000100:
         mazdaLed->showStatusMessage("    WHAT    ", 2000);
       break;
       case B00000010:
         mazdaLed->showStatusMessage("    WHAT    ", 2000);
       break;
       case B00000001:
         mazdaLed->showStatusMessage("    WHAT    ", 2000);
       break;
       */


       case B10000001:
         // Decrement service pid
         serviceCall->decServiceIndex();
         mazdaLed->showNewPageMessage();
         CANBus2.setMode(CONFIGURATION);
         CANBus2.setFilter( serviceCall->filterPids[0], serviceCall->filterPids[1] );
         CANBus2.setMode(NORMAL);
       break;
       case B01000001:
         // Increment service pid
         serviceCall->incServiceIndex();
         mazdaLed->showNewPageMessage();
         CANBus2.setMode(CONFIGURATION);
         CANBus2.setFilter( serviceCall->filterPids[0], serviceCall->filterPids[1] );
         CANBus2.setMode(NORMAL);
       break;
       case B01000010:
         toggleMazdaLed();
       break;

     }
  }

}

void MazdaWheelButton::commandHandler(byte* bytes, int length){}

void MazdaWheelButton::toggleMazdaLed()
{
  cbt_settings.displayEnabled = mazdaLed->enabled = !mazdaLed->enabled;
  EEPROM.write( offsetof(struct cbt_settings, displayEnabled), cbt_settings.displayEnabled);
  if(mazdaLed->enabled)
    mazdaLed->showStatusMessage("MazdaLED ON ", 2000);
}



#endif

