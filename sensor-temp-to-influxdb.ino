#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const byte wifiRxPin = 3; // Wire this to Tx Pin of ESP8266
const byte wifiTxPin = 2; // Wire this to Rx Pin of ESP8266

#define ONE_WIRE_BUS 8

// We'll use a software serial interface to connect to ESP8266
SoftwareSerial ESP8266 (wifiRxPin, wifiTxPin);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);


void setup() {
  
  Serial.begin(9600);

  Serial.println("Start...");
  delay(1000); // Let the modules self-initialize
  
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
    
  delay(60000);
}

void readResponseFromESP8266() {
  while (ESP8266.available()){
     String inData = ESP8266.readStringUntil('\n');
     Serial.println("ESP8266: " + inData);
  }

}

