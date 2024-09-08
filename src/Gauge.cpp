//
// Created by Paul on 8/27/2024.
//

#include "Gauge.h"
#include <Adafruit_NeoPixel.h>

namespace Gauge {
    /**
     *
     * @param use_min {bool} should calibrate at 0? set to true if the physical stop is on the 0 side. false means use `max_steps`
     * @param max_steps_to_find_stop {uint16_t} how many steps to move towards the stop before guaranteeing hitting the stop
     *
     * @return int16_t how many steps to move the stepper motor. This is done by the user to allow for compatibility with all stepper libraries.
     * If using the arduino stepper library, you can feed this directly into the `step` method
     */
    int32_t Gauge::calibrate(bool use_min, uint16_t max_steps_to_find_stop){
        if(use_min){
            this->calibrated = true;
            this->position_steps = 0;
            return -max_steps_to_find_stop;
        }
        this->calibrated = true;
        this->position_steps = this->max_steps;
        return max_steps_to_find_stop;

    }
    /**
     *
     * @param max_steps the maximum position of the dial, measured in steps of the stepper motor.
     * @param min_steps the smallest value of the dial, measured in steps of the stepper motor.
     * Either min or max need to be a physical hard stop that can be calibrated against.
     * @param max_val the maximum value the gauge will display. alternatively, the value to be associated with `max_steps`
     * @param min_val the minimum value the gauge will display. alternatively, the value to be associated with `min_steps`
     */
    Gauge::Gauge(uint16_t max_steps,
                 uint16_t min_steps,
                 uint32_t max_val,
                 uint32_t min_val) {

        this->max_steps = max_steps;
        this->min_steps = min_steps;
        this->max_val = max_val;
        this->min_val = min_val;

        this->calibrated = false;
        this->position_steps = min_steps;
        this->val = min_val;

    }
    /**
     * Given a new value, returns how many steps to move to stepper motor to reflect this value. Also updates the lights
     *
     * @param new_value the value the gauge should point to
     * @param light_brightness a multiplier for brightness of the lights
     * @return how many steps to move to stepper motor to reflect the new value.
     * If using the arduino stepper library, you can feed this directly into the `step` method
     */
    int32_t Gauge::set_val(uint32_t new_value, uint8_t light_brightness){
        this->val = new_value;

        uint8_t val_mapped = map(new_value, this->min_val, this->max_val, 0, 18);
        // map(value, fromLow, fromHigh, toLow, toHigh)

        for(int i = 0; i < 18; i++){
            if(i<=val_mapped){
                color_array[i] = get_color_template(i, light_brightness);
            } else {
                color_array[i] = Adafruit_NeoPixel::Color(0,0,0);
            }
        }


        if(calibrated){
            uint16_t target_step_pos = constrain(
                    map(new_value, min_val, max_val, min_steps, max_steps),
            // map(value, fromLow, fromHigh, toLow, toHigh)
                    min_steps,
                    max_steps);
            int32_t delta_steps = (int32_t)target_step_pos - this->position_steps;
            this->position_steps = target_step_pos;
            return delta_steps;
        } else {
            return 0; // NEEDS TO BE CALIBRATED
        }
    }


    /**
     *
     * @param index the index of the color array to grab from
     * @return a color in the Adafruit_NeoPixel::color format. (Packed 32-bit RGB with the most significant byte set to 0)
     */
    uint32_t Gauge::get_color_at_index(uint8_t index){
        return this->color_array[index];
    }


    /**
     * Internal helper function. this just gives you the color of a pixel that the dial has passed.
     * @param light_index index of the light (pixel/LED) to get the color of
     * @param brightness brightness scalar
     * @return a color in the Adafruit_NeoPixel::color format. (Packed 32-bit RGB with the most significant byte set to 0)
     */
    uint32_t Gauge::get_color_template(uint8_t light_index, uint8_t brightness){
        uint32_t hue = (light_index * 65536) / 24;
        return Adafruit_NeoPixel::ColorHSV(hue, 255, brightness);
    }
} // Gauge