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

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <WebSocketsServer.h>

#include <OneWire.h>
#include <DallasTemperature.h> // https://github.com/milesburton/Arduino-Temperature-Control-Library
#include <PID_v1.h>
#include <time.h>
#include <string.h>
#include <Wire.h>
#include "Adafruit_MCP9808.h"



bool blinkVal = true;
#define TMPC_SCL D1  // GPIO5
#define TMPC_SDA D2 // GPIO4
// NB: GPIO0 (D3) is pulled high during normal operation, so you can’t use it as a Hi-Z input
// See https://tttapa.github.io/ESP8266/Chap04%20-%20Microcontroller.html
// NB: GPIO2 (D4) MUST REMAIN HIGH at boot
#define FLOAT_SWITCH D5 // GPIO14
#define ONE_WIRE_BUS D6 // GPIO12
#define TAP_HOT D7  // GPIO13
#define TAP_COLD D8 // GPIO15

String serialInputString = ""; // a String to hold incoming serial data
bool serialMsgReady = false;   // whether the string is complete

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
/*
  The resolution of the temperature sensor is user-configurable to
  9, 10, 11, or 12 bits, corresponding to increments of 0.5C,
  0.25C, 0.125C, and 0.0625C, respectively.
  The default resolution at power-up is 12-bit.
  */
int resolution = 11;
float temperature = 0.0;

Adafruit_MCP9808 mpcsensor = Adafruit_MCP9808();
float mpctemperature = -127.0;

/* Configuration of NTP */
#define MY_NTP_SERVER "time.nist.gov"
#define MY_TZ "GMT0BST,M3.5.0/1,M10.5.0" // Europe/London

WiFiUDP udp;

IPAddress broadcast = IPAddress(233, 252, 18, 18);
const int broadcastPort = 9912;
char broadcastBuf[256]; // UDP broadcasting packet
String errString = "";

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Serving a web page (from flash memory)
// formatted as a string literal!
char webpage[] PROGMEM = R"=====(
<html>
  <!-- Adding a data chart using Chart.js -->
  <head>
    <link rel="icon" href="https://raw.githubusercontent.com/msemtd/robotub/main/media/favicon.ico">
    <!-- <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/3.3.1/chart.js"></script> -->
    <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/3.3.1/chart.min.js"></script>
    <script>
      var webSocket, dataPlot
      var maxDataPoints = 20
      function removeData() {
        dataPlot.data.labels.shift()
        dataPlot.data.datasets[0].data.shift()
      }
      function addData(label, data) {
        if (dataPlot.data.labels.length > maxDataPoints) removeData()
        dataPlot.data.labels.push(label)
        dataPlot.data.datasets[0].data.push(data)
        dataPlot.update()
      }
      function startWebsock() {
        webSocket = new WebSocket('ws://' + window.location.hostname + ':81/')
        webSocket.onmessage = function (event) {
          try {
            const data = JSON.parse(event.data)
            const now = new Date()
            const t = now.toLocaleTimeString('en-GB')
            // var t = today.getHours() + ':' + today.getMinutes() + ':' + today.getSeconds()
            addData(t, data.value)
          } catch (error) {
            console.error('hmm, ws message not good', error)
          }
        }
      }
      function init() {
        dataPlot = new Chart(document.getElementById('line-chart'), {
          type: 'line',
          data: {
            labels: [],
            datasets: [
              {
                data: [],
                label: 'Temperature (C)',
                borderColor: '#3e95cd',
                fill: false,
              },
            ],
          },
          options: {
            animation: false,
            tension: 0,
            scales: {
              y: {
                type: 'linear',
                min: 15,
                max: 50,
              },
            },
          },
        })
        startWebsock()
      }
      function sendDataRate() {
        var dataRate = document.getElementById('dataRateSlider').value
        webSocket.send(dataRate)
        dataRate = 1.0 / dataRate
        document.getElementById('dataRateLabel').innerHTML = 'Rate: ' + dataRate.toFixed(2) + 'Hz'
      }
      function sendTapControl(tap, control) {
        webSocket.send('TAP_' + tap + '_' + control)
      }
    </script>
  </head>
  <body onload="javascript:init()">
    <div id="hdn">RoboTub 10000</div>
    <div>
      <input type="range" min="1" max="10" value="5" id="dataRateSlider" oninput="sendDataRate()" />
      <label for="dataRateSlider" id="dataRateLabel">Rate: 0.2Hz</label>
      <input type="button" value="HOT ON" onclick="sendTapControl('HOT', '1')" />
      <input type="button" value="HOT OFF" onclick="sendTapControl('HOT', '0')" />
      <input type="button" value="COLD ON" onclick="sendTapControl('COLD', '1')" />
      <input type="button" value="COLD OFF" onclick="sendTapControl('COLD', '0')" />
    </div>
    <hr />
    <div style="width: 640px; height: 480px;">
      <canvas id="line-chart"></canvas>
    </div>
  </body>
