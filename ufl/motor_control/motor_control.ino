// Pin assignments
const int sensor1 = 3;
const int sensor2 = 5;

const int motorPin1 = 9;
const int motorPin2 = 11;

bool movingForward = true;
bool motorEnabled = false;  // New flag

// Debounce settings
unsigned long lastTriggerTime = 0;
const unsigned long debounceDelay = 6000;  // 6 seconds

void setup() {
  pinMode(sensor1, INPUT_PULLUP);
  pinMode(sensor2, INPUT_PULLUP);

  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);

  Serial.begin(9600);
  Serial.println("System Ready. Waiting for 'G' command to start.");
  stopMotor();
}

void loop() {
  // --- Check for serial commands ---
  if (Serial.available() > 0) {
    char command = Serial.read();
    if (command == 'G') {
      motorEnabled = true;
      moveForward();
      Serial.println("Received 'G' — Moving Forward.");
    } else if (command == 'S') {
      motorEnabled = false;
      stopMotor();
      Serial.println("Received 'S' — Motor stopped.");
    }
  }

  // --- Sensor and motor logic only if motorEnabled ---
  if (motorEnabled) {
    int s1 = digitalRead(sensor1);
    int s2 = digitalRead(sensor2);

    unsigned long currentTime = millis();

    if ((s1 == LOW || s2 == LOW) && (currentTime - lastTriggerTime > debounceDelay)) {
      toggleDirection();
      lastTriggerTime = currentTime;
    }
  }
}

void toggleDirection() {
  movingForward = !movingForward;
  if (movingForward) {
    moveForward();
    
  } else {
    moveBackward();
  }
}

void moveForward() {
  digitalWrite(motorPin1, HIGH);
  digitalWrite(motorPin2, LOW);
  Serial.println("Moving Forward");
}

void moveBackward() {
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, HIGH);
  Serial.println("Moving Backward");
}

void stopMotor() {
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  Serial.println("Motor Stopped");
}
