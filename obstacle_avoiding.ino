i#include <Servo.h>

// —— Pin Definitions ——
// Motors
const int ENA = 5;    // PWM left speed
const int IN1 = 7;
const int IN2 = 8;
const int IN3 = 9;
const int IN4 = 11;
const int ENB = 6;    // PWM right speed

// Ultrasonic (analog pins used as digital I/O)
const int TRIG_PIN = A5;
const int ECHO_PIN = A4;

// Servo
const int SERVO_PIN = 3;
Servo scanner;

// —— Parameters ——
// angles
const int CENTER_ANGLE = 90;
const int LEFT_ANGLE   = 15;
const int RIGHT_ANGLE  = 165;

// thresholds & timings (tune these)
const int DIST_THRESHOLD   = 21;   // cm — stops earlier to have time to react
const int FRONT_QUICK_READS= 2;    // small fast average for front checks
const int FRONT_QUICK_DELAY= 10;   // ms between quick front pings
const int SCAN_READS       = 4;    // stable average for side scans
const int SCAN_DELAY_MS    = 20;   // ms between side scan pings
const int SERVO_SETTLE_MS  = 120;  // servo settle time
const int MEASURE_TIMEOUT  = 20000; // pulseIn timeout (us)

// motion
const int BASE_SPEED       = 120;  // forward PWM (0..255)
const int TURN_SPEED       = 140;  // in-place turn PWM
const int TURN_STEP_MS     = 450;   // small slice while turning, then re-check front
const int REVERSE_MS       = 300;  // reverse if completely blocked

// —— Low-level motor helpers ——
void setLeftMotor(int speedSigned) {
  int pwm = abs(speedSigned);
  pwm = constrain(pwm, 0, 255);
  if (speedSigned >= 0) {
    digitalWrite(IN2, HIGH);
    digitalWrite(IN1, LOW);
  } else {
    digitalWrite(IN2, LOW);
    digitalWrite(IN1, HIGH);
  }
  analogWrite(ENA, pwm);
}

void setRightMotor(int speedSigned) {
  int pwm = abs(speedSigned);
  pwm = constrain(pwm, 0, 255);
  if (speedSigned >= 0) {
    digitalWrite(IN4, LOW);
    digitalWrite(IN3, HIGH);
  } else {
    digitalWrite(IN4, HIGH);
    digitalWrite(IN3, LOW);
  }
  analogWrite(ENB, pwm);
}

void driveBoth(int leftSpeed, int rightSpeed) {
  setLeftMotor(leftSpeed);
  setRightMotor(rightSpeed);
}

void driveForward(int speed) {
  driveBoth(speed, speed);
}

void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  // optional: clear direction outputs to safe state
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// —— Ultrasonic helpers ——
long measureDistanceOnce() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long dur = pulseIn(ECHO_PIN, HIGH, MEASURE_TIMEOUT);
  return (dur > 0) ? (dur * 0.034 / 2) : 300;
}

long measureAvgFront(int reads, int delayMs) {
  long sum = 0;
  for (int i = 0; i < reads; ++i) {
    sum += measureDistanceOnce();
    delay(delayMs);
  }
  return sum / reads;
}

long scanAngleAvg(int angle, int reads, int settleMs, int delayMs) {
  scanner.write(angle);
  delay(settleMs);
  long sum = 0;
  for (int i = 0; i < reads; ++i) {
    sum += measureDistanceOnce();
    delay(delayMs);
  }
  return sum / reads;
}

// —— Setup & loop ——
void setup() {
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  scanner.attach(SERVO_PIN);
  scanner.write(CENTER_ANGLE);
  delay(SERVO_SETTLE_MS);

  Serial.begin(9600);
  delay(100);
  stopMotors();
}

void loop() {
  // fast front check while driving
  long dFront = measureAvgFront(FRONT_QUICK_READS, FRONT_QUICK_DELAY);
  Serial.print("Front: "); Serial.println(dFront);

  if (dFront > DIST_THRESHOLD) {
    // clear - keep moving forward
    driveForward(BASE_SPEED);
    return;
  }

  // obstacle detected: stop and examine sides (RIGHT first per your request)
  stopMotors();
  Serial.println("Obstacle detected - scanning RIGHT first");

  long dRight = scanAngleAvg(RIGHT_ANGLE, SCAN_READS, SERVO_SETTLE_MS, SCAN_DELAY_MS);
  Serial.print("Right: "); Serial.println(dRight);

  if (dRight > DIST_THRESHOLD) {
    // turn right in place until front becomes clear
    Serial.println("Right clear -> turning right until center clear");
    while (true) {
      // in-place right turn: left forward, right backward
      setLeftMotor(TURN_SPEED);
      setRightMotor(-TURN_SPEED);
      delay(TURN_STEP_MS);
      stopMotors();

      long dCenter = measureAvgFront(FRONT_QUICK_READS, FRONT_QUICK_DELAY);
      Serial.print("Center during turn: "); Serial.println(dCenter);
      if (dCenter > DIST_THRESHOLD) break;
    }
    // re-center sensor and go forward
    scanner.write(CENTER_ANGLE);
    delay(SERVO_SETTLE_MS);
    driveForward(BASE_SPEED);
    delay(50); // brief advance to exit obstacle zone
    return;
  }

  // Right blocked -> check left
  long dLeft = scanAngleAvg(LEFT_ANGLE, SCAN_READS, SERVO_SETTLE_MS, SCAN_DELAY_MS);
  Serial.print("Left: "); Serial.println(dLeft);

  if (dLeft > DIST_THRESHOLD) {
    // turn left in place until front becomes clear
    Serial.println("Left clear -> turning left until center clear");
    while (true) {
      // in-place left turn: left backward, right forward
      setLeftMotor(-TURN_SPEED);
      setRightMotor(TURN_SPEED);
      delay(TURN_STEP_MS);
      stopMotors();

      long dCenter = measureAvgFront(FRONT_QUICK_READS, FRONT_QUICK_DELAY);
      Serial.print("Center during turn: "); Serial.println(dCenter);
      if (dCenter > DIST_THRESHOLD) break;
    }
    scanner.write(CENTER_ANGLE);
    delay(SERVO_SETTLE_MS);
    driveForward(BASE_SPEED);
    delay(50);
    return;
  }

  // Neither side clear -> reverse a bit and stop (prevents crash)
  Serial.println("Both sides blocked -> reversing short distance");
  setLeftMotor(-BASE_SPEED);
  setRightMotor(-BASE_SPEED);
  delay(REVERSE_MS);
  stopMotors();
  scanner.write(CENTER_ANGLE);
  delay(SERVO_SETTLE_MS);
  // after reversing, loop will re-check front and decide again
}
