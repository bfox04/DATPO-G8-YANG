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

// Your spec is 1000PPR. Using one interrupt with CHANGE gives 2000 ticks per rev.
const float PULSES_PER_REV = 2000.0; 

volatile long encoderTicks = 0;
long lastReportedTicks = 0;

// --- MOTION SETTINGS ---
// 18.0 is a common multiple for 0.225 (motor) and 0.18 (encoder)
const float TARGET_DEGREES = 18.0;  
const int MICROSTEP_SETTING = 8;     
const long STEPS_PER_REV = 200 * MICROSTEP_SETTING;
const long TARGET_STEPS = (TARGET_DEGREES / 360.0) * STEPS_PER_REV;

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

// ISR updated to read both pins for direction sensing
void handleEncoder() {
    int aState = digitalRead(EAplus);
    int bState = digitalRead(EBplus);
    if (aState == bState) encoderTicks++; else encoderTicks--;
}

void setup() {
    Serial.begin(115200);

    // SETUP FANS
    pinMode(FAN0_PIN, OUTPUT);
    digitalWrite(FAN0_PIN, HIGH); 

    // SETUP MOTORS
    for(int i = 0; i < numMotors; i++) {
        pinMode(enPins[i], OUTPUT);
        digitalWrite(enPins[i], LOW);
        
        steppers[i].setMaxSpeed(100);
        steppers[i].setAcceleration(100);
        
        // Move all 8 motors for the dual airfoil platform
        steppers[i].moveTo(TARGET_STEPS); 
    }

    pinMode(EAplus, INPUT_PULLUP);
    pinMode(EBplus, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(EAplus), handleEncoder, CHANGE);

    delay(3000);
    Serial.print("Target set to: ");
    Serial.print(TARGET_DEGREES);
    Serial.println(" degrees."); 
    Serial.println("Startup delay complete. Program starting...");
}

void loop() {
    static bool motionComplete = false;

    // Run all motors simultaneously
    for(int i = 0; i < numMotors; i++) {
        steppers[i].run();
    }

    // Check completion once
    if (!motionComplete && steppers[0].distanceToGo() == 0) {
        Serial.println("--- Target Reached: All motors stopped ---");
        motionComplete = true;
    }

    // ENCODER REPORTING
    if (encoderTicks != lastReportedTicks) {
        // Calculate angle based on the 2000 tick feedback resolution
        float angle = (static_cast<float>(encoderTicks) / PULSES_PER_REV) * 360.0;
        Serial.print("Encoder Angle: ");
        Serial.print(angle, 4); 
        Serial.println("°");
        lastReportedTicks = encoderTicks;
    }
}