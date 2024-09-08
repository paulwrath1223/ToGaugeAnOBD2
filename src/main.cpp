#include "Arduino.h"
#include <KWP2000ELM.h>
#include <Adafruit_NeoPixel.h>
#include <Gauge.h>
#include <LiquidCrystal_I2C.h>


#define ELM_PORT Serial1
#define DEBUG_PORT Serial

#define STEPS_PER_315 600

#define NEOP_BRIGHT 255
#define NEOP_DIM 50

#define LCD_BACKLIGHT_PIN 10
#define LCD_BACKLIGHT_BRIGHT 60
#define LCD_BACKLIGHT_DIM 220

#define CLUSTER_BACKLIGHT_PIN A0
#define CLUSTER_BACKLIGHT_LOW_THRESHOLD 500

#define LCD_CUSTOM_CHAR_DEGREES_C 0 // mapping for LCD lib between custom char data and index
#define LCD_CUSTOM_CHAR_TEMP_L 1
#define LCD_CUSTOM_CHAR_TEMP_R 2
#define LCD_CUSTOM_CHAR_DIVIDER 3
#define LCD_CUSTOM_CHAR_DIVIDER_LEFT 4
#define LCD_CUSTOM_CHAR_VBAT_L 5
#define LCD_CUSTOM_CHAR_VBAT_R 6

#define STEPPER_PIN_1 4
#define STEPPER_PIN_2 5
#define STEPPER_PIN_3 6
#define STEPPER_PIN_4 7

#define STEPPER_MIN_DELAY_MICROS 35

#define TIMER_INTERVAL_MS 16L

const char ERROR_MSG_NONE[] = "No    Errors";
const char ERROR_MSG_LOW_VOLTAGE[] = "LowBatVoltag";
const char ERROR_MSG_BAD_RPM_DATA[] = "Bad Rpm Data";
const char ERROR_MSG_BAD_COOLANT_TEMP_DATA[] = "Bad TempData";
const char ERROR_MSG_STEPPER_DELTA[] = "StpDltExesiv";

#define LOW_VOLTAGE_THRESHOLD 11

Adafruit_NeoPixel ring = Adafruit_NeoPixel(24, 15);
LiquidCrystal_I2C lcd(0x27,16,2);
Gauge::Gauge tach = Gauge::Gauge(540,0,9000,0);

uint8_t DegreesC[8] = { B01000, B10100, B01000, B00011, B00100, B00100, B00100, B00011 };
uint8_t TempL[8] = { B00001, B00001, B00001, B00001, B00001, B00000, B11001, B00110 };
uint8_t TempR[8] = { B00000, B11000, B00000, B11000, B00000, B00000, B10011, B01100 };
uint8_t Divider[8] = { B00100, B00100, B00100, B00100, B00100, B00100, B00100, B00100 };
uint8_t Divider_Left_Edge[8] = { B10000, B10000, B10000, B10000, B10000, B10000, B10000, B10000 };
uint8_t VbatL[8] = { B00000, B01100, B11111, B10000, B10100, B10100, B10000, B11111 };
uint8_t VbatR[8] = { B00000, B00110, B11111, B00001, B01101, B00001, B00001, B11111 };

const uint32_t white = Adafruit_NeoPixel::Color(NEOP_BRIGHT, NEOP_BRIGHT, NEOP_BRIGHT);
const uint32_t grey = Adafruit_NeoPixel::Color(NEOP_DIM, NEOP_DIM, NEOP_DIM);
uint32_t rpm = 0;
float raw_rpm = 0.0;
volatile int32_t stepper_delta = 0;
uint8_t loop_counter = 0;
String global_voltage;
int16_t coolant_temp_c;
bool bright_lights = true;
String current_error_msg = ERROR_MSG_NONE;
uint64_t last_request_millis;
uint64_t last_step_micros;
volatile uint8_t stepper_current_step = 0; // should be one of [0,1,2,3]


