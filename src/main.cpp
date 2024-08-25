#include "BluetoothSerial.h"
#include "ELMduino.h"
#include "Arduino.h"


BluetoothSerial SerialBTElm;
BluetoothSerial SerialBTSlave;
#define ELM_PORT   SerialBTElm
#define DEBUG_PORT Serial
#define PROXY_PORT SerialBTSlave


ELM327 myELM327;


uint32_t rpm = 0;

int temp_in;

void setup()
{
#if LED_BUILTIN
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
#endif

    DEBUG_PORT.begin(115200);
    //SerialBT.setPin("1234");
    ELM_PORT.begin("ArduHUD", true);

//    BTScanResults* results = ELM_PORT.discover(60000);
//    const int num_results = results->getCount();
//    BTAdvertisedDevice* temp_device;
//    String device_name;
//    String device_mac;
//
//    for(int i = 0; i < num_results; i++){
//        temp_device = results->getDevice(i);
//        device_name = temp_device->getName().c_str();
//        device_mac = temp_device->getAddress().toString().c_str();
//
//        DEBUG_PORT.print("found device {");
//        DEBUG_PORT.print(device_name);
//        DEBUG_PORT.print("} with address {");
//        DEBUG_PORT.print(device_mac);
//        DEBUG_PORT.println("}.");
//    }

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
//    >ATE0
//    ATE0
//    OK
//
//    >ATH1
//    OK
//
//    >ATSP5
//    OK
//
//    >ATST64
//    OK
//
//    >ATS0
//    OK
//
//    >ATM0
//    OK
//
//    >ATAT1
//    OK
//
//    >ATSH8210F0
//    OK
//
//    >210001
    myELM327.sendCommand_Blocking("ATZ");
    myELM327.sendCommand_Blocking("ATE0");
    myELM327.sendCommand_Blocking("ATH1");
    myELM327.sendCommand_Blocking("ATSP5");
    myELM327.sendCommand_Blocking("ATST64");
    myELM327.sendCommand_Blocking("ATS0");
    myELM327.sendCommand_Blocking("ATM0");
    myELM327.sendCommand_Blocking("ATAT1");
    myELM327.sendCommand_Blocking("ATSH8210F0");
    myELM327.sendCommand_Blocking("210001");
    

    Serial.println("Connected to ELM327");

}


void loop()
{


//    >210D011
//    83 F0 10 61 0D 00 F1
//
//    >2105011
//    83 F0 10 61 05 7E 67
//
//    >210C011
//    84 F0 10 61 0C 00 00 F1

//    myELM327.sendCommand_Blocking("210D011");
//    myELM327.sendCommand_Blocking("2105011");
//    myELM327.sendCommand_Blocking("210C011");

    delay(1000);

    float tempRPM = myELM327.rpm();

    if (myELM327.nb_rx_state == ELM_SUCCESS)
    {
        rpm = (uint32_t)tempRPM;
        Serial.print("RPM: "); Serial.println(rpm);
    }
    else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
        myELM327.printError();
//
//    if (ELM_PORT.available()) {
//        temp_in = ELM_PORT.read();
//        PROXY_PORT.write(temp_in);
//        DEBUG_PORT.print("Elm -> App: ");
//        DEBUG_PORT.println(temp_in);
//    }
//    if (PROXY_PORT.available()) {
//        temp_in = PROXY_PORT.read();
//        ELM_PORT.write(temp_in);
//        DEBUG_PORT.print("App -> Elm: ");
//        DEBUG_PORT.println(temp_in);
//    }
}