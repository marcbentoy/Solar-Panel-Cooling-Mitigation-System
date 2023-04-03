#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

#include "ZMPT101B.h"
#include "ACS712.h"

/* Put your SSID & Password */
const char* ssid = "Cooling Mitigation";  // Enter SSID here
const char* password = "12345678";  //Enter Password here

/* Put IP Address details */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

WebServer server(80);

#define DHTTYPE DHT11
const int DHT1_PIN = 27;
const int DHT2_PIN = 26;
const int DHT3_PIN = 25;
const int DHT4_PIN = 33;

const int PUMP_PIN = 14;
bool pumpStatus = HIGH;

DHT dht1(DHT1_PIN, DHTTYPE);
DHT dht2(DHT2_PIN, DHTTYPE);
DHT dht3(DHT3_PIN, DHTTYPE);
DHT dht4(DHT4_PIN, DHTTYPE);

unsigned long previousMillis = 0;
const long interval = 2000;

float t1 = 0.0;
float h1 = 0.0;
float t2 = 0.0;
float h2 = 0.0;
float t3 = 0.0;
float h3 = 0.0;
float t4 = 0.0;
float h4 = 0.0;

float average_temperature = 0.0;

const float TEMP_THRESHOLD = 30.0; // in degree celsius
const int PUMPING_DURATION = 10; // in seconds

unsigned long pumping_start_time = 0;
bool isPumping = false;

ZMPT101B voltageSensor(13);
ACS712 currentSensor(ACS712_20A, 12);

float power=0;
float voltage=0;
float current=0;

void setup() {
    Serial.begin(115200);
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(DHT1_PIN, INPUT);
    pinMode(DHT2_PIN, INPUT);

    WiFi.softAP(ssid, password);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    delay(100);
    
    server.on("/", handle_OnConnect);
    server.on("/pumpOn", handlePumpOn);
    server.on("/pumpOff", handlePumpOff);
    server.onNotFound(handle_NotFound);
    
    server.begin();
    Serial.println("HTTP server started");

    dht1.begin();
    dht2.begin();
    dht3.begin();
    dht4.begin();

    digitalWrite(PUMP_PIN, HIGH);

    delay(100);
    voltageSensor.setSensitivity(0.0025);
    voltageSensor.setZeroPoint(2621);
    
    currentSensor.setZeroPoint(2943);
    currentSensor.setSensitivity(0.15);

    // Caliberation Command Need To Be Run On First Upload.  
    // Calibrate();
}

void loop() {
    server.handleClient();

    readDHT();
    readPower();

    average_temperature = (t1 + t2 + t3 + t4) / 4;

    if (millis() - previousMillis > interval) {
        Serial.print("Temp1: ");
        Serial.print(t1);
        Serial.print(" Humid1: ");
        Serial.println(h1);

        Serial.print("Temp2: ");
        Serial.println(t2);
        Serial.print(" Humid2: ");
        Serial.println(h2);

        Serial.print("Temp3: ");
        Serial.println(t3);
        Serial.print(" Humid3: ");
        Serial.println(h3);

        Serial.print("Temp4: ");
        Serial.println(t4);
        Serial.print(" Humid4: ");
        Serial.println(h4);

        Serial.print("Voltage: ");
        Serial.println(voltage);
        Serial.print(" Current: ");
        Serial.print(current);
        Serial.print(" Power: ");
        Serial.println(power);

        Serial.print("Is pumping: ");
        Serial.println(isPumping);

        Serial.print("Average Temp: ");
        Serial.println(average_temperature);

        Serial.print("Pumping start time: ");
        Serial.println(pumping_start_time);  
        Serial.println("- - - - - -");

        previousMillis = millis();
    }

    analyzeCoolingMitigation();  

    if (pumpStatus) digitalWrite(PUMP_PIN, LOW);
    else digitalWrite(PUMP_PIN, HIGH);
}

void analyzeCoolingMitigation() {
    unsigned long current_pumping_time = millis();

    if (isPumping && current_pumping_time - pumping_start_time >= (PUMPING_DURATION * 1000)) {  
        digitalWrite(PUMP_PIN, HIGH);
        isPumping = false;
    }
    
    if (average_temperature > TEMP_THRESHOLD && !isPumping) {
        digitalWrite(PUMP_PIN, LOW);
        isPumping = true;
        pumping_start_time = millis();
    }
}

void handle_OnConnect() {
    pumpStatus = LOW;
    Serial.println("Pump Status: OFF");
    server.send(200, "text/html", SendHTML()); 
}

void handlePumpOn() {
    pumpStatus = LOW;
    Serial.println("Pump Status: ON");
    server.send(200, "text/html", SendHTML()); 
}

void handlePumpOff() {
    pumpStatus = HIGH;
    Serial.println("Pump Status: OFF");
    server.send(200, "text/html", SendHTML()); 
}

void handle_NotFound(){
    server.send(404, "text/plain", "Not found");
}

