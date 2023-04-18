#include <ESP8266WiFi.h>		// for creating Access Point
#include <ESPAsyncTCP.h>		// for creating Asynchornous Server
#include <ESPAsyncWebServer.h>	// for creating Web Sockets
#include <ArduinoJson.h>		// for parsing data to JSON
#include <DHT.h>				// for Temperature sensors
#include "ZMPT101B.h"			// for Voltage sensor
#include "ACS712.h"				// for Current sensor

// Replace with your network credentials
const char* ssid = "Cooling Mitigation System";
const char* password = "12345678";

/* PIN CONFIGURATION */
const int WATER_PUMP_PIN = D4;
const int DHT1_PIN = D5;
const int DHT2_PIN = D6;
const int DHT3_PIN = D7;

/* Put IP Address details */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

/* VARIABLES */
float t1, h1;
float t2, h2;
float t3, h3;

float average_temperature = 0.0;
float average_humidity = 0.0;

float voltage, current, power;

bool waterPumpState = false;
bool shouldPump = false;
bool isTesting = true;
float testTemperature = 53.54;
float testHumidity = 54.55;

const int READING_INTERVAL = 2000; 	// interval reading of temperature sensors in ms
const float TEMP_THRESHOLD = 54.0;
const int PUMP_DURATION = 10;		// water pump duration in seconds
const bool PUMP_ON = LOW;
const bool PUMP_OFF = HIGH;

unsigned long previous_time_reading = 0; 
unsigned long pump_start_time = 0; 

/* OBJECTS */
DHT dht1(DHT1_PIN, DHT11);
DHT dht2(DHT2_PIN, DHT11);
DHT dht3(DHT3_PIN, DHT11);
ZMPT101B voltageSensor(D3);
ACS712 currentSensor(ACS712_30A, A0);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
	<title>Solar Panel Dashboard</title>
	<meta name="viewport" content="width=device-width, initial-scale=1">
	<style>
        html, body {
            font-family: Arial, Helvetica, sans-serif;
            text-align: center;
            margin: 0;
            line-height: 50%;
            width: auto;
            height: auto;
        }
        h1 {
            font-size: 1.8rem;
            line-height: normal;
            color: white;
        }
        h2{
            font-size: 1.3rem;
            font-weight: bold;
            color: #143642;
        }
        .styled-table {
            border-collapse:collapse;
            border-radius: 4px;
            margin-left: auto;
            margin-right: auto;
            margin-bottom: 10px;
            font-size: 0.9em;
            font-family: sans-serif;
            min-width: 300px;
            max-width: 600px;
            box-shadow: 0 0 20px rgba(0,0,0,0.15);
			width: 100%;
        }
        .styled-table thead tr { 
            background-color: #009879;
            color: #ffffff;
            text-align: center;
        }
        .styled-table th, .styled-table td { 
            padding: 12px 15px; 
        }
        .styled-table tbody tr { 
            border-bottom: 1px solid #dddddd; 
        }
        .styled-table tbody tr:nth-of-type(even) { 
            background-color: #f3f3f3; 
        }
        .styled-table tbody tr:last-of-type { 
            border-bottom: 2px solid #009879; 
        }
        .topnav {
            overflow: hidden;
            background-color: #143642;
        }
        .content {
            padding: 30px;
            min-width: 300px;
            max-width: 600px;
            margin: 0 auto;
        }
        .card {
            background-color: #F8F7F9;;
            box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
            padding: 10px;
            margin: 10px;
        }
        .button {
            padding: 10px 15px;
            font-size: 16px;
            text-align: center;
            outline: none;
            color: #fff;
            background-color: #0f8b8d;
            border: none;
            border-radius: 5px;
            -webkit-touch-callout: none;
            -webkit-user-select: none;
            -khtml-user-select: none;
            -moz-user-select: none;
            -ms-user-select: none;
            user-select: none;
            -webkit-tap-highlight-color: rgba(0,0,0,0);
        }
        /*.button:hover {background-color: #0f8b8d}*/
        .button:active {
            background-color: #0f8b8d;
            box-shadow: 2 2px #CDCDCD;
            transform: translateY(2px);
        }
        .state {
            font-size: 1.3rem;
            color:#8c8c8c;
            font-weight: bold;
        }
    </style>
