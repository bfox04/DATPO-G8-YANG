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

//E0 — X left (M0)
#define EB0plus     PG6
#define EA0plus     PG9

//E1 — X right (M1)
#define EB1plus     PG10
#define EA1plus     PG11

//E2 — Y rear left (M2)
#define EB2plus     PG12
#define EA2plus     PG13

//E3 — Y front right (M5)
#define EB3plus     PG14
#define EA3plus     PG15

// --- FAN DEFINITIONS ---
#define FAN0_PIN    PA8  
bool fanOn = true;

const float PULSES_PER_REV = 2000.0; 
volatile long encoder0Ticks = 0;
volatile long encoder1Ticks = 0;
volatile long encoder2Ticks = 0;
volatile long encoder3Ticks = 0;

// --- X ENCODER SYNC ALARM ---
const float X_SYNC_THRESHOLD_MM = 5.0;
// --- Y ENCODER SYNC ALARM ---
const float Y_SYNC_THRESHOLD_MM = 5.0;
bool syncAlarm = false;
String alarmSource = "";  // "X" or "Y" — which axis triggered

// --- MOTION SETTINGS ---
const int MICROSTEP_SETTING = 8;     
const long STEPS_PER_REV = 200 * MICROSTEP_SETTING;
const float X_PITCH = 5.0; 
const float Y_PITCH = 2.0; 
const float ZA_GEAR_RATIO = 5.197539843600339;
const float ZB_GEAR_RATIO = 5.197539843600339;

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
void handleEncoder2() {
    if (digitalRead(EA2plus) == digitalRead(EB2plus)) encoder2Ticks++; else encoder2Ticks--;
}
void handleEncoder3() {
    if (digitalRead(EA3plus) == digitalRead(EB3plus)) encoder3Ticks++; else encoder3Ticks--;
}

void sendResponse(const String& msg, InputSource source) {
    if (source == SOURCE_USB || source == SOURCE_NONE) Serial.println(msg);
    if (source == SOURCE_BT || source == SOURCE_NONE) Serial1.println(msg);
}

// --- HELPER: encoder ticks to mm ---
float encoderMM_X(volatile long &ticks) {
    return (static_cast<float>(ticks) / PULSES_PER_REV) * X_PITCH;
}
float encoderMM_Y(volatile long &ticks) {
    return (static_cast<float>(ticks) / PULSES_PER_REV) * Y_PITCH;
}

// --- HELPER: steps to real units ---
float stepsToMM_X(long steps) {
    return (static_cast<float>(steps) / STEPS_PER_REV) * X_PITCH;
}
float stepsToMM_Y(long steps) {
    return (static_cast<float>(steps) / STEPS_PER_REV) * Y_PITCH;
}
float stepsToDeg_Z(long steps, float gearRatio) {
    return (static_cast<float>(steps) / STEPS_PER_REV) * 360.0 / gearRatio;
}

// --- HELPER: check if any motor is actively moving ---
bool anyMotorMoving() {
    for (int i = 0; i < numMotors; i++) {
        if (steppers[i].distanceToGo() != 0) return true;
    }
    return false;
}

// --- HELPER: send machine-readable position line for GUI ---
// Format: POS:X=12.50,Y=6.25,ZA=15.00,ZB=-3.00,E0=12.48,E1=12.52,E2=6.20,E3=6.22
void sendPositionUpdate(InputSource source) {
    float xMM  = stepsToMM_X(steppers[0].currentPosition());
    float yMM  = stepsToMM_Y(steppers[2].currentPosition());
    float zaDeg = stepsToDeg_Z(steppers[6].currentPosition(), ZA_GEAR_RATIO);
    float zbDeg = stepsToDeg_Z(steppers[7].currentPosition(), ZB_GEAR_RATIO);
    float e0mm = encoderMM_X(encoder0Ticks);
    float e1mm = encoderMM_X(encoder1Ticks);
    float e2mm = encoderMM_Y(encoder2Ticks);
    float e3mm = encoderMM_Y(encoder3Ticks);

    String pos = "POS:X=" + String(xMM, 2)
               + ",Y=" + String(yMM, 2)
               + ",ZA=" + String(zaDeg, 2)
               + ",ZB=" + String(zbDeg, 2)
               + ",E0=" + String(e0mm, 2)
               + ",E1=" + String(e1mm, 2)
               + ",E2=" + String(e2mm, 2)
               + ",E3=" + String(e3mm, 2);
    sendResponse(pos, source);
}

