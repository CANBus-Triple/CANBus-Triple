

#ifndef SelfTest_H
#define SelfTest_H

class SelfTest
{
  public:
    SelfTest(){};
    ~SelfTest(){};
    
    static void run();
    static boolean inerruptTriggered;
    static boolean btInterruptTest();
    static void handleInterrupt1();
    static boolean CANLoopback();
    static boolean AIO();
    static void flashError(int);
    static void flashSuccess(int);
    
    
};

boolean SelfTest::inerruptTriggered = false;

/* Slave Analog Setup
*/
/*
pinMode(A0, OUTPUT);
pinMode(A1, OUTPUT);
pinMode(A4, OUTPUT);
pinMode(A5, OUTPUT);
digitalWrite(A0, HIGH);
digitalWrite(A1, HIGH);
digitalWrite(A4, HIGH);
digitalWrite(A5, HIGH);
*/


void SelfTest::run(){
  
  // Flash start
  for( byte i=0; i<10; i++ ){
      PORTE |= B00000100;
      delay(100);
      PORTE &= B00000000;
      delay(100);
    }
  
  digitalWrite( BT_SLEEP, HIGH ); // Keep BLE112 Awake
  
  delay(500);
  
  Serial.println("Hardware Self Test Start");
  
  boolean result = false;
  
  // Run tests  
  result = btInterruptTest();
  
  if( SelfTest::inerruptTriggered == false ){
    Serial.println("BLE112 -> CAN INT 1 FAILURE");
    flashError(1);
  }else{
    //flashSuccess(1);
  }
  
  if( result )
    Serial.println("BLE112 UART Pass");
  else
    flashError(2);
  
  // Test CAN Busses with Slave CBT attached
  result = SelfTest::CANLoopback();
  if(result){
    // flashSuccess(5);
  }else{
    flashError(5);
  }
  
  // Test all analog IO is high
  result = SelfTest::AIO();
  if(result){
    flashSuccess(7);
  }else{
    flashError(7);
  }
  

}

boolean SelfTest::btInterruptTest(){

  SelfTest::inerruptTriggered = false;
  attachInterrupt(CAN1INT, SelfTest::handleInterrupt1, LOW);
  delay(50);
  Serial1.write(0xE8);
  Serial1.write(0x84);
  delay(200);
  Serial1.write(0xE8);
  Serial1.write(0x84);
  delay(200);
  
  if(Serial1.available()){
    while(Serial1.available()) Serial.write(Serial1.read());
    Serial.println();
    return true;
  }else{
    Serial.println("No data returned from BLE112");
    return false;
  }
  
}

void SelfTest::handleInterrupt1(){
  Serial.println("CAN1 Interrupt Triggered");
  SelfTest::inerruptTriggered = true;
  detachInterrupt(CAN1INT);
}


boolean SelfTest::CANLoopback(){
  
  CANBus *bus;
  
  for( int i = 0; i<3; i++ ){
    
    for( byte ii=0; ii<i; ii++ ){
      PORTE |= B00000100;
      delay(100);
      PORTE &= B00000000;
      delay(100);
    }
    
    bus = &busses[i];
    
    Serial.print("Testing bus ");
    Serial.println(bus->busId);
    
    Message msg;
    msg.length = 8;
    msg.frame_id = 0x0290;
    msg.frame_data[0] = 0x80;
    msg.frame_data[1] = 0x80;
    msg.frame_data[2] = 0x80;
    msg.frame_data[3] = 0x80;
    msg.frame_data[4] = 0x80;
    msg.frame_data[5] = 0x80;
    msg.frame_data[6] = 0x80;
    msg.frame_data[7] = 0x80;
    
    bus->load_ff_0( msg.length, msg.frame_id, msg.frame_data );
    bus->send_0();
    delay(200);
    
    byte stat = bus->readStatus();
    Serial.println(stat, BIN);
    
    // read response
    if( (stat & 0x1) == 0x1 || (stat & 0x2) == 0x2 ){
      Message msg_in;
      msg_in.busId = bus->busId;
      
      if( (stat & 0x1) == 0x1 )
        bus->readDATA_ff_0( &msg_in.length, msg_in.frame_data, &msg_in.frame_id );
      
      if( (stat & 0x2) == 0x2 )
        bus->readDATA_ff_1( &msg_in.length, msg_in.frame_data, &msg_in.frame_id );
      
      Serial.print( "Return frame id " );
      Serial.println( msg_in.frame_id, HEX );
      
      if( msg_in.frame_id != 0x0298 ){
        Serial.println( "Bad Packet FAIL" );
        flashError(10+i);
      }
      
    }else{
      Serial.println( "No Packet FAIL" );
      flashError(10+i);
    }
    
  }
  
  return true;
  
}


boolean SelfTest::AIO(){
  
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);
  
  Serial.println("Testing Analog IO");
  
  if( digitalRead(A0) != HIGH ){
    Serial.println("A0 FAIL");
    return false;
  }
  
  if( digitalRead(A1) != HIGH ){
    Serial.println("A1 FAIL");
    return false;
  }
  
  if( digitalRead(A4) != HIGH ){
    Serial.println("A4 FAIL");
    return false;
  }
  
  if( digitalRead(A5) != HIGH ){
    Serial.println("A5 FAIL");
    return false;
  }
  
  Serial.println("Analog PASS");
  
  return true;
  
}


void SelfTest::flashError(int ii){
  
  PORTE &= B00000000;
  
  for( byte i=0; i<ii; i++ ){
      digitalWrite(BOOT_LED, HIGH);
      delay(300);
      digitalWrite(BOOT_LED, LOW);
      delay(300);
    }
    
  digitalWrite(BOOT_LED, HIGH);
  while(1){
    delay(1);
  }
  
}

void SelfTest::flashSuccess(int ii){
  
  // Write pass to settings
  cbt_settings.hwselftest = 1;
  Settings::save( &cbt_settings );
  
  for( byte i=0; i<ii; i++ ){
      PORTE |= B00000100;
      delay(300);
      PORTE &= B00000000;
      delay(300);
    }
  PORTE |= B00000100;
  
  while(1){
    delay(1);
  }
  
}




#endif
