#include "BluetoothSerial.h"
#include "Arduino.h"

BluetoothSerial SerialBtElm;

#define ELM_PORT   SerialBtElm
#define DEBUG_PORT Serial

String send_command(const char* input, uint32_t timeout_ms=1000);
void flushInputBuff();

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
    send_command("ATZ");
    delay(100);
    send_command("ATE0");
    delay(100);
    send_command("ATH1");
    delay(100);
    send_command("ATSP5");
    delay(100);
    send_command("ATST64");
    delay(100);
    send_command("ATS0");
    delay(100);
    send_command("ATM0");
    delay(100);
    send_command("ATAT1");
    delay(100);
    send_command("ATSH8210F0");
    delay(100);
    send_command("210001");
    delay(100);

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

    send_command("210D011");
    delay(100);
    send_command("2105011");
    delay(100);
    send_command("210C011");

    delay(1000);
//
//    float tempRPM = myELM327.rpm();
//
//    if (myELM327.nb_rx_state == ELM_SUCCESS)
//    {
//        rpm = (uint32_t)tempRPM;
//        Serial.print("RPM: "); Serial.println(rpm);
//    }
//    else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
//        myELM327.printError();
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

void flushInputBuff(){
    while (ELM_PORT.available())
    {
        ELM_PORT.read();
    }
}

String send_command(const char* input, uint32_t timeout_ms) {
    String output = "";

    flushInputBuff();

    ELM_PORT.print(input);
    ELM_PORT.print('\r');
    DEBUG_PORT.print("Sending: "); DEBUG_PORT.println(input);
    unsigned long initial_millis = millis();
    while(!ELM_PORT.available() || millis() - initial_millis > timeout_ms){

    }
    if(ELM_PORT.available()){
        while(ELM_PORT.available()){
            output.concat((char)ELM_PORT.read());
        }
        DEBUG_PORT.print("Received: "); DEBUG_PORT.println(output);
    } else {
        DEBUG_PORT.println("Timed out! Returning empty string");
    }
    return output;
}
