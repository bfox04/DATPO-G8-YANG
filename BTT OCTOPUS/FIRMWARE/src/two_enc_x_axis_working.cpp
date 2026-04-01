#include <Arduino.h>
#include <AccelStepper.h>

// --- MOTOR DEFINITIONS ---
#define ENABLE_PIN0 PF14
#define STEP_PIN0   PF13
#define DIR_PIN0    PF12

#define ENABLE_PIN1 PF15
#define STEP_PIN1   PG0
#define DIR_PIN1    PG1

#define ENABLE_PIN2 PG5
#define STEP_PIN2   PF11
#define DIR_PIN2    PG3

#define ENABLE_PIN3 PA0
#define STEP_PIN3   PG4
#define DIR_PIN3    PC1

#define ENABLE_PIN4 PG2
#define STEP_PIN4   PF9
#define DIR_PIN4    PF10

#define ENABLE_PIN5 PF1
#define STEP_PIN5   PC13
#define DIR_PIN5    PF0

#define ENABLE_PIN6 PD4
#define STEP_PIN6   PE2
#define DIR_PIN6    PE3

#define ENABLE_PIN7 PE0
#define STEP_PIN7   PE6
#define DIR_PIN7    PA14

// --- FAN DEFINITIONS ---
#define FAN0_PIN    PA8  
bool fanOn = true;

// --- ENCODER DEFINITIONS ---
#define EA1plus     PG6
#define EB1plus     PG9
#define EA2plus     PG10
#define EB2plus     PG11

const float PULSES_PER_REV = 2000.0; 
volatile long encoder1Ticks = 0;
volatile long encoder2Ticks = 0;

// --- MOTION SETTINGS ---
const int MICROSTEP_SETTING = 8;     
const long STEPS_PER_REV = 200 * MICROSTEP_SETTING;
const float X_PITCH = 5.0; 
const float Y_PITCH = 2.0; 

// --- ACCELSTEPPER OBJECTS ---
AccelStepper steppers[] = {
    AccelStepper(AccelStepper::DRIVER, STEP_PIN0, DIR_PIN0),
    AccelStepper(AccelStepper::DRIVER, STEP_PIN1, DIR_PIN1),
    AccelStepper(AccelStepper::DRIVER, STEP_PIN2, DIR_PIN2),
    AccelStepper(AccelStepper::DRIVER, STEP_PIN3, DIR_PIN3),
    AccelStepper(AccelStepper::DRIVER, STEP_PIN4, DIR_PIN4),
    AccelStepper(AccelStepper::DRIVER, STEP_PIN5, DIR_PIN5),
    AccelStepper(AccelStepper::DRIVER, STEP_PIN6, DIR_PIN6),
    AccelStepper(AccelStepper::DRIVER, STEP_PIN7, DIR_PIN7)
};

const int numMotors = 8;
int enPins[] = {ENABLE_PIN0, ENABLE_PIN1, ENABLE_PIN2, ENABLE_PIN3, ENABLE_PIN4, ENABLE_PIN5, ENABLE_PIN6, ENABLE_PIN7};

enum InputSource { SOURCE_NONE, SOURCE_USB, SOURCE_BT };
InputSource lastSource = SOURCE_NONE;

// --- ISRs ---
void handleEncoder1() {
    if (digitalRead(EA1plus) == digitalRead(EB1plus)) encoder1Ticks++; else encoder1Ticks--;
}

void handleEncoder2() {
    if (digitalRead(EA2plus) == digitalRead(EB2plus)) encoder2Ticks++; else encoder2Ticks--;
}

void sendResponse(const String& msg, InputSource source) {
    if (source == SOURCE_USB || source == SOURCE_NONE) Serial.println(msg);
    if (source == SOURCE_BT || source == SOURCE_NONE) Serial1.println(msg);
}

