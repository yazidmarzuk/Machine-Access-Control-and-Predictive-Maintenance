#include <EEPROM.h>    
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <SPI.h>
#include <Wire.h>
MFRC522 mfrc522(10, 9); 
LiquidCrystal_I2C lcd(0x3F, 16, 2);
Servo myServo;
constexpr int greenLed = 7;
constexpr int blueLed = 6;
constexpr int redLed = 5;
constexpr int ServoPin = 8;
constexpr int wipeB = 3;     
boolean match = false;          
boolean programMode = false;  
boolean replaceMaster = false;
int successRead;    
byte storedCard[4];   
byte readCard[4];   
byte masterCard[4];   
void setup() {
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  pinMode(wipeB, INPUT_PULLUP); 
  digitalWrite(redLed, LOW);
  digitalWrite(greenLed, LOW);
  digitalWrite(blueLed, LOW);
  
  lcd.begin();  
  lcd.backlight();
  SPI.begin();           
  mfrc522.PCD_Init();    
  myServo.attach(ServoPin);  
  myServo.write(10);   
  
  //mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
  ShowReaderDetails(); 
    if (digitalRead(wipeB) == LOW) {  
    digitalWrite(redLed, HIGH); 
    lcd.setCursor(0, 0);
    lcd.print("Button Pressed");
    delay(1000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("This will remove");
    lcd.setCursor(0, 1);
    lcd.print("all records");
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("You have 10 ");
    lcd.setCursor(0, 1);
    lcd.print("secs to Cancel");
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Unpress to cancel");
    lcd.setCursor(0, 1);
    lcd.print("Counting: ");
    bool buttonState = monitorWipeButton(10000);
    if (buttonState == true && digitalRead(wipeB) == LOW) { 
      lcd.print("Wiping EEPROM...");
      for (int x = 0; x < EEPROM.length(); x = x + 1) {   
        if (EEPROM.read(x) == 0) {            
                  }
        else {
          EEPROM.write(x, 0);       
        }
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Wiping Done");
      digitalWrite(redLed, LOW);
      delay(200);
      digitalWrite(redLed, HIGH);
      delay(200);
      digitalWrite(redLed, LOW);
      delay(200);
      digitalWrite(redLed, HIGH);
      delay(200);
      digitalWrite(redLed, LOW);
    }
    else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Wiping Cancelled"); 
      digitalWrite(redLed, LOW);
    }
  }
   if (EEPROM.read(1) != 143) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No Master Card ");
    lcd.setCursor(0, 1);
    lcd.print("Defined");
    delay(2000);
    lcd.setCursor(0, 0);
    lcd.print("Scan A Tag to ");
    lcd.setCursor(0, 1);
    lcd.print("Define as Master");
    do {
      successRead = getID();           
      digitalWrite(blueLed, HIGH);
      delay(200);
      digitalWrite(blueLed, LOW);
      delay(200);
    }
    while (!successRead);                  
    for ( int j = 0; j < 4; j++ ) {    
      EEPROM.write( 2 + j, readCard[j] );  
    }
    EEPROM.write(1, 143);                  
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Master Defined");
    delay(2000);
  }
  for ( int i = 0; i < 4; i++ ) {      
    masterCard[i] = EEPROM.read(2 + i);    
  }
  ShowOnLCD();    
  cycleLeds();    
}
void loop () {
  do {
    successRead = getID();  
    if (programMode) {
      cycleLeds();             
    }
    else {
      normalModeOn();     
    }
  }
  while (!successRead);   
  if (programMode) {
    if ( isMaster(readCard) ) { 
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Exiting Program Mode");
      delay(1000);
      ShowOnLCD();
      programMode = false;
      return;
    }
    else {
      if ( findID(readCard) ) { 
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Already there");
        deleteID(readCard);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Tag to ADD/REM");
        lcd.setCursor(0, 1);
        lcd.print("Master to Exit");
      }
      else {                    
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("New Tag,adding...");
        writeID(readCard);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Scan to ADD/REM");
        lcd.setCursor(0, 1);
        lcd.print("Master to Exit");
      }
    }
  }
  else {
    if ( isMaster(readCard)) {    
      programMode = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Program Mode");
      int count = EEPROM.read(0);   
      lcd.setCursor(0, 1);
      lcd.print("I have ");
      lcd.print(count);
      lcd.print(" records");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Scan a Tag to ");
      lcd.setCursor(0, 1);
      lcd.print("ADD/REMOVE");
    }
    else {
      if ( findID(readCard) ) {       
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Access Granted");
        granted();        
        myServo.write(10);
        ShowOnLCD();
      }
      else {      
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Access Denied");
        denied();
        ShowOnLCD();
      }
    }
  }
}
void granted() {
  digitalWrite(blueLed, LOW);   
  digitalWrite(redLed, LOW);  
  digitalWrite(greenLed, HIGH);   
  myServo.write(90);
  
  delay(6000);
}
void denied() {
  digitalWrite(greenLed, LOW);  
  digitalWrite(blueLed, LOW);   
  digitalWrite(redLed, HIGH);   
}
int getID() {

  if ( ! mfrc522.PICC_IsNewCardPresent()) { 
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   
    return 0;
  }
  
  for ( int i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
  }
  mfrc522.PICC_HaltA();
  return 1;
}
void ShowReaderDetails() {
 
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    if ((v == 0x00) || (v == 0xFF)) {
    lcd.setCursor(0, 0);
    lcd.print("Communication Failure");
    lcd.setCursor(0, 1);
    lcd.print("Check Connections");
   
    digitalWrite(greenLed, LOW);  
    digitalWrite(blueLed, LOW);   
    digitalWrite(redLed, HIGH);
    while (true); 
  }
}
void cycleLeds() {
  digitalWrite(redLed, LOW);  
  digitalWrite(greenLed, HIGH);
  digitalWrite(blueLed, LOW);
  delay(200);
  digitalWrite(redLed, LOW); 
  digitalWrite(greenLed, LOW);
  digitalWrite(blueLed, HIGH);
  delay(200);
  digitalWrite(redLed, HIGH);   
  digitalWrite(greenLed, LOW);  
  digitalWrite(blueLed, LOW);   
  delay(200);
}
void normalModeOn () {
  digitalWrite(blueLed, HIGH);  
  digitalWrite(redLed, LOW);  
  digitalWrite(greenLed, LOW);
}
void readID( int number ) {
  int start = (number * 4 ) + 2;    
  for ( int i = 0; i < 4; i++ ) {   
    storedCard[i] = EEPROM.read(start + i);
  }
}
void writeID( byte a[] ) {
  if ( !findID( a ) ) {
    int num = EEPROM.read(0);
    int start = ( num * 4 ) + 6;
    num++;                
    EEPROM.write( 0, num );
    for ( int j = 0; j < 4; j++ ) { 
      EEPROM.write( start + j, a[j] );  
    }
    BlinkLEDS(greenLed);
    lcd.setCursor(0, 1);
    lcd.print("Added");
    delay(1000);
  }
  else {
    BlinkLEDS(redLed);
    lcd.setCursor(0, 0);
    lcd.print("Failed!");
    lcd.setCursor(0, 1);
    lcd.print("wrong ID or bad EEPROM");
    delay(2000);
  }
}

