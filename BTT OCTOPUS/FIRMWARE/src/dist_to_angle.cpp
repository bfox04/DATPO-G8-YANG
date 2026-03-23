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

// --- ENCODER DEFINITIONS ---
#define EAplus      PG6
#define EBplus      PG9

// 1000PPR encoder with CHANGE interrupt = 2000 ticks per rev
const float PULSES_PER_REV = 2000.0; 
volatile long encoderTicks = 0;
long lastReportedTicks = 0;

// --- MOTION SETTINGS ---
const int MICROSTEP_SETTING = 8;     
const long STEPS_PER_REV = 200 * MICROSTEP_SETTING;
const float X_PITCH = 5.0; // mm per revolution
const float Y_PITCH = 2.0; // mm per revolution

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

// ISR for encoder feedback
void handleEncoder() {
    int aState = digitalRead(EAplus);
    int bState = digitalRead(EBplus);
    if (aState == bState) encoderTicks++; else encoderTicks--;
}

void setup() {
    Serial.begin(115200);

    pinMode(FAN0_PIN, OUTPUT);
    digitalWrite(FAN0_PIN, HIGH); 

    for(int i = 0; i < numMotors; i++) {
        pinMode(enPins[i], OUTPUT);
        digitalWrite(enPins[i], LOW); 
        steppers[i].setMaxSpeed(5000);   
        steppers[i].setAcceleration(1000); 
    }

    pinMode(EAplus, INPUT_PULLUP);
    pinMode(EBplus, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(EAplus), handleEncoder, CHANGE);

    delay(3000);
    Serial.println("--- Airfoil Group Controller Ready ---");
    Serial.println("Commands: X[mm], Y[mm], ZA[deg], ZB[deg], or HOME");
}

void loop() {
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        input.toUpperCase();

        int startMotor = -1;
        int endMotor = -1;
        float targetDegrees = 0;
        bool valid = false;

        if (input == "HOME") {
            Serial.println(">> Homing all motors to 0.");
            for(int i = 0; i < numMotors; i++) steppers[i].moveTo(0);
        } 
        else if (input.startsWith("X")) {
            startMotor = 0; endMotor = 1;
            float mm = input.substring(1).toFloat();
            targetDegrees = (mm / X_PITCH) * 360.0;
            valid = true;
        } else if (input.startsWith("Y")) {
            startMotor = 2; endMotor = 5;
            float mm = input.substring(1).toFloat();
            targetDegrees = (mm / Y_PITCH) * 360.0;
            valid = true;
        } else if (input.startsWith("ZA")) {
            startMotor = 6; endMotor = 6;
            targetDegrees = input.substring(2).toFloat();
            targetDegrees*=5.197539843600339;
            valid = true;
        } else if (input.startsWith("ZB")) {
            startMotor = 7; endMotor = 7;
            targetDegrees = input.substring(2).toFloat();
            targetDegrees*=5.197539843600339;
            valid = true;
        }

        if (valid) {
            long targetSteps = (targetDegrees / 360.0) * STEPS_PER_REV;
            Serial.print(">> Moving to target steps: ");
            Serial.println(targetSteps);
            for(int i = startMotor; i <= endMotor; i++) steppers[i].moveTo(targetSteps);
        } else if (input != "HOME") {
            Serial.println("!! Invalid Command.");
        }
    }

    bool moving = false;
    for(int i = 0; i < numMotors; i++) {
        steppers[i].run();
        if (steppers[i].distanceToGo() != 0) moving = true;
    }

    static bool wasMoving = false;
    if (!moving && wasMoving) Serial.println("--- Motion Complete. ---");
    wasMoving = moving;

    if (encoderTicks != lastReportedTicks) {
        float angle = (static_cast<float>(encoderTicks) / PULSES_PER_REV) * 360.0;
        Serial.print("Encoder Angle: ");
        Serial.print(angle, 2); 
        Serial.println("°");
        lastReportedTicks = encoderTicks;
    }
}