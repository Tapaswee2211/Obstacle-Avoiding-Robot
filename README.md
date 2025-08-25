# 🚗 Arduino Obstacle-Avoiding Robot with Servo Scanning

This project implements an **autonomous obstacle-avoiding robot** using an **Arduino**, **ultrasonic distance sensor**, and a **servo-mounted scanner**.  
The robot drives forward until an obstacle is detected, then **scans left and right** with a servo-mounted ultrasonic sensor to choose the clearest path (right-first preference).  
If both sides are blocked, it reverses slightly and retries.

---

## 🛠️ Hardware Requirements
- Arduino Uno / Nano / Mega
- Motor driver (e.g., L298N or L293D)
- 2 DC motors with wheels
- Servo motor (for ultrasonic sensor scanning)
- Ultrasonic distance sensor (HC-SR04 or compatible)
- Power supply (battery pack)
- Jumper wires and chassis

---

## ⚡ Pin Configuration
| Component         | Arduino Pin |
|-------------------|-------------|
| Motor ENA (left)  | D5 (PWM)    |
| Motor IN1         | D7          |
| Motor IN2         | D8          |
| Motor IN3         | D9          |
| Motor IN4         | D11         |
| Motor ENB (right) | D6 (PWM)    |
| Servo             | D3          |
| Ultrasonic TRIG   | A5          |
| Ultrasonic ECHO   | A4          |

---

## ⚙️ Parameters & Behavior
- **Servo Angles**  
  - Center: `90°`  
  - Right: `165°`  
  - Left: `15°`  

- **Distance Threshold**:  
  - Stops if an obstacle is closer than `21 cm`  

- **Motion**  
  - Forward speed: `120` (PWM)  
  - Turn speed: `140` (PWM)  
  - Reverses for `300 ms` if blocked on both sides  

- **Scanning**  
  - Quick forward checks: 2 pings  
  - Side scans: 4 averaged pings  
  - Servo settle delay: `120 ms`  

---

## 📖 How It Works
1. **Drive Forward** while checking front distance with ultrasonic.
2. If **clear** → keep moving forward.
3. If **obstacle detected**:
   - Stop motors.
   - Scan **RIGHT first**.  
     - If clear, rotate right until forward is clear.
   - If right blocked, scan **LEFT**.  
     - If clear, rotate left until forward is clear.
   - If both sides blocked, **reverse briefly** and retry.
4. Loop continues autonomously.

---

## ▶️ Usage
1. Upload the Arduino code to your board.
2. Assemble the hardware as per the wiring diagram.
3. Place the robot on the ground and power it up.
4. Watch it navigate around obstacles!

---

## 🖥️ Serial Monitor (for debugging)
The robot prints debug information such as:

