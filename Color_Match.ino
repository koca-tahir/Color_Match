// ==== LED Pin Definitions ====
#define led1 2
#define led2 3
#define led3 4
#define led4 5
#define led5 6
#define led6 7
#define led7 8
#define led8 9
#define button 13

int ledPins[] = {led1, led2, led3, led4, led5, led6, led7, led8};
const int ledCount = 8;

int targetLed;                // the LED the player must "catch"
int currentLed = 0;           // LED currently lit during the chase
int gameState = 0;            // 0: waiting for button press, 1: chasing, 2: showing result
bool won = false;

const int startDelayTime = 150; // starting speed (slower = easier)
int delayTime = startDelayTime;
const int minDelayTime = 30;    // fastest/hardest speed before resetting
const int delayStep = 10;       // how much faster it gets after each win
int roundNumber = 0;

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < ledCount; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }

  pinMode(button, INPUT); // wired with external pull-down resistor (HIGH = pressed)
  randomSeed(analogRead(A0)); // seed randomness from a floating analog pin
}

void loop() {
  if (gameState == 0) {
    // Idle state: all LEDs off, waiting for the player to press the button
    if (digitalRead(button) == HIGH) {
      waitForButtonRelease(); // avoid the same press being read as the "catch"
      startNewRound();
    }
    return;
  }

  if (gameState == 2) {
    showResult();
    return;
  }

  // Forward chase
  for (int i = 0; i < ledCount; i++) {
    currentLed = i;
    if (i != targetLed) digitalWrite(ledPins[i], HIGH);
    if (waitAndCheckButton()) return;
    if (i != targetLed) digitalWrite(ledPins[i], LOW);
  }

  // Backward chase
  for (int i = ledCount - 2; i > 0; i--) {
    currentLed = i;
    if (i != targetLed) digitalWrite(ledPins[i], HIGH);
    if (waitAndCheckButton()) return;
    if (i != targetLed) digitalWrite(ledPins[i], LOW);
  }
}

// Blocks briefly until the button is physically released.
// Prevents a single press from being counted twice (start + catch).
void waitForButtonRelease() {
  while (digitalRead(button) == HIGH) {
    delay(5);
  }
  delay(20); // small debounce after release
}

// Waits delayTime ms while continuously checking for a button press.
// Returns true if the button was pressed during this LED's "on" time.
bool waitAndCheckButton() {
  unsigned long start = millis();
  while (millis() - start < delayTime) {
    if (digitalRead(button) == HIGH) {
      if (currentLed != targetLed) digitalWrite(ledPins[currentLed], LOW);
      won = (currentLed == targetLed);
      gameState = 2;
      return true;
    }
  }
  return false;
}

// Blinks the relevant LED(s) 3 times to show the result, then resets for the next round.
void showResult() {
  static int blinkCount = 0;
  static unsigned long lastBlink = 0;
  static bool ledOn = true;

  if (blinkCount < 6) { // 6 state changes = 3 full blinks
    if (millis() - lastBlink >= 200) {
      ledOn = !ledOn;
      if (won) {
        digitalWrite(ledPins[targetLed], ledOn ? HIGH : LOW);
      } else {
        digitalWrite(ledPins[currentLed], ledOn ? HIGH : LOW);
        digitalWrite(ledPins[targetLed], ledOn ? HIGH : LOW);
      }
      lastBlink = millis();
      blinkCount++;
    }
    return;
  }

  // Turn everything off
  digitalWrite(ledPins[currentLed], LOW);
  digitalWrite(ledPins[targetLed], LOW);
  blinkCount = 0;

  if (won) {
    roundNumber++;
    delayTime -= delayStep;
    if (delayTime <= minDelayTime) {
      delayTime = startDelayTime; // reached max speed -> back to easy
      roundNumber = 0;
    }
  }

  delay(400);

  // Wait for button release before returning to idle,
  // so the same press can't instantly start a new round
  waitForButtonRelease();

  gameState = 0; // back to idle, wait for next press
}

// Picks a new random target LED and turns it on (it stays on for the whole round)
void startNewRound() {
  targetLed = random(0, ledCount);
  digitalWrite(ledPins[targetLed], HIGH);
  gameState = 1;
}