#define USE_TIMER_1     false
#define USE_TIMER_2     false
#define USE_TIMER_3     false
#define USE_TIMER_4     true
#define USE_TIMER_5     false

#include "TimerInterrupt.h"

#define TIMER_INTERRUPT_USING_ATMEGA_32U4 true

// Init timer ITimer1


void tick_stepper();
void display_lcd_stuff(int16_t coolant_temp_in, char const voltage[], char const message[]);

void stepper_set_step(uint8_t step);
void stepper_single_step(bool is_forwards);
void do_n_steps(int16_t steps_to_do);

KWP2000ELM elm = KWP2000ELM(ELM_PORT);

void setup()
{

    ITimer4.init();

    DEBUG_PORT.begin(115200);
    ELM_PORT.begin(115200, SERIAL_8N1);


    lcd.init();
    lcd.backlight();

    ring.begin();
    ring.show();

    lcd.createChar(LCD_CUSTOM_CHAR_DIVIDER, Divider);
    lcd.createChar(LCD_CUSTOM_CHAR_DIVIDER_LEFT, Divider_Left_Edge);
    lcd.createChar(LCD_CUSTOM_CHAR_DEGREES_C, DegreesC);
    lcd.createChar(LCD_CUSTOM_CHAR_TEMP_L, TempL);
    lcd.createChar(LCD_CUSTOM_CHAR_TEMP_R, TempR);
    lcd.createChar(LCD_CUSTOM_CHAR_VBAT_L, VbatL);
    lcd.createChar(LCD_CUSTOM_CHAR_VBAT_R, VbatR);

    if (ITimer4.attachInterruptInterval(TIMER_INTERVAL_MS, tick_stepper))
        DEBUG_PORT.println("Starting  ITimer OK, millis() = " + String(millis()));
    else
        DEBUG_PORT.println("Can't set ITimer. Select another freq. or timer");



    DEBUG_PORT.println("Connected to ELM327");
    elm.init_ECU_connection();

    lcd.home();
    lcd.print("conecting to ECU");

    do_n_steps(tach.calibrate());

    if(elm.check_ECU_Connection()){
        DEBUG_PORT.println("Communication established with the ECU");
        lcd.home();
        lcd.clear();
        lcd.print("success");
    } else {
        lcd.home();
        lcd.clear();
        lcd.print("failed to conect");
        DEBUG_PORT.println("Communication with the ECU was unable to be verified or did not work. Try restarting");
    }
    last_request_millis = millis();
}


void loop()
{
    while((last_request_millis + 100) > millis()){ // wait for ECU to be chillin

    }

    current_error_msg = ERROR_MSG_NONE;

    raw_rpm = elm.getEngineSpeedRpm();
    rpm = (uint32_t)raw_rpm;
    DEBUG_PORT.println("speed: ");
    DEBUG_PORT.println(rpm);

    if(raw_rpm < 0){
        current_error_msg = ERROR_MSG_BAD_RPM_DATA;
    }

    if(abs(stepper_delta) > 100){
        DEBUG_PORT.print("stepper_delta excessive, possible error if this keeps reoccurring");
        DEBUG_PORT.println(stepper_delta);
        current_error_msg = ERROR_MSG_STEPPER_DELTA;
    }

    if((loop_counter & B0001111) == 0){
        global_voltage = elm.send_command("ATRV");
        DEBUG_PORT.println("voltage: ");
        DEBUG_PORT.println(global_voltage);
        global_voltage.trim();

        coolant_temp_c = elm.getEngineCoolantTempC();
        DEBUG_PORT.println("coolant_temp_c: ");
        DEBUG_PORT.println(coolant_temp_c);

        if(coolant_temp_c < -40){
            current_error_msg = ERROR_MSG_BAD_COOLANT_TEMP_DATA;
        }

        DEBUG_PORT.println("voltage trimmed as c_str: ");
        DEBUG_PORT.println(global_voltage.c_str());

        display_lcd_stuff(coolant_temp_c, global_voltage.c_str(), current_error_msg.c_str());
    }
    last_request_millis = millis();

    bright_lights = analogRead(CLUSTER_BACKLIGHT_PIN) < CLUSTER_BACKLIGHT_LOW_THRESHOLD;




    if(bright_lights){
        analogWrite(LCD_BACKLIGHT_PIN, LCD_BACKLIGHT_BRIGHT);
        for(int i = 0; i < 5; i++){
            ring.setPixelColor(10+i, white);
        }
        stepper_delta += tach.set_val(rpm, NEOP_BRIGHT);
    } else {
        analogWrite(LCD_BACKLIGHT_PIN, LCD_BACKLIGHT_DIM);
        for(int i = 0; i < 5; i++){
            ring.setPixelColor(10+i, grey);
        }
        stepper_delta += tach.set_val(rpm, NEOP_DIM);
    }

    for(int i = 0; i < 18; i++){
        ring.setPixelColor((i+15)%24, tach.get_color_at_index(i));
    }
    ring.show();
    loop_counter++;

    DEBUG_PORT.print("stepper_delta: ");
    DEBUG_PORT.println(stepper_delta);
}

