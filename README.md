# robotub
RoboTub 10000 - bath preparation automation

# ロボット浴槽10000
Robotto yokusō 10000

![Robotto yokusō](text881.png)

I was enthralled by videos of Japanese automated bathtubs and whilst I am unable to afford such a wonderful consumer item I am able to mess around with modern cheap electronics. My initial aim was to automatically stop the taps when the bath was full. It wouldn't be much more trouble to control the volume of hot and cold water to reach a target temperature. Adding WiFi control is almost trivial with inexpensive ESP8266-based micros! And a project like this keeps me off the streets.

Initial hardware tests are promising: I'm currently using components from online hacker stores, eBay sellers, and high street stores here in the UK.
* The solenoid valves I bought from The Pi Hut here in the UK (£6.50 each https://thepihut.com/products/plastic-water-solenoid-valve-12v-1-2-nominal) and they work well with my hot tap pressure. My water is heated by an ageing gas combi boiler that is quite typical in the UK. I followed instructions from BC Robotics (https://bc-robotics.com/tutorials/controlling-a-solenoid-valve-with-arduino/) using Darlington transistors (TIP120) to switch 12v from the micro.
* The tap connectors are from Wilkos (£3 each https://www.wilko.com/en-uk/wilko-mixer-watering-tap-connector/p/0298812) and are joined to the solenoids with brass 3/4" BSP Female-Female connectors (£1.44 each from Willbond's plumbers merchants)
* The Dallas DS18B20 sensor (£2.49 eBay) is quick to respond to being warmed up, say, by a hand, but when the heat is removed it is slow to cool down again. I'm not sure why this is but I'm gonna go with it since it's cheap and it may be quick enough for this application
* The float switch (£3.49 eBay) is pretty neat and just kinda works.


## PlatformIO
Now ported from an Arduino sketch to PlatformIO (https://platformio.org/)

## ESP8266 D1 Mini Clone
Now running on the very inexpensive D1 Mini: a clone of Wemos D1 Mini R2; board "LOLIN(WEMOS) D1 R2 & mini" in Arduino and board ID "d1_mini" in PlatformIO. Bought for £3.65 from a UK eBay seller but equivalent to this on AliExpress: https://www.aliexpress.com/item/32630518881.html

* NB: This board can only use 2.4GHz networks (same as NodeMCU v1.0)
* Hardware docs for esp8266 modules: https://docs.ai-thinker.com/en/esp8266
* docs for esp8266 Arduino https://arduino-esp8266.readthedocs.io/en/latest/reference.html


```
Pin	Function	ESP-8266 Pin
TX	TXD	TXD
RX	RXD	RXD
A0	Analog input, max 3.3V input	A0
D0	IO	GPIO16
D1	IO, SCL	GPIO5
D2	IO, SDA	GPIO4
D3	IO, 10k Pull-up	GPIO0
D4	IO, 10k Pull-up, BUILTIN_LED	GPIO2
D5	IO, SCK	GPIO14
D6	IO, MISO	GPIO12
D7	IO, MOSI	GPIO13
D8	IO, 10k Pull-down, SS	GPIO15
G	Ground	GND
5V	5V	-
3V3	3.3V	3.3V
RST	Reset	RST
```

Arduino15\packages\esp8266\hardware\esp8266\2.7.4\variants\d1_mini\pins_arduino.h
```
static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t RX   = 3;
static const uint8_t TX   = 1;
```
## NTP to set the clock

On previous projects I was doing way too much work messing around crafting my own NTP packets and decoding responses when it turns out time on the ESP8266 can be set automagically internally with NTP by adding just a couple of lines of code! Here's a sequence of improvements I followed...

* https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/NTPClient/NTPClient.ino
* https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/NTP-TZ-DST/NTP-TZ-DST.ino
* https://werner.rothschopf.net/202011_arduino_esp8266_ntp_en.htm

My timezone: Europe/London GMT0BST,M3.5.0/1,M10.5.0

## Multicast UDP messages

I've been messing with UDP multicast for various projects around the house so I can identify and monitor devices without having to know their IP addresses. I have a nice NodeJS module to listen for these messages.

## TCP Server

A simple socket server running on a chosen port or could use a HTTP server and expose a simple web interface.

Tried a few things and found that the WiFiManager (https://github.com/tzapu/WiFiManager)
doesn't play nice with async webserver and 
khoih-prog/ESPAsync_WiFiManager just doesn't work reliably on my D1 mini for some reason.
So using the simplest possible webserver and a websocket conenction to hand out the current status information.

## Add-on temperature display

I do like 7-Segment displays! There's not many pins on the D1 Mini so perhaps use an additional cheap micro like a ProMini to do that for me and drive it over the serial.
Mine is labelled SMA420564
https://wokwi.com/arduino/libraries/SevSeg/SevSeg_Counter
