#include <Arduino.h>
#include <AccelStepper.h>
#include <SPI.h>

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

// --- AMT25 SPI ABSOLUTE ENCODER DEFINITIONS ---
#define SPI_MOSI    PB5
#define SPI_MISO    PB4
#define SPI_SCLK    PB3

#define PIN_CS_E6   PE10
#define PIN_CS_E7   PE15

// --- AMT25 state ---
float    e6Degrees      = 0.0;
float    e7Degrees      = 0.0;
uint16_t e6RawPos       = 0;
uint16_t e7RawPos       = 0;
uint16_t e6FullWord     = 0;
uint16_t e7FullWord     = 0;
bool     e6Valid        = false;
bool     e7Valid        = false;
bool     e6Ever         = false;
bool     e7Ever         = false;
uint32_t e6Reads        = 0;
uint32_t e7Reads        = 0;
uint32_t e6Fails        = 0;
uint32_t e7Fails        = 0;

const float PULSES_PER_REV = 2000.0;
volatile long encoder0Ticks = 0;
volatile long encoder1Ticks = 0;
volatile long encoder2Ticks = 0;
volatile long encoder3Ticks = 0;

const float X_SYNC_THRESHOLD_MM = 5.0;
const float Y_SYNC_THRESHOLD_MM = 5.0;
bool syncAlarm = false;
String alarmSource = ""; 

const int MICROSTEP_SETTING = 8;
const long STEPS_PER_REV = 200 * MICROSTEP_SETTING;
const float X_PITCH = 5.0;
const float Y_PITCH = 2.0;
const float ZA_GEAR_RATIO = 5.197539843600339;
const float ZB_GEAR_RATIO = 5.197539843600339;

bool zeroingMode = false;
float jogStepMM  = 0.25;
float jogStepDeg = 0.25;

long getJogStepsX()  { return max(1L, lround((jogStepMM  / X_PITCH)  * STEPS_PER_REV)); }
long getJogStepsY()  { return max(1L, lround((jogStepMM  / Y_PITCH)  * STEPS_PER_REV)); }
long getJogStepsZA() { return max(1L, lround((jogStepDeg * ZA_GEAR_RATIO / 360.0) * STEPS_PER_REV)); }
long getJogStepsZB() { return max(1L, lround((jogStepDeg * ZB_GEAR_RATIO / 360.0) * STEPS_PER_REV)); }

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

void handleEncoder0() { if (digitalRead(EA0plus) == digitalRead(EB0plus)) encoder0Ticks++; else encoder0Ticks--; }
void handleEncoder1() { if (digitalRead(EA1plus) == digitalRead(EB1plus)) encoder1Ticks++; else encoder1Ticks--; }
void handleEncoder2() { if (digitalRead(EA2plus) == digitalRead(EB2plus)) encoder2Ticks++; else encoder2Ticks--; }
void handleEncoder3() { if (digitalRead(EA3plus) == digitalRead(EB3plus)) encoder3Ticks++; else encoder3Ticks--; }

void sendResponse(const String& msg, InputSource source) {
    if (source == SOURCE_USB || source == SOURCE_NONE) Serial.println(msg);
    if (source == SOURCE_BT || source == SOURCE_NONE) Serial1.println(msg);
}

float encoderMM_X(volatile long &ticks) { return (static_cast<float>(ticks) / PULSES_PER_REV) * X_PITCH; }
float encoderMM_Y(volatile long &ticks) { return (static_cast<float>(ticks) / PULSES_PER_REV) * Y_PITCH; }
float stepsToMM_X(long steps) { return (static_cast<float>(steps) / STEPS_PER_REV) * X_PITCH; }
float stepsToMM_Y(long steps) { return (static_cast<float>(steps) / STEPS_PER_REV) * Y_PITCH; }
float stepsToDeg_Z(long steps, float gearRatio) { return (static_cast<float>(steps) / STEPS_PER_REV) * 360.0 / gearRatio; }

bool anyMotorMoving() {
    for (int i = 0; i < numMotors; i++) {
        if (steppers[i].distanceToGo() != 0) return true;
    }
    return false;
}

// =====================================================================
//  AMT25 SPI ENCODER FUNCTIONS (2MHz DATASHEET COMPLIANT)
// =====================================================================

// 2MHz shrinks the transaction time to ~15µs, easily fitting between PWM noise
SPISettings amt25Settings(2000000, MSBFIRST, SPI_MODE0);

