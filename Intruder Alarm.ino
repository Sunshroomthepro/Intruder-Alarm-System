
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ========== PIN ASSIGNMENTS ==========
#define IR_PIN       5
#define ARM_BUTTON   10
#define DISARM_BUTTON 11
#define MODE_BUTTON  12

#define GREEN_LED    13
#define RED_LED      14
#define BUZZER_PIN   15

// ========== OLED SETTINGS ==========
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1
);

// Most FC-51-style IR modules output LOW when detecting an object.
// Change this to HIGH if your sensor test shows the opposite.
#define IR_DETECTED LOW

// ========== SYSTEM VARIABLES ==========
bool armed = false;
bool alarmTriggered = false;
bool soundMode = false;

unsigned long lastDisplayUpdate = 0;
unsigned long lastAlarmUpdate = 0;

bool buzzerState = false;

// ========== BUTTON DEBOUNCING ==========
struct Button {
  int pin;
  bool lastReading = HIGH;
  bool stableState = HIGH;
  unsigned long lastChange = 0;
};

Button armButton   = {ARM_BUTTON};
Button disarmButton = {DISARM_BUTTON};
Button modeButton  = {MODE_BUTTON};

bool buttonPressed(Button &button) {
  bool reading = digitalRead(button.pin);

  if (reading != button.lastReading) {
    button.lastChange = millis();
    button.lastReading = reading;
  }

  if (millis() - button.lastChange > 40) {
    if (reading != button.stableState) {
      button.stableState = reading;

      if (button.stableState == LOW) {
        return true;
      }
    }
  }

  return false;
}

// ========== OLED DISPLAY ==========
void updateDisplay() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("INTRUDER ALARM SYSTEM");

  display.drawLine(0, 12, 127, 12, SSD1306_WHITE);

  display.setCursor(0, 20);

  if (!armed) {
    display.println("STATUS: DISARMED");
    display.println();
    display.println("Press ARM to activate");
  }
  else if (alarmTriggered) {
    display.println("!!! INTRUDER !!!");
    display.println();
    display.println("ALARM ACTIVATED");
    display.println("Press DISARM");
  }
  else {
    display.println("STATUS: ARMED");
    display.println();
    display.println("Monitoring area...");
  }

  display.setCursor(0, 55);
  display.print("Sound mode: ");
  display.println(soundMode ? "PULSE" : "STEADY");

  display.display();
}

// ========== BUZZER CONTROL ==========
void stopBuzzer() {
  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, LOW);
}

void updateBuzzer() {
  if (!alarmTriggered) {
    stopBuzzer();
    return;
  }

  if (!soundMode) {
    // Steady alarm tone
    tone(BUZZER_PIN, 2000);
  }
  else {
    // Pulsing alarm tone
    if (millis() - lastAlarmUpdate >= 300) {
      lastAlarmUpdate = millis();
      buzzerState = !buzzerState;

      if (buzzerState) {
        tone(BUZZER_PIN, 2000);
      }
      else {
        stopBuzzer();
      }
    }
  }
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);

  pinMode(IR_PIN, INPUT);

  pinMode(ARM_BUTTON, INPUT_PULLUP);
  pinMode(DISARM_BUTTON, INPUT_PULLUP);
  pinMode(MODE_BUTTON, INPUT_PULLUP);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  stopBuzzer();

  Wire.begin(8, 9);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED initialization failed!");
    while (true) {
      delay(100);
    }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);
  display.println("INTRUDER ALARM");
  display.println("SYSTEM STARTING...");
  display.display();

  delay(1500);

  updateDisplay();

  Serial.println("Intruder Alarm Ready!");
}

// ========== MAIN LOOP ==========
void loop() {

  // ARM button
  if (buttonPressed(armButton)) {
    armed = true;
    alarmTriggered = false;
    buzzerState = false;
    stopBuzzer();

    Serial.println("SYSTEM ARMED");
  }

  // DISARM button
  if (buttonPressed(disarmButton)) {
    armed = false;
    alarmTriggered = false;
    buzzerState = false;
    stopBuzzer();

    Serial.println("SYSTEM DISARMED");
  }

  // MODE button
  if (buttonPressed(modeButton)) {
    soundMode = !soundMode;
    buzzerState = false;
    lastAlarmUpdate = millis();

    Serial.println(soundMode ? "PULSE MODE" : "STEADY MODE");
  }

  // IR sensor detection
  if (armed && !alarmTriggered) {
    if (digitalRead(IR_PIN) == IR_DETECTED) {
      alarmTriggered = true;

      Serial.println("!!! INTRUDER DETECTED !!!");
    }
  }

  // LEDs
  digitalWrite(GREEN_LED, armed ? HIGH : LOW);
  digitalWrite(RED_LED, alarmTriggered ? HIGH : LOW);

  // Buzzer
  updateBuzzer();

  // Refresh OLED
  if (millis() - lastDisplayUpdate >= 150) {
    lastDisplayUpdate = millis();
    updateDisplay();
  }
}
