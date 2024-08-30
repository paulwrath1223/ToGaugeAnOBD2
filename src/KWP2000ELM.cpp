//
// Created by Paul on 8/29/2024.
//

#include "KWP2000ELM.h"

#define ELM_PORT   SerialBtElm
#define DEBUG_PORT Serial

#define SANITY_MAX_RPM 9000 // the highest RPM value to accept. higher values will be clamped and issue warnings.
#define SANITY_MIN_COOLANT_TEMP_CELSIUS (-100) // the highest RPM value to accept. higher values will be clamped and issue warnings.
#define SANITY_MAX_COOLANT_TEMP_CELSIUS 250 // the highest RPM value to accept. higher values will be clamped and issue warnings.

#define DO_SEND_COMMAND_DEBUG

KWP2000ELM::KWP2000ELM(Stream& stream): stream(stream) {}

void KWP2000ELM::clear_stream(){
    while(this->stream.available()){
        this->stream.read();
    }
}

String KWP2000ELM::send_command(const char* input, uint32_t timeout_ms) {
    String output = "";
    char current_char = '\0';
    clear_stream();
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

String KWP2000ELM::byte_to_hex_string(byte input_byte) {
    String out = String(input_byte, HEX);
    if(out.length() == 1){
        String padded_out = "0";
        padded_out.concat(out);
        return padded_out;
    }
    return out;
}

long KWP2000ELM::hex_string_to_int(const char *hex_str_input) {
    return strtol(hex_str_input, nullptr, 16);
}

int KWP2000ELM::hex_string_to_byte_array(const char *hex_str_input) {
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

float KWP2000ELM::getEngineSpeedRpm() {
    String result = send_command("210C011");

    // 84 F0 10 61 0C 00 00 F1
    int length = hex_string_to_byte_array(result.c_str());

    int actual_checksum = 0;

    for(int i = 0; i<length-1; i++){
        actual_checksum += byte_array[i];
    }
    if(byte_array[length-1] == (actual_checksum % 256) && byte_array[4] == 0x0C){
        float result_rpm = ((((float)byte_array[5] * (float)255.0) + (float)byte_array[6])/(float)4.0);
        // (255*BA[5] + BA[6])/4

        if(result_rpm>SANITY_MAX_RPM){
            DEBUG_PORT.println("RPM data returned value over SANITY_MAX_RPM. clamping and returning SANITY_MAX_RPM");
            return 9000.0;
        }
        if(result_rpm<0){
            DEBUG_PORT.println("RPM data returned a negative number!! "
                               "This physically isn't possible and must be a problem with my code, "
                               "but clamping and returning 0 nonetheless");
            return 0.0;
        }

        return result_rpm;
    }



    DEBUG_PORT.println("RPM data either failed checksum or did not match request");
    return -0.0;
}

int16_t KWP2000ELM::getEngineCoolantTempC() {
    String result = send_command("2105011");

    // 83 F0 10 61 05 7E 67
    int length = hex_string_to_byte_array(result.c_str());

    int actual_checksum = 0;

    for(int i = 0; i<length-1; i++){
        actual_checksum += byte_array[i];
    }
    if(byte_array[length-1] == (actual_checksum % 256) && byte_array[4] == 0x05){
        int16_t result_celsius = (int16_t)(byte_array[5]) - 40; // linter seems to get booty tickled by this line.
        // Thinks the custom type `byte` defined in arduino is actually an int, not a uint8_t.


        if(result_celsius > SANITY_MAX_COOLANT_TEMP_CELSIUS){
            DEBUG_PORT.println("Engine coolant temp data returned value over SANITY_MAX_COOLANT_TEMP_CELSIUS."
                               "clamping and returning SANITY_MAX_COOLANT_TEMP_CELSIUS");
            return SANITY_MAX_COOLANT_TEMP_CELSIUS;
        }
        if(result_celsius < SANITY_MIN_COOLANT_TEMP_CELSIUS){
            DEBUG_PORT.println("Engine coolant temp data returned value under SANITY_MIN_COOLANT_TEMP_CELSIUS."
                               "clamping and returning SANITY_MIN_COOLANT_TEMP_CELSIUS");
            return SANITY_MIN_COOLANT_TEMP_CELSIUS;
        }

        return result_celsius;
    }



    DEBUG_PORT.println("RPM data either failed checksum or did not match request."
                       "Returning SANITY_MIN_COOLANT_TEMP_CELSIUS");
    return SANITY_MIN_COOLANT_TEMP_CELSIUS;
}

bool KWP2000ELM::check_ECU_Connection(){
    String result = send_command("210001");

    int length = hex_string_to_byte_array(result.c_str());

    int actual_checksum = 0;

    for(int i = 0; i<length-1; i++){
        actual_checksum += byte_array[i];
    }
    if(byte_array[length-1] == (actual_checksum % 256) && byte_array[4] == 0x00){
        return true;
    }



    DEBUG_PORT.println("ECU Connection Failed");
    return false;
}
/**
 * start communication with the ECU
 * @param stream stream of the serial port with ELM
 * @return true if communication was successfully established
 */
bool KWP2000ELM::init_ECU_connection(){
    send_command("ATZ");
    send_command("ATE0");
    send_command("ATH1");
    send_command("ATSP5");
    send_command("ATST64");
    send_command("ATS0");
    send_command("ATM0");
    send_command("ATAT1");
    send_command("ATSH8210F0");
    return check_ECU_Connection();
}