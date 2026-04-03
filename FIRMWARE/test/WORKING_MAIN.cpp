#include <Arduino.h>
#include <AccelStepper.h>

// --- MOTOR DEFINITIONS ---

//m0
#define ENABLE_PIN0 PF14
#define STEP_PIN0   PF13
#define DIR_PIN0    PF12

//m1
#define ENABLE_PIN1 PF15
#define STEP_PIN1   PG0
#define DIR_PIN1    PG1

//m2
#define ENABLE_PIN2 PG5
#define STEP_PIN2   PF11
#define DIR_PIN2    PG3

//m3
#define ENABLE_PIN3 PA0
#define STEP_PIN3   PG4
#define DIR_PIN3    PC1

//m4
#define ENABLE_PIN4 PG2
#define STEP_PIN4   PF9
#define DIR_PIN4    PF10

//m5
#define ENABLE_PIN5 PF1
#define STEP_PIN5   PC13
#define DIR_PIN5    PF0

//m6
#define ENABLE_PIN6 PD4
#define STEP_PIN6   PE2
#define DIR_PIN6    PE3

//m7
#define ENABLE_PIN7 PE0
#define STEP_PIN7   PE6
#define DIR_PIN7    PA14

// --- ENCODER DEFINITIONS ---

//E0
#define EB0plus     PG6
#define EA0plus     PG9

//E1
#define EB1plus     PG10
#define EA1plus     PG11

// --- FAN DEFINITIONS ---
#define FAN0_PIN    PA8  
bool fanOn = true;

const float PULSES_PER_REV = 2000.0; 
volatile long encoder0Ticks = 0;
volatile long encoder1Ticks = 0;

// --- X ENCODER SYNC ALARM ---
const float X_SYNC_THRESHOLD_MM = 5.0;
bool syncAlarm = false;

// --- MOTION SETTINGS ---
const int MICROSTEP_SETTING = 8;     
const long STEPS_PER_REV = 200 * MICROSTEP_SETTING;
const float X_PITCH = 5.0; 
const float Y_PITCH = 2.0; 

// --- ZEROING MODE ---
bool zeroingMode = false;
// Jog increments: 0.25mm for X and Y, ~0.25deg for ZA/ZB
const long JOG_X_STEPS  = 80;   // 0.25mm: (0.25/5.0)*360/360*1600 = 80 steps
const long JOG_Y_STEPS  = 200;  // 0.25mm: (0.25/2.0)*360/360*1600 = 200 steps
const long JOG_ZA_STEPS = 6;    // ~0.251deg: 0.25*5.1975/360*1600 ≈ 6 steps
const long JOG_ZB_STEPS = 6;    // ~0.251deg: same as ZA

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
void handleEncoder0() {
    if (digitalRead(EA0plus) == digitalRead(EB0plus)) encoder0Ticks++; else encoder0Ticks--;
}
void handleEncoder1() {
    if (digitalRead(EA1plus) == digitalRead(EB1plus)) encoder1Ticks++; else encoder1Ticks--;
}

void sendResponse(const String& msg, InputSource source) {
    if (source == SOURCE_USB || source == SOURCE_NONE) Serial.println(msg);
    if (source == SOURCE_BT || source == SOURCE_NONE) Serial1.println(msg);
}

