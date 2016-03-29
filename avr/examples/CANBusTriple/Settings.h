

#include <avr/eeprom.h>
#include <CANBus.h>

struct pid {
  byte busId;
  byte settings; // unused, unused, unused, unused, unused, unused, unused, add decimal flag
  unsigned int value;
  byte txd[8];
  byte rxf[6];
  byte rxd[2];
  byte mth[6];
  char name[8];
};

struct busConfig {
  int baud;
  CANMode mode;
};

struct cbt_settings {
  byte displayEnabled;
  byte firstboot;
  byte displayIndex;
  struct busConfig busCfg[3];  // 4bytes x 3 = 12bytes
  byte hwselftest;
  byte placeholder4;
  byte placeholder5;
  byte placeholder6;
  byte placeholder7;
  struct pid pids[8];  // 34bytes x 8 pids = 272bytes
  byte padding[220];  // 512bytes - 292 bytes
} cbt_settings;


class Settings
{
  public:
   static void init();
   static void save( struct cbt_settings *settings );
   static void clear();
   static void firstbootSetup();
   const static int pidLength = 8;
   static void setBaudRate(byte busId, int rate);
   static int getBaudRate(byte busId);
   static void setCanMode(byte busId, int mode);
   static CANMode getCanMode(byte busId);
};


void Settings::init()
{
  memset(&cbt_settings, 0, sizeof(cbt_settings));
  eeprom_read_block((void*)&cbt_settings, (void*)0, sizeof(cbt_settings));
  if( cbt_settings.firstboot == 0 || cbt_settings.firstboot == 0xFF )
    Settings::firstbootSetup();
}


void Settings::save( struct cbt_settings *settings )
{
  eeprom_write_block((const void*)settings, (void*)0, sizeof(cbt_settings));
}

void Settings::setBaudRate(byte busId, int rate){
  if( (busId < 1 || busId > 3) || rate < 1 ) return;

  cbt_settings.busCfg[busId-1].baud = rate;
  save(&cbt_settings);
}

int Settings::getBaudRate(byte busId){
  if( (busId < 1 || busId > 3)) return 0;

  return cbt_settings.busCfg[busId-1].baud;
}

void Settings::setCanMode(byte busId, int mode){
  if( (busId < 1 || busId > 3)) return;

  cbt_settings.busCfg[busId-1].mode = (CANMode) mode;
  save(&cbt_settings);
}

CANMode Settings::getCanMode(byte busId){
  if( (busId < 1 || busId > 3)) return UNKNOWN;

  return cbt_settings.busCfg[busId-1].mode;
}


void Settings::clear()
{
  for (int i = 0; i < 512; i++)
    EEPROM.write(i, 0);
}

