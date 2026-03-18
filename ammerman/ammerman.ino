#include <AccelStepper.h>

const int stepPin = 3;
const int dirPin = 4;
const int reedPin = 13;

long motorMaxSpeed = 1000;  
long unwindSteps = 3000;      
unsigned long pauseTime = 1000;
float accelerationValue = 800.0;

enum State { TILTING, PAUSE_TOP, UNWINDING, PAUSE_BOTTOM, KILLED, MANUAL };
State currentState = KILLED; 
unsigned long stateStartTime = 0;
bool isKilled = true;

AccelStepper stepper(1, stepPin, dirPin);

void setup() {
  Serial.begin(115200);
  pinMode(reedPin, INPUT_PULLUP);
  stepper.setMaxSpeed(motorMaxSpeed);
  stepper.setAcceleration(accelerationValue);
  stepper.disableOutputs(); 
}

void loop() {
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    data.trim();

    if (data == "STOP") {
      isKilled = true;
      stepper.stop();
      stepper.disableOutputs();
    } else if (data == "START") {
      isKilled = false;
      stepper.enableOutputs();
      currentState = TILTING;
    } else if (data == "CAL") {
      isKilled = false;
      stepper.enableOutputs();
      currentState = MANUAL;
    } else if (data == "SET_ZERO") {
      stepper.setCurrentPosition(0);
    } else if (data.startsWith("MOVE,")) {
      int steps = data.substring(5).toInt();
      stepper.move(steps);
    } else {
      parseIncomingData(data);
    }
  }

  // Live Position Reporting
  static unsigned long lastRep = 0;
  if (millis() - lastRep > 150) {
    Serial.print("POS:"); Serial.println(stepper.currentPosition());
    lastRep = millis();
  }

  if (isKilled) return;

  if (currentState == MANUAL) {
    stepper.run();
    return;
  }

  // --- Main Operation State Machine ---
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
      if (millis() - stateStartTime >= pauseTime) currentState = TILTING;
      break;
  }
  stepper.run();
}

void parseIncomingData(String data) {
  int commaIndex = data.indexOf(',');
  if (commaIndex > 0) {
    motorMaxSpeed = data.substring(0, commaIndex).toInt();
    unwindSteps = data.substring(commaIndex + 1).toInt();
    stepper.setMaxSpeed(motorMaxSpeed);
    stepper.setAcceleration(motorMaxSpeed / 2);
  }
}      break;

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
