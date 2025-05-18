#define NO_LED_FEEDBACK_CODE   // The  WS2812 on pin 8 of AI-C3 board crashes if used as receive feedback LED, other I/O pins are working...
#define IR_RECEIVE_PIN           4  
#define IR_SEND_PIN              7
#define TONE_PIN                10
#define APPLICATION_PIN         18
#define TABLE_SERVO_PIN          15
#define EXTRUDER_MOVE_SERVO_PIN  23
#define EXTRUDER_PUSH_SERVO_PIN  22
#define RED_LED_PIN               5
#define GREEN_LED_PIN             6
#define YELLOW_LED_PIN           11


#include <Arduino.h>
#include <IRremote.hpp>
#include <ESP32Servo.h>
#include <stdlib.h>
using namespace std;
//#include <Servo.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
//#include <SPI.h>
//#include <SD.h>

class Pill{
    public:
        int _plateAgnle;
        int _extruderAngle;
        bool _present;
        Pill() {
            this->_plateAgnle = 1;
            this->_extruderAngle = 1;
            this-> _present = false;
        };
        Pill(int plateAgnle, int extruderAngle){
            this->_plateAgnle = plateAgnle;
            this->_extruderAngle = extruderAngle;
            this-> _present = true;
        }
};

class Blister{
    public:
        String _name;
        int _position;
        Pill pills[10] = { Pill(), Pill(), Pill(), Pill(), Pill(), Pill(), Pill(), Pill(), Pill(), Pill() };
        Blister(String name, int pos) {
            this->_name = name;
            this->_position = pos;
        }
        String GetName() {
            return this->_name;
        }
};


Servo  plateServo, extruderMoveServo, extruderPushServo;
//ESP32PWM pwm;
//ESP32PWM extruderMovePwm;
//ESP32PWM extruderPushPwm;
int plateServoCurrentPosition = 52;
int plateServoMinPosition = 0;
int plateServoMaxPosition = 300;
int plateServoReplacePosition = 55;

int extruderMoveCurrentPosition = 90;
int extruderMoveMinPosition = 50;
int extruderMoveMaxPosition = 153;

int extruderPushCurrentPosition = 90;
int extruderPushMinPosition = 0;
int extruderPushMaxPosition = 133;
//int extruderPushMaxPosition = 80;

enum ExtruderMode {
  SinglePill,
  RecipeScenario,
  ReplaceBlister
};
ExtruderMode mode = SinglePill;

Blister blisters[3] = { Blister( "Vitamin Yellow", 1), Blister( "Vitamin Green", 2), Blister( "Vitamin Blue", 3)};
int recipies[9][3] = { {1,1,2}, {1,-1,3}, {3,3,-1}, {-1,-1,-1}, {-1,-1,-1}, {-1,-1,-1}, {-1,-1,-1}, {-1,-1,-1}, {-1,-1,-1}};

LiquidCrystal_I2C lcd(0x27,16,2);
//int cs = D8;

void setup() {
    Serial.begin(115200);

    // Wire.begin();
    // lcd.init();                 // initialize the lcd 
    // lcd.backlight();
    // lcd.setCursor(0,0);
    // lcd.print("Hello, world!");
    // lcd.setCursor(0,1);
    // lcd.print("ESP32!");
    //while (!Serial)
    //    ; // Wait for Serial to become available. Is optimized away for some cores.

    // if(!SD.begin(cs)) {
    //     Serial.prontln("cannot init SD!");
    // } else {
    //     Serial.println("SD init ok")
    // }

    // File f = SD.Open("config.txt", "w")];
    // f.write("test");
    // f.close();
    // f = SD.Open("config.txt", "r")];
    // Serial.print("file name: ");
    // Serial.println(f.fullName());
    // Serial.print("file size: ");
    // Serial.println(f.size());

    Serial.println(F("Using IRremote library version " VERSION_IRREMOTE));
    Serial.println(F("Enabling IR input..."));

    // Start the receiver and if not 3. parameter specified, take LED_BUILTIN pin from the internal boards definition as default feedback LED
    IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(YELLOW_LED_PIN, OUTPUT);

    plateServo.attach(TABLE_SERVO_PIN);
    plateServo.write(plateServoMinPosition);
    delay(50);
    plateServo.write(plateServoCurrentPosition);
    delay(50);

    extruderMoveServo.attach(EXTRUDER_MOVE_SERVO_PIN);
    //extruderMoveServo.write(extruderMoveMinPosition);
    //delay(50);
    extruderMoveServo.write(extruderMoveCurrentPosition);
    delay(50);

    extruderPushServo.attach(EXTRUDER_PUSH_SERVO_PIN);
    extruderPushServo.write(20);
    delay(50);
    extruderPushServo.write(extruderPushMinPosition);
    delay(50);

    initPlate();
    void initBlister2();
    digitalWrite(GREEN_LED_PIN, HIGH);
}

