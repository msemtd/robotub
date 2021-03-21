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
* 
