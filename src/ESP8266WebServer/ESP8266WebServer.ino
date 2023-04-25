#include <IRremote.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

#include "ZMPT101B.h"
#include "ACS712.h"

/* PIN CONFIGURATION */
#define DHT_PIN 2
#define DHT_TYPE DHT11
#define IR_PIN 3
#define VOLTAGE_PIN A0
#define CURRENT_PIN A1
#define COMPRESSOR_RELAY_PIN 4

/* SYSTEM VARIABLES */
float temperature = 0;
float humidity = 0;
float heatIndex = 0;

int desired_temperature = 0;

/* COMPRESSOR VARS */
#define COMPRESSOR_OFF HIGH
#define COMPRESSOR_ON LOW

/* IR REMOTE VALUES */
const String IR_TEMP_DOWN = "b";        // ***
const String IR_TEMP_UP = "7";          // to be change depending on the 
const String IR_POWER = "2";            // specific remote signal values 
const String IR_POWER_SAVING = "68";    // *** 

/* BOOLEANS */
bool isPowerSaving = false;
bool isTesting = true;

/* POWER CONSUMPTION VARS */
float voltage = 0.0;
float current = 0.0;
float power = 0;      // in watts

DHT dht(DHT_PIN, DHT_TYPE);

LiquidCrystal_I2C lcd(0x27, 16, 2);

/* LCD CUSTOM CHARACTERS */
byte lightning[] = { 0x00, 0x02, 0x04, 0x0C, 0x1F, 0x06, 0x04, 0x08 };
byte degreeCelsius[] = { 0x00, 0x10, 0x06, 0x09, 0x08, 0x08, 0x09, 0x06 };
byte lock[] = { 0x00, 0x0E, 0x11, 0x11, 0x1F, 0x1B, 0x1B, 0x1F };
byte arrowUp[] = { 0x00, 0x04, 0x0E, 0x15, 0x04, 0x04, 0x00, 0x00 };
byte arrowDown[] = { 0x00, 0x00, 0x04, 0x04, 0x15, 0x0E, 0x04, 0x00 };
byte check[] = { 0x00, 0x01, 0x03, 0x16, 0x1C, 0x08, 0x00, 0x00 };
byte unlock[] = { 0x0E, 0x11, 0x11, 0x01, 0x1F, 0x1B, 0x1B, 0x1F };
byte separator[] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 };

/* POWER SAVING VARS */
const int MAX_POWER_SAVING_TEMP = 27; // inclusive
const int MIN_POWER_SAVING_TEMP = 24; // inclusive

unsigned long previous_display_time = 0;

ZMPT101B voltageSensor(A3);
ACS712 currentSensor(ACS712_30A, A2);

void setup() {
    // Initializes Serial for debugging
    Serial.begin(9600);

    // Starts up IR sensor
    IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);

    // pin initialization 
    pinMode(COMPRESSOR_RELAY_PIN, OUTPUT);

    // Starts up DHT sensor
    dht.begin();

    // voltage and current sensor initialization
    delay(100);
    voltageSensor.setSensitivity(0.0025);
    voltageSensor.setZeroPoint(2621);
    
    currentSensor.setZeroPoint(2943);
    currentSensor.setSensitivity(0.15);

    // Caliberation Command Need To Be Run On First Upload.  
    // Calibrate();

    // initialize LCD
    lcd.init();
    lcd.backlight();

    // create custom characters
    lcd.createChar(0, lightning);
    lcd.createChar(1, degreeCelsius);
    lcd.createChar(2, lock);
    lcd.createChar(3, arrowUp);
    lcd.createChar(4, arrowDown);
    lcd.createChar(5, check);
    lcd.createChar(6, unlock);
    lcd.createChar(7, separator);

    // for testing purposes
    if (isTesting) temperature = 20;

    // sets the initial state of compressor relay as low or off
    digitalWrite(COMPRESSOR_RELAY_PIN, COMPRESSOR_OFF);
    desired_temperature = 24;
}

void loop() {
    // read Temperature
    readTemperature();

    // read voltage
    readVoltage();

    // read current
    readCurrent();

    // calculate power
    power = voltage * current;

    // read IRremote
    readIRremote();

    // controls the action for the aircon compressor
    controlCompressor();

    // display temperature and power
    displayData();
}

void controlCompressor() {
    // create integer value of temperature
    int converted_temperature = int(temperature);
    if (converted_temperature > desired_temperature) digitalWrite(COMPRESSOR_RELAY_PIN, COMPRESSOR_ON);
    else digitalWrite(COMPRESSOR_RELAY_PIN, COMPRESSOR_OFF);
}

void readIRremote() {
    if (IrReceiver.decode()) {
        String ir_code = String(IrReceiver.decodedIRData.command, HEX);
        Serial.println(ir_code);

        // adjusting the desired temperature
        if (ir_code == "15") {
            if (!isPowerSaving || (isPowerSaving && desired_temperature < MAX_POWER_SAVING_TEMP)) {
                desired_temperature++;
                delay(1000); 
            }
        }
        if (ir_code == "7") {
            if (!isPowerSaving || (isPowerSaving && desired_temperature > MIN_POWER_SAVING_TEMP)) {
                desired_temperature--;
                delay(1000); 
            }
        }

        // only testing 
        // actual temperature can be adjusted
        if (isTesting) {
            if (ir_code == "d") {
                temperature++;
                delay(1000);
            }
            if (ir_code == "19") {
                temperature--;
                delay(1000);
            }
        }

        // toggles the power saving mode
        if (ir_code == "43") {
            Serial.println("Power Saving Mode INITIALIZED!");
            Serial.println("Waiting for 5 seconds to fully toggle...");
            delay(1000);
            IrReceiver.resume();
            delay(4000);
            IrReceiver.decode();
            ir_code = String(IrReceiver.decodedIRData.command, HEX);

            // checks whether the user still clicks the power saving mode button
            if (ir_code == "43") {
                isPowerSaving = !isPowerSaving;
                Serial.println("Power Saving Mode Toggled!");
                Serial.print("PSM: ");
                Serial.println(isPowerSaving);
                if (isPowerSaving && (desired_temperature > MAX_POWER_SAVING_TEMP || desired_temperature < MIN_POWER_SAVING_TEMP)) desired_temperature = MIN_POWER_SAVING_TEMP;
                delay(1000);
            }
        }

        IrReceiver.resume();
    }
}