void loop() {    
    processIRsignal();
    IrReceiver.resume();
}


void handleOverflow() {
    Serial.println(F("Overflow detected"));
    Serial.println("Try to increase the \"RAW_BUFFER_LENGTH\" value of RAW_BUFFER_LENGTH");
    // see also https://github.com/Arduino-IRremote/Arduino-IRremote#compile-options--macros-for-this-library

#if !defined(ESP8266) && !defined(NRF5) // tone on esp8266 works once, then it disables IrReceiver.restartTimer() / timerConfigForReceive().
    /*
     * Stop timer, generate a double beep and start timer again
     */
#  if defined(ESP32) // ESP32 uses another timer for tone()
    tone(TONE_PIN, 1100, 10);
    delay(50);
    tone(TONE_PIN, 1100, 10);
#  else
    IrReceiver.stopTimer();
    tone(TONE_PIN, 1100, 10);
    delay(50);
    tone(TONE_PIN, 1100, 10);
    delay(50);
    IrReceiver.restartTimer();
#  endif
#endif
}

bool processIRsignal() {
    if (IrReceiver.decode()) {
        digitalWrite(GREEN_LED_PIN, LOW);
        int cmd = IrReceiver.decodedIRData.command;
        switch (cmd) {
            case 70:
                if (mode == SinglePill) {
                    Serial.println("up");                    
                    if (extruderMoveCurrentPosition < extruderMoveMaxPosition){
                        extruderMoveCurrentPosition+=1;
                        moveExtruder();
                    }
                }                    
                break;
            case 21:
                if (mode == SinglePill) {
                    Serial.println("down");
                    if (extruderMoveCurrentPosition > extruderMoveMinPosition){
                        extruderMoveCurrentPosition-=1;
                        moveExtruder();
                    }
                }
                break;
            case 68:
                if (mode == SinglePill) {
                    Serial.println("left");
                    if (plateServoCurrentPosition > plateServoMinPosition){
                        plateServoCurrentPosition-=1;
                        movePlate();
                    }
                }
                break;
            case 67:
                if (mode == SinglePill) {
                    Serial.println("right");
                    if (plateServoCurrentPosition < plateServoMaxPosition){
                        plateServoCurrentPosition+=1;
                        movePlate();
                    }
                }
                break;
            case 64:
                if (mode == SinglePill) {
                    Serial.println("OK");
                    pushExtruder();
                }
                break;
            case 66:
                Serial.println("*");
                mode = RecipeScenario;
                Serial.print("Recepe mode, select recepe number. ");
                digitalWrite(YELLOW_LED_PIN, HIGH);
                delay(500);
                break;
            case 74:
                Serial.println("#");
                mode = ReplaceBlister;
                Serial.print("Repalce blister mode, select blister number appeared to  be empty. ");
                plateServoCurrentPosition = plateServoReplacePosition;
                movePlate();
                delay(500);
                digitalWrite(RED_LED_PIN, HIGH);
                break;
            case 22:
                Serial.println("1");
                processNumericCommand(1);
                break;
            case 25:
                Serial.println("2");
                processNumericCommand(2);
                break;
            case 13:
                Serial.println("3");
                processNumericCommand(3);
                break;
            case 12:
                Serial.println("4");
                processNumericCommand(4);
                break;
            case 24:
                Serial.println("5");
                processNumericCommand(5);
                break;
            case 94:
                Serial.println("6");
                processNumericCommand(6);
                break;
            case 8:
                Serial.println("7");
                processNumericCommand(7);
                break;
            case 28:
                Serial.println("8");
                processNumericCommand(8);
                break;
            case 90:
                Serial.println("9");
                processNumericCommand(9);
                break;
            case 82:
                Serial.println("0");
                processNumericCommand(0);
                break;
            default:
                Serial.print("unknown command:");
                Serial.println(cmd);
                break;
        }
        digitalWrite(GREEN_LED_PIN, HIGH);
        //delay(100);        
        return true;
    } 
    return false;
}

void processNumericCommand(int number) {
    switch (mode) {
        case SinglePill:
            extrudePill(number-1); //extrude next pill from blister #number
            break;
        case RecipeScenario:
            extrudeRecipe(number-1);
            break;
        case ReplaceBlister:
            replaceBlister(number-1);
            break;
        default:
            break;
    }
    mode = SinglePill;
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, LOW);
}

void movePlate() {
    if( plateServoCurrentPosition >= plateServoMinPosition && plateServoCurrentPosition <= plateServoMaxPosition) {
        Serial.print("move plate to: ");
        Serial.println(plateServoCurrentPosition);
        plateServo.write(plateServoCurrentPosition);
        //delay(1000);
    } else {
        Serial.print("plate reached threshould angle");
        alarm(1);
    }
    
}

