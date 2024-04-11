// PTimer
// Mario Termine
// 04-10-2024
// V1
//
// This code was written to pair with my custom relay board. The purpose of the project is to reduce engine bay temperature that
// climbs when the engine is turned off and allowed to heat soak. Over time, this damages the PCM and puts heavy wear on all components in
// the engine bay.
//
// For more info, please visit https://mariotermine.me/ptimer

#include <Wire.h>
#include <Adafruit_INA260.h>

Adafruit_INA260 ina260 = Adafruit_INA260();

const int relayPin = 2; // Relay connected to pin D2
const float chargingVoltageThreshold = 13.5; // Voltage threshold to determine if the car is off
const float lowVoltageCutoff = 11.9; // Low voltage cutoff to deactivate the relay
const unsigned long relayTime = 300000; // Relay on time: 5 minutes
const unsigned long debounceTime = 5000; // Debounce time for voltage threshold: 5 seconds

unsigned long relayStartTime = 0; // Start timer for relay acitvation duration
unsigned long cooldownStartTime = 0; // Start timer for cooldown
const unsigned long cooldownPeriod = 45000;  // Cooldown period: 45 seconds
unsigned long lastVoltageCheckTime = 0; // Start helper timer for cooldown 
bool relayActive = false; // Relay state
bool relayActivatedDuringOff = false; // Flag to track if relay was activated during car off (one activation per off cycle)
bool voltageLow = false; // Low voltage state

void setup() {
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  
  Serial.begin(115200);
  if (!ina260.begin()) {
    // Serial.println("Couldn't find INA260 chip");
    while (1); // Halt if device not found
  }
  // Serial.println("INA260 found and initialized");
}

void loop() {
  float voltage = ina260.readBusVoltage();  // Read bus voltage in mV
  float voltageInVolts = voltage / 1000.0;  // Convert mV to V
  unsigned long currentTime = millis(); // Update current time

  // Serial.print("Current Time: ");
  // Serial.print(currentTime);
  // Serial.print(", Voltage: ");
  // Serial.print(voltageInVolts);
  // Serial.println(" V");

  // Serial.print("Relay Active: ");
  // Serial.print(relayActive);
  // Serial.print(", Voltage Low: ");
  // Serial.print(voltageLow);
  // Serial.print(", Relay Activated During Off: ");
  // Serial.println(relayActivatedDuringOff);

  delay(10);

  // Low voltage cutoff logic
  if (voltageInVolts < lowVoltageCutoff && relayActive) {
    digitalWrite(relayPin, LOW);
    relayActive = false;
    cooldownStartTime = currentTime;  // Start the cooldown period
    // Serial.println("Relay deactivated due to low voltage cutoff");
    return;  // Exit the loop to prevent other operations until voltage recovers
  }

  // Debounce logic for voltage threshold
  if (voltageInVolts < chargingVoltageThreshold) {
    if (!voltageLow && currentTime - lastVoltageCheckTime > debounceTime) {
      voltageLow = true;
      lastVoltageCheckTime = currentTime;
    }
  } else {
    if (voltageLow && currentTime - lastVoltageCheckTime > debounceTime) {
      voltageLow = false;
      lastVoltageCheckTime = currentTime;
      relayActivatedDuringOff = false; // Reset when car is on again
      // Serial.println("Voltage stabilized above threshold; resetting system.");
    }
  }

  // Reactivate relay when car turns off if conditions are met
  if (!relayActive && voltageLow && !relayActivatedDuringOff && currentTime - cooldownStartTime >= cooldownPeriod) {
    digitalWrite(relayPin, HIGH);
    relayActive = true;
    relayStartTime = currentTime;  // Setting the start time for relay activation
    relayActivatedDuringOff = true;  // Ensure relay is activated only once per off cycle
    // Serial.println("Relay activated due to car turning off.");
  }

  // Handle relay deactivation after the set duration
  if (relayActive && currentTime - relayStartTime >= relayTime) {
    digitalWrite(relayPin, LOW);
    relayActive = false;
    cooldownStartTime = currentTime;  // Start the cooldown period
    // Serial.println("Relay deactivated after 30 seconds");
  }
}