String SendHTML(){
    String ptr = "<!DOCTYPE html> <html>\n";
    ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
    ptr +="<title>LED Control</title>\n";
    ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
    ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
    ptr +=".button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 20px;text-decoration: none;font-size: 18px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
    ptr +=".button-on {background-color: #1abc9c;}\n";
    ptr +=".button-on:active {background-color: #16a085;}\n";
    ptr +=".button-off {background-color: #34495e;}\n";
    ptr +=".button-off:active {background-color: #2c3e50;}\n";
    ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
    ptr += ".styled-table{border-collapse:collapse; border-radius: 4px; margin-left: auto; margin-right: auto; margin-bottom: 10px; font-size: 0.9em; font-family: sans-serif; min-width: 350px; box-shadow: 0 0 20px rgba(0,0,0,0.15);}";
    ptr += ".styled-table thead tr{ background-color: #009879; color: #ffffff; text-align: center; }";
    ptr += ".styled-table th, .styled-table td{ padding: 12px 15px; }";
    ptr += ".styled-table tbody tr { border-bottom: 1px solid #dddddd; }";
    ptr += ".styled-table tbody tr:nth-of-type(even) { background-color: #f3f3f3; }";
    ptr += ".styled-table tbody tr:last-of-type { border-bottom: 2px solid #009879; }";
    ptr +="</style>\n";
    ptr +="</head>\n";
    ptr +="<body>\n";
    ptr +="<h1>Solar Panel Cooling Mitigation System</h1>\n";
    ptr +="<h3>Web Server</h3>\n";

    // Temparature Table
    ptr += "<table class=\"styled-table\"><thead><tr><th>DHT</th><th>Temperature</th><th>Humidity</th></tr></thead>"; 
    ptr += "<tbody>";

    ptr += "<tr><td>1</td><td>";  // dht1
    ptr += t1;
    ptr += " *C</td><td>";
    ptr += h1;
    ptr += " %</td></tr>";

    ptr += "<tr><td>2</td><td>"; // dht2 
    ptr += t2;
    ptr += " *C</td><td>";
    ptr += h2;
    ptr += " %</td></tr>";

    ptr += "<tr><td>3</td><td>"; // dht3 
    ptr += t3;
    ptr += " *C</td><td>";
    ptr += h3;
    ptr += " %</td></tr>";

    ptr += "<tr><td>4</td><td>"; // dht4 
    ptr += t4;
    ptr += " *C</td><td>";
    ptr += h4;
    ptr += " %</td></tr>";

    ptr += "<tbody></table>";

    // Power Table
    ptr += "<table class=\"styled-table\"><thead><tr><th>Voltage</th><th>Current</th><th>Power</th></tr></thead>"; 
    ptr += "<tbody>";
    ptr += "<tr><td>";
    ptr += voltage;
    ptr += " V</td><td>";
    ptr += current;
    ptr += "</td><td>";
    ptr += power;
    ptr += " W</td></tr>";
    ptr += "<tbody></table>";

    // Pump Button
    if (!pumpStatus) ptr +="<p>Water Pump Status: ON</p><a class=\"button button-off\" href=\"/pumpOff\">OFF</a>\n";
    else ptr +="<p>Water Pump Status: OFF</p><a class=\"button button-on\" href=\"/pumpOn\">ON</a>\n";

    ptr +="</body>\n";
    ptr +="</html>\n";

    return ptr;
}

void readDHT() {
    if (millis() - previousMillis >= interval) {
        // save the last time you updated the DHT values
        previousMillis = millis();
        
        // * Read temperature as Celsius (the default)
        float newT1 = dht1.readTemperature();
        float newT2 = dht2.readTemperature();
        float newT3 = dht3.readTemperature();
        float newT4 = dht4.readTemperature();

        // if temperature read failed, don't change t value
        if (isnan(newT1)) Serial.println("Failed to read from DHT sensor 1!");
        else t1 = newT1;
        
        if (isnan(newT2)) Serial.println("Failed to read from DHT sensor 2!");
        else t2 = newT2;

        if (isnan(newT3)) Serial.println("Failed to read from DHT sensor 3!");
        else t3 = newT3;

        if (isnan(newT4)) Serial.println("Failed to read from DHT sensor 4!");
        else t4 = newT4;

        // * Read Humidity
        float newH1 = dht1.readHumidity();
        float newH2 = dht2.readHumidity();
        float newH3 = dht3.readHumidity();
        float newH4 = dht4.readHumidity();
        
        // if humidity read failed, don't change h value 
        if (isnan(newH1)) Serial.println("Failed to read humidity from DHT sensor 1!");
        else h1 = newH1;

        if (isnan(newH2)) Serial.println("Failed to read humidity from DHT sensor 2!");
        else h2 = newH2;

        if (isnan(newH3)) Serial.println("Failed to read humidity from DHT sensor 3!");
        else h3 = newH3;

        if (isnan(newH4)) Serial.println("Failed to read humidity from DHT sensor 4!");
        else h4 = newH4;
    }
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

void readPower() {
    voltage = voltageSensor.getVoltageAC();
    if(voltage<55) voltage=0;
  
    current = currentSensor.getCurrentAC();
    if(current<0.15) current=0;

    power = voltage * current;
}