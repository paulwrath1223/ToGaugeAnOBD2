//
// Created by Paul on 8/27/2024.
//

#ifndef STEPPER_GAUGE_TEST_GAUGE_H
#define STEPPER_GAUGE_TEST_GAUGE_H

#include <Arduino.h>

namespace Gauge {

    class Gauge{
        uint16_t position_steps;
        bool calibrated;
        uint32_t color_array[24] = {0};
        uint32_t val;

    public:
        uint16_t max_steps;
        uint16_t min_steps;
        uint32_t max_val;
        uint32_t min_val;

        Gauge(uint16_t max_steps,
              uint16_t min_steps,
              uint32_t max_val,
              uint32_t min_val);

        int32_t calibrate(bool use_min = true, uint16_t max_steps_to_find_stop = 1000);

        int32_t set_val(uint32_t new_value, uint8_t light_brightness);

        static uint32_t get_color_template(uint8_t light_index, uint8_t brightness);

        uint32_t get_color_at_index(uint8_t index);
    };
} // Gauge

#endif //STEPPER_GAUGE_TEST_GAUGE_H
