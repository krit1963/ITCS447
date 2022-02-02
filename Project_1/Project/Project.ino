//Define the variables
#define trigPin 32 //GPIO32 as Trigger pin
#define echoPin 33 //GPIO33 as Echo pin
#define LED 22 //GPIO22 as LED
#define SW 18 //GPIO18 as SWITCH
long duration, distance;
volatile bool sonicState;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR handleInterrupt();
 
void setup(){ 
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  pinMode(LED, OUTPUT);
  pinMode(SW, INPUT);
  
  //Set the interrupt
  attachInterrupt(digitalPinToInterrupt(SW), handleInterrupt, FALLING); // for when the pin goes from high to low.

  sonicState = true; //Set the state to true
  
  Serial.begin (9600); //Starts the serial communication
}

void loop(){ 
  if(sonicState){
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  distance = (duration/2) / 29.1; //Convert the echo duration to distance in cm
  Serial.println("Distance: " + String(distance) + " cm"); // Prints the distance on the Serial Monitor
  
  if (distance < 15) { //If distance is less than 15 cm will trigger the LED
    digitalWrite(LED,HIGH);
  }
  else { //Else it will turn LED off
  digitalWrite(LED,LOW);
  }
 }
 delay(100);
}

void IRAM_ATTR handleInterrupt(){
  portENTER_CRITICAL_ISR(&mux);
  sonicState = !sonicState; //To change state
  portEXIT_CRITICAL_ISR(&mux);
  }
