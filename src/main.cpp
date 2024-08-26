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
int hex_string_to_byte_array(const char *hex_str_input);


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
    String out = String(input_byte, HEX);
    if(out.length() == 1){
        String padded_out = "0";
        padded_out.concat(out);
        return padded_out;
    }
    return out;
}

long hex_string_to_int(const char *hex_str_input) {
    return strtol(hex_str_input, nullptr, 16);
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

float getEngineSpeedRpm(Stream &stream){
    String result = send_command(stream, "210C011");
    // 84 F0 10 61 0C 00 00 F1
    int length = hex_string_to_byte_array(result.c_str());

    int actual_checksum = 0;

    for(int i = 0; i<length-1; i++){
        actual_checksum += byte_array[i];
    }
    if(byte_array[length-1] == (actual_checksum % 255) && byte_array[4] == 0x0C){
        return((((float)byte_array[5] * (float)255.0) + (float)byte_array[6])/(float)4.0);
        // (255*BA[5] + BA[6])/4
    }
    return -0.0;
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



