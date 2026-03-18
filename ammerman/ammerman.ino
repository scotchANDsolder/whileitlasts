#include <AccelStepper.h>

const int stepPin = 3;
const int dirPin = 4;
const int reedPin = 13;

// --- Motion Settings ---
long motorMaxSpeed = 1000;  
long unwindSteps = 3000;      
unsigned long pauseTime = 1000;
float accelerationValue = 800.0;

// --- State Machine ---
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

void killMotor() {
  isKilled = true;
  stepper.stop();
  stepper.disableOutputs();
}

void loop() {
  // 1. Process Serial Commands
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    data.trim();

    if (data == "STOP") {
      killMotor();
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
    } else if (data.indexOf(',') > 0) {
      int commaIndex = data.indexOf(',');
      motorMaxSpeed = data.substring(0, commaIndex).toInt();
      unwindSteps = data.substring(commaIndex + 1).toInt();
      stepper.setMaxSpeed(motorMaxSpeed);
      stepper.setAcceleration(motorMaxSpeed / 2);
    }
  }

  // 2. SAFETY CHECK: Only runs during Auto-Cycle (TILTING/UNWINDING)
  // If it goes 10,000 steps past the magnet while AUTO is running, kill it.
  bool isAutoRunning = (currentState == TILTING || currentState == UNWINDING);
  if (!isKilled && isAutoRunning && stepper.currentPosition() < -10000) {
    Serial.println("STATUS:ERROR_LIMIT");
    killMotor();
  }

  // 3. Status Reporting
  static unsigned long lastRep = 0;
  if (millis() - lastRep > 250) {
    Serial.print("POS:"); Serial.print(stepper.currentPosition());
    Serial.print("|STATE:");
    switch(currentState) {
      case TILTING: Serial.println("TILTING"); break;
      case PAUSE_TOP: Serial.println("PAUSE_TOP"); break;
      case UNWINDING: Serial.println("UNWINDING"); break;
      case PAUSE_BOTTOM: Serial.println("PAUSE_BOTTOM"); break;
      case MANUAL: Serial.println("MANUAL"); break;
      case KILLED: Serial.println("KILLED"); break;
    }
    lastRep = millis();
  }

  if (isKilled) return;

  // 4. Logic States
  if (currentState == MANUAL) {
    stepper.run();
    return;
  }

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