bool amt25Parity(uint16_t msg) {
    bool p1 = !!(msg & 0x8000);
    bool p0 = !!(msg & 0x4000);
    bool odd  = !!(msg&0x0002)^!!(msg&0x0008)^!!(msg&0x0020)^!!(msg&0x0080)^!!(msg&0x0200)^!!(msg&0x0800)^!!(msg&0x2000);
    bool even = !!(msg&0x0001)^!!(msg&0x0004)^!!(msg&0x0010)^!!(msg&0x0040)^!!(msg&0x0100)^!!(msg&0x0400)^!!(msg&0x1000);
    return (p1 != odd) && (p0 != even);
}

bool amt25ReadOnce(int csPin, uint16_t &word) {
    SPI.beginTransaction(amt25Settings);
    digitalWrite(csPin, LOW);
    delayMicroseconds(4); // MUST be >3us per AMT datasheet

    noInterrupts();
    uint8_t msb = SPI.transfer(0x00);
    delayMicroseconds(3); // MUST be >2.5us per AMT datasheet
    uint8_t lsb = SPI.transfer(0x00);
    interrupts();

    digitalWrite(csPin, HIGH);
    SPI.endTransaction();

    word = ((uint16_t)msb << 8) | lsb;
    return amt25Parity(word);
}

void pollE6() {
    e6Reads++;
    bool success = false;
    
    for (int i = 0; i < 50; i++) {
        if (amt25ReadOnce(PIN_CS_E6, e6FullWord)) {
            if (e6FullWord != 0xFFFF && e6FullWord != 0x0000) {
                e6RawPos = (e6FullWord & 0x3FFF) >> 2;
                e6Degrees = (e6RawPos * 360.0f) / 4096.0f;
                success = true;
                break;
            }
        }
        // CRITICAL: AMT25 requires CS to be HIGH for >40us between reads.
        // 45us satisfies the hardware and phase-shifts against PWM noise.
        delayMicroseconds(45); 
    }
    
    if (success) { e6Valid = true; } else { e6Valid = false; e6Fails++; }
    e6Ever = true;
}

void pollE7() {
    e7Reads++;
    bool success = false;
    
    for (int i = 0; i < 50; i++) {
        if (amt25ReadOnce(PIN_CS_E7, e7FullWord)) {
            if (e7FullWord != 0xFFFF && e7FullWord != 0x0000) {
                e7RawPos = (e7FullWord & 0x3FFF) >> 2;
                e7Degrees = (e7RawPos * 360.0f) / 4096.0f;
                success = true;
                break;
            }
        }
        delayMicroseconds(45);
    }
    
    if (success) { e7Valid = true; } else { e7Valid = false; e7Fails++; }
    e7Ever = true;
}

void sendPositionUpdate(InputSource source) {
    pollE6();
    pollE7();

    float xMM  = stepsToMM_X(steppers[0].currentPosition());
    float yMM  = stepsToMM_Y(steppers[2].currentPosition());
    float zaDeg = stepsToDeg_Z(steppers[6].currentPosition(), ZA_GEAR_RATIO);
    float zbDeg = stepsToDeg_Z(steppers[7].currentPosition(), ZB_GEAR_RATIO);
    float e0mm = encoderMM_X(encoder0Ticks);
    float e1mm = encoderMM_X(encoder1Ticks);
    float e2mm = encoderMM_Y(encoder2Ticks);
    float e5mm = encoderMM_Y(encoder3Ticks);

    String pos = "POS:X=" + String(xMM, 2)
            + ",Y=" + String(yMM, 2)
            + ",ZA=" + String(zaDeg, 2)
            + ",ZB=" + String(zbDeg, 2)
            + ",E0=" + String(e0mm, 2)
            + ",E1=" + String(e1mm, 2)
            + ",E2=" + String(e2mm, 2)
            + ",E5=" + String(e5mm, 2)
            + ",E6=" + String(e6Degrees, 2)
            + ",E7=" + String(e7Degrees, 2);
    sendResponse(pos, source);
}

String motorPosStr(int mNum) {
    long pos = steppers[mNum].currentPosition() + (steppers[mNum].distanceToGo() == 0 ? 0 : steppers[mNum].distanceToGo());
    long target = steppers[mNum].targetPosition();
    if (mNum <= 1) return String(stepsToMM_X(target), 2) + "mm";
    else if (mNum <= 5) return String(stepsToMM_Y(target), 2) + "mm";
    else return String(stepsToDeg_Z(target, (mNum == 6) ? ZA_GEAR_RATIO : ZB_GEAR_RATIO), 2) + "deg";
}

void processCommand(String input, InputSource source) {
    input.trim();
    input.toUpperCase();

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

    if (input == "READE6" || input == "READE7" || input == "AMT") {
        pollE6();
        pollE7();
        sendResponse(">> E6: " + String(e6Degrees, 2) + "deg"
                    + (e6Valid ? " [OK]" : " [FAIL]") + " | E7: " + String(e7Degrees, 2) + "deg"
                    + (e7Valid ? " [OK]" : " [FAIL]"), source);
        sendPositionUpdate(source);
        return;
    }

    if (input == "DIAG") {
        pollE6();
        pollE7();
        float e6p = e6Reads ? (e6Fails*100.0f/e6Reads) : 0;
        float e7p = e7Reads ? (e7Fails*100.0f/e7Reads) : 0;
        sendResponse("--- DIAG ---", source);
        sendResponse("E6: " + String(e6Degrees,2) + "deg " + (e6Valid?"OK":"FAIL")
            + " | Reads:" + String(e6Reads) + " Fails:" + String(e6Fails)
            + " (" + String(e6p,1) + "%) | Word:0x" + String(e6FullWord,HEX), source);
        sendResponse("E7: " + String(e7Degrees,2) + "deg " + (e7Valid?"OK":"FAIL")
            + " | Reads:" + String(e7Reads) + " Fails:" + String(e7Fails)
            + " (" + String(e7p,1) + "%) | Word:0x" + String(e7FullWord,HEX), source);
        sendResponse("---", source);
        return;
    }

    if (input == "POSITIONS") {
        float xMM   = stepsToMM_X(steppers[0].currentPosition());
        float yMM   = stepsToMM_Y(steppers[2].currentPosition());
        float zaDeg = stepsToDeg_Z(steppers[6].currentPosition(), ZA_GEAR_RATIO);
        float zbDeg = stepsToDeg_Z(steppers[7].currentPosition(), ZB_GEAR_RATIO);

        sendResponse("Positions: X=" + String(xMM, 2) + "mm  Y=" + String(yMM, 2)
                    + "mm  AoA Bot=" + String(zaDeg, 2) + "deg  AoA Top=" + String(zbDeg, 2) + "deg", source);

        float e0mm = encoderMM_X(encoder0Ticks);
        float e1mm = encoderMM_X(encoder1Ticks);
        float e2mm = encoderMM_Y(encoder2Ticks);
        float e5mm = encoderMM_Y(encoder3Ticks);
        sendResponse("Quad Encoders: E0=" + String(e0mm, 2) + "mm  E1=" + String(e1mm, 2)
                    + "mm  E2=" + String(e2mm, 2) + "mm  E5=" + String(e5mm, 2) + "mm", source);

        sendResponse("AMT25 Encoders: E6(AoA Bot)=" + String(e6Degrees, 2) + "deg"
                    + (e6Ever ? "" : " [no read yet]")
                    + "  E7(AoA Top)=" + String(e7Degrees, 2) + "deg"
                    + (e7Ever ? "" : " [no read yet]"), source);

        if (zeroingMode) sendResponse("[JOG & ZERO MODE ACTIVE]", source);
        if (syncAlarm)   sendResponse("[SYNC ALARM ACTIVE]", source);

        sendPositionUpdate(source);
        return;
    }

    if (input == "ZERO" && !zeroingMode) {
        zeroingMode = true;
        if (syncAlarm) syncAlarm = false;
        for (int i = 0; i < numMotors; i++) {
            steppers[i].stop();
            steppers[i].setCurrentPosition(steppers[i].currentPosition());
        }
        sendResponse(">> JOG & ZERO MODE — Jog with M0+/- X+/- Y+/- ZA+/- ZB+/- (ZA=AoA Bot, ZB=AoA Top)", source);
        sendPositionUpdate(source);
        return;
    }

    if (zeroingMode) {
        if (input.length() >= 3 && input.charAt(0) == 'M') {
            int mNum = input.substring(1, input.length() - 1).toInt();
            char dir = input.charAt(input.length() - 1);
            if (mNum >= 0 && mNum <= 5 && (dir == '+' || dir == '-')) {
                long jogAmount;
                if (mNum <= 1) jogAmount = getJogStepsX();
                else if (mNum <= 5) jogAmount = getJogStepsY();
                else jogAmount = getJogStepsZA();
                long delta = (dir == '+') ? jogAmount : -jogAmount;
                steppers[mNum].move(delta);
                sendResponse(">> Jog M" + String(mNum) + " " + String(dir) + " → " + motorPosStr(mNum), source);
                return;
            }
        }
        if (input == "X+" || input == "X-") {
            long delta = (input.charAt(1) == '+') ? getJogStepsX() : -getJogStepsX();
            steppers[0].move(delta);
            steppers[1].move(delta);
            sendResponse(">> Jog X " + String(input.charAt(1)) + " → " + motorPosStr(0), source);
            return;
        }
        if (input == "Y+" || input == "Y-") {
            long delta = (input.charAt(1) == '+') ? getJogStepsY() : -getJogStepsY();
            for (int i = 2; i <= 5; i++) steppers[i].move(delta);
            sendResponse(">> Jog Y " + String(input.charAt(1)) + " → " + motorPosStr(2), source);
            return;
        }
        if (input == "YL+" || input == "YL-") {
            long delta = (input.charAt(2) == '+') ? getJogStepsY() : -getJogStepsY();
            steppers[2].move(delta);
            steppers[3].move(delta);
            sendResponse(">> Jog Y Left " + String(input.charAt(2)) + " → " + motorPosStr(2), source);
            return;
        }
        if (input == "YR+" || input == "YR-") {
            long delta = (input.charAt(2) == '+') ? getJogStepsY() : -getJogStepsY();
            steppers[4].move(delta);
            steppers[5].move(delta);
            sendResponse(">> Jog Y Right " + String(input.charAt(2)) + " → " + motorPosStr(4), source);
            return;
        }
        if (input == "ZA+" || input == "ZA-") {
            long delta = (input.charAt(2) == '+') ? getJogStepsZA() : -getJogStepsZA();
            steppers[6].move(delta);
            sendResponse(">> Jog AoA Bot " + String(input.charAt(2)) + " → " + motorPosStr(6), source);
            return;
        }
        if (input == "ZB+" || input == "ZB-") {
            long delta = (input.charAt(2) == '+') ? getJogStepsZB() : -getJogStepsZB();
            steppers[7].move(delta);
            sendResponse(">> Jog AoA Top " + String(input.charAt(2)) + " → " + motorPosStr(7), source);
            return;
        }
        if (input.startsWith("STEP")) {
            String szStr = input.substring(4);
            float sz = szStr.toFloat();
            if (szStr == "0.1" || szStr == "0.25" || szStr == "0.5" || szStr == "1.0" || szStr == "1" || szStr == "2.0" || szStr == "2" || szStr == "3.0" || szStr == "3") {
                jogStepMM  = sz;
                jogStepDeg = sz;
                sendResponse(">> Step size: " + String(sz, 2) + "mm / " + String(sz, 2) + "deg", source);
            } else {
                sendResponse("!!! Invalid step. Options: 0.1  0.25  0.5  1.0  2.0  3.0", source);
            }
            return;
        }
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
        if (input == "EXIT") {
            zeroingMode = false;
            sendResponse(">> Exited jog & zero mode", source);
            sendPositionUpdate(source);
            return;
        }
        if (input == "COMMANDS") {
            sendResponse("--- JOG & ZERO MODE ---", source);
            sendResponse("Jog: M0+ M0- (X Left)  M1+ M1- (X Right)", source);
            sendResponse("Jog: M2+ M2- (Y Left Close)  M3+ M3- (Y Left Far)", source);
            sendResponse("Jog: M4+ M4- (Y Right Close)  M5+ M5- (Y Right Far)", source);
            sendResponse("Jog groups: X+ X-  Y+ Y-  YL+ YL- (Y Left)  YR+ YR- (Y Right)", source);
            sendResponse("Jog: ZA+ ZA- (AoA Bot)  ZB+ ZB- (AoA Top)", source);
            sendResponse("STEP0.1 STEP0.25 STEP0.5 STEP1.0 STEP2.0 STEP3.0 = set jog increment", source);
            sendResponse("SET = save current pos as zero", source);
            sendResponse("EXIT = leave without saving", source);
            sendResponse("POSITIONS, FAN, ESTOP also available", source);
            return;
        }
        sendResponse("!!! Jog & zero mode — invalid command", source);
        return;
    }

    if (input == "COMMANDS") {
        sendResponse("--- Airfoil Group Controller ---", source);
        sendResponse("Commands: X[mm], Y[mm], ZA[deg](AoA Bot), ZB[deg](AoA Top), HOME, POSITIONS, COMMANDS, FAN, ESTOP, ZERO, RESUME", source);
        sendResponse("Encoders: AMT (read both E6+E7), READE6, READE7, DIAG", source);
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
        if (mm > 415 || mm < -415) {
            sendResponse("!!! Out of range, X limit is +/- 415mm", source);
            return;
        }
        startMotor = 0; endMotor = 1;
        targetDegrees = (mm / X_PITCH) * 360.0;
        axisName = "X";
        valid = true;
    } else if (input.startsWith("Y")) {
        float mm = input.substring(1).toFloat();
        if (mm > 225 || mm < -225) {
            sendResponse("!!! Out of range, Y limit is +/- 225mm", source);
            return;
        }
        startMotor = 2; endMotor = 5;
        targetDegrees = (mm / Y_PITCH) * 360.0;
        axisName = "Y";
        valid = true;
    } else if (input.startsWith("ZA")) {
        float deg = input.substring(2).toFloat();
        if (deg > 20 || deg < -20) {
            sendResponse("!!! Out of range, AoA Bot limit is +/- 20deg", source);
            return;
        }
        startMotor = 6; endMotor = 6;
        targetDegrees = deg * ZA_GEAR_RATIO;
        axisName = "AoA Bot";
        valid = true;
    } else if (input.startsWith("ZB")) {
        float deg = input.substring(2).toFloat();
        if (deg > 20 || deg < -20) {
            sendResponse("!!! Out of range, AoA Top limit is +/- 20deg", source);
            return;
        }
        startMotor = 7; endMotor = 7;
        targetDegrees = deg * ZB_GEAR_RATIO;
        axisName = "AoA Top";
        valid = true;
    }

    if (syncAlarm && valid) {
        sendResponse("!!! SYNC ALARM ACTIVE", source);
        return;
    }

    if (valid) {
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
    for (int i = 5; i >= 0; i--) {
        sendResponse(String(i), SOURCE_NONE);
        delay(1000);
    }

    pinMode(FAN0_PIN, OUTPUT);
    digitalWrite(FAN0_PIN, HIGH);

    // --- ALL MOTORS ENABLED ---
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

    pinMode(PIN_CS_E6, OUTPUT);
    digitalWrite(PIN_CS_E6, HIGH);
    pinMode(PIN_CS_E7, OUTPUT);
    digitalWrite(PIN_CS_E7, HIGH);

#if defined(STM32F4xx)
    pinMode(PA14, OUTPUT);
#endif

    SPI.setMISO(SPI_MISO);
    SPI.setMOSI(SPI_MOSI);
    SPI.setSCLK(SPI_SCLK);
    SPI.begin();

    steppers[6].moveTo(50);
    steppers[7].moveTo(50);
    while (steppers[6].distanceToGo() != 0 || steppers[7].distanceToGo() != 0) {
        steppers[6].run();
        steppers[7].run();
    }
    steppers[6].moveTo(0);
    steppers[7].moveTo(0);
    while (steppers[6].distanceToGo() != 0 || steppers[7].distanceToGo() != 0) {
        steppers[6].run();
        steppers[7].run();
    }

    delay(100);
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
                syncAlarm = false;
                zeroingMode = true;
                sendResponse(">> Entering JOG & ZERO MODE — realign motors, then EXIT to resume", lastSource);
            }
        }

        bool yMoving = (steppers[2].distanceToGo() != 0) || (steppers[3].distanceToGo() != 0)
                    || (steppers[4].distanceToGo() != 0) || (steppers[5].distanceToGo() != 0);
        if (yMoving && !zeroingMode && !syncAlarm) {
            float e2mm = encoderMM_Y(encoder2Ticks);
            float e5mm = encoderMM_Y(encoder3Ticks);
            float diff = abs(e2mm - e5mm);

            if (diff > Y_SYNC_THRESHOLD_MM) {
                for (int i = 0; i < numMotors; i++) {
                    steppers[i].stop();
                    steppers[i].setCurrentPosition(steppers[i].currentPosition());
                }
                syncAlarm = true;
                alarmSource = "Y";
                sendResponse("!!! SYNC ALARM Y — E2=" + String(e2mm, 2) + "mm E5=" + String(e5mm, 2) + "mm (diff=" + String(diff, 2) + "mm)", lastSource);
                sendPositionUpdate(lastSource);
                syncAlarm = false;
                zeroingMode = true;
                sendResponse(">> Entering JOG & ZERO MODE — realign motors, then EXIT to resume", lastSource);
            }
        }
    } else {
        moving = false;
    }

    static bool wasMoving = false;
    if (!moving && wasMoving) {
        if (!zeroingMode) {
            sendResponse("--- Motion Complete. ---", lastSource);
            pollE6();
            pollE7();
            sendResponse(">> E6: " + String(e6Degrees, 2) + "deg"
                        + (e6Valid ? " [OK]" : " [FAIL]") + " | E7: " + String(e7Degrees, 2) + "deg"
                        + (e7Valid ? " [OK]" : " [FAIL]"), lastSource);
            sendPositionUpdate(lastSource);
        }
    }
    wasMoving = moving;
}