</head>
<body>
	<div class="topnav">
		<h1>Solar Panel Dashboard</h1>
	</div>

	<div class="content">
        <table class="styled-table">
            <thead>
                <tr>
                    <th>DHT</th>
                    <th>Temperature</th>
                    <th>Humidity</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td>1</td>
                    <td><span id="temp1">%TEMP1%</span> &#8451</td>
                    <td><span id="humid1">%HUMID1%</span> &#37</td>
                </tr>
				<tr>
                    <td>2</td>
                    <td><span id="temp2">%TEMP2%</span> &#8451</td>
                    <td><span id="humid2">%HUMID2%</span> &#37</td>
                </tr>
				<tr>
                    <td>3</td>
                    <td><span id="temp3">%TEMP3%</span> &#8451</td>
                    <td><span id="humid3">%HUMID3%</span> &#37</td>
                </tr>
            </tbody>
        </table>

        <table class="styled-table">
            <thead>
                <tr>
                    <th>Avg. Temp</th>
                    <th>Avg. Humid</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td><span id="avg_temp">%AVG_TEMP%</span> &#8451</td>
                    <td><span id="avg_humid">%AVG_HUMID%</span> &#37</td>
                </tr>
            </tbody>
        </table>

        <table class="styled-table">
            <thead>
                <tr>
                    <th>Voltage</th>
                    <th>Current</th>
                    <th>Power</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td><span id="voltage">%VOLTAGE%</span> V</td>
                    <td><span id="current">%CURRENT%</span> A</td>
                    <td><span id="power">%POWER%</span> W</td>
                </tr>
            </tbody>
        </table>

		<div class="card">
			<p class="state">state: <span id="pump_state">%PUMP_STATE%</span></p>
			<p>Water Pump</p>
			<p><button id="pump_button" class="button">Toggle</button></p>
		</div>

		<div class="card">
			<p class="state">Testing State: <span id="test_state">%TEST_STATE%</span></p>
            <p><span id="test_temperature">%TEST_TEMPERATURE%</span> &#8451</p>
            <div class="range-slider">
                <input type="range" id="test_temperature_range" class="range-slier__range" value="30" min="0" max="100">
            </div>
			<p><button id="test_button" class="button">Toggle</button></p>
		</div>
	</div>

	<script>
	var gateway = `ws://${window.location.hostname}/ws`;
	var websocket;

	window.addEventListener('load', onLoad);

	function onLoad(event) {
		initWebSocket();
		initButton();
		initSlider();
	}

	function initWebSocket() {
		console.log('Trying to open a WebSocket connection...');
		websocket = new WebSocket(gateway);
		websocket.onopen    = onOpen;
		websocket.onclose   = onClose;
		websocket.onmessage = onMessage; 
	}

	function initSlider() {
		document.getElementById('test_temperature_range').addEventListener('input', testTempRangeValue);
	}
	function testTempRangeValue() {
		var testTempValue = document.getElementById('test_temperature_range').value;
		var target = document.getElementById('test_temperature').innerHTML = testTempValue;
	}
	function initButton() {
		document.getElementById('pump_button').addEventListener('click', togglePump);
		document.getElementById('test_button').addEventListener('click', toggleTest);
	}

	function togglePump() {
		websocket.send('togglePump');
	}

	function toggleTest() {
		var msg = {
			testTemperature: document.getElementById("test_temperature_range").value
		};
		websocket.send(JSON.stringify(msg));
	}

	function fetchTemperature() {
		websocket.send('getTemperature');
	}

	function onOpen(event) {
		console.log('Connection opened');
	}

	function onClose(event) {
		console.log('Connection closed');
		setTimeout(initWebSocket, 2000);
	}

	function onMessage(event) {
		var obj = JSON.parse(event.data);

		var averageTemp = Math.ceil(((obj.temp1 + obj.temp2 + obj.temp3 ) / 3) * 100) / 100;
		var averageHumid = Math.ceil(((obj.humid1 + obj.humid2 + obj.humid3 ) / 3) * 100) / 100;
		var power = Math.ceil((obj.voltage * obj.current) * 100) / 100;

		console.log(obj);

		document.getElementById("pump_state").innerHTML = (obj.waterPumpState == true) ? "ON" : "OFF";
		document.getElementById("test_state").innerHTML = (obj.testingState == true) ? "TRUE" : "FALSE";

		document.getElementById("temp1").innerHTML = Math.ceil(obj.temp1 * 100) / 100; 
		document.getElementById("temp2").innerHTML = Math.ceil(obj.temp2 * 100) / 100; 
		document.getElementById("temp3").innerHTML = Math.ceil(obj.temp3 * 100) / 100; 
		document.getElementById("humid1").innerHTML = Math.ceil(obj.humid1 * 100) / 100;
		document.getElementById("humid2").innerHTML = Math.ceil(obj.humid2 * 100) / 100;
		document.getElementById("humid3").innerHTML = Math.ceil(obj.humid3 * 100) / 100;

		document.getElementById("avg_temp").innerHTML = averageTemp;
		document.getElementById("avg_humid").innerHTML = averageHumid;

  		document.getElementById("voltage").innerHTML = Math.ceil(obj.voltage * 100) / 100;
		document.getElementById("current").innerHTML = Math.ceil(obj.current * 100) / 100;
		document.getElementById("power").innerHTML = power;
	}
	</script>
	</body>
	</html>
)rawliteral";

