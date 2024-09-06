
#include "Arduino.h"
#include "PIDs.h"
#include <KWP2000ELM.h>



#define ELM_PORT Serial1
#define DEBUG_PORT Serial




uint32_t rpm = 0;




KWP2000ELM elm = KWP2000ELM(ELM_PORT);

void setup()
{

    DEBUG_PORT.begin(115200);
    ELM_PORT.begin(115200, SERIAL_8N1);
    DEBUG_PORT.println("waiting 3 seconds");

    for(uint8_t i = 0; i < 3; i++){
        DEBUG_PORT.print(".");
        delay(1000);
    }
    DEBUG_PORT.println("BEGIN");

    DEBUG_PORT.println("Connected to ELM327");

    if(elm.init_ECU_connection()){
        DEBUG_PORT.println("Communication established with the ECU");
    } else {
        DEBUG_PORT.println("Communication with the ECU was unable to be verified or did not work. Try restarting");
    }
    delay(1000);
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

//Connected to ELM327
//Sending: ATZ
//Waiting for response
//First char arrived: E
//Next char arrived: L
//Next char arrived: M
//Next char arrived: 3
//Next char arrived: 2
//Next char arrived: 7
//Next char arrived:
//Next char arrived: v
//Next char arrived: 1
//Next char arrived: .
//Next char arrived: 5
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//ELM327 v1.5
//Sending: ATE0
//Waiting for response
//First char arrived: O
//Next char arrived: K
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//OK
//Sending: ATH1
//Waiting for response
//First char arrived: O
//Next char arrived: K
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//OK
//Sending: ATSP5
//Waiting for response
//First char arrived: O
//Next char arrived: K
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//OK
//Sending: ATST64
//Waiting for response
//First char arrived: ?
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//?
//Sending: ATS0
//Waiting for response
//First char arrived: O
//Next char arrived: K
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//OK
//Sending: ATM0
//Waiting for response
//First char arrived: O
//Next char arrived: K
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//OK
//Sending: ATAT1
//Waiting for response
//First char arrived: ?
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//?
//Sending: ATSH8210F0
//Waiting for response
//First char arrived: O
//Next char arrived: K
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//OK
//Sending: 210001
//Waiting for response.....
//First char arrived: B
//Next char arrived: U
//Next char arrived: S
//Next char arrived:
//Next char arrived: I
//Next char arrived: N
//Next char arrived: I
//Next char arrived: T
//Next char arrived: :
//Next char arrived:
//Next char arrived: .
//Next char arrived: .
//Next char arrived: .
//Next char arrived: O
//Next char arrived: K
//Next char arrived:
//Next char arrived: 8
//Next char arrived: 6
//Next char arrived: F
//Next char arrived: 0
//Next char arrived: 1
//Next char arrived: 0
//Next char arrived: 6
//Next char arrived: 1
//Next char arrived: 0
//Next char arrived: 0
//Next char arrived: 0
//Next char arrived: 8
//Next char arrived: 3
//Next char arrived: E
//Next char arrived: 9
//Next char arrived: 0
//Next char arrived: 0
//Next char arrived: 1
//Next char arrived: B
//Next char arrived: E
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//86F0106100083E9001BE
//ECU Connection Failed
//Communication with the ECU was unable to be verified or did not work. Try restarting
//Sending: 210C011
//Waiting for response
//First char arrived: ?
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//?
//RPM data either failed checksum or did not match request
//speed:
//0
//Sending: ATRV
//Waiting for response
//First char arrived: 1
//Next char arrived: 2
//Next char arrived: .
//Next char arrived: 1
//Next char arrived: 7
//Next char arrived: V
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//12.17V
//voltage:
//12.17V
//Sending: 2105011
//Waiting for response
//First char arrived: ?
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//?
//RPM data either failed checksum or did not match request.Returning SANITY_MIN_COOLANT_TEMP_CELSIUS
//coolant_temp_c:
//-100
//Sending: 210C011
//Waiting for response
//First char arrived: ?
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//?
//RPM data either failed checksum or did not match request
//speed:
//0
//Sending: ATRV
//Waiting for response
//First char arrived: 1
//Next char arrived: 2
//Next char arrived: .
//Next char arrived: 1
//Next char arrived: 8
//Next char arrived: V
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//12.18V
//voltage:
//12.18V
//Sending: 2105011
//Waiting for response
//First char arrived: ?
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//?
//RPM data either failed checksum or did not match request.Returning SANITY_MIN_COOLANT_TEMP_CELSIUS
//coolant_temp_c:
//-100
//Sending: 210C011
//Waiting for response
//First char arrived: ?
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//?
//RPM data either failed checksum or did not match request
//speed:
//0
//Sending: ATRV
//Waiting for response
//First char arrived: 1
//Next char arrived: 2
//Next char arrived: .
//Next char arrived: 1
//Next char arrived: 7
//Next char arrived: V
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response







