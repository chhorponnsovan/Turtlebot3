#include <Arduino.h>

// ── Motor pins ────────────────────────────────
const uint8_t ENA = 13, IN1 = 12, IN2 = 14;  // Left
const uint8_t ENB = 21, IN3 = 18, IN4 = 19;  // Right

// ── Encoder pins ──────────────────────────────
const uint8_t ENC_LA = 23, ENC_LB = 22;
const uint8_t ENC_RA = 26, ENC_RB = 25;

// ── PWM config ────────────────────────────────
const uint16_t PWM_FREQ = 20000;
const uint8_t  PWM_RES  = 8;        // 0–255
const uint8_t  CH_L     = 0;
const uint8_t  CH_R     = 1;
 
// ── Deadband ──────────────────────────────────
const uint8_t MIN_PWM = 140;        // Motors don't spin below this — tune to your motors

// ── Encoder state ─────────────────────────────
volatile long enc_L = 0;
volatile long enc_R = 0;

void IRAM_ATTR isr_L() {
    enc_L += (digitalRead(ENC_LA) == digitalRead(ENC_LB)) ? -1 : 1;
}
void IRAM_ATTR isr_R() {
    enc_R += (digitalRead(ENC_RA) == digitalRead(ENC_RB)) ? -1 : 1;
}

// ── Helpers ───────────────────────────────────
 
// Scale [1..127] → [MIN_PWM..255]
uint8_t scalePwm(uint8_t raw) {
    if (raw == 0) return 0;
    return (uint8_t)(MIN_PWM + (float)raw / 127.0f * (255 - MIN_PWM));
}

uint8_t scalePwmRight(uint8_t raw) {
    if (raw == 0) return 0;
    float basePwm = MIN_PWM + (float)raw / 127.0f * (255 - MIN_PWM);
    float r_correction = 1.0055f - (0.000125f * basePwm);
    int correctedPwm = (int)(basePwm * r_correction);
    return (uint8_t)constrain(correctedPwm, MIN_PWM, 255);
}
 
void driveMotor(uint8_t ch, uint8_t in_a, uint8_t in_b, int8_t val, bool useRightCorrection = false) {
    if (val == 0) {
        digitalWrite(in_a, LOW);
        digitalWrite(in_b, LOW);
        ledcWrite(ch, 0);
        return;
    }
    ledcWrite(ch, useRightCorrection ? scalePwmRight((uint8_t)abs(val)) : scalePwm((uint8_t)abs(val)));
    if (val > 0) { digitalWrite(in_a, HIGH); digitalWrite(in_b, LOW);  }
    else         { digitalWrite(in_a, LOW);  digitalWrite(in_b, HIGH); }
}


// ── Watchdog ──────────────────────────────────
unsigned long lastCmdTime = 0;
const unsigned long TIMEOUT_MS = 500;


// ── Setup ─────────────────────────────────────

void setup() {
    Serial.begin(115200);

        // Motor direction pins
    pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
 
    // PWM channels
    ledcSetup(CH_L, PWM_FREQ, PWM_RES); ledcAttachPin(ENA, CH_L);
    ledcSetup(CH_R, PWM_FREQ, PWM_RES); ledcAttachPin(ENB, CH_R);


    // Encoder pins + interrupts
    pinMode(ENC_LA, INPUT); pinMode(ENC_LB, INPUT);
    pinMode(ENC_RA, INPUT); pinMode(ENC_RB, INPUT);
    attachInterrupt(digitalPinToInterrupt(ENC_LA), isr_L, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_RA), isr_R, CHANGE);

    driveMotor(CH_L, IN1, IN2, 0);
    driveMotor(CH_R, IN3, IN4, 0);

}

// ── Loop ──────────────────────────────────────

// Command format (3 bytes from RPi):
//   [ left : int8 ]  [ right : int8 ]  [ 0xFF : delimiter ]
//   Range: -127 (full back) to +127 (full forward), 0 = stop

// Feedback format sent back to RPi every 20ms (11 bytes):
//   [ 0xAA : start ]  [ enc_L : int32 little-endian ]  [ enc_R : int32 little-endian ]  [ 0xFF : end ]

void loop() {
    unsigned long now = millis();

    // ── Receive velocity command ───────────────
    if (Serial.available() >= 3) {
        uint8_t buf[3];
        Serial.readBytes(buf, 3);
 
        if (buf[2] == 0xFF) {                       // valid frame
            int8_t cmd_L = (int8_t)buf[0];
            int8_t cmd_R = (int8_t)buf[1];

            driveMotor(CH_L, IN1, IN2, cmd_L);
            driveMotor(CH_R, IN3, IN4, cmd_R, true);
            lastCmdTime = now;
        } else {
            while (Serial.available()) Serial.read(); // resync on bad frame
        }
    }


    // ── Send encoder feedback at 50 Hz ────────
    static unsigned long lastTx = 0;
    if (now - lastTx >= 20) {
        lastTx = now;

        // Atomic read
        noInterrupts();
        long snap_L = enc_L;
        long snap_R = enc_R;
        interrupts();


        // Pack: 0xAA | enc_L (4 bytes LE) | enc_R (4 bytes LE) | speed_L | speed_R | 0xFF
        uint8_t pkt[11];
        pkt[0] = 0xAA;
        memcpy(&pkt[1], &snap_L, 4);
        memcpy(&pkt[5], &snap_R, 4);
        pkt[9]  = 0xFF;
        pkt[10] = '\n';   // extra newline makes it human-readable in Serial Monitor too

        Serial.write(pkt, 11);
    }
}
