#include <Arduino.h>

// --- MOTOR DEFINITIONS (0-7) ---
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
#define FAN2_PIN    PD12  
#define FAN4_PIN    PD14

// --- ENCODER DEFINITIONS ---
#define EAplus      PG6   // Phase A
#define EBplus      PG9   // Phase B

// --- ENCODER SETTINGS ---
// Adjust this to match your encoder's datasheet (e.g., 400, 600, 1000)
const float PULSES_PER_REV = 600.0; 

// Volatile variables are required for variables modified inside interrupts
volatile long encoderTicks = 0;
long lastReportedTicks = 0;

// --- TIMING VARIABLES ---
unsigned long previousTime = 0;
const long interval = 3000;          // Switch direction every 3 seconds
bool motorDirection = HIGH;

// --- ENCODER INTERRUPT SERVICE ROUTINE (ISR) ---
// This runs instantly whenever EAplus (Phase A) changes state
void handleEncoder() {
    int aState = digitalRead(EAplus);
    int bState = digitalRead(EBplus);

    if (aState == bState) {
        encoderTicks++;
    } else {
        encoderTicks--;
    }
}

void setup() {
    Serial.begin(115200);

    // SETUP FANS
    pinMode(FAN0_PIN, OUTPUT);
    pinMode(FAN2_PIN, OUTPUT);
    pinMode(FAN4_PIN, OUTPUT);
    digitalWrite(FAN0_PIN, HIGH); 
    digitalWrite(FAN2_PIN, HIGH); 
    digitalWrite(FAN4_PIN, HIGH); 

    // SETUP MOTORS (Using a loop for brevity)
    int stepPins[] = {STEP_PIN0, STEP_PIN1, STEP_PIN2, STEP_PIN3, STEP_PIN4, STEP_PIN5, STEP_PIN6, STEP_PIN7};
    int dirPins[] = {DIR_PIN0, DIR_PIN1, DIR_PIN2, DIR_PIN3, DIR_PIN4, DIR_PIN5, DIR_PIN6, DIR_PIN7};
    int enPins[] = {ENABLE_PIN0, ENABLE_PIN1, ENABLE_PIN2, ENABLE_PIN3, ENABLE_PIN4, ENABLE_PIN5, ENABLE_PIN6, ENABLE_PIN7};

    for(int i=0; i<8; i++) {
        pinMode(stepPins[i], OUTPUT);
        pinMode(dirPins[i], OUTPUT);
        pinMode(enPins[i], OUTPUT);
        digitalWrite(enPins[i], LOW); // Enable motors
        digitalWrite(dirPins[i], HIGH);
    }

    // SETUP ENCODER
    pinMode(EAplus, INPUT_PULLUP);
    pinMode(EBplus, INPUT_PULLUP);
    
    // Attach interrupt to Phase A on any logic change
    attachInterrupt(digitalPinToInterrupt(EAplus), handleEncoder, CHANGE);

    Serial.println("Startup Complete! Encoder homed to 0.");
}

void loop() {
    unsigned long currentTime = millis();

    // 1. DIRECTION TOGGLE LOGIC
    if (currentTime - previousTime >= interval) {
        previousTime = currentTime;
        motorDirection = !motorDirection;
        
        digitalWrite(DIR_PIN0, motorDirection);
        digitalWrite(DIR_PIN1, motorDirection);
        digitalWrite(DIR_PIN2, motorDirection);
        digitalWrite(DIR_PIN3, motorDirection);
        digitalWrite(DIR_PIN4, motorDirection);
        digitalWrite(DIR_PIN5, motorDirection);
        digitalWrite(DIR_PIN6, motorDirection);
        digitalWrite(DIR_PIN7, motorDirection);

        Serial.println("--- Switching Direction ---");
    }

    // 2. ENCODER REPORTING LOGIC
    // Print only if the position has changed to keep Serial clean
    if (encoderTicks != lastReportedTicks) {
        float angle = (encoderTicks / PULSES_PER_REV) * 360.0;
        
        Serial.print("Ticks: ");
        Serial.print(encoderTicks);
        Serial.print(" | Angle: ");
        Serial.print(angle);
        Serial.println("°");

        lastReportedTicks = encoderTicks;
    }

    // 3. MOTOR STEPPING (Square Wave)
    digitalWrite(STEP_PIN0, HIGH);
    digitalWrite(STEP_PIN1, HIGH);
    digitalWrite(STEP_PIN2, HIGH);
    digitalWrite(STEP_PIN3, HIGH);
    digitalWrite(STEP_PIN4, HIGH);
    digitalWrite(STEP_PIN5, HIGH);
    digitalWrite(STEP_PIN6, HIGH);
    digitalWrite(STEP_PIN7, HIGH);
    
    delayMicroseconds(500);
    
    digitalWrite(STEP_PIN0, LOW);
    digitalWrite(STEP_PIN1, LOW);
    digitalWrite(STEP_PIN2, LOW);
    digitalWrite(STEP_PIN3, LOW);
    digitalWrite(STEP_PIN4, LOW);
    digitalWrite(STEP_PIN5, LOW);
    digitalWrite(STEP_PIN6, LOW);
    digitalWrite(STEP_PIN7, LOW);
    
    delayMicroseconds(500);
}