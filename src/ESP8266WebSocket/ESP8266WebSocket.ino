#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>

// Set your Wi-Fi AP credentials
const char* ssid = "Websocket";
const char* password = "qwerty";
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

// Create an instance of the server
ESP8266WebServer server(80);

// Create an instance of the WebSocket server
WebSocketsServer webSocketServer(81);

void handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    // Handle WebSocket events
    // This function will be called automatically when a WebSocket event occurs
    // You can implement your own logic to process the received data here
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocketServer.remoteIP(num);
                Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
                // Send a welcome message to the connected client
                webSocketServer.sendTXT(num, "Hello, WebSocket client!");
            }
            break;
        case WStype_TEXT:
            Serial.printf("[%u] Received text: %s\n", num, payload);
            // Process the received text data as needed
            break;
        case WStype_BIN:
            Serial.printf("[%u] Received binary data of length %u\n", num, length);
            // Process the received binary data as needed
            break;
        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    
    // Configure Wi-Fi
    WiFi.softAPConfig(local_ip, gateway, subnet);

    // Connect to Wi-Fi
    WiFi.softAP(ssid, password);

    // Start the server
    server.begin();

    // Set up the WebSocket server
    webSocketServer.begin();
    webSocketServer.onEvent(handleWebSocketEvent);
}

void loop() {
    // Handle client connections
    server.handleClient();

    // Handle
    webSocketServer.loop();
}