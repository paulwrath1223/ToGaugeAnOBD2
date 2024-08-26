#include "BluetoothSerial.h"
#include "Arduino.h"
#include "PIDs.h"

BluetoothSerial SerialBtElm;

#define ELM_PORT   SerialBtElm
#define DEBUG_PORT Serial

#define DO_SEND_COMMAND_DEBUG

String send_command(Stream &stream, const char* input, uint32_t timeout_ms=2000);
void clear_stream(Stream &stream);
String byte_to_hex_string(byte input_byte);
long hex_string_to_int(const char* hex_str_input);
byte calculateChecksum(const byte data[], int length);
int hex_string_to_byte_array(const char *hex_str_input);
void getPID(Stream &stream, byte pid);
String generateRequest(const byte data[], int length, byte pid);


uint32_t rpm = 0;
byte byte_array[100] = {0};

int temp_in;

void setup()
{
#if LED_BUILTIN
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
#endif

    DEBUG_PORT.begin(115200);
    ELM_PORT.begin("ArduHUD", true);


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
    send_command(ELM_PORT, "210001");

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

    getPID(ELM_PORT, VEHICLE_SPEED);

    delay(1000);

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

String byte_to_hex_string(byte input_byte) {
    return String(input_byte, HEX);
}

long hex_string_to_int(const char *hex_str_input) {
    return strtol(hex_str_input, nullptr, 16);
}

byte calculateChecksum(const byte *data, int length) {
    byte checksum = 0;
    for (int i = 0; i < length; i++) {
        checksum += data[i];
    }
    return checksum % 256;
}

int hex_string_to_byte_array(const char *hex_str_input) {
    int i = 0;
    char char_pair[2];
    char_pair[0] = hex_str_input[2*i];
    char_pair[1] = hex_str_input[2*i+1];
    while(char_pair[0] != '\0' && char_pair[1] != '\0'){
        byte_array[i] = hex_string_to_int(char_pair);
        i++;
        char_pair[0] = hex_str_input[2*i];
        char_pair[1] = hex_str_input[2*i+1];
    }
    return i;
}

void getPID(Stream &stream, byte pid) {
    // example Request: C2 33 F1 01 0C F3
    // example Response: 84 F1 11 41 0C 1F 40 32
    DEBUG_PORT.print("getPID called with PID ");
    DEBUG_PORT.println(byte_to_hex_string(pid));

    const char* request = generateRequest(live_data, sizeof(live_data), pid).c_str();

    DEBUG_PORT.print("Generated request: ");
    DEBUG_PORT.println(request);

    String response = send_command(stream, request);

    DEBUG_PORT.print("Got response: ");
    DEBUG_PORT.println(response);

    int length = hex_string_to_byte_array(response.c_str());

    DEBUG_PORT.print("response as bytes:");
    for(int index = 0; index < length; index++){
        DEBUG_PORT.println(byte_to_hex_string(byte_array[index]));
    }
}

String generateRequest(const byte *data, int length, const byte pid) {

    String request = "";

    byte extendedData[length + 2];
    memcpy(extendedData, data, length);
    extendedData[length] = pid;
    byte checksum = calculateChecksum(extendedData, length + 1);
    extendedData[length + 1] = checksum;

    for (int i = 0; i < length + 2; i++) {
        request.concat(byte_to_hex_string(extendedData[i]));
    }
    return request;
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



