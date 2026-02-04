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
float targetDegrees = 0;            // Variable to hold user input
const int MICROSTEP_SETTING = 8;     
const long STEPS_PER_REV = 200 * MICROSTEP_SETTING;

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

    // SETUP FANS
    pinMode(FAN0_PIN, OUTPUT);
    digitalWrite(FAN0_PIN, HIGH); 

    // SETUP MOTORS
    for(int i = 0; i < numMotors; i++) {
        pinMode(enPins[i], OUTPUT);
        digitalWrite(enPins[i], LOW); // Enable drivers
        
        steppers[i].setMaxSpeed(1000);   // Set a usable speed
        steppers[i].setAcceleration(500); // Set a usable acceleration
    }

    // SETUP ENCODER
    pinMode(EAplus, INPUT_PULLUP);
    pinMode(EBplus, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(EAplus), handleEncoder, CHANGE);

    delay(3000);
    Serial.println("--- Dual Airfoil Controller Ready ---");
    Serial.println("Enter target angle and press Enter:");
}

void loop() {
    // 1. CHECK FOR USER INPUT
    if (Serial.available() > 0) {
        // Read the input as a float
        targetDegrees = Serial.parseFloat();
        
        // Calculate steps required
        long targetSteps = (targetDegrees / 360.0) * STEPS_PER_REV;

        Serial.print(">> Command Received: Moving to ");
        Serial.print(targetDegrees);
        Serial.println(" degrees.");

        // Update all motors with the new target
        for(int i = 0; i < numMotors; i++) {
            steppers[i].moveTo(targetSteps);
        }

        // Clear the serial buffer of any leftover newline characters
        while(Serial.available() > 0) { Serial.read(); }
    }

    // 2. CONSTANTLY UPDATE MOTORS
    // This needs to run every loop iteration to step the motors
    bool moving = false;
    for(int i = 0; i < numMotors; i++) {
        steppers[i].run();
        if (steppers[i].distanceToGo() != 0) {
            moving = true;
        }
    }

    // 3. REPORT COMPLETION ONCE
    static bool wasMoving = false;
    if (!moving && wasMoving) {
        Serial.println("--- Motion Complete. Ready for next angle. ---");
    }
    wasMoving = moving;

    // 4. ENCODER REPORTING
    if (encoderTicks != lastReportedTicks) {
        float angle = abs((static_cast<float>(encoderTicks) / PULSES_PER_REV) * 360.0);
        Serial.print("Current Encoder Angle: ");
        Serial.print(angle, 2); 
        Serial.println("°");
        lastReportedTicks = encoderTicks;
    }
}