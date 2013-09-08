
#include <avr/eeprom.h>

struct pids {
  byte txd[6];
  byte rxf[6];
  byte rxd[6];
  byte mth[6];
  char name[6];
} pids[4];

struct cbt_settings {
  byte firstboot;
  byte displayEnabled;
  byte placeholder2;
  byte placeholder3;
  byte placeholder4;
  byte placeholder5;
  byte placeholder6;
  byte placeholder7;
  struct pids pids;
} cbt_settings;


class Settings
{
  public:
   static void init();
   static void save();
   static void clear();
   static void firstbootSetup();
};

void Settings::init()
{
  eeprom_read_block((void*)&cbt_settings, (void*)0, sizeof(cbt_settings));
  if( cbt_settings.firstboot == 0 )
    Settings::firstbootSetup();
}


void Settings::save()
{
  eeprom_write_block((const void*)&cbt_settings, (void*)0, sizeof(cbt_settings));
}

void Settings::clear()
{
  for (int i = 0; i < 512; i++)
    EEPROM.write(i, 0);
}

void Settings::firstbootSetup()
{
  cbt_settings.firstboot = 1;
  cbt_settings.displayEnabled = 1;
}


