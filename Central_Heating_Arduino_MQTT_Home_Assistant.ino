#include <Ethernet.h>
#include <ArduinoHA.h>
#include <Watchdog.h>
#include <EEPROM.h>

#define DEBUG_MODE

//#define ARDUINOHA_DEBUG
#define EX_ARDUINOHA_BUTTON
#define EX_ARDUINOHA_DEVICE_TRIGGER
#define EX_ARDUINOHA_HVAC
#define EX_ARDUINOHA_NUMBER
#define EX_ARDUINOHA_SELECT

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
#define IRQ_HANDLER_ATTR ICACHE_RAM_ATTR
#else
#define IRQ_HANDLER_ATTR
#endif

#define BROKER_ADDR     IPAddress(192,168,1,188)
// Set the static IP address to use if the DHCP fails to assign
#define MYIPADDR 192,168,1,177
#define MYIPMASK 255,255,255,0
#define MYDNS 192,168,1,1
#define MYGW 192,168,1,1
#define RELAY_ON LOW // GPIO value to write to turn on attached relay
#define RELAY_OFF HIGH // GPIO value to write to turn off attached relay

byte mac[] = {0x00, 0x10, 0xF1, 0x6E, 0x01, 0x88};
IPAddress ip(MYIPADDR);
IPAddress dns(MYDNS);
IPAddress gw(MYGW);
IPAddress sn(MYIPMASK);

EthernetClient client;
HADevice device(mac, sizeof(mac));
HAMqtt mqtt(client, device, 50);

unsigned long lastReadAt = millis();
bool gasBoilerON = false;
bool gasBoilerONact = false;
bool mqttConnected = false;
static bool initial_state_sent = false;

typedef struct {
  int relay;
  uint8_t relayTrigger;
  const char* name;
  HASwitch* switchObj;
  char id_name[20];
  int trigger_boiler;
} Relays;

// CONFIGURE ONLY THIS ARRAY!
// Row params: relay pin, relay trigger [HIGH|LOW]
#define GAS_BOILER_ID 15 //set this to the index of your gas boiler relay, that you defined in myRelays, beginning from 0, so line 16 will be the value of 15
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

    {46, LOW, "Circuit 17", nullptr, "", 1},
    {45, LOW, "Circuit 18", nullptr, "", 1},
    {44, LOW, "Circuit 19", nullptr, "", 1},
    {43, LOW, "Circuit 20", nullptr, "", 1},
    {42, LOW, "Circuit 21", nullptr, "", 1},
    {41, LOW, "Circuit 22", nullptr, "", 1},
    {40, LOW, "Circuit 23", nullptr, "", 1},
    {39, LOW, "Circuit 24", nullptr, "", 1},
    {38, LOW, "Circuit 25", nullptr, "", 1},
    {37, LOW, "Circuit 26", nullptr, "", 1},
    
    {36, LOW, "Circuit 27", nullptr, "", 0},
    {35, LOW, "Circuit 28", nullptr, "", 0},
    {34, LOW, "Circuit 29", nullptr, "", 0},
    {33, LOW, "Circuit 30", nullptr, "", 0},
    {32, LOW, "Circuit 31", nullptr, "", 0},
    {31, LOW, "Circuit 32", nullptr, "", 0},

    {14, HIGH, "Main shutoff valve", nullptr, "", 0},
    
    /*{15, LOW, "Circuit 34", nullptr, "", 0},
    {16, LOW, "Circuit 35", nullptr, "", 0},
    {17, LOW, "Circuit 36", nullptr, "", 0},
    {18, LOW, "Circuit 37", nullptr, "", 0},
    {19, LOW, "Circuit 38", nullptr, "", 0},
    {20, LOW, "Circuit 39", nullptr, "", 0},
    {21, LOW, "Circuit 40", nullptr, "", 0},*/
};

const int numberOfRelays = sizeof(myRelays) / sizeof(myRelays[0]);


void onSwitchCommand(bool state, HASwitch* sender) {
  for (int i = 0; i < numberOfRelays; i++) {
    if (sender == myRelays[i].switchObj) {
      int pin = myRelays[i].relay;
      uint8_t trigger = myRelays[i].relayTrigger;

      digitalWrite(pin, state ? trigger : !trigger);

#ifdef DEBUG_MODE
      Serial.print("Relay '");
      Serial.print(myRelays[i].name);
      Serial.print("' (pin ");
      Serial.print(pin);
      Serial.print(") set to ");
      Serial.println(state ? "ON" : "OFF");
#endif

      sender->setState(state);

      saveState(i, state);

      return;
    }
  }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
#ifdef DEBUG_MODE
    Serial.println("Setup begins");
#endif

    // START WATCHDOG
    pinMode(LED_BUILTIN, OUTPUT);
    wdt_enable(WDTO_8S);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
    // START WATCHDOG END

    //EEPROM.begin(512);

#ifdef DEBUG_MODE
    Serial.println("Init LAN...");
#endif
    //Ethernet.init(17); // Raspberry Pi Pico with w5500
    //Ethernet.init(48); // Easyswitch with w5500
    Ethernet.init(); // Easyswitch with w5100

    Ethernet.begin(mac, ip, dns, gw, sn);

#ifdef DEBUG_MODE
    Serial.println("LAN OK!");

    Serial.print("Local IP : ");
    Serial.println(Ethernet.localIP());
    Serial.print("Subnet Mask : ");
    Serial.println(Ethernet.subnetMask());
    Serial.print("Gateway IP : ");
    Serial.println(Ethernet.gatewayIP());
    Serial.print("DNS Server : ");
    Serial.println(Ethernet.dnsServerIP());
#endif

    device.setName("Gas boiler");
    device.setSoftwareVersion("1.0.1");
    
#ifdef DEBUG_MODE
    Serial.println("setup OK");
#endif
    wdt_reset();
    delay(1000);
    wdt_reset();
    delay(1000);
    wdt_reset();
#ifdef DEBUG_MODE
    Serial.println("wait OK");
    Serial.println("connecting to MQTT broker...");
#endif

    for (int i = 0; i < numberOfRelays; i++) {
#ifdef DEBUG_MODE
    Serial.print("Setting up relay ");
    Serial.print(i);
    Serial.print(", ");
#endif
      int pin = myRelays[i].relay;
      pinMode(pin, OUTPUT);

      uint8_t isTurnedOn = loadState(i); //read eeprom
#ifdef DEBUG_MODE
      Serial.print("state loaded: ");
      Serial.println(isTurnedOn);
#endif
      uint8_t relayState = isTurnedOn ? myRelays[i].relayTrigger : ! myRelays[i].relayTrigger;
      digitalWrite(pin, relayState);
      
      sprintf(myRelays[i].id_name, "relay_%d", i);
      myRelays[i].switchObj = new HASwitch(myRelays[i].id_name);
      myRelays[i].switchObj->setName(myRelays[i].name);
      myRelays[i].switchObj->setIcon("mdi:relay");
      myRelays[i].switchObj->onCommand(onSwitchCommand);
      myRelays[i].switchObj->setState(isTurnedOn);
    }

    mqttMaintainConnection();

#ifdef DEBUG_MODE
    //manual test for debugging with MQTT5 Explorer
    //mqtt.publish("aha/test321/boot", "ok");
#endif
}

