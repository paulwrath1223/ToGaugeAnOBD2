#include "BluetoothSerial.h"
#include "Arduino.h"
#include "PIDs.h"
#include <KWP2000ELM.h>

BluetoothSerial SerialBtElm;

#define ELM_PORT   SerialBtElm
#define DEBUG_PORT Serial




uint32_t rpm = 0;




KWP2000ELM elm = KWP2000ELM(ELM_PORT);

void setup()
{

    DEBUG_PORT.begin(115200);
    ELM_PORT.begin("ArduHUD", true);


    if (!ELM_PORT.connect("VEEPEAK"))
    {
        DEBUG_PORT.println("Couldn't connect to OBD scanner - Phase 1");
        while(1);
    }

    DEBUG_PORT.println("Connected to ELM327");

    if(elm.init_ECU_connection()){
        DEBUG_PORT.println("Communication established with the ECU");
    } else {
        DEBUG_PORT.println("Communication with the ECU was unable to be verified or did not work. Try restarting");
    }

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

    rpm = (uint32_t)elm.getEngineSpeedRpm();
    DEBUG_PORT.println("speed: ");
    DEBUG_PORT.println(rpm);

    String voltage = elm.send_command("ATRV");
    DEBUG_PORT.println("voltage: ");
    DEBUG_PORT.println(voltage);

    int16_t coolant_temp_c = elm.getEngineCoolantTempC();
    DEBUG_PORT.println("coolant_temp_c: ");
    DEBUG_PORT.println(coolant_temp_c);

    delay(1000);

}





//getPID called with PID d
//Generated request:
//Sending:
//Waiting for response...
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
//Got response: 84F010610C0000F1
//response as bytes:84
//f0
//10
//61
//c
//0
//0
//f1
//0



