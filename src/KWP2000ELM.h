//
// Created by Paul on 8/29/2024.
//

#ifndef TOGAUGEANOBD2_KWP2000ELM_H
#define TOGAUGEANOBD2_KWP2000ELM_H
#include <Arduino.h>


class KWP2000ELM {
    public:
        byte byte_array[100] = {0};
        Stream& stream;

        explicit KWP2000ELM(Stream& stream);

        String send_command(const char* input, uint32_t timeout_ms=2000);
        void clear_stream();
        static String byte_to_hex_string(byte input_byte);
        static long hex_string_to_int(const char* hex_str_input);
        int hex_string_to_byte_array(const char *hex_str_input);
        float getEngineSpeedRpm();
        int16_t getEngineCoolantTempC();
        bool init_ECU_connection();
        bool check_ECU_Connection();

};


#endif //TOGAUGEANOBD2_KWP2000ELM_H