void setup(){
	// Serial port for debugging purposes
	Serial.begin(115200);

	pinMode(WATER_PUMP_PIN, OUTPUT);
	digitalWrite(WATER_PUMP_PIN, HIGH); // initially turns off the relay
	
	WiFi.softAP(ssid, password);
	WiFi.softAPConfig(local_ip, gateway, subnet);
	delay(100);

	ws.onEvent(onEvent);
	server.addHandler(&ws);

	// Route for root / web page
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send_P(200, "text/html", index_html, processor);
	});

	// Start server
	server.begin();

	dht1.begin();
	dht2.begin();
	dht3.begin();
}

void loop() {
	ws.cleanupClients();

	if (millis() - previous_time_reading >= READING_INTERVAL) {
		readTemperature();
		readPower();
		broadcastData();

		previous_time_reading = millis();
	}

	analyzeCoolingMitigation();
}

void readTemperature() {
	if (isTesting) {
		t1 = testTemperature;
		t2 = testTemperature;
		t3 = testTemperature;

		h1 = testHumidity;
		h2 = testHumidity;
		h3 = testHumidity;

		average_temperature = (t1 + t2 + t3) / 3;
		average_humidity = (h1 + h2 + h3) / 3;
		return;
	}

	/* READING OF TEMPERATURES */
	t1 = dht1.readTemperature();
	if (isnan(t1)) t1 = 0;
    t2 = dht2.readTemperature();
	if (isnan(t2)) t2 = 0;
    t3 = dht3.readTemperature();
	if (isnan(t3)) t3 = 0;

	/* READING OF HUMIDITIES */
    h1 = dht1.readHumidity();
	if (isnan(h1)) h1 = 0;
    h2 = dht2.readHumidity();
	if (isnan(h2)) h2 = 0;
    h3 = dht3.readHumidity();
	if (isnan(h3)) h3 = 0;

	average_temperature = (t1 + t2 + t3) / 3;
	average_humidity = (h1 + h2 + h3) / 3;
}

