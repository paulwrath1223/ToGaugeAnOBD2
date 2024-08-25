#include "BluetoothSerial.h"
#include "Arduino.h"

BluetoothSerial SerialBtElm;

#define ELM_PORT   SerialBtElm
#define DEBUG_PORT Serial

#define DO_SEND_COMMAND_DEBUG

String send_command(Stream &stream, const char* input, uint32_t timeout_ms=2000);
void clear_stream(Stream &stream);

uint32_t rpm = 0;

int temp_in;

void setup()
{
#if LED_BUILTIN
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
#endif

    DEBUG_PORT.begin(115200);
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
    send_command(ELM_PORT, "ATZ");
    send_command(ELM_PORT, "ATE0");
    send_command(ELM_PORT, "ATH1");
    send_command(ELM_PORT, "ATSP5");
    send_command(ELM_PORT, "ATST64");
    send_command(ELM_PORT, "ATS0");
    send_command(ELM_PORT, "ATM0");
    send_command(ELM_PORT, "ATAT1");
    send_command(ELM_PORT, "ATSH8210F0");
    send_command(ELM_PORT, "ATDP");
    send_command(ELM_PORT, "ATFI");


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

    send_command(ELM_PORT, "210D011");
    send_command(ELM_PORT, "2105011");
    send_command(ELM_PORT, "210C011");
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

void clear_stream(Stream &stream){
    while(stream.available()){
        stream.read();
    }
}

String send_command(Stream &stream, const char* input, uint32_t timeout_ms) {
    String output = "";
    char current_char = '\0';
    clear_stream(stream);
    stream.print(input);
    stream.print("\r\n");
    #ifdef DO_SEND_COMMAND_DEBUG
    DEBUG_PORT.print("Sending: "); DEBUG_PORT.println(input);
    #endif
    delay(100);
    unsigned long initial_millis = millis();
    #ifdef DO_SEND_COMMAND_DEBUG
    DEBUG_PORT.print("Waiting for response");
    #endif
    while(!stream.available() && millis() - initial_millis < timeout_ms){
        #ifdef DO_SEND_COMMAND_DEBUG
        DEBUG_PORT.print(".");
        delay(20);
        #endif
    }
    if(stream.available()){
        current_char = (char)stream.read();
        initial_millis = millis();
        #ifdef DO_SEND_COMMAND_DEBUG
        DEBUG_PORT.print("\nFirst char arrived: "); DEBUG_PORT.println(current_char);
        #endif
        while(current_char != '>' && millis() - initial_millis < timeout_ms){
            while(!stream.available() && millis() - initial_millis < timeout_ms){}
            if(stream.available()){
                output.concat(current_char);
                current_char = (char)stream.read();
                #ifdef DO_SEND_COMMAND_DEBUG
                DEBUG_PORT.print("Next char arrived: "); DEBUG_PORT.println(current_char);
                #endif
            }
        }
        if(current_char == '>'){
            #ifdef DO_SEND_COMMAND_DEBUG
            DEBUG_PORT.println("Found delimiter! Good response");
            #endif
        } else {
            #ifdef DO_SEND_COMMAND_DEBUG
            DEBUG_PORT.println("No Delimiter, Timed out after received at least one byte.");
            #endif
        }
        #ifdef DO_SEND_COMMAND_DEBUG
        DEBUG_PORT.println(output);
        #endif
    } else {
        #ifdef DO_SEND_COMMAND_DEBUG
        DEBUG_PORT.println("Timed out! Returning empty string");
        #endif
    }
    return output;
}

//Sending: 210C011
//Waiting for response....
//First char arrived: 8
//Next char arrived: 4
//Next char arrived: F
//Next char arrived: 0
//Next char arrived: 1
//Next char arrived: 0
//Next char arrived: 6
//Next char arrived: 1
//Next char arrived: 0
//Next char arrived: C
//Next char arrived: 0
//Next char arrived: 0
//Next char arrived: 0
//Next char arrived: 0
//Next char arrived: F
//Next char arrived: 1
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//84F010610C0000F1
//Sending: 210D011
//Waiting for response.....
//First char arrived: 8
//Next char arrived: 3
//Next char arrived: F
//Next char arrived: 0
//Next char arrived: 1
//Next char arrived: 0
//Next char arrived: 6
//Next char arrived: 1
//Next char arrived: 0
//Next char arrived: D
//Next char arrived: 0
//Next char arrived: 0
//Next char arrived: F
//Next char arrived: 1
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//83F010610D00F1
//Sending: 2105011
//Waiting for response....
//First char arrived: 8
//Next char arrived: 3
//Next char arrived: F
//Next char arrived: 0
//Next char arrived: 1
//Next char arrived: 0
//Next char arrived: 6
//Next char arrived: 1
//Next char arrived: 0
//Next char arrived: 5
//Next char arrived: 8
//Next char arrived: 3
//Next char arrived: 6
//Next char arrived: C
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//83F0106105836C
//Sending: 210C011
//Waiting for response.....
//First char arrived: 8
//Next char arrived: 4
//Next char arrived: F
//Next char arrived: 0
//Next char arrived: 1
//Next char arrived: 0
//Next char arrived: 6
//Next char arrived: 1
//Next char arrived: 0
//Next char arrived: C
//Next char arrived: 0
//Next char arrived: 0
//Next char arrived: 0
//Next char arrived: 0
//Next char arrived: F
//Next char arrived: 1
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//84F010610C0000F1
//Sending: 210D011
//Waiting for response.....
//First char arrived: 8
//Next char arrived: 3
//Next char arrived: F
//Next char arrived: 0
//Next char arrived: 1
//Next char arrived: 0
//Next char arrived: 6
//Next char arrived: 1
//Next char arrived: 0
//Next char arrived: D
//Next char arrived: 0
//Next char arrived: 0
//Next char arrived: F
//Next char arrived: 1
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//83F010610D00F1
//Sending: 2105011
//Waiting for response.....
//First char arrived: 8
//Next char arrived: 3
//Next char arrived: F
//Next char arrived: 0
//Next char arrived: 1
//Next char arrived: 0
//Next char arrived: 6
//Next char arrived: 1
//Next char arrived: 0
//Next char arrived: 5
//Next char arrived: 8
//Next char arrived: 3
//Next char arrived: 6
//Next char arrived: C
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//83F0106105836C
//Sending: 210C011
//Waiting for response....
//First char arrived: 8
//Next char arrived: 4
//Next char arrived: F
//Next char arrived: 0
//Next char arrived: 1
//Next char arrived: 0
//Next char arrived: 6
//Next char arrived: 1
//Next char arrived: 0
//Next char arrived: C
//Next char arrived: 0
//Next char arrived: 0
//Next char arrived: 0
//Next char arrived: 0
//Next char arrived: F
//Next char arrived: 1
//Next char arrived:
//Next char arrived:
//Next char arrived: >