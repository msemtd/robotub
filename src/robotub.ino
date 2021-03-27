
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
// NodeMCU 1.0 (ESP-12E Module) https://github.com/nodemcu/nodemcu-devkit-v1.0
//

#include <OneWire.h>
#include <DallasTemperature.h> // https://github.com/milesburton/Arduino-Temperature-Control-Library
#include <WiFiUdp.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

bool blinkVal = false;
#define TAP_HOT D0
#define TAP_COLD D1
#define ONE_WIRE_BUS D2
// NB: GPIO0 (D3) is pulled high during normal operation, so you can’t use it as a Hi-Z input
// See https://tttapa.github.io/ESP8266/Chap04%20-%20Microcontroller.html
// NB: GPIO2 (D4) MUST REMAIN HIGH at boot
#define FLOAT_SWITCH D5

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
    WiFi.mode(WIFI_STA);
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

void loop()
{
    int floatVal = digitalRead(FLOAT_SWITCH);
    // Get temperature - TODO don't wait here for result - see lib docs
    sensors.requestTemperatures();
    float tempC = sensors.getTempCByIndex(0);
    errString = (tempC == DEVICE_DISCONNECTED_C) ? "TEMP-FAIL" : "TEMP-OK";
    sprintf(broadcastBuf, "Temp: %.2fC, Float: %d, Hot: ?, Cold: ?, Err: %s\n", tempC, floatVal, errString.c_str());
    Serial.write(broadcastBuf, strlen(broadcastBuf));
    blinkVal = !blinkVal;
    digitalWrite(LED_BUILTIN, (blinkVal ? HIGH : LOW));
    serialEventNotOnESP();
    procSerialMsg();
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

inline void procSerialMsg()
{
    if (!serialMsgReady)
        return;
    serialInputString.trim();
    procCommand(serialInputString);
    serialInputString = "";
    serialMsgReady = false;
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

void sendBroadcastUDP()
{
    udp.beginPacketMulticast(broadcast, broadcastPort, WiFi.localIP(), 4);
    udp.write(broadcastBuf);
    udp.endPacket();
}
