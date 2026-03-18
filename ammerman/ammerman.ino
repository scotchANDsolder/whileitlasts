#include <AccelStepper.h>

const int stepPin = 3;
const int dirPin = 4;
const int reedPin = 13;

// --- Motion Settings ---
long motorMaxSpeed = 1000;    
long unwindSteps = 3000;      
unsigned long pauseTime = 1000; // 1 second pause at each end
float accelerationValue = 800.0;

// --- State Tracking ---
enum State { TILTING, PAUSE_TOP, UNWINDING, PAUSE_BOTTOM, KILLED };
State currentState = TILTING;
unsigned long stateStartTime = 0;
bool isKilled = false;

AccelStepper stepper(1, stepPin, dirPin);

void setup() {
  Serial.begin(115200);
  pinMode(reedPin, INPUT_PULLUP);
  stepper.setMaxSpeed(motorMaxSpeed);
  stepper.setAcceleration(accelerationValue);
  stepper.moveTo(-2000000); // Start pulling
}

void loop() {
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    if (data == "STOP") {
      isKilled = true;
      stepper.stop();
      stepper.disableOutputs(); // Turns off current to motor
    } else {
      isKilled = false;
      stepper.enableOutputs();
      parseIncomingData(data);
    }
  }

  if (isKilled) return;

  int reedState = digitalRead(reedPin);

  switch (currentState) {
    case TILTING:
      if (reedState == LOW) {
        stepper.stop();
        stepper.setCurrentPosition(0);
        stateStartTime = millis();
        currentState = PAUSE_TOP;
      } else {
        stepper.moveTo(-2000000); 
      }
      break;

    case PAUSE_TOP:
      if (millis() - stateStartTime >= pauseTime) {
        stepper.moveTo(unwindSteps);
        currentState = UNWINDING;
      }
      break;

    case UNWINDING:
      if (stepper.distanceToGo() == 0) {
        stateStartTime = millis();
        currentState = PAUSE_BOTTOM;
      }
      break;

    case PAUSE_BOTTOM:
      if (millis() - stateStartTime >= pauseTime) {
        currentState = TILTING;
      }
      break;
  }

  stepper.run();
}

void parseIncomingData(String data) {
  data.trim();
  int commaIndex = data.indexOf(',');
  if (commaIndex > 0) {
    motorMaxSpeed = data.substring(0, commaIndex).toInt();
    unwindSteps = data.substring(commaIndex + 1).toInt();
    stepper.setMaxSpeed(motorMaxSpeed);
    stepper.setAcceleration(motorMaxSpeed);
  }
}
