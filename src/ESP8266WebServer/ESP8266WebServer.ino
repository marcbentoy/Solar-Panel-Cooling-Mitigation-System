#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include "ZMPT101B.h"
#include "ACS712.h"

/* Put your SSID & Password */
const char* ssid = "Cooling Mitigation";  // Enter SSID here
const char* password = "12345678";  //Enter Password here

const float TEMP_THRESHOLD = 54; // in degree celsius
const int PUMPING_DURATION = 10; // in seconds

/* Put IP Address details */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

ESP8266WebServer server(80);

#define DHTTYPE DHT11
uint8_t DHT1_PIN = D5;
uint8_t DHT2_PIN = D6;
uint8_t DHT3_PIN = D7;
uint8_t DHT4_PIN = D8;

uint8_t PUMP_PIN = D4;
bool pumpStatus = HIGH;

bool isTesting = true;

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

float average_temperature = 54.3;

unsigned long pumping_start_time = 0;
bool isPumping = false;

float power = 0;
float voltage = 0;
float current = 0;

ZMPT101B voltageSensor(D3);
ACS712 currentSensor(ACS712_20A, A0);

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

  digitalWrite(PUMP_PIN, HIGH);
}

void loop() {
  server.handleClient();

  readDHT();
  readVoltage();
  readCurrent();

  power = voltage * current;

  analyzeCoolingMitigation();  

  average_temperature = (t1 + t2 + t3 + t4) / 4;

  Serial.print("Temp1: ");
  Serial.print(t1);
  Serial.print("*C | Humid1: ");
  Serial.print(h1);
  Serial.println("%");

  Serial.print("Temp2: ");
  Serial.print(t2);
  Serial.print("*C | Humid2: ");
  Serial.print(h2);
  Serial.println("%");

  Serial.print("Temp3: ");
  Serial.print(t3);
  Serial.print("*C | Humid3: ");
  Serial.print(h3);
  Serial.println("%");

  Serial.print("Temp4: ");
  Serial.print(t4);
  Serial.print("*C | Humid4: ");
  Serial.print(h4);
  Serial.println("%");

  Serial.print("Voltage: ");
  Serial.print(voltage);
  Serial.print(" V | Current: ");
  Serial.print(current);
  Serial.print("A | Power: ");
  Serial.print(power);
  Serial.println(" W");

  Serial.print("Is pumping: ");
  Serial.println(isPumping);

  Serial.print("Average Temp: ");
  Serial.println(average_temperature);

  Serial.print("Pumping start time: ");
  Serial.println(pumping_start_time);  
  Serial.println("- - - - - -");

  if (isPumping) return;
  
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
  ptr +="<title>Solar Panel Dashboard</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin: 0;} h1{color: #fff;} h3 {color: #444444;}\n";
  ptr += "h2{ font-size: 1.5rem; font-weight: bold; color: #143642; }";
  ptr +=".button {margin-top: 50px; padding: 10px 20px; font-size: 16px; text-align: center; outline: none; color: #fff; background-color: #0f8b8d; border: none; border-radius: 5px; -webkit-touch-callout: none; -webkit-user-select: none; -khtml-user-select: none; -moz-user-select: none; -ms-user-select: none; user-select: none; -webkit-tap-highlight-color: rgba(0,0,0,0);}\n";
  ptr += ".button:active { background-color: #0f8b8d; box-shadow: 2 2px #CDCDCD; transform: translateY(2px); }";
  ptr += ".topnav { overflow: hidden; background-color: #143642; margin-bottom: 20px; }";
  ptr += ".card { background-color: #F8F7F9; padding-top: 20px; padding-bottom: 20px; height: 150px; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); margin: 20px auto; min-width: 350px; max-width: 350px}";
  ptr += ".temp-card { background-color: #F8F7F9; padding-top: 10px; padding-bottom: 10px; height: 80px; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); margin: 20px auto; min-width: 350px; max-width: 350px}";
  ptr += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr += ".styled-table{border-collapse:collapse; border-radius: 4px; margin-left: auto; margin-right: auto; margin-bottom: 10px; font-size: 0.9em; font-family: sans-serif; min-width: 350px; box-shadow: 0 0 20px rgba(0,0,0,0.15);}";
  ptr += ".styled-table thead tr{ background-color: #009879; color: #ffffff; text-align: center; }";
  ptr += ".styled-table th, .styled-table td{ padding: 12px 15px; }";
  ptr += ".styled-table tbody tr { border-bottom: 1px solid #dddddd; }";
  ptr += ".styled-table tbody tr:nth-of-type(even) { background-color: #f3f3f3; }";
  ptr += ".styled-table tbody tr:last-of-type { border-bottom: 2px solid #009879; }";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<div class=\"topnav\"><h1>Solar Panel Dashboard</h1></div>\n";

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

    // Average Temperature 
  ptr += "<div class=\"temp-card\"><h3>";
  ptr += average_temperature;
  ptr +=" *C</h3><p>Average Tempearture</p>";
  ptr += "</div>";

  // Power Table
  ptr += "<table class=\"styled-table\"><thead><tr><th>Voltage</th><th>Current</th><th>Power</th></tr></thead>"; 
  ptr += "<tbody>";
  ptr += "<tr><td>";
  ptr += voltage;
  ptr += " V</td><td>";
  ptr += current;
  ptr += " A</td><td>";
  ptr += power;
  ptr += " W</td></tr>";
  ptr += "<tbody></table>";

  // Pump Button
  ptr += "<div class=\"card\"><h2>Water Pump</h2>";
  if (!pumpStatus) ptr +="<p>Water Pump Status: ON</p><a class=\"button button-off\" href=\"/pumpOff\">Toggle Pump</a>\n";
  else ptr +="<p>Water Pump Status: OFF</p><a class=\"button button-on\" href=\"/pumpOn\">Toggle Pump</a>\n";
  ptr += "</div>";

  ptr +="</body>\n";
  ptr +="</html>\n";

  return ptr;
}

