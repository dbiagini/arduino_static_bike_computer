
// Pin connected to the reed switch
#define LED_PIN 3
#define REEDSWITCH_PIN 2
#define DEBOUNCE 10// ms 250//156
#define WHEELSIZE 

// Variables for counting and timing
volatile unsigned long pulseCount = 0;       // Counts reed switch triggers
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
  unsigned long pulse = pulseCount;
  float rotPerMinute = rpmsIntraPulses; 
  float speed = speedKmh;
  sei();

  // Update total distance in km
  totalDistanceKm = (pulse * wheelCircumference) / 1000.0;

  Serial.print("Variable_1:");
  Serial.println(speed);

  // Serial.print("Frequency (Hz): ");
  // Serial.println(freq);
  // Serial.print("Rpms: ");
  // Serial.println(rotPerMinute);
  // Serial.print("Total Distance Km: ");
  // Serial.println(totalDistanceKm);
  // if (freq > 0) {
  //   Serial.print("Time Period (ms): ");
  //   Serial.println(period); // Convert to milliseconds
  //   Serial.print("Speed Km/h: ");
  //   Serial.println(speed);
  // } else {
  //   Serial.println("Time Period: - (Wheel stopped)");
  // }

  delay(250); // Update every quarter second
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
    frequencyIntraPulses = 1000.0 / periodIntraPulses; 
    rpmsIntraPulses = frequencyIntraPulses * 60;
    speedKmh = frequencyIntraPulses * wheelCircumference * 3.6;

    //secs = 0; //reset seconds count should be not needed as it should fit 136 years of count
  }

  
}

// Timer1 Compare Interrupt Service Routine for frequency calculation
ISR(TIMER1_COMPA_vect) {
  static unsigned long lastPulseCount = 0;
  static unsigned int countsStuck = 0;

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

    if(pulseCount == lastPulseCount) {
      if (countsStuck < 7) {
        //no NEW pulses in the last second
        countsStuck++;
        ///simulate slow down at constant rate
          periodIntraPulses += countsStuck * 1000;
          if (periodIntraPulses)
             frequencyIntraPulses = 1000.0 / periodIntraPulses; 
             else frequencyIntraPulses = 0;
          rpmsIntraPulses = frequencyIntraPulses * 60;
          speedKmh = frequencyIntraPulses * wheelCircumference * 3.6;

       } else{
        // exceeded 7 seconds with no rotations
        //countsStuck = 0;
        periodIntraPulses = 0;
        frequencyIntraPulses = 0; 
        rpmsIntraPulses = 0;
        speedKmh = 0;
       }
    } else {
      countsStuck = 0; //unstuck
    }

  // Update the last pulse count
  lastPulseCount = pulseCount;
}