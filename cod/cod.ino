#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BUZZER_PIN 13
#define SERVO1_PIN 18
#define SERVO2_PIN 19
#define BUTTON_PIN 4
#define LED_PIN 5

Servo servo1;
Servo servo2;

// Stări robotel
#define STATE_SLEEPING       1
#define STATE_WAKEUP         2
#define STATE_BLINK          3
#define STATE_AWAKE          4
#define STATE_ANGRY          5
#define STATE_FALLING_ASLEEP 6

volatile int currentState = STATE_SLEEPING;

// Pentru multitasking servo
volatile int servo1Target = 90; // corp: 45..135, poziția de bază 90
volatile int servo2Target = 0;  // brațe: 0..80

// Buton debounce și apăsare lungă
volatile bool buttonPressed = false;
volatile unsigned long buttonPressTime = 0;
volatile unsigned long buttonReleaseTime = 0;
bool canTriggerAngry = false;

unsigned long lastAwakeTimestamp = 0;
unsigned long buttonHoldStart = 0;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED; // Mutex pentru variabile volatile

/*----------------------------
// Variabile pentru ochi
------------------------------*/
int ref_eye_height = 40;
int ref_eye_width = 40;
int ref_space_between_eye = 10;
int ref_corner_radius = 10;

// current state of the eyes
int left_eye_height = ref_eye_height;
int left_eye_width = ref_eye_width;
int left_eye_x = 32;
int left_eye_y = 32;

int right_eye_x = 32 + ref_eye_width + ref_space_between_eye;
int right_eye_y = 32;
int right_eye_height = ref_eye_height;
int right_eye_width = ref_eye_width;

/*-------------------
-----Funcții Ochi
---------------------*/
void draw_eyes(bool update=true)
{
    display.clearDisplay();        
    //draw from center
    int x = int(left_eye_x-left_eye_width/2);
    int y = int(left_eye_y-left_eye_height/2);
    display.fillRoundRect(x,y,left_eye_width,left_eye_height,ref_corner_radius,SSD1306_WHITE);
    x = int(right_eye_x-right_eye_width/2);
    y = int(right_eye_y-right_eye_height/2);
    display.fillRoundRect(x,y,right_eye_width,right_eye_height,ref_corner_radius,SSD1306_WHITE);    
    if(update)
    {
      display.display();
    }
    
}
void center_eyes(bool update=true)
{
  left_eye_x = SCREEN_WIDTH / 2 - ref_eye_width / 2 - ref_space_between_eye / 2;
  left_eye_y = SCREEN_HEIGHT / 2;
  right_eye_x = SCREEN_WIDTH / 2 + ref_eye_width / 2 + ref_space_between_eye / 2;
  right_eye_y = SCREEN_HEIGHT / 2;
}

void sleep_animation()
{
    for(int i=2; i <= 10; i++){
      left_eye_height = i;
      right_eye_height = i;
      draw_eyes(true);
      delay(40);  
    }
    delay(100);
    
    for(int i=10; i >= 2; i=i-1){
      left_eye_height = i;
      right_eye_height = i;
      draw_eyes(true);
     delay(40);  
    } 
    draw_z_animation();
}

void draw_z_animation() {
  int z_start_x = 128 - 30;
  int z_y = 15;

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(z_start_x, z_y);
  display.print("Z");
  display.display();
  delay(100);

  display.setCursor(z_start_x + 10, z_y - 5);
  display.print("Z");
  display.display();
  delay(100);

  display.setCursor(z_start_x + 20, z_y - 10);
  display.print("Z");
  display.display();
  delay(100);
}

void wakeup_animation()
{
  for(int h=2; h <= ref_eye_height; h+=2)
  {
    left_eye_height = h;
    right_eye_height = h;
    draw_eyes(true);
  }
}

void blink()
{
  draw_eyes();
  
  for(int i=0;i<3;i++)
  {
    left_eye_height -= 12;
    right_eye_height -= 12;    
    draw_eyes();
    delay(1);
  }
  for(int i=0;i<3;i++)
  {
    left_eye_height += 12;
    right_eye_height += 12;
    draw_eyes();
    delay(1);
  }
}

