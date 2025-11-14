#define LINE_NOTREADY 1234
#include <Servo.h>

Servo motorL;
Servo motorR;
int MOTOR_PIN[2] = {8, 9};
int motor_spd[2];

int ULTRA_TRIG[4] = {22,23,24,25}; 
int ULTRA_ECHO[4] = {19,20,21,2};
int ULTRA_MAXDIST = 20;
int ULTRA_STEER_DURATION = 300;

float ultra_dist[4];
float ULTRA_ZONE  = 0.5;
float ultra_front;
int ultra_i = 0; 
boolean ultra_busy = 0;
long ultra_t[4];
long ultra_t2;
long ultra_t3;
long ultra_t4;
long ultra_t5;
boolean ultra_turn;
float ultra_steer;
boolean ultra_inZone;

int IR[5] = {A8,A9,A10,A11,A12};
int IR_ZONE = 100;
int ir_int[5];
boolean ir_bool[5];
float ir_val;
long ir_t;
long ir_t2;
boolean ir_inZone;
boolean candle_detected = false;

Servo servo;
int SERVO_PIN = 10;
bool servo_trig = false;
long servo_myt;
int servo_s = 0;

int LINE_PIN[3] = {5, 6, 7};
int LINE_ZONE = 1000;
float line_val;
boolean line_inZone;
long line_t = -10000;

class PID{
  public:
    double P;
    double I;
    double D;
    double integral;
    double prev_error;
    double prev_out;
    long prev_t;
    int SAMPLING_PERIOD;
  
    PID(double p, double i, double d, int period){
      P = p;
      I = i;
      D = d;
      SAMPLING_PERIOD = period;
    }
  
    void reset(){
      integral = 0;
      prev_error = 0;
    }
  
    double get(double error){
      double unit = 1.0*(millis()-prev_t)/SAMPLING_PERIOD;
      if(unit >= 1){
        double out = P*error;
        integral += error*unit;
        out += I*integral;
        out += D*(error-prev_error)/unit;
    
        prev_error = error;
        prev_out = out;
        prev_t = millis();
  
        return out;
      }
      return prev_out;
    }
};

PID linePID(30, 0.5, 5, 50);

void setup() {
  Serial.begin(9600);
  initCandle(1000);
  motorInit();
  initUltra();
  initServo();
  initLine();
  pinMode(13, OUTPUT);
}

void loop() {
  readUltra();
  readCandle();
  runLine();


  //Speed
  motor_spd[0] = 20;
  motor_spd[1] = 20;

  //Line_Follower
  if(line_inZone and !ultra_inZone and !candle_detected){
    int line_turn = linePID.get(line_val);
    motor_spd[0] += line_turn*(line_turn < 0);
    motor_spd[1] += -1*line_turn*(line_turn > 0);
  }
  else{
    linePID.reset();
  }
  
  //Object Avoidance
  motor_spd[0] += -70*ultra_steer*(!ir_inZone);
  motor_spd[1] += 70*ultra_steer*(!ir_inZone);

  //Candle Follower
  motor_spd[0] += 15*ir_val*candle_detected*(!ultra_inZone or ir_inZone);
  motor_spd[1] += -15*ir_val*candle_detected*(!ultra_inZone or ir_inZone);

  motor_spd[0] -= 30*ultra_front*ir_inZone;
  motor_spd[1] -= 30*ultra_front*ir_inZone;

  //Fan
//  digitalWrite(13, servo_trig);
//  if(!servo_trig){servo_trig = ir_inZone and  ultra_front > 0.5 and abs(ir_val) <= 1 ;}
//  if(servo_trig){
//      motor_spd[0] = -3;
//      motor_spd[1] = 1;
//  } 
  updateServo();
  motorRun(motor_spd[0], motor_spd[1]);
}

void motorInit(){
  motorL.attach(MOTOR_PIN[0]);
  motorR.attach(MOTOR_PIN[1]);
}

void motorRun(int l, int r){
  l = 90+l;
  r = 90-1.4*r;
  if(l < 0){l = 0;}
  else if(l > 180){l = 180;}
  if(r < 0){r = 0;}
  else if(r > 180){r = 180;}
  motorL.write(l);
  motorR.write(r);
}

void initUltra(){
  pinMode(ULTRA_TRIG[0], OUTPUT);
  pinMode(ULTRA_TRIG[1], OUTPUT);
  pinMode(ULTRA_TRIG[2], OUTPUT);
  pinMode(ULTRA_TRIG[3], OUTPUT);
  pinMode(ULTRA_ECHO[0], INPUT);
  pinMode(ULTRA_ECHO[1], INPUT);
  pinMode(ULTRA_ECHO[2], INPUT);
  pinMode(ULTRA_ECHO[3], INPUT);
  attachInterrupt(digitalPinToInterrupt(ULTRA_ECHO[0]), u0, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ULTRA_ECHO[1]), u1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ULTRA_ECHO[2]), u2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ULTRA_ECHO[3]), u3, CHANGE);
}

