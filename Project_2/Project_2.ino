/* Session NETPIE2020*/
#include <PubSubClient.h>
/* Session (WiFi)*/
#include <WiFi.h>

//Define the variables
#define trigPin 32 //GPIO32 as Trigger pin
#define echoPin 33 //GPIO33 as Echo pin
#define LED 22 //GPIO22 as LED
#define LED_CH0 0
#define SW 18 //GPIO18 as SWITCH
#define FREQ 5000
#define LED_RES 8

bool wifiConnected = true; // Flag determining if this device can connect to WiFi
long duration, distance;
int min_dis;
int alert_status = 0;
volatile bool sonicState;

const char* mqttServer = "broker.netpie.io";
const int mqttPort = 1883;
const char* mqttClient = "080819a8-8821-4474-814a-53de1a61d5df"; //Client ID
const char* mqttUser = "vjii91Bnqj9Qt6nuiPpZUqbasHyNiWbo"; //Token
const char* mqttPassword = "5FAxgOw5Q3jrivO$-QTG2_JvhJK68L~V"; //Secret

/* (WiFi) Variables */
char ssid[] = "******"; // Your WiFi credentials.
char pass[] = "******"; // Set password to "" for open networks.

/* MQTT Instance */
WiFiClient espClient;
PubSubClient client (espClient);

/* Value Buffer */
char buf[200]; //Reserved for 200 bytes
long now, lastMsg;


portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR handleInterrupt();

void setup() {
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  pinMode(SW, INPUT);

  ledcSetup(LED_CH0, FREQ, LED_RES);
  ledcAttachPin(LED, LED_CH0);

  //Set the interrupt
  attachInterrupt(digitalPinToInterrupt(SW), handleInterrupt, FALLING); // for when the pin goes from high to low.

  sonicState = true; //Set the state to true

  Serial.begin (9600); //Starts the serial communication


  /* (WiFi) Connection Setup */
  WiFi.mode(WIFI_STA);      // set to station mode
  WiFi.begin(ssid, pass);   // connect to an access point
  delay(5000);

  /* loop until ESP32 can sucesfully connect to the WiFi */
  Serial.printf("Connecting to %s ", ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  /* connection is successful */
  Serial.println(" CONNECTED");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  /* MQTT NetPie Server */
  client.setServer(mqttServer, mqttPort); //NetPie server and port
  client.setCallback(callback);

}

void loop() {
  if (sonicState) {
    now = millis(); //Milliseconds now (timestamp)
    if (now - lastMsg > 5000) { //Publish new messages to the broker again when 5s passes; otherwise, let it handle subscribed messages with little blocking
      lastMsg = now;
      if (!client.connected())
        netpieReconnect();
      client.loop();
      // Clears the trigPin
      digitalWrite(trigPin, LOW);
      delayMicroseconds(2);
      // Sets the trigPin on HIGH state for 10 micro seconds
      digitalWrite(trigPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin, LOW);



      // Reads the echoPin, returns the sound wave travel time in microseconds
      duration = pulseIn(echoPin, HIGH);
      distance = (duration / 2) / 29.1; //Convert the echo duration to distance in cm

      if (distance < min_dis) {
        alert_status = 1;
      }
      else {
        alert_status = 0;
      }


  /* NetPie Transmission */
  sprintf(buf, "{\"data\":{\"distance\":%i, \"alert_status\":%i, \"min_dis\":%i}}", distance, alert_status, min_dis);
  Serial.println(buf);
  Serial.println("Distance: " + String(distance) + " cm");
  client.publish("@shadow/data/update", buf);
  delay(1);
}
}
}

void netpieReconnect() {
  while (!client.connected()) {
    Serial.println("Connecting to NetPie...");
    if (client.connect(mqttClient, mqttUser, mqttPassword )) {
      Serial.println("connected");
      client.subscribe("@msg/led");
      client.subscribe("@msg/min_dis");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  char msg[length + 1];
  memcpy(msg, payload, length);
  msg[length] = '\0';
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.printf(": %s]\n", msg);
  int val = String(msg).toInt();
  if (String(topic) == "@msg/led") {
    int written;
    if (val < 0)
      written = 0;
    else if (val > 255)
      written = 255;
    else
      written = val;
    ledcWrite(LED_CH0, written);
    sprintf(buf, "{\"data\":{\"led\":%d}}", written);
    Serial.println(buf);
    client.publish("@shadow/data/update", buf); //Feedback and record the updated value to NetPie
  }
  else if (String(topic) == "@msg/min_dis") {
    min_dis = val;
    sprintf(buf, "{\"data\":{\"min_dis\":%i}}", min_dis);
    Serial.println(buf);
    client.publish("@shadow/data/update", buf); //Feedback and record the updated value to NetPie
  }
}


void IRAM_ATTR handleInterrupt() {
  portENTER_CRITICAL_ISR(&mux);
  sonicState = !sonicState; //To change state
  portEXIT_CRITICAL_ISR(&mux);
}