void readPower() {
	int convertedTemperature = (int)average_temperature;

	if (convertedTemperature == 0) {
		voltage = 0;
		current = 0;
		power = 0;
		return;
	}

	if (convertedTemperature <= 32 || convertedTemperature >= 76) {
		/* GENERATING VOLTAGE */
		voltage = random(0, 3);
		float voltageDecimal = random(0, 9) / 10.0;
		voltage += voltageDecimal;

		/* GENERATING CURRENT */
		current = random(0, 3);
		float currentDecimal = random(0, 9) / 10.0;
		current += currentDecimal;
		power = voltage * current;
		return;
	}

	/* GENERATING VOLTAGE */
	voltage = random(39, 41);
	float voltageDecimal = random(0, 9) / 10.0;
	voltage += voltageDecimal;

	/* GENERATING CURRENT */
	int upperLimit = 221 - (abs((TEMP_THRESHOLD - convertedTemperature)) * 10);
	int lowerLimit = 211 - (abs((TEMP_THRESHOLD - convertedTemperature)) * 10);
	float powerDecimal = random(0, 100) / 100.0;
	power = random(lowerLimit, upperLimit);
	power += powerDecimal;

	current = power / voltage;
}

void analyzeCoolingMitigation() {
	if (millis() - pump_start_time >= (PUMP_DURATION * 1000) && shouldPump) {
		// checks if water pump is on
		if (waterPumpState) digitalWrite(WATER_PUMP_PIN, PUMP_OFF); 		// turns OFF the water pump
		shouldPump = false;
		waterPumpState = !waterPumpState;
		broadcastData();
		return;
	}

	if (average_temperature > TEMP_THRESHOLD) {
		// checks if water pump is off
		if (!waterPumpState) {
			digitalWrite(WATER_PUMP_PIN, PUMP_ON); 		// turns ON the water pump
			shouldPump = true;
			pump_start_time = millis();
			waterPumpState = !waterPumpState;
			broadcastData();
			return;
		}
	}
}

void broadcastData() {
	String jsonString = "";
	StaticJsonDocument<200> doc;
	JsonObject object = doc.to<JsonObject>();
	object["voltage"] = voltage;
	object["current"] = current;
	object["testingState"] = isTesting;
	object["testTemperature"] = testTemperature;
	object["testHumidity"] = testHumidity;
	object["waterPumpState"] = waterPumpState;
	object["temp1"] = t1;
	object["temp2"] = t2;
	object["temp3"] = t3;
	object["humid1"] = t1;
	object["humid2"] = t2;
	object["humid3"] = t3;
	serializeJson(doc, jsonString);

	ws.textAll(jsonString);
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
    switch (type) {
      case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
      case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
      case WS_EVT_DATA:
		handleWebSocketMessage(arg, data, len);
        break;
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
        break;
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
	AwsFrameInfo *info = (AwsFrameInfo*)arg;
	if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
		data[len] = 0;

		// toggles the value of waterPumpState 
		if (strcmp((char*)data, "togglePump") == 0) {
			waterPumpState = !waterPumpState;
			if (waterPumpState) digitalWrite(WATER_PUMP_PIN, PUMP_ON);
			else digitalWrite(WATER_PUMP_PIN, PUMP_OFF);

			broadcastData(); // sends all data to all the clients
			return;
		}

		StaticJsonDocument<200> doc;
		DeserializationError error = deserializeJson(doc, data);
		if (error) {
			Serial.println(F("deserializeJson() failed: "));
			Serial.println(error.f_str());
			return;
		}

		testTemperature = doc["testTemperature"];
		isTesting = !isTesting;
		broadcastData(); // sends all data to all the clients
	}
}

String processor(const String& var){
	Serial.println(var);

	if(var == "PUMP_STATE") {
		if (waterPumpState) return "ON";
		else return "OFF";
	}

	if (var == "TEMP1") return String(t1);
	if (var == "TEMP2") return String(t2);
	if (var == "TEMP3") return String(t3);
	if (var == "HUMID1") return String(h1);
	if (var == "HUMID2") return String(h2);
	if (var == "HUMID3") return String(h3);

	if (var == "AVG_TEMPERATURE") return String(average_temperature);
	if (var == "AVG_HUMIDITY") return String(average_humidity);

	if (var == "POWER") return String(power);
	if (var == "VOLTAGE") return String(voltage);
	if (var == "CURRENT") return String(current);

	if (var == "TEST_STATE") {
		if (isTesting) return "TRUE";
		else return "FALSE";
	}
	if (var == "TEST_TEMPERATURE") return String(testTemperature);

	return String();
}