void loop() {
  if(mqttMaintainConnection()) {
    digitalWrite(LED_BUILTIN, HIGH);
    mqtt.loop();
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }

  // RESET WATCHDOG TIMER
  wdt_reset();

  if (!initial_state_sent) {
#ifdef DEBUG_MODE
    Serial.println("initial state send");
#endif
    delay(2000);
  
    // RESET WATCHDOG TIMER
    wdt_reset();
    
    for(int i = 0; i < numberOfRelays; i++) {
      uint8_t isTurnedOn = loadState(i); //read eeprom
      myRelays[i].switchObj->setState(isTurnedOn);
    }

    initial_state_sent = true;
    
    // RESET WATCHDOG TIMER
    wdt_reset();
  }

  if ((millis() - lastReadAt) > 3000) {
    lastReadAt = millis();

    // Check every Relay status and turn on or off gas boiler
    gasBoilerONact = false;
    for (int i = 0; i <= numberOfRelays; i++) {
      if(myRelays[i].trigger_boiler == 1) {
#ifdef DEBUG_MODE
        Serial.println("(turn on or off gas boiler) CHECK relay status");
        Serial.println(i);
        Serial.println(loadState(i));
#endif
        if (loadState(i) == 1) {
#ifdef DEBUG_MODE
          Serial.println("GAS BOILER TO BE ON!!!");
#endif
          gasBoilerONact = true;
        }
      }
    }
    
    if(gasBoilerONact != gasBoilerON) {
#ifdef DEBUG_MODE
  Serial.println("GAS BOILER CHANGE!!!");
  Serial.println(gasBoilerONact);
  Serial.println("GAS BOILER CHANGED!!!");
#endif
      gasBoilerON = gasBoilerONact;
      triggerGasBoiler();
    }
  }

  // RESET WATCHDOG TIMER
  wdt_reset();
}

void triggerGasBoiler() {
  uint8_t newState = gasBoilerON ? myRelays[GAS_BOILER_ID].relayTrigger : ! myRelays[GAS_BOILER_ID].relayTrigger;

  digitalWrite(myRelays[GAS_BOILER_ID].relay, newState);

  if (loadState(GAS_BOILER_ID) != gasBoilerON) {
    saveState(GAS_BOILER_ID, gasBoilerON);
  }

  uint8_t isTurnedOn = loadState(GAS_BOILER_ID); // 1 - true, 0 - false
  myRelays[GAS_BOILER_ID].switchObj->setState(isTurnedOn);

#ifdef DEBUG_MODE
  // Write some debug info
  Serial.print("GAS BOILER id: ");
  Serial.print(GAS_BOILER_ID);
  Serial.println(", New status: " + gasBoilerON);
#endif
}

int mqttMaintainConnection() {
  if(Ethernet.linkStatus() == 2) {
#ifdef DEBUG_MODE
    Serial.println("ERROR: Cable disconnected");
#endif
    delay(1000);
  }
  else {
    if (Ethernet.maintain() % 2 == 1) {
#ifdef DEBUG_MODE
      Serial.println("ERROR: DHCP");
#endif
      delay(1000);
    }
    else {
      //(re)connect to MQTT Broker
      int nTries = 1;
      while(!mqtt.isConnected()) {
        mqttConnected = false;
#ifdef DEBUG_MODE
        Serial.println("MQTT... FAIL");
#endif
        if(nTries <= 10) {
          wdt_reset();
        }
        delay(1000);
        mqtt.begin(BROKER_ADDR, 1883);
        mqtt.loop();
        nTries += 1;
      }
      if(nTries == 1) {
        mqtt.loop();
      }
#ifdef DEBUG_MODE
      Serial.println("MQTT... OK");
#endif
      mqttConnected = true;
      return 1;
    }
  }
  mqttConnected = false;
  return 0;
}

void saveState(int index, bool state) {
  EEPROM.update(index, state ? 1 : 0);
}

bool loadState(int index) {
  uint8_t val = EEPROM.read(index);
  if (val == 0xFF) {
    return false;
  }
  return val == 1;
}