void deleteID( byte a[] ) {
  if ( !findID( a ) ) {
    BlinkLEDS(redLed); 
    lcd.setCursor(0, 0);
    lcd.print("Failed!");
    lcd.setCursor(0, 1);
    lcd.print("wrong ID or bad EEPROM");
    delay(2000);
  }
  else {
    int num = EEPROM.read(0); 
    int slot;       
    int start;      
    int looping;    
    int j;
    int count = EEPROM.read(0); 
    slot = findIDSLOT( a );   
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;      
    EEPROM.write( 0, num );   
    for ( j = 0; j < looping; j++ ) {         
      EEPROM.write( start + j, EEPROM.read(start + 4 + j));   
    }
    for ( int k = 0; k < 4; k++ ) {         
      EEPROM.write( start + j + k, 0);
    }
    BlinkLEDS(blueLed);
    lcd.setCursor(0, 1);
    lcd.print("Removed");
    delay(1000);
  }
}
boolean checkTwo ( byte a[], byte b[] ) {
  if ( a[0] != 0 )      
    match = true;      
  for ( int k = 0; k < 4; k++ ) {  
    if ( a[k] != b[k] )     
      match = false;
  }
  if ( match ) {      
    return true;    
  }
  else  {
    return false;    
  }
}
int findIDSLOT( byte find[] ) {
  int count = EEPROM.read(0);       
  for ( int i = 1; i <= count; i++ ) {    
    readID(i);                
    if ( checkTwo( find, storedCard ) ) {  
      
      return i;        
      break;          
    }
  }
}
boolean findID( byte find[] ) {
  int count = EEPROM.read(0);     
  for ( int i = 1; i <= count; i++ ) { 
    readID(i);          
    if ( checkTwo( find, storedCard ) ) {
      return true;
      break;
    }
    else { 
    }
  }
  return false;
}
void BlinkLEDS(int led) {
  digitalWrite(blueLed, LOW);
  digitalWrite(redLed, LOW);
  digitalWrite(greenLed, LOW);
  delay(200);
  digitalWrite(led, HIGH);
  delay(200);
  digitalWrite(led, LOW);
  delay(200);
  digitalWrite(led, HIGH);
  delay(200);
  digitalWrite(led, LOW); 
  delay(200);
  digitalWrite(led, HIGH);
  delay(200);
}

boolean isMaster( byte test[] ) {
  if ( checkTwo( test, masterCard ) )
    return true;
  else
    return false;
}
bool monitorWipeButton(uint32_t interval) {
  unsigned long currentMillis = millis();
  while (millis() - currentMillis < interval)  {
    int timeSpent = (millis() - currentMillis) / 1000;
    Serial.println(timeSpent);
    lcd.setCursor(10, 1);
    lcd.print(timeSpent);
    if (((uint32_t)millis() % 10) == 0) {
      if (digitalRead(wipeB) != LOW) {
        return false;
      }
    }
  }
  return true;
}
void ShowOnLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Access Control");
  lcd.setCursor(0, 1);
  lcd.print("   Scan a Tag");
}
