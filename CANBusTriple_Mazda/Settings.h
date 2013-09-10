
#include <avr/eeprom.h>

struct pid {
  byte busId;
  byte txd[8];
  byte rxf[8];
  byte rxd[8];
  byte mth[8];
  char name[8];
};

struct cbt_settings {
  byte firstboot;
  byte displayEnabled;
  byte placeholder2;
  byte placeholder3;
  byte placeholder4;
  byte placeholder5;
  byte placeholder6;
  byte placeholder7;
  struct pid pids[4];
} cbt_settings;


class Settings
{
  public:
   static void init();
   static void save( struct cbt_settings settings );
   static void clear();
   static void firstbootSetup();
};

void Settings::init()
{
  eeprom_read_block((void*)&cbt_settings, (void*)0, sizeof(cbt_settings));
  if( cbt_settings.firstboot == 0 )
    Settings::firstbootSetup();
}


void Settings::save( struct cbt_settings settings )
{
  eeprom_write_block((const void*)&settings, (void*)0, sizeof(settings));
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
    1, // firstboot
    1, // displayEnabled
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
        { 0x07, 0xE0, 0x02, 0x01, 0x3C, 0x00, 0x00, 0x00 },   /* TXD */
        { 0x04, 0x41, 0x05, 0x3c, 0x00, 0x00, 0x00, 0x00 },   /* RXF */
        { 0x28, 0x01 },                                       /* RXD */
        { 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00 },   /* MTH */
        { 0x45, 0x47, 0x54 }                                  /* NAM */
      },
      {
        // AFR
        2,
        { 0x07, 0xDF, 0x02, 0x01, 0x34, 0x00, 0x00, 0x00 },   /* TXD */
        { 0x04, 0x41, 0x45, 0x34, 0x00, 0x00, 0x00, 0x00 },   /* RXF */
        { 0x28, 0x08 },                                       /* RXD */
        { 0x05, 0xB9, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00 },   /* MTH */
        { 0x41, 0x46, 0x00 }                                  /* NAM */
      },
      {},
      {}
    }
  };
  
  Settings::save(stockSettings);
  
  // Slow flash to show first boot successful
  for(int i=0;i<10;i++){
    digitalWrite( 7, HIGH );
    delay(500);
    digitalWrite( 7, LOW );
    delay(500);
  }
  
  
}