void processCommand(String input, InputSource source) {
    input.trim();
    input.toUpperCase();

    // --- Commands available in ANY mode ---
    if (input == "STOP") {
        for(int i = 0; i < numMotors; i++) {
            steppers[i].stop();
            steppers[i].setCurrentPosition(steppers[i].currentPosition());
        }
        syncAlarm = false;
        if (zeroingMode) { zeroingMode = false; sendResponse(">> STOPPED — Exited zeroing mode", source); }
        else { sendResponse(">> STOPPED", source); }
        return;
    }
    if (input == "FAN") {
        fanOn = !fanOn;
        digitalWrite(FAN0_PIN, fanOn ? HIGH : LOW);
        sendResponse(fanOn ? ">> Fan ON" : ">> Fan OFF", source);
        return;
    }
    if (input == "RESUME") {
        if (syncAlarm) {
            syncAlarm = false;
            sendResponse(">> Alarm cleared. Resuming motion.", source);
        } else {
            sendResponse(">> No alarm active.", source);
        }
        return;
    }
    if (input == "STATUS") {
        String status = "Positions: ";
        for(int i = 0; i < numMotors; i++) {
            status += "M" + String(i) + "=" + String(steppers[i].currentPosition()) + " ";
        }
        sendResponse(status, source);
        float angle0 = (static_cast<float>(encoder0Ticks) / PULSES_PER_REV) * 360.0;
        float angle1 = (static_cast<float>(encoder1Ticks) / PULSES_PER_REV) * 360.0;
        float disp0 = (angle0 / 360.0) * 5.0;
        float disp1 = (angle1 / 360.0) * 5.0;
        sendResponse("Encoders: E0=" + String(angle0, 2) + "deg (" + String(disp0, 2) + "mm) E1=" + String(angle1, 2) + "deg (" + String(disp1, 2) + "mm)", source);
        if (zeroingMode) sendResponse("[ZEROING MODE ACTIVE]", source);
        return;
    }

    // --- ENTER ZEROING MODE (available in any state) ---
    if (input == "ZERO" && !zeroingMode) {
        zeroingMode = true;
        if (syncAlarm) syncAlarm = false;  // clear alarm when entering zero mode
        sendResponse(">> ZEROING MODE — Jog with M0+/- X+/- Y+/- ZA+/- ZB+/-", source);
        sendResponse(">> SET to save zero, EXIT to cancel. OPTIONS for help.", source);
        return;
    }

    // --- ZEROING MODE ---
    if (zeroingMode) {
        // Jog individual motors: M0+ M0- M1+ M1- ... M7+ M7-
        if (input.length() >= 3 && input.charAt(0) == 'M') {
            int mNum = input.substring(1, input.length() - 1).toInt();
            char dir = input.charAt(input.length() - 1);
            if (mNum >= 0 && mNum < numMotors && (dir == '+' || dir == '-')) {
                long jogAmount;
                if (mNum <= 1) jogAmount = JOG_X_STEPS;
                else if (mNum <= 5) jogAmount = JOG_Y_STEPS;
                else jogAmount = JOG_ZA_STEPS;
                long delta = (dir == '+') ? jogAmount : -jogAmount;
                steppers[mNum].move(delta);
                sendResponse(">> Jog M" + String(mNum) + " " + String(dir) + " (" + String(delta) + " steps)", source);
                return;
            }
        }
        // Jog groups: X+ X- Y+ Y- ZA+ ZA- ZB+ ZB-
        if (input == "X+" || input == "X-") {
            long delta = (input.charAt(1) == '+') ? JOG_X_STEPS : -JOG_X_STEPS;
            steppers[0].move(delta);
            steppers[1].move(delta);
            sendResponse(">> Jog X group " + String(input.charAt(1)) + " (" + String(delta) + " steps)", source);
            return;
        }
        if (input == "Y+" || input == "Y-") {
            long delta = (input.charAt(1) == '+') ? JOG_Y_STEPS : -JOG_Y_STEPS;
            for (int i = 2; i <= 5; i++) steppers[i].move(delta);
            sendResponse(">> Jog Y group " + String(input.charAt(1)) + " (" + String(delta) + " steps)", source);
            return;
        }
        if (input == "ZA+" || input == "ZA-") {
            long delta = (input.charAt(2) == '+') ? JOG_ZA_STEPS : -JOG_ZA_STEPS;
            steppers[6].move(delta);
            sendResponse(">> Jog ZA " + String(input.charAt(2)) + " (" + String(delta) + " steps)", source);
            return;
        }
        if (input == "ZB+" || input == "ZB-") {
            long delta = (input.charAt(2) == '+') ? JOG_ZB_STEPS : -JOG_ZB_STEPS;
            steppers[7].move(delta);
            sendResponse(">> Jog ZB " + String(input.charAt(2)) + " (" + String(delta) + " steps)", source);
            return;
        }
        // SET: save current positions as zero
        if (input == "SET") {
            for (int i = 0; i < numMotors; i++) steppers[i].setCurrentPosition(0);
            encoder0Ticks = 0;
            encoder1Ticks = 0;
            sendResponse(">> Zero point SET — all positions reset to 0", source);
            return;
        }
        // EXIT: leave zeroing mode without saving
        if (input == "EXIT") {
            zeroingMode = false;
            sendResponse(">> Exited zeroing mode (positions unchanged)", source);
            return;
        }
        if (input == "OPTIONS") {
            sendResponse("--- ZEROING MODE ---", source);
            sendResponse("Jog: M0+ M0- ... M7+  M7-  (individual motors)", source);
            sendResponse("Jog: X+ X- Y+ Y- ZA+ ZA- ZB+ ZB- (groups)", source);
            sendResponse("SET = save current pos as zero", source);
            sendResponse("EXIT = leave without saving", source);
            sendResponse("STATUS, FAN, STOP also available", source);
            return;
        }
        sendResponse("!! Zeroing mode — use jog commands, SET, EXIT, or OPTIONS", source);
        return;
    }

    // --- NORMAL MODE ---
    if (input == "OPTIONS") {
        sendResponse("--- Airfoil Group Controller ---", source);
        sendResponse("Commands: X[mm], Y[mm], ZA[deg], ZB[deg], HOME, STATUS, OPTIONS, FAN, STOP, ZERO, RESUME", source);
        return;
    }
    if (input == "HOME") {
        sendResponse(">> Homing...", source);
        for(int i = 0; i < numMotors; i++) steppers[i].moveTo(0);
        return;
    }

    int startMotor = -1;
    int endMotor = -1;
    float targetDegrees = 0;
    bool valid = false;

    if (input.startsWith("X")) {
        float mm = input.substring(1).toFloat();
        if (mm > 415) {
            sendResponse("TOO LARGE, 415mm is the maximum", source);
            return;
        }
        startMotor = 0; endMotor = 1;
        targetDegrees = (mm / X_PITCH) * 360.0;
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
    }

    if (syncAlarm && valid) {
        sendResponse("!! SYNC ALARM ACTIVE — send RESUME or STOP first", source);
        return;
    }

    if (valid) {
        long targetSteps = (targetDegrees / 360.0) * STEPS_PER_REV;
        sendResponse(">> Target Steps: " + String(targetSteps), source);
        for(int i = startMotor; i <= endMotor; i++) {
            if(i==99){
                steppers[i].moveTo(-targetSteps);
            } else {
                steppers[i].moveTo(targetSteps);
            }
        }
    } else if (input.length() > 0) {
        sendResponse("!!! Invalid Command. See OPTIONS for valid commands.", source);
    }
}

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200); 

    sendResponse("Startup in", SOURCE_NONE);
    // Countdown before starting
    for (int i = 5; i >= 0; i--) {
        sendResponse(String(i), SOURCE_NONE);
        delay(1000);
    }

    pinMode(FAN0_PIN, OUTPUT);
    digitalWrite(FAN0_PIN, HIGH); 

    for(int i = 0; i < numMotors; i++) {
        pinMode(enPins[i], OUTPUT);
        digitalWrite(enPins[i], LOW); 

        steppers[i].setMaxSpeed(5000);   
        steppers[i].setAcceleration(750); 
    }

    pinMode(EA0plus, INPUT_PULLUP);
    pinMode(EB0plus, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(EA0plus), handleEncoder0, CHANGE);

    pinMode(EA1plus, INPUT_PULLUP);
    pinMode(EB1plus, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(EA1plus), handleEncoder1, CHANGE);

    delay(500);
    sendResponse("--- System Ready (Sync Test) ---", SOURCE_NONE);
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

    if (!syncAlarm) {
        for(int i = 0; i < numMotors; i++) {
            steppers[i].run();
            if (steppers[i].distanceToGo() != 0) moving = true;
        }

        // --- X encoder sync check (only while X motors are moving, NOT in zeroing mode) ---
        bool xMoving = (steppers[0].distanceToGo() != 0) || (steppers[1].distanceToGo() != 0);
        if (xMoving && !zeroingMode) {
            float e0mm = (static_cast<float>(encoder0Ticks) / PULSES_PER_REV) * X_PITCH;
            float e1mm = (static_cast<float>(encoder1Ticks) / PULSES_PER_REV) * X_PITCH;
            float diff = abs(e0mm - e1mm);

            if (diff > X_SYNC_THRESHOLD_MM) {
                for (int i = 0; i < numMotors; i++) steppers[i].stop();
                syncAlarm = true;
                sendResponse("!!! SYNC ALARM — E0=" + String(e0mm, 2) + "mm E1=" + String(e1mm, 2) + "mm (diff=" + String(diff, 2) + "mm)", lastSource);
                sendResponse("!!! Motors paused. Send RESUME to continue or STOP to cancel.", lastSource);
            }
        }
    } else {
        moving = false;
    }

    static bool wasMoving = false;
    if (!moving && wasMoving) {
        sendResponse("--- Motion Complete. ---", lastSource);
    }
    wasMoving = moving;
}