void angry_eyes() {
  display.clearDisplay();

  int tri_width = 40;
  int tri_height = 35;
  int center_y = 32;

  display.fillTriangle
  (19, center_y + tri_height / 2, 
  19, center_y - tri_height / 2, 
  19 + tri_width, center_y + tri_height / 2, 
  SSD1306_WHITE);
  display.fillTriangle
  (109, center_y + tri_height / 2,
   109, center_y - tri_height / 2, 
   109 - tri_width, center_y + tri_height / 2,
    SSD1306_WHITE);
  display.display();
}

void move_right_big_eye()
{
  move_big_eye(1);
}
void move_left_big_eye()
{
  move_big_eye(-1);
}
void move_big_eye(int direction) {
  int direction_oversize = 1;
  int direction_movement_amplitude = 2;
  int blink_amplitude = 5;

  for (int i = 0; i < 3; i++) {
    left_eye_x += direction_movement_amplitude * direction;
    right_eye_x += direction_movement_amplitude * direction;    
    right_eye_height -= blink_amplitude;
    left_eye_height -= blink_amplitude;
    
    if (direction > 0) {
      right_eye_height += direction_oversize;
      right_eye_width += direction_oversize;
    } else {
      left_eye_height += direction_oversize;
      left_eye_width += direction_oversize;
    }

    draw_eyes();
    delay(1);
  }

  for (int i = 0; i < 3; i++) {
    left_eye_x += direction_movement_amplitude * direction;
    right_eye_x += direction_movement_amplitude * direction;
    right_eye_height += blink_amplitude;
    left_eye_height += blink_amplitude;

    if (direction > 0) {
      right_eye_height += direction_oversize;
      right_eye_width += direction_oversize;
    } else {
      left_eye_height += direction_oversize;
      left_eye_width += direction_oversize;
    }

    draw_eyes();
    delay(1);
  }

  
}

void reset_eyes_position_and_size() {
  left_eye_width = ref_eye_width;
  right_eye_width = ref_eye_width;
  left_eye_height = ref_eye_height;
  right_eye_height = ref_eye_height;

  center_eyes(); // recalculează și poziția X/Y
}


void falling_asleep_eyes() {

   for(int k=ref_eye_height; k >= 2; k-=2)
  {
    left_eye_height = k;
    right_eye_height = k;
    draw_eyes(true);
  }
}
/*-------------------
-----Funcții Sunete
---------------------*/ 
void playBeep(int freq, int duration) {
  tone(BUZZER_PIN, freq); delay(duration); noTone(BUZZER_PIN);
}

void playSweep(int startFreq, int endFreq, int stepDelay) {
  int step = (startFreq < endFreq) ? 10 : -10;
  for (int f = startFreq; (step > 0) ? f <= endFreq : f >= endFreq; f += step) {
    tone(BUZZER_PIN, f); delay(stepDelay);
  }
  noTone(BUZZER_PIN);
}

void playGlitch(int count) {
  for (int i = 0; i < count; i++) {
    tone(BUZZER_PIN, random(300, 1200));
    delay(random(30, 80));
    noTone(BUZZER_PIN);
    delay(random(20, 70));
  }
}

void playTrill(int f1, int f2, int dur, int reps) {
  for (int i = 0; i < reps; i++) {
    tone(BUZZER_PIN, f1); delay(dur);
    tone(BUZZER_PIN, f2); delay(dur);
  }
  noTone(BUZZER_PIN);
}

void playMelody1() {
  playBeep(600, 100); delay(50);
  playBeep(700, 100); delay(50);
  playBeep(800, 150); delay(100);
  playSweep(800, 400, 5); delay(200);
}

void playMelody2() {
  playBeep(500, 120); delay(60);
  playBeep(550, 120); delay(60);
  playBeep(600, 120); delay(60);
  playSweep(600, 900, 4); delay(150);
  playBeep(400, 100); delay(50);
}

void playRandomBeeps(int count) {
  for (int i = 0; i < count; i++) {
    playBeep(random(300, 1200), random(80, 150));
    delay(random(50, 150));
  }
}

