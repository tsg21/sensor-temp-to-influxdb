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
#define ESP8266_PWR 7
#define LED_PIN 13

// We'll use a software serial interface to connect to ESP8266
SoftwareSerial esp8266 (wifiRxPin, wifiTxPin);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Expecting to have two temp sensors...
byte addrA[8];
byte addrB[8];

void setup() {
  
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  pinMode(ESP8266_PWR, OUTPUT);
  digitalWrite(ESP8266_PWR, HIGH);

  Serial.println("Start...");
  flashes(2);
  delay(5000); // Let everything self-initialize
  flashes(4);

  setupForSleep();

  pinMode(wifiRxPin, INPUT);
  pinMode(wifiTxPin, OUTPUT);
  esp8266.begin(9600); 
  sensors.begin();
  
  sendAndRead("AT");
//  sendAndRead("AT+RST"); // Restart yourself...

  Serial.print("Locating temperature devices... Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  sensors.setResolution(11);
  oneWire.reset_search();
  oneWire.search(addrA);
  oneWire.search(addrB);
  oneWire.reset_search();
  printAddress("addrA", addrA);
  printAddress("addrB", addrB);

  // wait for the wifi module to connect
  delay(10000);

  // what wifi are you connected to?
  sendAndRead("AT+CWJAP?");
  delay(500);

  digitalWrite(ESP8266_PWR, LOW);
  
  sleepFor8s();
}

int count = 0;
char buffer[256];
char command[256];

void loop() {
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(ESP8266_PWR, HIGH);
  delay(5000);
  sendAndRead("AT");
  sendAndRead("AT+CIPSTART=\"UDP\",\"34.248.64.57\",8089");
  
  // read the temperature
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);

  int idx = sprintf(buffer, "weather temp=");
  dtostrf(temp, 0, 2, buffer+idx);

//  Serial.println(buffer);
  
  sprintf(command, "AT+CIPSEND=%i", strlen(buffer));

  sendAndRead(command);
  
  esp8266.print(buffer);
  readResponseFromESP8266();

  digitalWrite(LED_PIN, LOW);
  sendAndRead("AT+CIPCLOSE");
  digitalWrite(ESP8266_PWR, LOW);
  
  delay(100); //alow the serial write to complete

  for (int i=0; i<5; i++) sleepFor8s();
}

void sendAndRead(char * toSend) {
  Serial.print("SEND: ");
  Serial.println(toSend);
  esp8266.println(toSend);
  readResponseFromESP8266();
}

void readResponseFromESP8266() {
  do {
    String inData = esp8266.readStringUntil('OK\r\n');
    Serial.print("ESP8266: [");
    Serial.print(inData);
    Serial.println("]");
  } while(esp8266.available());
}



void sleepFor8s() {
  f_wdt = 0;
  
  // Maximum power saving
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  cbi(ADCSRA,ADEN);  // switch Analog to Digitalconverter OFF
 
  // Set sleep enable (SE) bit:
  sleep_enable();

  //disable brown-out detection while sleeping (20-25µA)
  sleep_bod_disable();
 
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

void flashes(int count) {
  for (int i=0; i<count; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
}

void printAddress(char* name, const uint8_t* deviceAddress) {
  Serial.print(name);
  Serial.print(": ");
  for (int i=0; i<8; i++) {
    Serial.print(deviceAddress[i], HEX);
  }
  Serial.println();
}

