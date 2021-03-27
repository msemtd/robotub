/*******************************************
 *   _____       _        _______    _     *
 *  |  __ \     | |      |__   __|  | |    *
 *  | |__) |___ | |__   ___ | |_   _| |__  *
 *  |  _  // _ \| '_ \ / _ \| | | | | '_ \ *
 *  | | \ \ (_) | |_) | (_) | | |_| | |_) |*
 *  |_|  \_\___/|_.__/ \___/|_|\__,_|_.__/ *
 *      /_ |/ _ \ / _ \ / _ \ / _ \        *
 *       | | | | | | | | | | | | | |       *
 *       | | | | | | | | | | | | | |       *
 *       | | |_| | |_| | |_| | |_| |       *
 *       |_|\___/ \___/ \___/ \___/        *
 *                                         *
 ******************************************/
//
// Currently building for...
// d1_mini ESP8266
//
#include <Arduino.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <OneWire.h>
#include <DallasTemperature.h> // https://github.com/milesburton/Arduino-Temperature-Control-Library

bool blinkVal = true;
#define TAP_HOT D1     // GPIO5
#define TAP_COLD D2 // GPIO4
// NB: GPIO0 (D3) is pulled high during normal operation, so you can’t use it as a Hi-Z input
// See https://tttapa.github.io/ESP8266/Chap04%20-%20Microcontroller.html
// NB: GPIO2 (D4) MUST REMAIN HIGH at boot
#define FLOAT_SWITCH D5 // GPIO14
#define ONE_WIRE_BUS D6 // GPIO12

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

WiFiUDP udp;
const char *ntpServerName = "time.nist.gov";
IPAddress timeServerIP;             // time.nist.gov NTP server address
const int NTP_PACKET_SIZE = 48;     // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing NTP packets

IPAddress broadcast = IPAddress(233, 252, 18, 18);
const int broadcastPort = 9912;
char broadcastBuf[256]; // UDP broadcasting packet

String serialInputString = ""; // a String to hold incoming serial data
bool serialMsgReady = false;   // whether the string is complete
String errString = "";

/*
  The resolution of the temperature sensor is user-configurable to
  9, 10, 11, or 12 bits, corresponding to increments of 0.5C,
  0.25C, 0.125C, and 0.0625C, respectively.
  The default resolution at power-up is 12-bit.
  */
int resolution = 11;
float temperature = 0.0;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, (blinkVal ? HIGH : LOW));
  pinMode(TAP_HOT, OUTPUT);
  digitalWrite(TAP_HOT, LOW);
  pinMode(TAP_COLD, OUTPUT);
  digitalWrite(TAP_COLD, LOW);
  pinMode(FLOAT_SWITCH, INPUT_PULLUP);

  Serial.begin(115200);
  serialInputString.reserve(200);

  sensors.begin();
  DeviceAddress tempDeviceAddress;
  sensors.getAddress(tempDeviceAddress, 0);
  sensors.setResolution(tempDeviceAddress, resolution);
  sensors.setWaitForConversion(false);

  WiFiManager wm;
  // wipe credentials for testing
  // wm.resetSettings();
  bool res = wm.autoConnect("RoboTub10000", "robotub4919"); // password protected ap
  if (!res)
  {
    Serial.println("Failed to connect");
    // ESP.restart();
  }
  else
  {
    Serial.println("connected :)");
  }
}

void blinky()
{
  static unsigned long last = 0;
  unsigned long now = millis();
  if (now - last < 500)
    return;
  blinkVal = !blinkVal;
  digitalWrite(LED_BUILTIN, (blinkVal ? HIGH : LOW));
  last = now;
}

void doTemperature()
{
  static unsigned long last = 0;
  unsigned long now = millis();
  // first time round...
  if(last == 0) {
    sensors.requestTemperatures();
    last = now;
    return;
  }
  unsigned long delayInMillis = 750 / (1 << (12 - resolution));
  if(now - last < delayInMillis)
    return;
  temperature = sensors.getTempCByIndex(0);
  Serial.print(" Temperature: ");
  Serial.println(temperature);
  sensors.requestTemperatures();
  last = millis();
}

// Hmm, serialEvent doesn't work on ESP8622 so we gotta call it in the loop
void serialEventNotOnESP()
{
  while (Serial.available())
  {
    char inChar = (char)Serial.read();
    serialInputString += inChar;
    if (inChar == '\n')
    {
      serialMsgReady = true;
      return; // allow immediate processing
    }
  }
}

void procCommand(const String &msg)
{
  Serial.print(msg);
  if (msg.startsWith("TAP_HOT_"))
  {
    digitalWrite(TAP_HOT, (msg[8] == '1' ? HIGH : LOW));
    Serial.println("OK :)");
  }
  else if (msg.startsWith("TAP_COLD_"))
  {
    digitalWrite(TAP_COLD, (msg[9] == '1' ? HIGH : LOW));
    Serial.println("OK :)");
  }
  else
  {
    Serial.println("dunno that :)");
  }
}

inline void procSerialMsg()
{
  if (!serialMsgReady)
    return;
  serialInputString.trim();
  procCommand(serialInputString);
  serialInputString = "";
  serialMsgReady = false;
}


void sendBroadcastUDP()
{
    udp.beginPacketMulticast(broadcast, broadcastPort, WiFi.localIP(), 4);
    udp.write(broadcastBuf);
    udp.endPacket();
}

void doBroadcast()
{
  static unsigned long last = 0;
  unsigned long now = millis();
  if (now - last < 600)
    return;
  last = now;
  Serial.write(broadcastBuf, strlen(broadcastBuf));
  sendBroadcastUDP();
}

void loop()
{
  int floatVal = digitalRead(FLOAT_SWITCH);
  serialEventNotOnESP();
  blinky();
  doTemperature();
  errString = (temperature == DEVICE_DISCONNECTED_C) ? "TEMP-FAIL" : "TEMP-OK";
  sprintf(broadcastBuf, "Temp: %.2fC, Float: %d, Hot: ?, Cold: ?, Status: %s\n", temperature, floatVal, errString.c_str());
  doBroadcast();
  delay(100);
}
