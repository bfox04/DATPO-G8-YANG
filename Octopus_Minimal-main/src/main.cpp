// working

// 2-second twist bump

#include <Arduino.h>

// PIN DEFINITIONS
// Motor Driver 0-7 
#define ENABLE_PIN0  PF14
#define STEP_PIN0    PF13
#define DIR_PIN0     PF12

#define ENABLE_PIN1  PF15
#define STEP_PIN1    PG0
#define DIR_PIN1     PG1

#define ENABLE_PIN2  PG5
#define STEP_PIN2    PF11
#define DIR_PIN2     PG3

#define ENABLE_PIN3  PA0
#define STEP_PIN3    PG4
#define DIR_PIN3     PC1

#define ENABLE_PIN4  PG2
#define STEP_PIN4    PF9
#define DIR_PIN4     PF10

#define ENABLE_PIN5  PF1
#define STEP_PIN5    PC13
#define DIR_PIN5     PF0

#define ENABLE_PIN6  PD4
#define STEP_PIN6    PE2
#define DIR_PIN6     PE3

#define ENABLE_PIN7  PE0
#define STEP_PIN7    PE6
#define DIR_PIN7     PA14

// Fan Definitions
#define FAN0_PIN    PA8  
#define FAN2_PIN    PD12  
#define FAN4_PIN    PD14

// VARIABLES FOR TIMING
unsigned long previousTime = 0;      // To store the last time we switched direction
const long interval = 3000;          // Interval in milliseconds (3 seconds)
bool motorDirection = HIGH;          // Direction state



void setup() {
    Serial.begin(115200); // baud rate 115200
    
    // SETUP FANS and subsequently their LEDS
    pinMode(FAN0_PIN, OUTPUT);
    pinMode(FAN2_PIN, OUTPUT);
    pinMode(FAN4_PIN, OUTPUT);

    digitalWrite(FAN0_PIN, HIGH); 
    digitalWrite(FAN2_PIN, HIGH); 
    digitalWrite(FAN4_PIN, HIGH); 

    // SETUP MOTOR
    pinMode(STEP_PIN0, OUTPUT);
    pinMode(DIR_PIN0, OUTPUT);
    pinMode(ENABLE_PIN0, OUTPUT);
    digitalWrite(ENABLE_PIN0, LOW); 
    digitalWrite(DIR_PIN0, HIGH);   

    pinMode(STEP_PIN1, OUTPUT);
    pinMode(DIR_PIN1, OUTPUT);
    pinMode(ENABLE_PIN1, OUTPUT);
    digitalWrite(ENABLE_PIN1, LOW); 
    digitalWrite(DIR_PIN1, HIGH);   

    pinMode(STEP_PIN2, OUTPUT);
    pinMode(DIR_PIN2, OUTPUT);
    pinMode(ENABLE_PIN2, OUTPUT);
    digitalWrite(ENABLE_PIN2, LOW); 
    digitalWrite(DIR_PIN2, HIGH);   

    pinMode(STEP_PIN3, OUTPUT);
    pinMode(DIR_PIN3, OUTPUT);
    pinMode(ENABLE_PIN3, OUTPUT);
    digitalWrite(ENABLE_PIN3, LOW); 
    digitalWrite(DIR_PIN3, HIGH);   

    pinMode(STEP_PIN4, OUTPUT);
    pinMode(DIR_PIN4, OUTPUT);
    pinMode(ENABLE_PIN4, OUTPUT);
    digitalWrite(ENABLE_PIN4, LOW); 
    digitalWrite(DIR_PIN4, HIGH);   

    pinMode(STEP_PIN5, OUTPUT);
    pinMode(DIR_PIN5, OUTPUT);
    pinMode(ENABLE_PIN5, OUTPUT);
    digitalWrite(ENABLE_PIN5, LOW); 
    digitalWrite(DIR_PIN5, HIGH);   

    pinMode(STEP_PIN6, OUTPUT);
    pinMode(DIR_PIN6, OUTPUT);
    pinMode(ENABLE_PIN6, OUTPUT);
    digitalWrite(ENABLE_PIN6, LOW); 
    digitalWrite(DIR_PIN6, HIGH);   

    pinMode(STEP_PIN7, OUTPUT);
    pinMode(DIR_PIN7, OUTPUT);
    pinMode(ENABLE_PIN7, OUTPUT);
    digitalWrite(ENABLE_PIN7, LOW); 
    digitalWrite(DIR_PIN7, HIGH);   

    Serial.println("Startup Complete!");
}

void loop() {
    Serial.println("I'm looping!");

    // GET CURRENT TIME
    unsigned long currentTime = millis();

    // CHECK IF INTERVAL HAVE PASSED
    if (currentTime - previousTime >= interval) {
        // Save the currentTime as previousTime for next iteration
        previousTime = currentTime;

        // Toggle the direction 
        motorDirection = !motorDirection;
        
        // Set new direction to the pin
        digitalWrite(DIR_PIN0, motorDirection);
        digitalWrite(DIR_PIN1, motorDirection);
        digitalWrite(DIR_PIN2, motorDirection);
        digitalWrite(DIR_PIN3, motorDirection);
        digitalWrite(DIR_PIN4, motorDirection);
        digitalWrite(DIR_PIN5, motorDirection);
        digitalWrite(DIR_PIN6, motorDirection);
        digitalWrite(DIR_PIN7, motorDirection);

        Serial.println("Switching Direction!");
    }

    // PERFORM ONE STEP - repeating high/low at a fast 
    // interval creates a square wave of pulses and allows for smooth, constant rotation
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