
// Pin connected to the reed switch
#define LED_PIN 3
#define REEDSWITCH_PIN 2
#define DEBOUNCE 156
#define WHEELSIZE 

// Variables for counting and timing
volatile unsigned int pulseCount = 0;       // Counts reed switch triggers
volatile unsigned long overflowCount = 0;  // Counts Timer1 overflows
volatile float frequency = 0;              // Frequency in Hz
volatile float timePeriod = 0;             // Time period in seconds
volatile float frequencyIntraPulses = 0;
volatile float periodIntraPulses = 0;
volatile unsigned long countIntraPulses = 0; //counter 
volatile float rpmsIntraPulses = 0;
volatile float rpms = 0;             // Time period in seconds
volatile unsigned long lastTimerValue = 0; // Last Timer1 value (including overflows)
volatile unsigned int secs = 0; // Elapsed seconds


// Parameters
const float wheelDiameter = 0.740; // Example: 0.7 meters (70 cm)
const float wheelCircumference = 3.14159 * wheelDiameter; // Circumference in meters
volatile float speedKmh = 0;                // Speed in km/h
float totalDistanceKm = 0;         // Total distance in km




//control LED
int ledState = LOW;  // ledState used to set the LED

void setup() {
  cli();
  // Set up the reed switch pin
  pinMode(REEDSWITCH_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);


  // Attach an interrupt to count reed switch triggers
  attachInterrupt(digitalPinToInterrupt(REEDSWITCH_PIN), countPulse, FALLING);

/// Configure Timer1 for 1-second intervals
  TCCR1A = 0;               // Normal mode
  TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10); // CTC mode, prescaler 1024
  OCR1A = 15624;            // Compare match value for 1-second interval
  TIMSK1 = (1 << OCIE1A) | (1 << TOIE1);   // Enable Timer1 compare interrupt / Enable Timer1 overflow interrupt

  // Enable global interrupts
  sei();

  // Serial communication for monitoring
  Serial.begin(9600);
}

void loop() {
  // Print the calculated frequency and time period
  cli();
  float freq = frequencyIntraPulses;
  unsigned long period = periodIntraPulses;
  unsigned int pulse = pulseCount;
  float rotPerMinute = rpmsIntraPulses; 
  float speed = speedKmh;
  sei();

  // Update total distance in km
  totalDistanceKm = (pulse * wheelCircumference) / 1000.0;

  Serial.print("Frequency (Hz): ");
  Serial.println(freq);
  Serial.print("Rpms: ");
  Serial.println(rotPerMinute);
  Serial.print("Total Distance Km: ");
  Serial.println(totalDistanceKm);
  if (freq > 0) {
    Serial.print("Time Period (ms): ");
    Serial.println(period); // Convert to milliseconds
    Serial.print("Speed Km/h: ");
    Serial.println(speed);
  } else {
    Serial.println("Time Period: - (Wheel stopped)");
  }

  delay(1000); // Update every quarter second
}

// Interrupt Service Routine for reed switch trigger
void countPulse() {
  static unsigned long lastRotationTime = 0;

  // Calculate elapsed time in milliseconds
  unsigned long currentTimerRegMs = TCNT1 * 1000UL / OCR1A;
  unsigned long currentTimeMs = (secs * 1000) + currentTimerRegMs;

  unsigned long elapsedTimeMs = currentTimeMs - lastRotationTime;

  if (elapsedTimeMs > DEBOUNCE) { // Debounce delay 

    pulseCount++;
    lastRotationTime = currentTimeMs;
    periodIntraPulses = elapsedTimeMs;
    //secs = 0; //reset seconds count should be not needed as it should fit 136 years of count
  }

  
}

// Timer1 Overflow Interrupt Service Routine
ISR(TIMER1_OVF_vect) {
  overflowCount++;
  //if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
     digitalWrite(LED_PIN, ledState);
}

// Timer1 Compare Interrupt Service Routine for frequency calculation
ISR(TIMER1_COMPA_vect) {
  static unsigned int lastPulseCount = 0;
  static unsigned int secsStuck = 0;

  // count seconds 
  secs++;

  // // Calculate frequency
  // frequency = pulseCount - lastPulseCount; // Pulses per second
  // if (frequency > 0) {
  //   timePeriod = 1.0 / frequency; // Time period in seconds
  // } else {
  //   timePeriod = 0; // No pulses
  // }
  // rpms = frequency * 60;

    if((pulseCount == lastPulseCount) && (secsStuck < 7)) {
      //no NEW pulses in the last second
      secsStuck++;
    } else if ((pulseCount == lastPulseCount) && (secsStuck == 7)) {
      // exceeded 7 seconds with no rotations
      secsStuck = 0;
      periodIntraPulses = 0;
      frequencyIntraPulses = 0; 
      rpmsIntraPulses = 0;
    } else {
        if (periodIntraPulses) {
        frequencyIntraPulses = 1000.0 / periodIntraPulses ; 
        } else frequencyIntraPulses = 0;
        rpmsIntraPulses = frequencyIntraPulses * 60;
        // Calculate speed in km/h
        speedKmh = frequencyIntraPulses * wheelCircumference * 3.6;
    }

  // Update the last pulse count
  lastPulseCount = pulseCount;
}