/*-------------------
----- Task-uri servo
---------------------*/
void servo1Task(void *pvParameters) {
  int currentPos = 85;
  servo1.write(currentPos);
  while(true) {
    portENTER_CRITICAL(&mux);
    int target = servo1Target;
    portEXIT_CRITICAL(&mux);

    if(currentPos < target) {
      currentPos+=1;
      servo1.write(currentPos);
      vTaskDelay(30 / portTICK_PERIOD_MS);
    } else if(currentPos > target) {
      currentPos-=1;
      servo1.write(currentPos);
      vTaskDelay(30 / portTICK_PERIOD_MS);
    } else {
      vTaskDelay(1 / portTICK_PERIOD_MS);
    }
  }
}

void servo2Task(void *pvParameters) {
  int currentPos = 0;
  servo2.write(currentPos);
  while(true) {
    portENTER_CRITICAL(&mux);
    int target = servo2Target;
    portEXIT_CRITICAL(&mux);

    if(currentPos < target) {
      currentPos+=1;
      servo2.write(currentPos);
      vTaskDelay(5 / portTICK_PERIOD_MS);
    } else if(currentPos > target) {
      currentPos-=1;
      servo2.write(currentPos);
      vTaskDelay(5 / portTICK_PERIOD_MS);
    } else {
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
  }
}

/*-------------------
----- ISR Buton
---------------------*/
void IRAM_ATTR buttonISR() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > 200) { // debounce 200ms
    if(digitalRead(BUTTON_PIN) == LOW) {
      buttonPressed = true;
      buttonPressTime = interruptTime;
    } else {
      buttonReleaseTime = interruptTime;
    }
  }
  lastInterruptTime = interruptTime;
}


/*-------------------
----- Funcții stare
---------------------*/

void sleeping_state()
{
  sleep_animation();
}

void wakeup_state()
{
  wakeup_animation();
  playMelody1();

 // Miscare lenta servo1 de la 90 la 120, servo2 de la 0 la 70
// Miscare servo1 de la 90 la 120, servo2 de la 0 la 70
  move_right_big_eye();

for(int pos = 90; pos <= 150; pos++) {
  portENTER_CRITICAL(&mux);
  servo1Target = pos;
  servo2Target = constrain(map(pos, 90, 120, 0, 70), 0, 70);
  portEXIT_CRITICAL(&mux);
  vTaskDelay(10 / portTICK_PERIOD_MS);
}
  reset_eyes_position_and_size();
  vTaskDelay(300 / portTICK_PERIOD_MS);
  center_eyes();
  move_left_big_eye();

for(int pos = 150; pos >= 40; pos--) {
  portENTER_CRITICAL(&mux);
  servo1Target = pos;
  servo2Target = constrain(120 - pos, 0, 70);
  portEXIT_CRITICAL(&mux);
  vTaskDelay(10 / portTICK_PERIOD_MS);
}
  reset_eyes_position_and_size();
  vTaskDelay(300 / portTICK_PERIOD_MS);
  move_right_big_eye();


// Miscare servo1 de la 60 la 90, servo2 inapoi de la 0 la 70
for(int pos = 40; pos <= 90; pos++) {
  portENTER_CRITICAL(&mux);
  servo1Target = pos;
  servo2Target = constrain(map(pos, 60, 90, 0, 70), 0, 70);
  portEXIT_CRITICAL(&mux);
  vTaskDelay(10 / portTICK_PERIOD_MS);
}

  reset_eyes_position_and_size();
  vTaskDelay(300 / portTICK_PERIOD_MS);
  draw_eyes();
}

