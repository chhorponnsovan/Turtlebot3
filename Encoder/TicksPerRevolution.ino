// --- Left Encoder Pins ---
const int encoderL_A = 23;
const int encoderL_B = 22;

// --- Right Encoder Pins ---
const int encoderR_A = 26;
const int encoderR_B = 25;

// Use 'volatile' for variables changed inside interrupts
volatile long pulseCountL = 0;
volatile long pulseCountR = 0;

// Interrupt Service Routine (ISR) for Left Motor
void IRAM_ATTR handleEncoderL() {
  if (digitalRead(encoderL_B) == LOW) {
    pulseCountL++;
  } else {
    pulseCountL--;
  }
}

// Interrupt Service Routine (ISR) for Right Motor
void IRAM_ATTR handleEncoderR() {
  if (digitalRead(encoderR_B) == LOW) {
    pulseCountR++;
  } else {
    pulseCountR--;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(encoderL_A, INPUT_PULLUP);
  pinMode(encoderL_B, INPUT_PULLUP);
  pinMode(encoderR_A, INPUT_PULLUP);
  pinMode(encoderR_B, INPUT_PULLUP);

  // Attach interrupts to Phase A of both encoders
  attachInterrupt(digitalPinToInterrupt(encoderL_A), handleEncoderL, RISING);
  attachInterrupt(digitalPinToInterrupt(encoderR_A), handleEncoderR, RISING);

  Serial.println("--- TAPE TEST READY ---");
  Serial.println("1. Align the tape markers.");
  Serial.println("2. Rotate wheel 10 times slowly.");
  Serial.println("3. Record the pulses from the Serial Monitor.");
}

void loop() {
  // Print current counts every 500ms
  static uint32_t lastPrint = 0;
  if (millis() - lastPrint > 500) {
    Serial.print("Left Pulses: ");
    Serial.print(pulseCountL);
    Serial.print(" | Right Pulses: ");
    Serial.println(pulseCountR);
    lastPrint = millis();
  }
}

