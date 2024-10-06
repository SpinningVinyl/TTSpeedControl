#include <AD9833.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

/* Encoder pins */
#define ENC_A 2 // D2
#define ENC_B 4 // D4
#define ENC_SW 3 // D3

/* LCD pins */
#define LCD_D7 14 // A0
#define LCD_D6 15 // A1
#define LCD_D5 16 // A2
#define LCD_D4 17 // A3
#define LCD_E  19 // A5
#define LCD_RS 5 // D5
#define LCD_V0 18 // A4

/* Frequency generator pins */
#define DATA 11 // D11
#define CLK 13 // D13
#define FSYNC 10 // D10

float currentFrequency;
float frequency45 = 67.5;
float frequency33 = 50;
float minFrequency = 30;
float maxFrequency = 90;
unsigned long int lastUpdateTime = 0;
int updateDelay = 5000;
boolean is45 = false;
boolean buttonPress = false;
boolean goingUp = false;
boolean goingDown = false;

LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
AD9833 ad(FSYNC);

void setup()
{
  ad.Begin();
  //Serial prints for debugging and testing
  Serial.begin(9600);
  lcd.begin(16,2);
  analogWrite(LCD_V0, 100);
  lcd.print("Please wait...");
  delay(2000);
  lcd.clear();
/* Setup encoder pins as inputs */
  pinMode(ENC_A, INPUT_PULLUP); // Pin 2
  pinMode(ENC_B, INPUT_PULLUP); // Pin 4 
  pinMode(ENC_SW, INPUT_PULLUP); // Pin 3

// encoder pin on interrupt 0 (digital pin 2)
  attachInterrupt(digitalPinToInterrupt(ENC_A), decoder, FALLING);
// switch on digital pin 3
  attachInterrupt(digitalPinToInterrupt(ENC_SW), changeState, FALLING);
  reset();
  currentFrequency = frequency33;
  ad.ApplySignal(SINE_WAVE,REG0,currentFrequency);
  ad.EnableOutput(true);
}

void loop()
{
//using while statement to stay in the loop for continuous
//interrupts
  while(goingUp==1 && currentFrequency < maxFrequency - 0.1) // CW motion in the rotary encoder
  {
    goingUp=0; // Reset the flag
    currentFrequency += 0.1;
    Serial.println(currentFrequency);
    if (is45) {
      frequency45 = currentFrequency;
    } else {
      frequency33 = currentFrequency;
    }
  }

  while(goingDown==1 && currentFrequency > minFrequency + 0.1) // CCW motion in rotary encoder
  {
    goingDown=0; // clear the flag
    currentFrequency -= 0.1;
    Serial.println(currentFrequency);
    if (is45) {
      frequency45 = currentFrequency;
    } else {
      frequency33 = currentFrequency;
    }
   }

   while(buttonPress) {
    buttonPress = 0;
    is45 = !is45;
    if (is45) {
      Serial.println("45");
      currentFrequency = frequency45;
    } else {
      Serial.println("33");
      currentFrequency = frequency33;
    }
   }

   ad.ApplySignal(SINE_WAVE,REG0,currentFrequency);
   lcd.setCursor(0,0);
   if (is45) {
      lcd.print("45 RPM  ");
   } else {
      lcd.print("33.3 RPM");
   }
   lcd.setCursor(0,1);
   lcd.print(currentFrequency);
   lcd.print(" Hz");
   saveSettings();
}

void changeState() {
  // button on 
  if (digitalRead(ENC_SW) == LOW) {
    buttonPress = 1;
  }
}

void decoder()
//very short interrupt routine 
//Remember that the routine is only called when ENC_A
//changes state, so it's the value of ENC_B that we're
//interested in here
{
  if (digitalRead(ENC_A) == digitalRead(ENC_B))
  {
    goingUp = 1; //if encoder channels are the same, direction is CW
  }
  else
  {
    goingDown = 1; //if they are not the same, direction is CCW
  }
}

void reset()
{
  if (EEPROM.read(0) == 80 && EEPROM.read(1) == 85) {
    EEPROM.get(33, frequency33);
    EEPROM.get(45, frequency45);
    if (frequency33 < minFrequency || frequency33 > maxFrequency) {
      frequency33 == 50;
    }
    if (frequency45 < minFrequency || frequency45 > maxFrequency) {
      frequency45 == 67.5;
    }
    
  }
  else {
    Serial.println("Clearing CMOS and resetting data");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Initial setup...");
    delay(updateDelay);
    eepromClear();
    EEPROM.write(0, 80);
    EEPROM.write(1, 85);
    saveSettings();
  }
}

void eepromClear() { 
  // filling the built-in EEPROM with 0s
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
}

void saveSettings() {
  float temp33;
  float temp45;
  // reading current values from EEPROM
  EEPROM.get(33, temp33);
  EEPROM.get(45, temp45);
  // if more than 5s since last update...
  if (millis() - lastUpdateTime >= updateDelay) {
    //if current frequency for 33 != saved frequency, then save
    if (frequency33 != temp33) {
      Serial.println("Saving 33");
      EEPROM.put(33, frequency33); 
    }
    // same for 45
    if (frequency45 != temp45) {
      Serial.println("Saving 45");
      EEPROM.put(45, frequency45);
    }
    //update lastUpdateTime
    lastUpdateTime = millis();
  }
}