void readDHT() {
  if (millis() - previousMillis >= interval) {
    previousMillis = millis();
    
    t1 = dht1.readTemperature();
	if (isnan(t1)) t1 = 0;
    t2 = dht2.readTemperature();
	if (isnan(t2)) t2 = 0;
    t3 = dht3.readTemperature();
	if (isnan(t3)) t3 = 0;
    t4 = dht4.readTemperature();
	if (isnan(t4)) t4 = 0;

    h1 = dht1.readHumidity();
	if (isnan(h1)) h1 = 0;
    h2 = dht2.readHumidity();
	if (isnan(h2)) h2 = 0;
    h3 = dht3.readHumidity();
	if (isnan(h3)) h3 = 0;
    h4 = dht4.readHumidity();
	if (isnan(h4)) h4 = 0;
  }
}

void readVoltage() {
	if (isTesting) {
		int intTemp = (int)average_temperature;
		if (intTemp == 0 || intTemp >= 76) {
			voltage = 0;
			return;
		}

		voltage = random(39,41);
		float voltageDecimal = random(0,9) / 10.0;
		voltage += voltageDecimal;
		return;
	}

	voltage = voltageSensor.getVoltageAC();
    if(voltage<55) voltage=0;
}

void readCurrent() {
	if (isTesting) {
		int intTemp = (int)average_temperature;
		if (intTemp == 0 || intTemp >= 76) {
			current = 0;
			return;
		}

		int upperLimit = 221 - (abs((TEMP_THRESHOLD - intTemp)) * 10);
		int lowerLimit = 211 - (abs((TEMP_THRESHOLD - intTemp)) * 10);
		float powerDecimal = random(0, 100) / 100.0;
		power = random(lowerLimit, upperLimit);
		power += powerDecimal;

		current = power / voltage;
		return;
	}

	current = currentSensor.getCurrentAC();
    if(current<0.15) current=0;
}