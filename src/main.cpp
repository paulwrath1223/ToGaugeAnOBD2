
#include "Arduino.h"
#include <KWP2000ELM.h>
#include <Stepper.h>
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
#define CLUSTER_BACKLIGHT_HIGH_THRESHOLD 200

#define LCD_CUSTOM_CHAR_DEGREES_C 0 // mapping for LCD lib between custom char data and index
#define LCD_CUSTOM_CHAR_TEMP_L 1
#define LCD_CUSTOM_CHAR_TEMP_R 2
#define LCD_CUSTOM_CHAR_DIVIDER 3
#define LCD_CUSTOM_CHAR_DIVIDER_LEFT 4
#define LCD_CUSTOM_CHAR_VBAT_L 5
#define LCD_CUSTOM_CHAR_VBAT_R 6

const char ERROR_MSG_NONE[] = "No    Errors";

Stepper stepper(STEPS_PER_315, 4, 5, 6, 7);
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
String global_voltage;
int16_t coolant_temp_c;
bool bright_lights = false;

void display_lcd_stuff(int16_t coolant_temp_in, const char voltage[], const char message[]);

KWP2000ELM elm = KWP2000ELM(ELM_PORT);

void setup()
{
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

    stepper.setSpeed(50);

    DEBUG_PORT.println("Connected to ELM327");
    elm.init_ECU_connection();

    lcd.home();
    lcd.print("conecting to ECU");

    stepper.step(tach.calibrate());

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
}


void loop()
{
    rpm = (uint32_t)elm.getEngineSpeedRpm();
    DEBUG_PORT.println("speed: ");
    DEBUG_PORT.println(rpm);

    global_voltage = elm.send_command("ATRV");
    DEBUG_PORT.println("voltage: ");
    DEBUG_PORT.println(global_voltage);
    global_voltage.trim();

    coolant_temp_c = elm.getEngineCoolantTempC();
    DEBUG_PORT.println("coolant_temp_c: ");
    DEBUG_PORT.println(coolant_temp_c);

    bright_lights = analogRead(CLUSTER_BACKLIGHT_PIN) > CLUSTER_BACKLIGHT_HIGH_THRESHOLD;

    display_lcd_stuff(coolant_temp_c, global_voltage.c_str(), ERROR_MSG_NONE);

    int32_t delta;
    if(bright_lights){
        analogWrite(LCD_BACKLIGHT_PIN, LCD_BACKLIGHT_BRIGHT);
        for(int i = 0; i < 5; i++){
            ring.setPixelColor(10+i, white);
        }
        delta = tach.set_val(rpm, NEOP_BRIGHT);
    } else {
        analogWrite(LCD_BACKLIGHT_PIN, LCD_BACKLIGHT_DIM);
        for(int i = 0; i < 5; i++){
            ring.setPixelColor(10+i, grey);
        }
        delta = tach.set_val(rpm, NEOP_DIM);
    }

    for(int i = 0; i < 18; i++){
        ring.setPixelColor((i+15)%24, tach.get_color_at_index(i));
    }
    ring.show();
    stepper.step(delta);
}

/**
 *
 * @param coolant_temp (-40..215)
 * @param voltage max 6 chars
 */
void display_lcd_stuff(const int16_t coolant_temp_in, const char voltage[], const char message[]){


    char message_fixed_len[12] = {' '};

    strncpy(message_fixed_len, message, 12);

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
    lcd.printstr(voltage);
    lcd.setCursor(9,1);
    lcd.write(LCD_CUSTOM_CHAR_DIVIDER_LEFT);
    while(message_index<12){
        lcd.write(message_fixed_len[message_index]);
        message_index++;
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






