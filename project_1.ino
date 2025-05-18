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
#define YELLOW_LED_PIN            7

#include <Arduino.h>
#include <IRremote.hpp>
#include <ESP32Servo.h>
#include <stdlib.h>
using namespace std;
//#include <Servo.h>

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
int plateServoPosition = 0;
int plateServoMinPosition = 0;
int plateServoMaxPosition = 180;

int extruderMoveServoPosition = 81;
int extruderMoveMinPosition = 80;
int extruderMoveMaxPosition = 180;

int extruderPushServoPosition = 0;
int extruderPushMinPosition = 0;
int extruderPushMaxPosition = 133;

enum ExtruderMode {
  SinglePill,
  RecipeScenario,
  ReplaceBlister
};
ExtruderMode mode = SinglePill;

Blister blisters[3] = { Blister( "Vitamin Yellow", 1), Blister( "Vitamin Green", 2), Blister( "Vitamin Blue", 3)};
int recipies[9][3] = { {1,1,2}, {1,-1,3}, {3,3,-1}, {-1,-1,-1}, {-1,-1,-1}, {-1,-1,-1}, {-1,-1,-1}, {-1,-1,-1}, {-1,-1,-1}};

// Published values for SG90 servos; adjust if needed
int minUs = 1000;
int maxUs = 2000;

void setup() {
    Serial.begin(115200);
    while (!Serial)
        ; // Wait for Serial to become available. Is optimized away for some cores.

    Serial.println(F("Using IRremote library version " VERSION_IRREMOTE));
    Serial.println(F("Enabling IR input..."));

    // Start the receiver and if not 3. parameter specified, take LED_BUILTIN pin from the internal boards definition as default feedback LED
    IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(YELLOW_LED_PIN, OUTPUT);

    //pwm.attachPin(7, 10000);//10khz
    //pwm.attachPin(2, 10000);//10khz
    //pwm.attachPin(3, 10000);//10khz
    //extruderMovePwm.attachPin(2, 10000);//10khz
    //extruderPushPwm.attachPin(3, 10000);//10khz
    //plateServo.setPeriodHertz(50);      // Standard 50hz servo
    //plateServo.attach(TABLE_SERVO_PIN, minUs, maxUs);
    plateServo.attach(TABLE_SERVO_PIN);
    plateServo.write(plateServoMaxPosition);
    delay(500);
    plateServo.write(plateServoMinPosition);

    extruderMoveServo.attach(EXTRUDER_MOVE_SERVO_PIN);
    extruderMoveServo.write(81);
    delay(50);
    //extruderMoveServo.write(extruderMoveMinPosition);

    extruderPushServo.attach(EXTRUDER_PUSH_SERVO_PIN);
    extruderPushServo.write(10);
    delay(50);
    extruderPushServo.write(extruderPushMinPosition);

    initPlate();
    // for(int i = 0; i < 10; i++){
    //     Serial.print(blisters[0].pills[i]._plateAgnle); 
    //     Serial.print(", "); 
    //     Serial.print(blisters[0].pills[i]._extruderAngle); 
    //     Serial.print(" - "); 
    //     Serial.println(blisters[0].pills[i]._present); 
    // }

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
        int cmd = IrReceiver.decodedIRData.command;
        switch (cmd) {
            case 70:
                if (mode == SinglePill) {
                    Serial.println("up");                    
                    if (extruderMoveServoPosition < extruderMoveMaxPosition){
                        extruderMoveServoPosition+=1;
                        moveExtruder(extruderMoveServoPosition);
                    }
                }                    
                break;
            case 21:
                if (mode == SinglePill) {
                    Serial.println("down");
                    if (extruderMoveServoPosition > extruderMoveMinPosition){
                        extruderMoveServoPosition-=1;
                        moveExtruder(extruderMoveServoPosition);
                    }
                }
                break;
            case 68:
                if (mode == SinglePill) {
                    Serial.println("left");
                    if (plateServoPosition > plateServoMinPosition){
                        plateServoPosition-=1;
                        movePlate(plateServoPosition);
                    }
                }
                break;
            case 67:
                if (mode == SinglePill) {
                    Serial.println("right");
                    if (plateServoPosition < plateServoMaxPosition){
                        plateServoPosition+=1;
                        movePlate(plateServoPosition);
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
                digitalWrite(RED_LED_PIN, HIGH);
                delay(500);
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

void movePlate(int angle) {
    Serial.print("move plate to: ");
    Serial.println(angle);
    plateServo.write(angle);
    delay(50);
}

void moveExtruder(int angle) {
    Serial.print("move extruder to: ");
    Serial.println(angle);
    extruderMoveServo.write(angle);
    delay(50);
}

void pushExtruder() {
    Serial.println("push");
    extruderPushServo.write(extruderPushMaxPosition);
    delay(500);
    extruderPushServo.write(extruderPushMinPosition);
}

void initPlate() {
    initBlister(0);
    initBlister(1);
    initBlister(2);
}

void initBlister(int blisterNumber) {
    blisters[blisterNumber].pills[0] = Pill(blisterNumber, 1);
    blisters[blisterNumber].pills[1] = Pill(blisterNumber, 2);
    blisters[blisterNumber].pills[2] = Pill(blisterNumber, 3);
    blisters[blisterNumber].pills[3] = Pill(blisterNumber, 4);
    blisters[blisterNumber].pills[4] = Pill(blisterNumber, 5);
    blisters[blisterNumber].pills[5] = Pill(blisterNumber, 6);
    blisters[blisterNumber].pills[6] = Pill(blisterNumber, 7);
    blisters[blisterNumber].pills[7] = Pill(blisterNumber, 8);
}

bool extrudePill(int blisterNumber) {
    bool pillFound = false;
    Serial.println(F("extrude pill from blister#" + blisterNumber));
    for (int i = 0; i < (sizeof(blisters[blisterNumber].pills)/sizeof(*blisters[blisterNumber].pills)); i++) {
        if (blisters[blisterNumber].pills[i]._present == true && pillFound == false) {
            movePlate(blisters[blisterNumber].pills[i]._plateAgnle);
            moveExtruder(blisters[blisterNumber].pills[i]._extruderAngle);
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