void moveExtruder() {
    if( extruderMoveCurrentPosition >= extruderMoveMinPosition && extruderMoveCurrentPosition <= extruderMoveMaxPosition) {
        Serial.print("move extruder to: ");
        Serial.println(extruderMoveCurrentPosition);
        extruderMoveServo.write(extruderMoveCurrentPosition);
    } else {
        Serial.print("extruder reached threshould angle");
        alarm(1);
    }
}

void pushExtruder() {
    Serial.println("push");
    //extruderPushServo.write(90);
    extruderPushServo.write(extruderPushMaxPosition);
    delay(1000);
    extruderPushServo.write(extruderPushMinPosition);
}

void initPlate() {
    initBlister(0);
    initBlister(1);
    initBlister(2);
}

void initBlister(int blisterNumber) {
    switch(blisterNumber) {
        case 0:
            blisters[blisterNumber].pills[0] = Pill(53, 133);
            blisters[blisterNumber].pills[1] = Pill(58, 148);
            blisters[blisterNumber].pills[2] = Pill(65, 123);
            blisters[blisterNumber].pills[3] = Pill(70, 137);
            blisters[blisterNumber].pills[4] = Pill(73, 109);
            blisters[blisterNumber].pills[5] = Pill(78, 120);
            blisters[blisterNumber].pills[6] = Pill(82, 89);
            blisters[blisterNumber].pills[7] = Pill(87, 102);
            break;
        case 1:
            blisters[blisterNumber].pills[0] = Pill(75, 160);
            blisters[blisterNumber].pills[1] = Pill(99, 160);
            blisters[blisterNumber].pills[2] = Pill(82, 147);
            blisters[blisterNumber].pills[3] = Pill(97, 148);
            blisters[blisterNumber].pills[4] = Pill(89, 130);
            blisters[blisterNumber].pills[5] = Pill(99, 130);
            blisters[blisterNumber].pills[6] = Pill(96, 109);
            blisters[blisterNumber].pills[7] = Pill(103, 109);
            break;
        case 2:
            blisters[blisterNumber].pills[0] = Pill(124, 149);
            blisters[blisterNumber].pills[1] = Pill(133, 136);
            blisters[blisterNumber].pills[2] = Pill(116, 139);
            blisters[blisterNumber].pills[3] = Pill(128, 127);
            blisters[blisterNumber].pills[4] = Pill(114, 120);
            blisters[blisterNumber].pills[5] = Pill(126, 110);
            blisters[blisterNumber].pills[6] = Pill(117, 101);
            blisters[blisterNumber].pills[7] = Pill(124, 93);
            break;
        default:
            break;
    }
}

bool extrudePill(int blisterNumber) {
    bool pillFound = false;
    Serial.print("extrude pill from blister #");
    Serial.println(blisterNumber);
    for (int i = 0; i < (sizeof(blisters[blisterNumber].pills)/sizeof(*blisters[blisterNumber].pills)); i++) {
        if (blisters[blisterNumber].pills[i]._present == true && pillFound == false) {
            plateServoCurrentPosition = blisters[blisterNumber].pills[i]._plateAgnle;
            movePlate();
            delay(500);
            extruderMoveCurrentPosition = blisters[blisterNumber].pills[i]._extruderAngle;
            moveExtruder();
            delay(100);
            pushExtruder();
            blisters[blisterNumber].pills[i]._present = false;
            pillFound = true;
        }
    }
    if (pillFound == false) {
        Serial.println(F("no pills found in blister#" + blisterNumber));
        alarm(3);
    }
    return pillFound;
}

void replaceBlister(int blisterNumber) {
    initBlister(blisterNumber);
    Serial.print("blister #");
    Serial.print(blisterNumber+1);
    Serial.println(" successfully replaced.");
}

void extrudeRecipe(int recipeNumber) {
    Serial.println(F("By the reipe #" + (recipeNumber+1) ));
    bool blisterReplaced = false;
    bool pillFound = false;
    for (int i = 0; i < 3; i++) {
        if (recipies[recipeNumber][i] >= 0) {
            pillFound = extrudePill(recipies[recipeNumber][i]-1);
            while( !pillFound ) {
                blisterReplaced = false;
                Serial.println("replace the blister to proceed with recipe!");
                digitalWrite(RED_LED_PIN, HIGH);
                while( !blisterReplaced ) {
                    mode = ReplaceBlister;
                    IrReceiver.resume();
                    blisterReplaced = processIRsignal();
                }
                pillFound = extrudePill(recipies[recipeNumber][i]-1);
            }
            digitalWrite(RED_LED_PIN, LOW);
            Serial.println(" extruded.");
        }
    }
    Serial.println("successfully processed.");
}

void alarm(int level){
    for (int i = 0; i < level; i++) {
        digitalWrite(RED_LED_PIN, HIGH);
        delay(500);
        digitalWrite(RED_LED_PIN, LOW);
        delay(100);
    }
}
