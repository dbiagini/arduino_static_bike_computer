#include <avr/io.h>
#include <avr/interrupt.h>

// Pin connected to the reed switch
const int reedSwitchPin = 2;

// Variables to track time and rotations
volatile unsigned long overflowCount = 0; // Count of timer overflows
volatile unsigned long lastTimerValue = 0; // Last Timer1 value
volatile unsigned long timeInterval = 0;  // Time interval between rotations in timer ticks

void setup() {
  // Set up the reed switch pin
  pinMode(reedSwitchPin, INPUT_PULLUP);

  // Configure Timer1
  TCCR1A = 0; // Normal mode
  TCCR1B = (1 << CS11); // Prescaler set to 8 (1 tick = 0.5 us on 16 MHz Arduino)
  TCNT1 = 0; // Initialize Timer1 counter

  // Enable Timer1 overflow interrupt
  TIMSK1 = (1 << TOIE1);

  // Attach an interrupt to detect the reed switch signal
  attachInterrupt(digitalPinToInterrupt(reedSwitchPin), measureTime, FALLING);

  // Enable global interrupts
  sei();

  // Serial communication for monitoring
  Serial.begin(9600);
}

void loop() {
  // Print the measured time interval and calculate frequency
  static unsigned long lastPrintTime = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastPrintTime >= 1000) { // Print every second
    lastPrintTime = currentTime;

    // Safely read the time interval value
    noInterrupts();
    unsigned long intervalCopy = timeInterval; // Get a safe copy of the interval
    interrupts();

    if (intervalCopy > 0) {
      float intervalMicroseconds = intervalCopy * 0.5; // Convert ticks to microseconds
      float frequency = 1.0 / (intervalMicroseconds / 1000000.0); // Frequency in Hz
      Serial.print("Time Interval (us): ");
      Serial.println(intervalMicroseconds);
      Serial.print("Frequency (Hz): ");
      Serial.println(frequency);
    } else {
      Serial.println("Waiting for first rotation...");
    }
  }
}

// Interrupt Service Routine (ISR) for time measurement
void measureTime() {
  unsigned long currentTimerValue;
  unsigned long overflows;

  noInterrupts();
  currentTimerValue = TCNT1;  // Read the current Timer1 value
  overflows = overflowCount; // Read the overflow count
  interrupts();

  // Calculate the full timer value, including overflows
  unsigned long fullTimerValue = (overflows << 16) + currentTimerValue;

  if (lastTimerValue > 0) { // Skip the first measurement
    timeInterval = fullTimerValue - lastTimerValue; // Calculate time interval (ticks)
  }
  lastTimerValue = fullTimerValue; // Update the last timer value
}

// Timer1 Overflow Interrupt Service Routine
ISR(TIMER1_OVF_vect) {
  overflowCount++;
}