void readTemperature() {
    // for testing only
    // disregards the actual temperature values
    if (isTesting) return;

    // Wait a few seconds between measurements.
    delay(2000);

    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    humidity = dht.readHumidity();
    // Read temperature as Celsius (the default)
    temperature = dht.readTemperature();

    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(temperature)) {
        Serial.println(F("Failed to read from DHT sensor!"));
        return;
    }

    // Compute heat index in Celsius (isFahreheit = false)
    heatIndex = dht.computeHeatIndex(temperature, humidity, false);

    // serial output for debugging
    Serial.print(F("Humidity: "));
    Serial.print(humidity);
    Serial.print(F("%  Temperature: "));
    Serial.print(temperature);
    Serial.print(F("°C "));
    Serial.print(F(" Heat index: "));
    Serial.print(heatIndex);
    Serial.println(F("°C "));
}

void readVoltage() {
    if (isTesting) {
        voltage = random(218, 223);
        return;
    }

    voltage = voltageSensor.getVoltageAC();
    if(voltage<55) voltage=0;
}

void readCurrent() {
    if (isTesting) {
        int intTemp = int(temperature);
        if (intTemp == 16) current = 6 + (random(67, 72));
        if (intTemp == 17) current = 6 + (random(41, 46));
        if (intTemp == 18) current = 6 + (random(16, 21));
        if (intTemp == 19) current = 5 + (random(95, 100));
        if (intTemp == 20) current = 5 + (random(74, 79));
        if (intTemp == 21) current = 5 + (random(51, 56));
        if (intTemp == 22) current = 5 + (random(26, 31));
        if (intTemp == 23) current = 5 + (random(2, 7));
        if (intTemp == 24) current = 4 + (random(76, 81));
        if (intTemp == 25) current = 4 + (random(67, 72));
        if (intTemp == 26) current = 4 + (random(33, 38));
        if (intTemp == 27) current = 4 + (random(0, 3));
    }

    current = currentSensor.getCurrentAC();
    if(current<0.15) current=0;
}

void displayData() {
    if (millis() - previous_display_time < 2000) return;
    previous_display_time = millis();

    lcd.clear();

    /* ROW 0*/
    // show lightning symbol
    lcd.setCursor(0, 0);
    lcd.write(0);

    // show power in watts
    lcd.setCursor(2, 0);
    lcd.print(power);
    // show W symbol
    lcd.setCursor(7, 0);
    lcd.print("W");

    // converts actual temperature from float to int for better comparison
    int converted_temperature = int(temperature);

    // show compressor status
    // clears the compressor status placeholder first before writing new one
    lcd.setCursor(10, 0);
    lcd.print(" ");
    lcd.setCursor(10, 1);
    lcd.print(" ");
    // create integer value of temperature
    if (desired_temperature == converted_temperature) {
        lcd.setCursor(10, 0);
        lcd.write(5); // writes check
    }  
    if (desired_temperature > converted_temperature) {
        lcd.setCursor(10, 1);
        lcd.write(4); // writes arrow down
    }
    if (desired_temperature < converted_temperature) {
        lcd.setCursor(10, 1);
        lcd.write(3); // writes arrow up 
    }

    // show desired temperature
    lcd.setCursor(13, 0);
    lcd.print(desired_temperature);
    // show degree celsius 
    lcd.setCursor(15, 0);
    lcd.write(1);

    // show actual temperature
    lcd.setCursor(13, 1);
    lcd.print(converted_temperature);
    // show degree celsius 
    lcd.setCursor(15, 1);
    lcd.write(1);

    /* ROW 1*/
    // show operation mode
    if (isPowerSaving) {
        lcd.setCursor(0, 1);
        lcd.print("SAVING");        
        // show lock symbol
        lcd.setCursor(7, 1);
        lcd.write(2);
    }
    else {
        lcd.setCursor(0, 1);
        lcd.print("NORMAL");        
        // show unlock symbol
        lcd.setCursor(7, 1);
        lcd.write(6);
    }

    Serial.print("Voltage: ");
    Serial.println(voltage);
    Serial.print("Current: ");
    Serial.println(current);
    Serial.print("Power: ");
    Serial.println(power);
    Serial.print("Temperature: ");
    Serial.println(temperature);
    Serial.print("Humidity: ");
    Serial.println(humidity);

    Serial.println("- - - - - - -");
}

void Calibrate() {
    while (1) {
        voltageSensor.calibrate();  
        Serial.print("Voltage Zero Point: ");
        Serial.println(voltageSensor.getZeroPoint());

        currentSensor.calibrate();  
        Serial.print("Current Zero Point:");
        Serial.println(currentSensor.getZeroPoint());

        delay(500);
    }
}
