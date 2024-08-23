#include "BluetoothSerial.h"
#include "ELMduino.h"
#include "Arduino.h"


BluetoothSerial SerialBT;
#define ELM_PORT   SerialBT
#define DEBUG_PORT Serial


ELM327 myELM327;


uint32_t rpm = 0;


void setup()
{
#if LED_BUILTIN
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
#endif

    DEBUG_PORT.begin(115200);
    //SerialBT.setPin("1234");
    ELM_PORT.begin("ArduHUD", true);

    BTScanResults* results = ELM_PORT.discover(60000);
    const int num_results = results->getCount();
    BTAdvertisedDevice* temp_device;
    String device_name;
    String device_mac;

    for(int i = 0; i < num_results; i++){
        temp_device = results->getDevice(i);
        device_name = temp_device->getName().c_str();
        device_mac = temp_device->getAddress().toString().c_str();

        DEBUG_PORT.print("found device {");
        DEBUG_PORT.print(device_name);
        DEBUG_PORT.print("} with address {");
        DEBUG_PORT.print(device_mac);
        DEBUG_PORT.println("}.");
    }

    if (!ELM_PORT.connect("VEEPEAK"))
    {
        DEBUG_PORT.println("Couldn't connect to OBD scanner - Phase 1");
        while(1);
    }

    if (!myELM327.begin(ELM_PORT, true, 2000))
    {
        Serial.println("Couldn't connect to OBD scanner - Phase 2");
        while (1);
    }

    Serial.println("Connected to ELM327");


}


void loop()
{
    float tempRPM = myELM327.rpm();

    if (myELM327.nb_rx_state == ELM_SUCCESS)
    {
        rpm = (uint32_t)tempRPM;
        Serial.print("RPM: "); Serial.println(rpm);
    }
    else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
        myELM327.printError();
}