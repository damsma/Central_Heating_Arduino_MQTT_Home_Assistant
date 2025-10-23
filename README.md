# Disclaimer

This code is provided "as is", without warranty of any kind. It has not been fully tested and may contain bugs or unintended behavior. Use at your own risk.

The author is not responsible for any damage or loss caused by the use of this code or hardware setup.

# Central_Heating_Arduino_MQTT_Home_Assistant (Ethernet)
MQTT-based central heating control system for Home Assistant. Supports managing underfloor heating, radiators, boiler, DHW circulation, and any other devices controllable via relays.

https://github.com/damsma/Central_Heating_Arduino_MQTT_Home_Assistant

# Screenshots

<img width="1059" height="581" alt="grafik" src="https://github.com/user-attachments/assets/812cb675-1c5d-415a-9fcd-096cdb5041d5" />


# How to use the code
1. Download newest .ino file
2. Install Arduino IDE
3. Install Libraries in Arduino IDE:
     - Watchdog 3.0.2 by Peter Polidoro
     - Ethernet 2.0.0 by Arduino
     - home-assistant-integration 2.0.0 by Dawid Chyrzynski

## define your relays
Change the declaration in the .ino file.

For example:
```
Relays myRelays[] = {
    {47, LOW, "Circuit 1", nullptr, "", 1},
    {48, LOW, "Circuit 2", nullptr, "", 1},
    {49, LOW, "Circuit 3", nullptr, "", 1},
    {A15, LOW, "Circuit 4", nullptr, "", 1},
    {A14, LOW, "Circuit 5", nullptr, "", 1},
    {A13, LOW, "Circuit 6", nullptr, "", 1},
    {A12, LOW, "Circuit 7", nullptr, "", 1},
    {A11, LOW, "Circuit 8", nullptr, "", 1},
    {A10, LOW, "Circuit 9", nullptr, "", 1},
    {A9, LOW, "Circuit 10", nullptr, "", 1},
    {A8, LOW, "Circuit 11", nullptr, "", 1},
    {A7, LOW, "Circuit 12", nullptr, "", 1},
    {A6, LOW, "Circuit 13", nullptr, "", 1},
    {A5, LOW, "Circuit 14", nullptr, "", 1},
    {A4, LOW, "Water circulation", nullptr, "", 0},
    {A3, LOW, "Gas boiler", nullptr, "", 0},
    ...
```

## define your gas boiler relay
Change the declaration in the .ino file.

This relay will be automatically turned ON when one of the other relays which has trigger_boiler set to 1 (the last value in the relay definition) will be turned ON by Home Assistant.

For example:

```
#define GAS_BOILER_ID 15 //set this to the index of your gas boiler relay, that you defined in myRelays, beginning from 0, so line 16 will be the value of 15
```


## define your network settings
Open NetworkConfig.h and change the values to match your network.

```
#define MYIPADDR 192,168,1,177
#define MYIPMASK 255,255,255,0
#define MYDNS 192,168,1,1
#define MYGW 192,168,1,1

...

byte mac[] = {0x00, 0x10, 0xF1, 0x6E, 0x01, 0x88};
```

Define the MQTT broker address

```
#define BROKER_ADDR     IPAddress(192,168,1,188)
```

Depending on your hardware you may choose one line by uncommenting it, this will change the pin of the network adapter:

```
//#define Standard_w5500 1
#define Easyswitch_with_w5500 1
//#define Pico_with_w5500 1
```

## upload your code to your Arduino Mega 2560

Make sure you check if everything works fine before connecting to anything.

## automations

Now that you can see your relay switches in Home Assistant, you will need to create an automation based on your needs. Simply set the automation to turn on the desired relay to start heating. The Arduino will then automatically activate the relay for your gas boiler.



# Other requirements

## Software
You may need a working installation of Home Assistant (open source home automation system) with the MQTT integration enabled, as well as Mosquitto (open source MQTT broker).

## Hardware
Tested on Arduino Mega 2560 with W5100.
W5500 will also work.

The code is prepared for the Wiznet W5500-EVB-Pico, but I have not tested it yet on this project. There's also the voltage difference (5V vs 3.3V) that needs to be considered and the custm board definition has to be installed in the Arduino IDE.

You may need some sort of a din board, for example https://sklep.easyswitch.pl/produkt/es-1-04-atmega-din-board-ethernet-w5500/ 

And some relays.