void Settings::firstbootSetup()
{

  Settings::clear();

  struct cbt_settings stockSettings = {

    #ifdef INCLUDE_DEFAULT_EEPROM
    
    1, // displayEnabled
    1, // firstboot
    0, // displayIndex
    {
      { 125, NORMAL },
      { 125, NORMAL },
      { 125, NORMAL }
    },
    0, // hwselftest
    0, // placeholder4
    0, // placeholder5
    0, // placeholder6
    0, // placeholder7
    {
      {
        // EGT
        2,
        B00000000, // Setting flags
        0, // Value
        { 0x07, 0xE0, 0x01, 0x3C, 0x00, 0x00, 0x00, 0x00 },   /* TXD */
        { 0x04, 0x41, 0x05, 0x3c, 0x00, 0x00 },               /* RXF */
        { 0x28, 0x10 },                                       /* RXD */
        { 0x00, 0x01, 0x00, 0x0A, 0xFF, 0xD8 },               /* MTH */
        { 0x45, 0x47, 0x54, 0x20, 0x20, 0x20, 0x20, 0x20 }    /* NAM */
      },
      {
        // AFR
        2,
        B00000001, // Setting flags
        0, // Value
        { 0x07, 0xE0, 0x01, 0x34, 0x00, 0x00, 0x00, 0x00 },   /* TXD */
        { 0x04, 0x41, 0x05, 0x34, 0x00, 0x00 },   /* RXF */
        { 0x28, 0x10 },                                       /* RXD */
        // { 0x00, 0x0F, 0x80, 0x00, 0x00, 0x00 },            /* MTH */
        // { 0x00, 0x0F, 0x83, 0xD7, 0x00, 0x00 },            /* MTH */ // ?? Calibrated
        { 0x00, 0x0F, 0x0D, 0x20, 0x00, 0x00 },   /* MTH */
        { 0x41, 0x46, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20 }    /* NAM */
      },
      {
      // AFR from OBD II PID
        2,
        B00000001, // Setting flags
        0, // Value
        { 0x07, 0xE0, 0x22, 0xDA, 0x85, 0x00, 0x00, 0x00 },   /* TXD */
        { 0x04, 0x62, 0x05, 0xDA, 0x06, 0x85 },               /* RXF */
        { 0x30, 0x08 },                                       /* RXD */
        { 0x00, 0x17, 0x00, 0x14, 0x00, 0x00 },               /* MTH */
        { 0x41, 0x46, 0x52, 0x4F, 0x42, 0x44, 0x20, 0x20 }    /* NAM */
      },
      {
        // KNOCK
        2,
        B00000000, // Setting flags
        0, // Value
        { 0x07, 0xE0, 0x22, 0x03, 0xEC, 0x00, 0x00, 0x00 },   /* TXD */
        { 0x04, 0x62, 0x05, 0x03, 0x06, 0xEC },               /* RXF */
        { 0x30, 0x10 },                                       /* RXD */
        { 0x00, 0x01, 0x00, 0x05, 0x00, 0x00 },               /* MTH */
        { 0x4b, 0x4e, 0x4f, 0x43, 0x4b, 0x20, 0x20, 0x20 }    /* NAM */
      },
      {
        // Fuel Pressure at rail
        2,
        B00000000, // Setting flags
        0, // Value
        { 0x07, 0xE0, 0x22, 0xF4, 0x23, 0x00, 0x00, 0x00 },   /* TXD */
        { 0x04, 0x62, 0x05, 0xF4, 0x06, 0x23 },               /* RXF */
        { 0x30, 0x10 },                                       /* RXD */
        { 0x00, 0x1D, 0x00, 0x14, 0x00, 0x00 },               /* MTH */
        { 0x46, 0x50, 0x52, 0x20, 0x4b, 0x20, 0x20, 0x20 }    /* NAM */
      },
      {
        // VAR CAM TIMING
        2,
        B00000000, // Setting flags
        0, // Value
        { 0x07, 0xE0, 0x22, 0x03, 0x18, 0x00, 0x00, 0x00 },   /* TXD */
        { 0x04, 0x62, 0x05, 0x03, 0x06, 0x18 },               /* RXF */
        { 0x30, 0x10 },                                       /* RXD */
        { 0x00, 0x06, 0x00, 0x01, 0x00, 0x00 },               /* MTH */
        { 0x43, 0x41, 0x4d, 0x44, 0x45, 0x47, 0x20, 0x20 }    /* NAM */
      },
      {
        // Passenger Weight
        2,
        B00000000, // Setting flags
        0, // Value
        { 0x07, 0x37, 0x22, 0x59, 0x6A, 0x00, 0x00, 0x00 },   /* TXD */
        { 0x04, 0x62, 0x05, 0x59, 0x06, 0x6A },               /* RXF */
        { 0x30, 0x10 },                                       /* RXD */
        { 0x00, 0x0B, 0x00, 0x64, 0x00, 0x00 },               /* MTH */
        { 0x50, 0x57, 0x45, 0x49, 0x47, 0x48, 0x54, 0x20 }    /* NAM */
      },
      {
        // Battery
        2,
        B00000000, // Setting flags
        0, // Value
        { 0x07, 0xE0, 0x22, 0x03, 0xCA, 0x00, 0x00, 0x00 },   /* TXD */
        { 0x04, 0x62, 0x05, 0x03, 0x06, 0xCA },               /* RXF */
        { 0x30, 0x08 },                                       /* RXD */
        { 0x00, 0x09, 0x00, 0x5, 0xFF, 0xD8 },               /* MTH */
        { 0x42, 0x41, 0x54, 0x54, 0x45, 0x52, 0x59, 0x20 }    /* NAM */
      }
    },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } // padding for future changes

    #endif
  
  };

  Settings::save(&stockSettings);
  Settings::init();
  Serial.println( F("{\"event\":\"eepromReset\", \"result\":\"success\"}" ));

  // Slow flash to show first boot successful
  for(int i=0;i<6;i++){
    digitalWrite( BOOT_LED, HIGH );
    delay(500);
    digitalWrite( BOOT_LED, LOW );
    delay(500);
  }

}