void processCommand(String input, InputSource source) {
    input.trim();
    input.toUpperCase();

    int startMotor = -1;
    int endMotor = -1;
    float targetDegrees = 0;
    bool valid = false;

    if (input == "STATUS") {
        String status = "Positions: ";
        for(int i = 0; i < numMotors; i++) {
            status += "M" + String(i) + "=" + String(steppers[i].currentPosition()) + " ";
        }
        sendResponse(status, source);
        
        float angle1 = (static_cast<float>(encoder1Ticks) / PULSES_PER_REV) * 360.0;
        float angle2 = (static_cast<float>(encoder2Ticks) / PULSES_PER_REV) * 360.0;
        sendResponse("Encoders: E1=" + String(angle1, 2) + " E2=" + String(angle2, 2), source);
        return;
    } 
    
    if (input == "HOME") {
        sendResponse(">> Homing...", source);
        for(int i = 0; i < numMotors; i++) steppers[i].moveTo(0);
        return;
    } 
    
    if (input.startsWith("X")) {
        startMotor = 0; endMotor = 1;
        targetDegrees = (input.substring(1).toFloat() / X_PITCH) * 360.0;
        valid = true;
    } else if (input.startsWith("Y")) {
        startMotor = 2; endMotor = 5;
        targetDegrees = (input.substring(1).toFloat() / Y_PITCH) * 360.0;
        valid = true;
    } else if (input.startsWith("ZA")) {
        startMotor = 6; endMotor = 6;
        targetDegrees = input.substring(2).toFloat() * 5.197539843600339;
        valid = true;
    } else if (input.startsWith("ZB")) {
        startMotor = 7; endMotor = 7;
        targetDegrees = input.substring(2).toFloat() * 5.197539843600339;
        valid = true;
    } else if (input == "STOP") {
        for(int i = 0; i < numMotors; i++) {
            steppers[i].stop();
            steppers[i].setCurrentPosition(steppers[i].currentPosition());
        }
        sendResponse(">> STOPPED", source);
        return;
    } else if (input == "FAN") {
        fanOn = !fanOn;
        digitalWrite(FAN0_PIN, fanOn ? HIGH : LOW);
        sendResponse(fanOn ? ">> Fan ON" : ">> Fan OFF", source);
        return;
    }

    if (valid) {
        long targetSteps = (targetDegrees / 360.0) * STEPS_PER_REV;
        sendResponse(">> Target: " + String(targetSteps), source);
        for(int i = startMotor; i <= endMotor; i++) steppers[i].moveTo(targetSteps);
    }
}

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200); // Set back to 115200 for HC-05

    pinMode(FAN0_PIN, OUTPUT);
    digitalWrite(FAN0_PIN, HIGH); 

    for(int i = 0; i < numMotors; i++) {
        pinMode(enPins[i], OUTPUT);
        digitalWrite(enPins[i], LOW); 
        steppers[i].setMaxSpeed(5000);   
        steppers[i].setAcceleration(1000); 
    }

    pinMode(EA1plus, INPUT_PULLUP);
    pinMode(EB1plus, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(EA1plus), handleEncoder1, CHANGE);

    pinMode(EA2plus, INPUT_PULLUP);
    pinMode(EB2plus, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(EA2plus), handleEncoder2, CHANGE);

    delay(2000);
    sendResponse("--- System Ready (Dual Encoders) ---", SOURCE_NONE);
}

void loop() {
    if (Serial.available() > 0) {
        lastSource = SOURCE_USB;
        processCommand(Serial.readStringUntil('\n'), SOURCE_USB);
    }
    
    if (Serial1.available() > 0) {
        lastSource = SOURCE_BT;
        processCommand(Serial1.readStringUntil('\n'), SOURCE_BT);
    }

    bool moving = false;
    for(int i = 0; i < numMotors; i++) {
        steppers[i].run();
        if (steppers[i].distanceToGo() != 0) moving = true;
    }

    static bool wasMoving = false;
    if (!moving && wasMoving) {
        sendResponse("--- Motion Complete. ---", lastSource);
    }
    wasMoving = moving;
}