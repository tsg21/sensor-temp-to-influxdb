#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#define LED_PIN 13

volatile int f_wdt=1;

const byte wifiRxPin = 3; // Wire this to Tx Pin of ESP8266
const byte wifiTxPin = 2; // Wire this to Rx Pin of ESP8266

#define ONE_WIRE_BUS 8
#define LED_PIN 13

// We'll use a software serial interface to connect to ESP8266
SoftwareSerial ESP8266 (wifiRxPin, wifiTxPin);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);


void setup() {
  
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);

  Serial.println("Start...");
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  delay(100);
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);

  delay(1000); // Let the modules self-initialize

  setupForSleep();
  
  ESP8266.begin(9600); 
  sensors.begin();
  
  Serial.println("Sending AT command...");
  // actual sending AT command
  ESP8266.println("AT");
  readResponseFromESP8266();

  // display output

  // what wifi are you connected to?
  ESP8266.println("AT+CWJAP?");
  readResponseFromESP8266();

  Serial.print("Locating temperature devices... Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  sensors.setResolution(0, 11);

  // close any existing connection - just in case
  ESP8266.println("AT+CIPCLOSE");
  readResponseFromESP8266();

  ESP8266.println("AT+CIPSTART=\"UDP\",\"34.248.64.57\",8089");
  readResponseFromESP8266();

}

int count = 0;
char buffer[256];
char command[256];

void loop() {
  digitalWrite(LED_PIN, HIGH);
  
  // read the temperature
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);

  int idx = sprintf(buffer, "weather temp=");
  dtostrf(temp, 0, 2, buffer+idx);

  Serial.println(buffer);
  
  sprintf(command, "AT+CIPSEND=%i", strlen(buffer));

  ESP8266.println(command);
  readResponseFromESP8266();
  
  ESP8266.print(buffer);
  readResponseFromESP8266();

  digitalWrite(LED_PIN, LOW);

  for (int i=0; i<1; i++) sleepFor8s();
}

void readResponseFromESP8266() {
  while (ESP8266.available()){
     String inData = ESP8266.readStringUntil('\n');
     Serial.println("ESP8266: " + inData);
  }

}



void sleepFor8s() {
  f_wdt = 0;
  
  // Maximum power saving
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  cbi(ADCSRA,ADEN);  // switch Analog to Digitalconverter OFF
 
  // Set sleep enable (SE) bit:
  sleep_enable();
 
  // Put the device to sleep:
  sleep_mode();
 
  // Upon waking up, sketch continues from this point.
  sleep_disable();
  sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
}

//
ISR(WDT_vect)
{
  if(f_wdt == 0)
  {
    f_wdt=1;
  }
  else
  {
    //Serial.println("WDT Overrun!!!");
  }
}

void setupForSleep() {

  /* Clear the reset flag. */
  MCUSR &= ~(1<<WDRF);
  
  /* In order to change WDE or the prescaler, we need to
   * set WDCE (This will allow updates for 4 clock cycles).
   */
  WDTCSR |= (1<<WDCE) | (1<<WDE);

  /* set new watchdog timeout prescaler value */
  WDTCSR = 1<<WDP0 | 1<<WDP3; /* 8.0 seconds */
  
  /* Enable the WD interrupt (note no reset). */
  WDTCSR |= _BV(WDIE);
  
}