void tick_stepper(){
    if(stepper_delta > 0) {
        stepper_single_step(true);
        stepper_delta -= 1;
    } else if (stepper_delta < 0){
        stepper_single_step(false);
        stepper_delta += 1;
    }
}


/**
 *
 * @param coolant_temp (-40..215)
 * @param voltage max 5 chars
 */
void display_lcd_stuff(int16_t coolant_temp_in, char const voltage[], char const message[]){
    char message_fixed_len[12] = {' '};
    strncpy(message_fixed_len, message, 12);

    char voltage_fixed_len[5] = {' '};
    strncpy(voltage_fixed_len, voltage, 5);

    bool was_negative = coolant_temp_in < 0;
    uint8_t coolant_temp = abs(coolant_temp_in);
    uint8_t digit_1_place = coolant_temp%10;
    coolant_temp/=10;
    uint8_t digit_10_place = coolant_temp%10;
    char coolant_temp_as_str[3];
    coolant_temp_as_str[2] = (char)(digit_1_place+0x30);
    coolant_temp_as_str[1] = (char)(digit_10_place+0x30);
    if(was_negative){
        coolant_temp_as_str[0] = '-';
    } else {
        coolant_temp/=10;
        uint8_t digit_100_place = coolant_temp%10;
        if(digit_100_place == 0){
            coolant_temp_as_str[0] = ' ';
        } else {
            coolant_temp_as_str[0] = (char)((digit_100_place)+0x30);
        }
    }

    uint8_t message_index = 0;

    lcd.clear();

    lcd.home();
    lcd.write(LCD_CUSTOM_CHAR_TEMP_L);
    lcd.write(LCD_CUSTOM_CHAR_TEMP_R);
    lcd.write(' ');
    lcd.write(' ');
    lcd.printstr(coolant_temp_as_str);
    lcd.write(LCD_CUSTOM_CHAR_DEGREES_C);
    lcd.write(' ');
    lcd.write(LCD_CUSTOM_CHAR_DIVIDER_LEFT);
    while(message_index<6){
        lcd.write(message_fixed_len[message_index]);
        message_index++;
    }
    lcd.setCursor(0,1);
    lcd.write(LCD_CUSTOM_CHAR_VBAT_L);
    lcd.write(LCD_CUSTOM_CHAR_VBAT_R);
    lcd.write(' ');
    lcd.printstr(voltage_fixed_len);
    lcd.setCursor(9,1);
    lcd.write(LCD_CUSTOM_CHAR_DIVIDER_LEFT);
    while(message_index<12){
        lcd.write(message_fixed_len[message_index]);
        message_index++;
    }
}

/**
 * executes n steps of the stepper motor
 * adds delays to cap the speed of the motor (`STEPPER_MIN_DELAY_MICROS` per step)
 * @param steps_to_do
 */