// --- HELPER: get motor position in real units as string ---
String motorPosStr(int mNum) {
    long pos = steppers[mNum].currentPosition() + (steppers[mNum].distanceToGo() == 0 ? 0 : steppers[mNum].distanceToGo());
    // Use target position (current + remaining) since move() is relative
    long target = steppers[mNum].targetPosition();
    if (mNum <= 1) return String(stepsToMM_X(target), 2) + "mm";
    else if (mNum <= 5) return String(stepsToMM_Y(target), 2) + "mm";
    else return String(stepsToDeg_Z(target, (mNum == 6) ? ZA_GEAR_RATIO : ZB_GEAR_RATIO), 2) + "deg";
}

void processCommand(String input, InputSource source) {
    input.trim();
    input.toUpperCase();

    // --- Commands available in ANY mode ---
    if (input == "ESTOP") {
        for(int i = 0; i < numMotors; i++) {
            steppers[i].stop();
            steppers[i].setCurrentPosition(steppers[i].currentPosition());
        }
        syncAlarm = false;
        if (zeroingMode) { zeroingMode = false; sendResponse(">> E-STOPPED — Exited jog & zero mode", source); }
        else { sendResponse(">> E-STOPPED", source); }
        sendPositionUpdate(source);
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

    // --- POSITIONS: human-readable real units + machine-readable POS line ---
    if (input == "POSITIONS") {
        float xMM   = stepsToMM_X(steppers[0].currentPosition());
        float yMM   = stepsToMM_Y(steppers[2].currentPosition());
        float zaDeg = stepsToDeg_Z(steppers[6].currentPosition(), ZA_GEAR_RATIO);
        float zbDeg = stepsToDeg_Z(steppers[7].currentPosition(), ZB_GEAR_RATIO);

        sendResponse("Positions: X=" + String(xMM, 2) + "mm  Y=" + String(yMM, 2)
                      + "mm  ZA=" + String(zaDeg, 2) + "deg  ZB=" + String(zbDeg, 2) + "deg", source);

        // Raw steps for debugging
        String raw = "Raw steps: ";
        for(int i = 0; i < numMotors; i++) {
            raw += "M" + String(i) + "=" + String(steppers[i].currentPosition()) + " ";
        }
        sendResponse(raw, source);

        float e0mm = encoderMM_X(encoder0Ticks);
        float e1mm = encoderMM_X(encoder1Ticks);
        float e2mm = encoderMM_Y(encoder2Ticks);
        float e3mm = encoderMM_Y(encoder3Ticks);
        sendResponse("Encoders: E0=" + String(e0mm, 2) + "mm  E1=" + String(e1mm, 2)
                      + "mm  E2=" + String(e2mm, 2) + "mm  E3=" + String(e3mm, 2) + "mm", source);

        if (zeroingMode) sendResponse("[JOG & ZERO MODE ACTIVE]", source);
        if (syncAlarm)   sendResponse("[SYNC ALARM ACTIVE]", source);

        // Machine-readable line for GUI
        sendPositionUpdate(source);
        return;
    }

    // --- ENTER JOG & ZERO MODE (available in any state) ---
    if (input == "ZERO" && !zeroingMode) {
        zeroingMode = true;
        if (syncAlarm) syncAlarm = false;
        for (int i = 0; i < numMotors; i++) {
            steppers[i].stop();
            steppers[i].setCurrentPosition(steppers[i].currentPosition());
        }
        sendResponse(">> JOG & ZERO MODE — Jog with M0+/- X+/- Y+/- ZA+/- ZB+/-", source);
        sendPositionUpdate(source);
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
                sendResponse(">> Jog M" + String(mNum) + " " + String(dir) + " → " + motorPosStr(mNum), source);
                return;
            }
        }
        // Jog groups: X+ X- Y+ Y- ZA+ ZA- ZB+ ZB-
        if (input == "X+" || input == "X-") {
            long delta = (input.charAt(1) == '+') ? JOG_X_STEPS : -JOG_X_STEPS;
            steppers[0].move(delta);
            steppers[1].move(delta);
            sendResponse(">> Jog X " + String(input.charAt(1)) + " → " + motorPosStr(0), source);
            return;
        }
        if (input == "Y+" || input == "Y-") {
            long delta = (input.charAt(1) == '+') ? JOG_Y_STEPS : -JOG_Y_STEPS;
            for (int i = 2; i <= 5; i++) steppers[i].move(delta);
            sendResponse(">> Jog Y " + String(input.charAt(1)) + " → " + motorPosStr(2), source);
            return;
        }
        if (input == "ZA+" || input == "ZA-") {
            long delta = (input.charAt(2) == '+') ? JOG_ZA_STEPS : -JOG_ZA_STEPS;
            steppers[6].move(delta);
            sendResponse(">> Jog ZA " + String(input.charAt(2)) + " → " + motorPosStr(6), source);
            return;
        }
        if (input == "ZB+" || input == "ZB-") {
            long delta = (input.charAt(2) == '+') ? JOG_ZB_STEPS : -JOG_ZB_STEPS;
            steppers[7].move(delta);
            sendResponse(">> Jog ZB " + String(input.charAt(2)) + " → " + motorPosStr(7), source);
            return;
        }
        // SET: save current positions as zero
        if (input == "SET") {
            for (int i = 0; i < numMotors; i++) steppers[i].setCurrentPosition(0);
            encoder0Ticks = 0;
            encoder1Ticks = 0;
            encoder2Ticks = 0;
            encoder3Ticks = 0;
            sendResponse(">> Zero point SET — all positions reset to 0", source);
            sendPositionUpdate(source);
            return;
        }
        // EXIT: leave zeroing mode without saving
        if (input == "EXIT") {
            zeroingMode = false;
            sendResponse(">> Exited jog & zero mode", source);
            sendPositionUpdate(source);
            return;
        }
        if (input == "COMMANDS") {
            sendResponse("--- JOG & ZERO MODE ---", source);
            sendResponse("Jog: M0+ M0- ... M7+  M7-  (individual motors)", source);
            sendResponse("Jog: X+ X- Y+ Y- ZA+ ZA- ZB+ ZB- (groups)", source);
            sendResponse("SET = save current pos as zero", source);
            sendResponse("EXIT = leave without saving", source);
            sendResponse("POSITIONS, FAN, ESTOP also available", source);
            return;
        }
        sendResponse("!!! Jog & zero mode — invalid command", source);
        return;
    }

    // --- NORMAL MODE ---
    if (input == "COMMANDS") {
        sendResponse("--- Airfoil Group Controller ---", source);
        sendResponse("Commands: X[mm], Y[mm], ZA[deg], ZB[deg], HOME, POSITIONS, COMMANDS, FAN, ESTOP, ZERO, RESUME", source);
        return;
    }
    if (input == "HOME") {
        if (anyMotorMoving()) {
            sendResponse(">> Warning: overriding active move — homing all axes", source);
        }
        sendResponse(">> Homing all axes...", source);
        for(int i = 0; i < numMotors; i++) steppers[i].moveTo(0);
        return;
    }

    int startMotor = -1;
    int endMotor = -1;
    float targetDegrees = 0;
    bool valid = false;
    String axisName = "";

    if (input.startsWith("X")) {
        float mm = input.substring(1).toFloat();
        if (mm > 415) {
            sendResponse("TOO LARGE, 415mm is the maximum", source);
            return;
        }
        startMotor = 0; endMotor = 1;
        targetDegrees = (mm / X_PITCH) * 360.0;
        axisName = "X";
        valid = true;
    } else if (input.startsWith("Y")) {
        startMotor = 2; endMotor = 5;
        targetDegrees = (input.substring(1).toFloat() / Y_PITCH) * 360.0;
        axisName = "Y";
        valid = true;
    } else if (input.startsWith("ZA")) {
        startMotor = 6; endMotor = 6;
        targetDegrees = input.substring(2).toFloat() * ZA_GEAR_RATIO;
        axisName = "ZA";
        valid = true;
    } else if (input.startsWith("ZB")) {
        startMotor = 7; endMotor = 7;
        targetDegrees = input.substring(2).toFloat() * ZB_GEAR_RATIO;
        axisName = "ZB";
        valid = true;
    }

    if (syncAlarm && valid) {
        sendResponse("!!! SYNC ALARM ACTIVE", source);
        return;
    }

    if (valid) {
        // --- Change 5: warn if motors on this axis are still moving ---
        bool axisBusy = false;
        for (int i = startMotor; i <= endMotor; i++) {
            if (steppers[i].distanceToGo() != 0) { axisBusy = true; break; }
        }
        if (axisBusy) {
            sendResponse(">> Warning: " + axisName + " axis still moving — overriding to new target", source);
        }

        long targetSteps = (targetDegrees / 360.0) * STEPS_PER_REV;
        sendResponse(">> Target Steps: " + String(targetSteps), source);
        for(int i = startMotor; i <= endMotor; i++) {
            steppers[i].moveTo(targetSteps);
        }
    } else if (input.length() > 0) {
        sendResponse("!!! Invalid Command", source);
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

    pinMode(EA2plus, INPUT_PULLUP);
    pinMode(EB2plus, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(EA2plus), handleEncoder2, CHANGE);

    pinMode(EA3plus, INPUT_PULLUP);
    pinMode(EB3plus, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(EA3plus), handleEncoder3, CHANGE);

    delay(500);
    sendResponse("--- System Ready ---", SOURCE_NONE);
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
            float e0mm = encoderMM_X(encoder0Ticks);
            float e1mm = encoderMM_X(encoder1Ticks);
            float diff = abs(e0mm - e1mm);

            if (diff > X_SYNC_THRESHOLD_MM) {
                for (int i = 0; i < numMotors; i++) {
                    steppers[i].stop();
                    steppers[i].setCurrentPosition(steppers[i].currentPosition());
                }
                syncAlarm = true;
                alarmSource = "X";
                sendResponse("!!! SYNC ALARM X — E0=" + String(e0mm, 2) + "mm E1=" + String(e1mm, 2) + "mm (diff=" + String(diff, 2) + "mm)", lastSource);
                sendPositionUpdate(lastSource);
                // Auto-enter jog & zero mode for realignment
                syncAlarm = false;  // clear alarm so jog mode works
                zeroingMode = true;
                sendResponse(">> Entering JOG & ZERO MODE — realign motors, then EXIT to resume", lastSource);
            }
        }

        // --- Y encoder sync check (only while Y motors are moving, NOT in zeroing mode) ---
        bool yMoving = (steppers[2].distanceToGo() != 0) || (steppers[3].distanceToGo() != 0)
                     || (steppers[4].distanceToGo() != 0) || (steppers[5].distanceToGo() != 0);
        if (yMoving && !zeroingMode && !syncAlarm) {
            float e2mm = encoderMM_Y(encoder2Ticks);
            float e3mm = encoderMM_Y(encoder3Ticks);
            float diff = abs(e2mm - e3mm);

            if (diff > Y_SYNC_THRESHOLD_MM) {
                for (int i = 0; i < numMotors; i++) {
                    steppers[i].stop();
                    steppers[i].setCurrentPosition(steppers[i].currentPosition());
                }
                syncAlarm = true;
                alarmSource = "Y";
                sendResponse("!!! SYNC ALARM Y — E2=" + String(e2mm, 2) + "mm E3=" + String(e3mm, 2) + "mm (diff=" + String(diff, 2) + "mm)", lastSource);
                sendPositionUpdate(lastSource);
                // Auto-enter jog & zero mode for realignment
                syncAlarm = false;  // clear alarm so jog mode works
                zeroingMode = true;
                sendResponse(">> Entering JOG & ZERO MODE — realign motors, then EXIT to resume", lastSource);
            }
        }
    } else {
        moving = false;
    }

    // --- Change 4: suppress "Motion Complete" in zeroing mode ---
    // --- Change 7: auto-send position update on motion complete ---
    static bool wasMoving = false;
    if (!moving && wasMoving) {
        if (!zeroingMode) {
            sendResponse("--- Motion Complete. ---", lastSource);
            sendPositionUpdate(lastSource);
        }
    }
    wasMoving = moving;
}
