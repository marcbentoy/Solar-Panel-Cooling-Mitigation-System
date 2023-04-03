#include "ZMPT101B.h"
#include "ACS712.h"

ZMPT101B voltageSensor(34);
ACS712 currentSensor(ACS712_20A, 36);

float power=0;
float voltage=0;
float current=0;

void setup()
{
    Serial.begin(9600);

    delay(100);
    voltageSensor.setSensitivity(0.0025);
    voltageSensor.setZeroPoint(2621);
    
    currentSensor.setZeroPoint(2943);
    currentSensor.setSensitivity(0.15);

    // Caliberation Command Need To Be Run On First Upload.  
    // Calibrate();
}


void loop()
{
    // To measure voltage/current we need to know the frequency of voltage/current
    // By default 50Hz is used, but you can specify desired frequency
    // as first argument to getVoltageAC and getCurrentAC() method, if necessary

    voltage = voltageSensor.getVoltageAC();
    if(voltage<55) voltage=0;
  
    current = currentSensor.getCurrentAC();
    if(current<0.15) current=0;
  
    power = voltage * current;
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