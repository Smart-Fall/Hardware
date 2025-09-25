#define BUZZER 13 // ESP32 GPIO21 pin connected to Buzzer's pin
#define BUTTON 2 // ESP32 GPIO16 pin connected to button's pin
#define BUTTON_2 4
#define LED_PIN 16

void setup() {
  pinMode(BUTTON, INPUT_PULLUP); // set ESP32 pin to input pull-up mode
  pinMode(BUZZER, OUTPUT); // set ESP32 pin to output mode
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_2, INPUT_PULLUP);
}

void loop() {
  /*
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(500);
  */
  int buttonSOS = digitalRead(BUTTON); // read new state

  if (buttonSOS == LOW) 
  {
    //digitalWrite(BUZZER, HIGH);  // turn on
    tone(BUZZER,100, 1000);
    delay(1000);
    noTone(BUZZER);
  }
  else
  if (buttonSOS == HIGH) 
  {
    //digitalWrite(BUZZER, LOW);  // turn off
    noTone(BUZZER);
  }

  int buttonHaptic = digitalRead(BUTTON_2);

  if (buttonHaptic == LOW){
    digitalWrite(LED_PIN, HIGH); // turn LED on
    delay(1000);                 // keep it on for 1 sec
    digitalWrite(LED_PIN, LOW);
  } else {
    digitalWrite(LED_PIN, LOW);  // make sure LED is off if not pressed
  }
}
    
