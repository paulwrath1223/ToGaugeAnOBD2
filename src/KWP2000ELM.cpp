//
// Created by Paul on 8/29/2024.
//

#include "KWP2000ELM.h"

#define DEBUG_PORT Serial
#define ENGINE_RPM_COMMAND "210C01"
#define ENGINE_COOLANT_COMMAND "210501"



#define SANITY_MAX_RPM 9000 // the highest RPM value to accept. higher values will be clamped and issue warnings.
#define SANITY_MIN_COOLANT_TEMP_CELSIUS (-100) // the highest RPM value to accept. higher values will be clamped and issue warnings.
#define SANITY_MAX_COOLANT_TEMP_CELSIUS 250 // the highest RPM value to accept. higher values will be clamped and issue warnings.

//#define DO_SEND_COMMAND_DEBUG
#define DO_DATA_AQUISITION_DEBUG

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
    stream.write(input);
    stream.write('\r');

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
            output.trim();
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
    String result = send_command(ENGINE_RPM_COMMAND);

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
#ifdef DO_DATA_AQUISITION_DEBUG
            DEBUG_PORT.println("RPM data returned value over SANITY_MAX_RPM. clamping and returning SANITY_MAX_RPM");
#endif
            return 9000.0;
        }
        if(result_rpm<0){
#ifdef DO_DATA_AQUISITION_DEBUG
            DEBUG_PORT.println("RPM data returned a negative number!! "
                               "This physically isn't possible and must be a problem with my code, "
                               "but clamping and returning 0 nonetheless");
#endif
            return 0.0;
        }

        return result_rpm;
    }
#ifdef DO_DATA_AQUISITION_DEBUG
    DEBUG_PORT.println("RPM data either failed checksum or did not match request");

    DEBUG_PORT.print("getEngineCoolantTempC:\nresponse length: ");
    DEBUG_PORT.println(length);

    DEBUG_PORT.print("actual checksum mod 256: (DECIMAL)");
    DEBUG_PORT.println(actual_checksum % 256);

    DEBUG_PORT.print("last byte: (DECIMAL)");
    DEBUG_PORT.println(byte_array[length-1]);

    DEBUG_PORT.println("parsed response:");
    for(int i = 0; i<length; i++){
        DEBUG_PORT.print(byte_to_hex_string(byte_array[i]));
        DEBUG_PORT.print(' ');
    }
#endif
    return -0.0;
}

int16_t KWP2000ELM::getEngineCoolantTempC() {
    String result = send_command(ENGINE_COOLANT_COMMAND);

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
#ifdef DO_DATA_AQUISITION_DEBUG
            DEBUG_PORT.println("Engine coolant temp data returned value over SANITY_MAX_COOLANT_TEMP_CELSIUS."
                               "clamping and returning SANITY_MAX_COOLANT_TEMP_CELSIUS");
#endif
            return SANITY_MAX_COOLANT_TEMP_CELSIUS;
        }
        if(result_celsius < SANITY_MIN_COOLANT_TEMP_CELSIUS){
#ifdef DO_DATA_AQUISITION_DEBUG
            DEBUG_PORT.println("Engine coolant temp data returned value under SANITY_MIN_COOLANT_TEMP_CELSIUS."
                               "clamping and returning SANITY_MIN_COOLANT_TEMP_CELSIUS");
#endif
            return SANITY_MIN_COOLANT_TEMP_CELSIUS;
        }

        return result_celsius;
    }

#ifdef DO_DATA_AQUISITION_DEBUG
    DEBUG_PORT.println("RPM data either failed checksum or did not match request."
                       "Returning SANITY_MIN_COOLANT_TEMP_CELSIUS");

    DEBUG_PORT.print("getEngineCoolantTempC:\nresponse length: ");
    DEBUG_PORT.println(length);

    DEBUG_PORT.print("actual checksum mod 256: ");
    DEBUG_PORT.println(actual_checksum % 256);

    DEBUG_PORT.print("last byte: ");
    DEBUG_PORT.println(byte_array[length-1]);

    DEBUG_PORT.println("parsed response:");
    for(int i = 0; i<length; i++){
        DEBUG_PORT.print(byte_to_hex_string(byte_array[i]));
        DEBUG_PORT.print(' ');
    }
#endif
    return SANITY_MIN_COOLANT_TEMP_CELSIUS;
}

bool KWP2000ELM::check_ECU_Connection(){
    String result = send_command("210001");
    // 86 F0 10 61 00 08 3E 90 01 BE

    int length = hex_string_to_byte_array(result.c_str());


    int actual_checksum = 0;

    for(int i = 0; i<length-1; i++){
        actual_checksum += byte_array[i];
    }

    if(byte_array[length-1] == (actual_checksum % 256) && byte_array[4] == 0x00){
        return true;
    }
#ifdef DO_DATA_AQUISITION_DEBUG
    DEBUG_PORT.println("ECU Connection Failed");

    DEBUG_PORT.print("check_ECU_Connection:\nresponse length: ");
    DEBUG_PORT.println(length);

    DEBUG_PORT.print("actual checksum mod 256: ");
    DEBUG_PORT.println(actual_checksum % 256);

    DEBUG_PORT.print("last byte: ");
    DEBUG_PORT.println(byte_array[length-1]);

    DEBUG_PORT.println("parsed response:");
    for(int i = 0; i<length; i++){
        DEBUG_PORT.print(byte_to_hex_string(byte_array[i]));
        DEBUG_PORT.print(' ');
    }
#endif
    return false;
}
/**
 * start communication with the ECU. NEED TO DELAY BY >100ms after calling
 * @param stream stream of the serial port with ELM
 *
 */
void KWP2000ELM::init_ECU_connection(){
    send_command("ATZ");
    send_command("ATE0");
    send_command("ATH1");
    send_command("ATSP5");
    send_command("ATST64");
    send_command("ATS0");
    send_command("ATM0");
    send_command("ATAT1");
    send_command("ATSH8210F0");
    send_command("210001");
}