void awake_state()
{
  blink();

   //Miscare la dreapta
  move_right_big_eye();
  
  portENTER_CRITICAL(&mux);
  servo1Target = 155;
  portEXIT_CRITICAL(&mux);
  vTaskDelay(2000 / portTICK_PERIOD_MS);

  reset_eyes_position_and_size();
  for(int pos = 0; pos <= 60; pos+=1) {   
    portENTER_CRITICAL(&mux);
    servo2Target = pos;
    portEXIT_CRITICAL(&mux);
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
  playMelody1();

  
  // Revenire
  portENTER_CRITICAL(&mux);
  servo1Target = 90;
  servo2Target = 0;
  portEXIT_CRITICAL(&mux);
  vTaskDelay(20 / portTICK_PERIOD_MS);
  
  // Miscare la stanga
  move_left_big_eye();
  
  portENTER_CRITICAL(&mux);
  servo1Target = 25;
  portEXIT_CRITICAL(&mux);
    vTaskDelay(2000 / portTICK_PERIOD_MS);

  reset_eyes_position_and_size();
  for(int pos = 0; pos <= 60; pos+=1) {
    portENTER_CRITICAL(&mux);
    servo2Target = pos;
    portEXIT_CRITICAL(&mux);
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
  playMelody1();
  
  portENTER_CRITICAL(&mux);
  servo1Target = 90;
  servo2Target = 0;
  portEXIT_CRITICAL(&mux);
  reset_eyes_position_and_size();
  vTaskDelay(20 / portTICK_PERIOD_MS);
    

}

void angry_state()
{
  angry_eyes();
  playGlitch(5);

  for (int i = 0; i < 20; i++) {
    portENTER_CRITICAL(&mux);
    servo1Target = 90 + random(-30, 30);  
    portEXIT_CRITICAL(&mux);
    servo2.write(random(0, 70));          
    vTaskDelay(200 / portTICK_PERIOD_MS); 
  }
}


void fallingasleep_state(){
  playRandomBeeps(3);
        portENTER_CRITICAL(&mux);
        servo1Target = 90;
        portEXIT_CRITICAL(&mux);
        vTaskDelay(50 / portTICK_PERIOD_MS);
      for (int i = 0; i < 3; i++) {
        portENTER_CRITICAL(&mux);
        servo2Target = 90;
        portEXIT_CRITICAL(&mux);
        vTaskDelay(200 / portTICK_PERIOD_MS);

        portENTER_CRITICAL(&mux);
        servo2Target = 0;
        portEXIT_CRITICAL(&mux);
        vTaskDelay(200 / portTICK_PERIOD_MS);
      }
  falling_asleep_eyes();

}

/*-------------------
----- Setup
---------------------*/
void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, CHANGE);

  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  servo1.write(90);
  servo2.write(0);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
    for(;;);
  }
  center_eyes();
  draw_eyes();

  // Creez task-uri multitasking servo
  xTaskCreatePinnedToCore(servo1Task, "Servo1Task", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(servo2Task, "Servo2Task", 2048, NULL, 1, NULL, 1);

  currentState = STATE_SLEEPING;
  lastAwakeTimestamp = millis();
}

/*-------------------
----- Loop
---------------------*/
void loop() {
  static bool lastButtonState = LOW;
  bool currentButtonState = digitalRead(BUTTON_PIN);

  // Detectare apăsare (rising edge: LOW -> HIGH)
  if (lastButtonState == LOW && currentButtonState == HIGH) {
    switch (currentState) {
      case STATE_SLEEPING:
        currentState = STATE_WAKEUP;
        Serial.println("State: WAKEUP");
        lastAwakeTimestamp = millis();
        break;

      case STATE_BLINK:
        currentState = STATE_ANGRY;
        Serial.println("State: ANGRY");
        break;

      default:
        // Alte stări nu necesită acțiune directă la apăsare
        break;
    }
  }
  // Comportament în funcție de starea curentă
  switch (currentState) {
    case STATE_SLEEPING:
      sleeping_state();
      break;

    case STATE_WAKEUP:
      wakeup_state();
      currentState = STATE_BLINK;
      lastAwakeTimestamp = millis();
      break;

    case STATE_BLINK: {
  static unsigned long lastBlinkTime = 0;
  unsigned long now = millis();
  const unsigned long blinkInterval = 2000; // interval blink mai rar (2 sec)

  if (now - lastBlinkTime >= blinkInterval) {
    blink();
    lastBlinkTime = now;
  }

  if (now - lastAwakeTimestamp > 10000) {
    currentState = STATE_FALLING_ASLEEP;
    Serial.println("State: FALLING ASLEEP");
  }
  break;
}

    /*case STATE_AWAKE:
      awake_state();
      reset_eyes_position_and_size();
      currentState = STATE_BLINK;
      // După 20s fără acțiune → adormire
      
      break;
*/
    case STATE_ANGRY:
      angry_state();
      currentState = STATE_BLINK;
      lastAwakeTimestamp = millis();
      break;

    case STATE_FALLING_ASLEEP:
      
      fallingasleep_state();
      currentState = STATE_SLEEPING;
      break;
  }
lastButtonState = currentButtonState;
  vTaskDelay(10 / portTICK_PERIOD_MS); // Pauză mică pentru multitasking
}
