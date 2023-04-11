bool isTesting = true;

float avgTemp = 0.0;

float voltage = 0.0;
float current = 0.0;

void setup() {

}

void loop() {

}

void readVoltage() {
    if (isTesting) {
        voltage = random(39,41);
        float voltageDecimal += random(1,10);
        voltage += voltageDecimal;
    }
}

void readPower() {
    if (isTesting) {
        int low = 0, high = 0;
        int intTemp = (int) avgTemp;
        current = random(39,41);
        float voltageDecimal += random(1,10);
        voltage += voltageDecimal;
    }
}