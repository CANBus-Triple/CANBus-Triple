/*
*  CANBus Triple
*  The Car Hacking Platform
*  https://canb.us
*  https://github.com/CANBus-Triple
*/

#include <avr/wdt.h>
#include <SPI.h>
#include <EEPROM.h>
#include <CANBus.h>
#include <MessageQueue.h>


// #define INCLUDE_DEFAULT_EEPROM
#define SLEEP_ENABLE

#define BUILDNAME "CANBus Triple Mazda"
#ifdef HAS_AUTOMATIC_VERSIONING
    #include "_Version.h"
#else
    #define BUILD_VERSION "0.7.0"
#endif

#define READ_BUFFER_SIZE 20
#define WRITE_BUFFER_SIZE 10


CANBus CANBus1(CAN1SELECT, CAN1RESET, 1, "Bus 1");
CANBus CANBus2(CAN2SELECT, CAN2RESET, 2, "Bus 2");
CANBus CANBus3(CAN3SELECT, CAN3RESET, 3, "Bus 3");
CANBus busses[] = { CANBus1, CANBus2, CANBus3 };

#include "Middleware.h"
#include "Settings.h"
#include "AutoBaud.h"
#include "WheelButton.h"
#include "MazdaWheelButton.h"
#include "ChannelSwap.h"
#include "SerialCommand.h"
#include "ServiceCall.h"
#include "MazdaLED.h"
#include "Naptime.h"

Message readBuffer[READ_BUFFER_SIZE];
Message writeBuffer[WRITE_BUFFER_SIZE];
MessageQueue readQueue(READ_BUFFER_SIZE, readBuffer);
MessageQueue writeQueue(WRITE_BUFFER_SIZE, writeBuffer);

/*
*  Middleware Setup
*  http://docs.canb.us/firmware/main.html
*/
SerialCommand *serialCommand = new SerialCommand( &writeQueue );
ServiceCall *serviceCall = new ServiceCall( &writeQueue );
#ifdef SLEEP_ENABLE
Naptime *naptime = new Naptime(0x0472);
#endif
MazdaLED *mazdaLed = new MazdaLED( &writeQueue );

Middleware *activeMiddleware[] = {
  serialCommand,
  new ChannelSwap(),
  mazdaLed,
  serviceCall,
#ifdef SLEEP_ENABLE
  naptime,
#endif
  new MazdaWheelButton(mazdaLed, serviceCall)
};
int activeMiddlewareLength = (int)( sizeof(activeMiddleware) / sizeof(activeMiddleware[0]) );


void setup()
{
    Settings::init();
    delay(1);

    /*
    *  Middleware Settings
    */
#ifdef SLEEP_ENABLE
    // Set a command callback to enable disable sleep (4E01 on 4E00 off)
    serialCommand->registerCommand(0x4E, 1, naptime);
#endif  
    serialCommand->registerCommand(0x16, 64, mazdaLed);

    mazdaLed->enabled = cbt_settings.displayEnabled;
    serviceCall->setFilterPids();

    Serial.begin( 115200 ); // USB
    Serial1.begin( 57600 ); // UART

    /*
    *  Power LED
    */
    DDRE |= B00000100;
    PORTE |= B00000100;

    /*
    *  BLE112 Init
    */
    pinMode( BT_SLEEP, OUTPUT );
    digitalWrite( BT_SLEEP, HIGH ); // Keep BLE112 Awake

    /*
    *  Boot LED
    */
    pinMode( BOOT_LED, OUTPUT );

    pinMode( CAN1INT_D, INPUT );
    pinMode( CAN2INT_D, INPUT );
    pinMode( CAN3INT_D, INPUT );
    pinMode( CAN1RESET, OUTPUT );
    pinMode( CAN2RESET, OUTPUT );
    pinMode( CAN3RESET, OUTPUT );
    pinMode( CAN1SELECT, OUTPUT );
    pinMode( CAN2SELECT, OUTPUT );
    pinMode( CAN3SELECT, OUTPUT );

    digitalWrite(CAN1RESET, LOW);
    digitalWrite(CAN2RESET, LOW);
    digitalWrite(CAN3RESET, LOW);


    // Setup CAN Busses
    for (int b = 0; b < 3; b++) {
        busses[b].begin(); // Resets bus and puts it in CONTROL mode
        busses[b].setClkPre(1);
        busses[b].baudConfig(cbt_settings.busCfg[b].baud);
        busses[b].setRxInt(true);
        busses[b].bitModify(RXB0CTRL, 0x04, 0x04); // Set buffer rollover enabled
        busses[b].disableFilters();
        busses[b].setMode(cbt_settings.busCfg[b].mode);
    }

    for(int l = 0; l < 5; l++) {
        digitalWrite( BOOT_LED, HIGH );
        delay(50);
        digitalWrite( BOOT_LED, LOW );
        delay(50);
    }

    // wdt_enable(WDTO_1S);
}


/*
*  Main Loop
*/
void loop() 
{
    // Run all middleware ticks
    for(int i = 0; i <= activeMiddlewareLength - 1; i++) 
        activeMiddleware[i]->tick();

    if (digitalRead(CAN1INT_D) == 0) readBus(CANBus1);
    if (digitalRead(CAN2INT_D) == 0) readBus(CANBus2);
    if (digitalRead(CAN3INT_D) == 0) readBus(CANBus3);

    // Process received CAN message through middleware
    if (!readQueue.isEmpty()) {
        Message msg = readQueue.pop();
        for(int i = 0; i <= activeMiddlewareLength - 1; i++) msg = activeMiddleware[i]->process(msg);
        if (msg.dispatch) writeQueue.push(msg);
    }

    if (!writeQueue.isEmpty()) {
        Message msg = writeQueue.pop();

        // When TX Failure, add back to queue
        if (msg.dispatch && !sendMessage(msg, busses[msg.busId - 1])) writeQueue.push(msg);
    }

    // Pet the dog
    // wdt_reset();

} // End loop()


/*
*  Load CAN Controller buffer and set send flag
*/
bool sendMessage( Message msg, CANBus bus )
{
    int ch = bus.getNextTxBuffer();
    if (ch < 0 || ch > 2) return false; // All TX buffers full

    digitalWrite(BOOT_LED, HIGH);
    bus.loadFullFrame(ch, msg.length, msg.frame_id, msg.frame_data );
    bus.transmitBuffer(ch);
    digitalWrite(BOOT_LED, LOW );

    return true;
}


/*
*  Read Can Controller Buffer
*/
void readBus( CANBus bus )
{
    byte rx_status = bus.readStatus();
    if (rx_status & 0x1) readMsgFromBuffer(bus, 0, rx_status);
    if (rx_status & 0x2) readMsgFromBuffer(bus, 1, rx_status);
}


bool readMsgFromBuffer(CANBus bus, byte bufferId, byte rx_status)
{
    Message * m = readQueue.write(bus.busId, rx_status);
    if (m == NULL) return false;
    bus.readFullFrame(bufferId, &m->length, m->frame_data, &m->frame_id);
    return true;
}