void do_n_steps(int16_t steps_to_do){
    last_step_micros = micros();
    bool is_forward = steps_to_do > 0;
    uint16_t steps_no_direction = abs(steps_to_do);
    uint16_t steps_done = 0;
    while(steps_done < steps_no_direction){
        while(last_step_micros+STEPPER_MIN_DELAY_MICROS > micros()){
        }
        stepper_single_step(is_forward);
        steps_done++;
    }
}

/**
 * does a single step in the given direction
 * @param is_forwards the direction. true means forwards (positive), false means backwards (negative)
 */
void stepper_single_step(bool is_forwards){
    if(is_forwards){
        stepper_current_step = (stepper_current_step+1)%4;
    } else {
        stepper_current_step = (stepper_current_step-1)%4;
    }
    stepper_set_step(stepper_current_step);
}

/**
 * sets the stepper motor to the given step position.
 * @param step the step of the motor to set (0..<4)
 * @attention if `step` is not one of [0,1,2,3], it will recurse with `step % 4`
 */
void stepper_set_step(uint8_t step){
    switch (step) {
        case 0:  // 1010
            digitalWrite(STEPPER_PIN_1, HIGH);
            digitalWrite(STEPPER_PIN_2, LOW);
            digitalWrite(STEPPER_PIN_3, HIGH);
            digitalWrite(STEPPER_PIN_4, LOW);
            break;
        case 1:  // 0110
            digitalWrite(STEPPER_PIN_1, LOW);
            digitalWrite(STEPPER_PIN_2, HIGH);
            digitalWrite(STEPPER_PIN_3, HIGH);
            digitalWrite(STEPPER_PIN_4, LOW);
            break;
        case 2:  //0101
            digitalWrite(STEPPER_PIN_1, LOW);
            digitalWrite(STEPPER_PIN_2, HIGH);
            digitalWrite(STEPPER_PIN_3, LOW);
            digitalWrite(STEPPER_PIN_4, HIGH);
            break;
        case 3:  //1001
            digitalWrite(STEPPER_PIN_1, HIGH);
            digitalWrite(STEPPER_PIN_2, LOW);
            digitalWrite(STEPPER_PIN_3, LOW);
            digitalWrite(STEPPER_PIN_4, HIGH);
            break;
        default:
            stepper_set_step(step%4);
    }
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
//Sending: 210001
//Waiting for response.
//First char arrived: 8
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
//Communication established with the ECU
//Sending: 0210C011
//Waiting for response.
//First char arrived: 8
//Next char arrived: 3
//Next char arrived: F
//Next char arrived: 0
//Next char arrived: 1
//Next char arrived: 0
//Next char arrived: 7
//Next char arrived: F
//Next char arrived: 0
//Next char arrived: 2
//Next char arrived: 1
//Next char arrived: 1
//Next char arrived: 1
//Next char arrived: 5
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//83F0107F021115
//RPM data either failed checksum or did not match request
//getEngineCoolantTempC:
//response length: 7
//actual checksum mod 256: (DECIMAL)21
//last byte: (DECIMAL)21
//parsed response:
//83 f0 10 7f 02 11 15 speed:
//0
//Sending: ATRV
//Waiting for response
//First char arrived: 1
//Next char arrived: 4
//Next char arrived: .
//Next char arrived: 2
//Next char arrived: 0
//Next char arrived: V
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//14.20V
//voltage:
//14.20V
//Sending: 02105011
//Waiting for response.
//First char arrived: 8
//Next char arrived: 3
//Next char arrived: F
//Next char arrived: 0
//Next char arrived: 1
//Next char arrived: 0
//Next char arrived: 7
//Next char arrived: F
//Next char arrived: 0
//Next char arrived: 2
//Next char arrived: 1
//Next char arrived: 1
//Next char arrived: 1
//Next char arrived: 5
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//83F0107F021115
//RPM data either failed checksum or did not match request.Returning SANITY_MIN_COOLANT_TEMP_CELSIUS
//getEngineCoolantTempC:
//response length: 7
//actual checksum mod 256: 21
//last byte: 21
//parsed response:
//83 f0 10 7f 02 11 15 coolant_temp_c:
//-100
//Sending: 0210C011
//Waiting for response.
//First char arrived: 8
//Next char arrived: 3
//Next char arrived: F
//Next char arrived: 0
//Next char arrived: 1
//Next char arrived: 0
//Next char arrived: 7
//Next char arrived: F
//Next char arrived: 0
//Next char arrived: 2
//Next char arrived: 1
//Next char arrived: 1
//Next char arrived: 1
//Next char arrived: 5
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//83F0107F021115
//RPM data either failed checksum or did not match request
//getEngineCoolantTempC:
//response length: 7
//actual checksum mod 256: (DECIMAL)21
//last byte: (DECIMAL)21
//parsed response:
//83 f0 10 7f 02 11 15 speed:
//0
//Sending: ATRV
//Waiting for response
//First char arrived: 1
//Next char arrived: 4
//Next char arrived: .
//Next char arrived: 3
//Next char arrived: 3
//Next char arrived: V
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//14.33V
//voltage:
//14.33V
//Sending: 02105011
//Waiting for response.
//First char arrived: 8
//Next char arrived: 3
//Next char arrived: F
//Next char arrived: 0
//Next char arrived: 1
//Next char arrived: 0
//Next char arrived: 7
//Next char arrived: F
//Next char arrived: 0
//Next char arrived: 2
//Next char arrived: 1
//Next char arrived: 1
//Next char arrived: 1
//Next char arrived: 5
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//83F0107F021115
//RPM data either failed checksum or did not match request.Returning SANITY_MIN_COOLANT_TEMP_CELSIUS
//getEngineCoolantTempC:
//response length: 7
//actual checksum mod 256: 21
//last byte: 21
//parsed response:
//83 f0 10 7f 02 11 15 coolant_temp_c:
//-100
//Sending: 0210C011
//Waiting for response.
//First char arrived: 8
//Next char arrived: 3
//Next char arrived: F
//Next char arrived: 0
//Next char arrived: 1
//Next char arrived: 0
//Next char arrived: 7
//Next char arrived: F
//Next char arrived: 0
//Next char arrived: 2
//Next char arrived: 1
//Next char arrived: 1
//Next char arrived: 1
//Next char arrived: 5
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//83F0107F021115
//RPM data either failed checksum or did not match request
//getEngineCoolantTempC:
//response length: 7
//actual checksum mod 256: (DECIMAL)21
//last byte: (DECIMAL)21
//parsed response:
//83 f0 10 7f 02 11 15 speed:
//0
//Sending: ATRV
//Waiting for response
//First char arrived: 1
//Next char arrived: 4
//Next char arrived: .
//Next char arrived: 2
//Next char arrived: 2
//Next char arrived: V
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//14.22V
//voltage:
//14.22V
//Sending: 02105011
//Waiting for response.
//First char arrived: 8
//Next char arrived: 3
//Next char arrived: F
//Next char arrived: 0
//Next char arrived: 1
//Next char arrived: 0
//Next char arrived: 7
//Next char arrived: F
//Next char arrived: 0
//Next char arrived: 2
//Next char arrived: 1
//Next char arrived: 1
//Next char arrived: 1
//Next char arrived: 5
//Next char arrived:
//Next char arrived:
//Next char arrived: >
//Found delimiter! Good response
//83F0107F021115
//RPM data either failed checksum or did not match request.Returning SANITY_MIN_COOLANT_TEMP_CELSIUS
//getEngineCoolantTempC:
//response length: 7
//actual checksum mod 256: 21
//last byte: 21
//parsed response:
//83 f0 10 7f 02 11 15 coolant_temp_c:
//-100