</html>

)=====";

void procCommand(const String &msg);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  // Do something with the data from the client
  if (type == WStype_TEXT)
  {
    if (!strncmp((const char *)payload, "TAP_", 4)) {
      String cmd((const char *)payload);
      Serial.print("websocket tap control ");
      Serial.println(cmd);
      procCommand(cmd);
      return;
    }
    float dataRate = (float)atof((const char *)&payload[0]);
    Serial.print("websocket says change data rate to ");
    Serial.println(dataRate);
  }
}

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
  configTime(MY_TZ, MY_NTP_SERVER);

  sensors.begin();
  DeviceAddress tempDeviceAddress;
  sensors.getAddress(tempDeviceAddress, 0);
  sensors.setResolution(tempDeviceAddress, resolution);
  sensors.setWaitForConversion(false);

  if(!mpcsensor.begin()) {
    Serial.println("Couldn't find MCP9808! Check your connections and verify the address is correct.");
  }
  mpcsensor.setResolution(3); // sets the resolution mode of reading, the modes are defined in the table bellow:

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
  // setupWebServer();
  server.on("/", []() {
    server.send_P(200, "text/html", webpage);
  });
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
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
  if (last == 0)
  {
    sensors.requestTemperatures();
    last = now;
    return;
  }
  unsigned long delayInMillis = 750 / (1 << (12 - resolution));
  if (now - last < delayInMillis)
    return;
  temperature = sensors.getTempCByIndex(0);
  // Serial.print(" Temperature: ");
  // Serial.println(temperature);
  sensors.requestTemperatures();
  last = millis();
}

void doMpcTemperature()
{
  static unsigned long last = 0;
  unsigned long now = millis();
  if (now - last < 400)
    return;
  mpcsensor.wake();
  mpctemperature = mpcsensor.readTempC();
  // Serial.print(" MPC Temperature: ");
  // Serial.println(mpctemperature);
  mpcsensor.shutdown_wake(1);
  last = millis();
}

// Hmm, serialEvent doesn't work on ESP8622 so we gotta call it in the loop.
// Read up to end of a single message then yield to allow message processing.
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
  if (now - last < 990)
    return;
  last = now;
  Serial.write(broadcastBuf, strlen(broadcastBuf));
  sendBroadcastUDP();
  // websocket stuff...
  String json = "{\"value\":";
  json += temperature;
  json += "}";
  webSocket.broadcastTXT(json.c_str(), json.length());
}

void loop()
{
  int floatVal = digitalRead(FLOAT_SWITCH);
  serialEventNotOnESP();
  blinky();
  doTemperature();
  doMpcTemperature();
  errString = (temperature == DEVICE_DISCONNECTED_C) ? "TEMP-FAIL" : "TEMP-OK";
  sprintf(broadcastBuf, "Temp: %.2fC, Temp2: %.2fC, Float: %d, Hot: ?, Cold: ?, Status: %s\n", temperature, mpctemperature, floatVal, errString.c_str());
  server.handleClient();
  webSocket.loop();
  doBroadcast();

  delay(100);
}