void readUltra(){
  if(!ultra_busy and (millis()-ultra_t2) >= 25){

    int j = 0;
    for(int i = 1; i < 4; i++){if(ultra_dist[j] < ultra_dist[i]){j = i;}}
    
    if(ultra_dist[j] > 0){
      if(millis()-ultra_t3 > 100){ultra_turn = j <= 1;}
      ultra_t3 = millis();
    }

    if(ultra_dist[j] != 0){
      ultra_steer = ultra_dist[j];
      if(ultra_turn){ultra_steer *= -1;}
      ultra_t5 = millis();
    }
    else if(millis()-ultra_t5 > ULTRA_STEER_DURATION){
      ultra_steer = 0;
    }

    if(abs(ultra_steer) > ULTRA_ZONE){
      ultra_inZone = true;
      ultra_t4 = millis();
    }
    else if(millis()-ultra_t4 > 1000){
      ultra_inZone = false;
    }

    if(ultra_dist[2] > ultra_dist[1]){ultra_front = ultra_dist[2];}
    else{ultra_front = ultra_dist[1];}
    
    digitalWrite(ULTRA_TRIG[ultra_i], HIGH);
    delayMicroseconds(10);
    digitalWrite(ULTRA_TRIG[ultra_i], LOW);
    
    ultra_t2 = millis();
    ultra_i = (++ultra_i)%4;
    ultra_busy = true;
  }
}

void u0(){
  if(digitalRead(ULTRA_ECHO[0])){ultra_t[0] = micros();}
  else{
    float dist =  (ULTRA_MAXDIST-(micros()-ultra_t[0])*0.034/2)/ULTRA_MAXDIST;
    if(dist < 0){dist = 0;}
    ultra_dist[0] = dist;
    ultra_busy = false;
  }
}

void u1(){
  if(digitalRead(ULTRA_ECHO[1])){ultra_t[1] = micros();}
  else{
    float dist =  (ULTRA_MAXDIST-(micros()-ultra_t[1])*0.034/2)/ULTRA_MAXDIST;
    if(dist < 0){dist = 0;}
    ultra_dist[1] = dist;
    ultra_busy = false;
  }
}

void u2(){
  if(digitalRead(ULTRA_ECHO[2])){ultra_t[2] = micros();}
  else{
    float dist =  (ULTRA_MAXDIST-(micros()-ultra_t[2])*0.034/2)/ULTRA_MAXDIST;
    if(dist < 0){dist = 0;}
    ultra_dist[2] = dist;
    ultra_busy = false;
  }
}

void u3(){
  if(digitalRead(ULTRA_ECHO[3])){ultra_t[3] = micros();}
  else{
    float dist =  (ULTRA_MAXDIST-(micros()-ultra_t[3])*0.034/2)/ULTRA_MAXDIST;
    if(dist < 0){dist = 0;}
    ultra_dist[3] = dist;
    ultra_busy = false;
  }
}

void initCandle(int dur){
  long t = millis();
  while(millis()-t < dur){
    readCandle();
  }
}
void readCandle(){
  int s = 0;
  int n = 0;
  int myMax = 0;
  int thresh;

  for(int i = 0; i < 5; i++){
    ir_int[i] = analogRead(IR[i]);
    if(ir_int[i] > myMax){
      myMax = ir_int[i];
    }
  }
  thresh = 0.9*myMax;
  
  for(int i = 0; i < 5; i++){
    if(myMax < 20 or ir_int[i] < thresh){ir_bool[i] = false;}
    else{
      ir_bool[i] = true;
      s += i-2;
      n++;
    }
  }
  
  if(n != 0){
    ir_val = 1.0*s/n;
    candle_detected = true;
    ir_t = millis();
    
  }
  else if(millis()-ir_t > 1000){
    candle_detected = false;
  }

  if(myMax > IR_ZONE){
    ir_inZone = true;
    ir_t2 = millis();
  }
  else if(millis()-ir_t2 > 1000){
    ir_inZone = false;
  }
  
}

void initServo(){
  servo.attach(SERVO_PIN); 
  servo.write(180);
}
void updateServo(){
  if(servo_s == 0 and servo_trig){
    servo.write(90);
    servo_myt = millis(); 
    servo_s = 1;
  }
  else if(servo_s == 1 and millis()-servo_myt > 500){
    servo.write(180);
    servo_myt = millis(); 
    servo_s = 2;
  }
  else if(servo_s == 2 and millis()-servo_myt >  500){
    servo_s = 0;
    servo_trig = false;
  }
}

void initLine(){
  for(int i = 0; i < 3;i++){
    pinMode(LINE_PIN[i], INPUT);
  }
}

void runLine(){
  int n = 0;
  int s = 0;
  for(int i = 0; i < 3; i++){
    if(!digitalRead(LINE_PIN[i])){
      s += i-1;
      n++;
    }
  }

  if(n != 0){
    line_val = 1.0*s/n;
    line_t = millis();
  }
  line_inZone = millis()-line_t < LINE_ZONE;
}
