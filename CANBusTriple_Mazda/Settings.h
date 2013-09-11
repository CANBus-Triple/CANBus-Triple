
#include <avr/eeprom.h>

struct pid {
  byte busId;
  byte settings; // for variables like dispatch
  unsigned int value;
  byte txd[8];
  byte rxf[8];
  byte rxd[8];
  byte mth[8];
  char name[8];
};

struct cbt_settings {
  byte displayEnabled;
  byte firstboot;
  byte placeholder2;
  byte placeholder3;
  byte placeholder4;
  byte placeholder5;
  byte placeholder6;
  byte placeholder7;
  struct pid pids[8];
} cbt_settings;


class Settings
{
  public:
   static void init();
   static void save( struct cbt_settings *settings );
   static void clear();
   static void firstbootSetup();
   static int pidLength;
};

// Static int for number of PIDs stored in settings
int Settings::pidLength = 8;

void Settings::init()
{
  eeprom_read_block((void*)&cbt_settings, (void*)0, sizeof(cbt_settings));
  if( cbt_settings.firstboot == 0 )
    Settings::firstbootSetup();
}


void Settings::save( struct cbt_settings *settings )
{
  eeprom_write_block((const void*)settings, (void*)0, sizeof(cbt_settings));
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
    1, // displayEnabled
    1, // firstboot
    0, // placeholder2
    0, // placeholder3
    0, // placeholder4
    0, // placeholder5
    0, // placeholder6
    0, // placeholder7
    {
      {
        // EGT 
        2,
        0,
        666,
        { 0x07, 0xE0, 0x01, 0x3C, 0x00, 0x00, 0x00, 0x00 },   /* TXD */
        { 0x02, 0x41, 0x03, 0x3c, 0x00, 0x00, 0x00, 0x00 },   /* RXF */
        { 0x28, 0x10 },                                       /* RXD */
        { 0x00, 0x01, 0x00, 0x0A, 0xFF, 0xD8 },               /* MTH */
        { 0x45, 0x47, 0x54 }                                  /* NAM */
      },
      {
        // AFR
        2,
        0,
        66,
        { 0x07, 0xE0, 0x01, 0x34, 0x00, 0x00, 0x00, 0x00 },   /* TXD */
        { 0x02, 0x41, 0x03, 0x34, 0x00, 0x00, 0x00, 0x00 },   /* RXF */
        { 0x28, 0x10 },                                       /* RXD */
        //         { 0x00, 0x0F, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 },   /* MTH */
        { 0x00, 0x0F, 0x01, 0x48, 0x00, 0x00, 0x00, 0x00 },   /* MTH */
        { 0x41, 0x46, 0x00 }                                  /* NAM */
      },
      {},
      {},
      {},
      {},
      {},
      {}
    }
  };
  
  Settings::save(&stockSettings);
  
  // Slow flash to show first boot successful
  for(int i=0;i<6;i++){
    digitalWrite( 7, HIGH );
    delay(500);
    digitalWrite( 7, LOW );
    delay(500);
  }
  
  
}


