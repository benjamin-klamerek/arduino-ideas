#include <FMTX.h>
#include <LiquidCrystal_I2C.h>


volatile float frequency = 90;  //default FM frequency

LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);

void setup(void)
{
  delay(3000);

  Serial.begin(9600);
  Serial.print("FM-TX Demo\r\n");
  fmtx_init(frequency, EUROPE);
  Serial.print("Channel:");
  Serial.print(frequency, 1);
  Serial.println("MHz");

  lcd.init();
  lcd.backlight();
}

void loop(void)
{
  int reading = analogRead(A3);
  int mappingReading = map(reading, 0, 1024, 7000, 11000);
  mappingReading = constrain(mappingReading, 7000, 10800);
  
  float newFrequency = round(mappingReading / 100.f * 2 ) / 2.;


  if (frequency != newFrequency) {
    lcd.setCursor(0, 0);
    lcd.print(mappingReading);

    lcd.setCursor(8, 0);
    lcd.print("RAW");

    lcd.setCursor(0, 1);
    lcd.print(newFrequency);

    lcd.setCursor(8, 1);
    lcd.print("APPLIED");

    frequency = newFrequency;
    fmtx_set_freq(frequency);
  }


  delay(100);
}
