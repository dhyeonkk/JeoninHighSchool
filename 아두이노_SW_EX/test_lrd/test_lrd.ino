#include <ArduinoJson.h> //ArduinoJson 라이브러리
#include <TimerOne.h> // TimerOne 라이브러리
#include "QGPMaker_MotorShield.h"
#include "QGPMaker_Encoder.h"
#include <Wire.h>
#define slave_addr 0x01

unsigned long preTime,currTime;//기준 시간 저장 함수


int rpm = 0;
int rpm1 = 0;
int rpm2 = 0;
int rpm3 = 0;
int rpm4 = 0;

char MsgBuf[100]; //Master로 부터 전송받은 데이터를 저장할 버퍼
volatile byte pos;
volatile boolean Check_Data = false;

// Create the motor shield object with the default I2C address
QGPMaker_MotorShield AFMS = QGPMaker_MotorShield();

// Select which 'port' M1, M2, M3 or M4. In this case, M3
QGPMaker_DCMotor *DCMotor_1 = AFMS.getMotor(1);
QGPMaker_DCMotor *DCMotor_2 = AFMS.getMotor(2);
QGPMaker_DCMotor *DCMotor_3 = AFMS.getMotor(3);
QGPMaker_DCMotor *DCMotor_4 = AFMS.getMotor(4);

QGPMaker_Encoder Encoder1(1); 
QGPMaker_Encoder Encoder2(2); 
QGPMaker_Encoder Encoder3(3); 
QGPMaker_Encoder Encoder4(4); 


// speed : 0 ~ 255
int speed = 50;

char cmd;
char cmd_arr[16];
int inx = 0;
bool success = false;

uint16_t space,strength;

#define LED 13
#define SAND_TIME 100
int32_t sandTime = 0;
bool IssandeTime = false;

#define TD_TIME 1000
int32_t TDTime = 0;
bool IsTDTime = false;

double distance1 = 0.00; // 1번 초음파 값
double distance2 = 0.00; // 2번 초음파 값
double Lidar = 0.00; // 라이다 값
float heading = 0.0f; //float 값에는 f가 붙음, 9축 지자기 센서 값
float tiltheading = 0.0f;
void mainTimer(void);

float td = 0.0f; // 이동거리
float td1 = 0.0f; // 이동거리
float td2 = 0.0f; // 이동거리
float td3 = 0.0f; // 이동거리
float td4 = 0.0f; // 이동거리
float averageTD = 0.0f; // 이동거리


void mainTimer(void){
  if(IssandeTime == false)
  {
    sandTime++;
    if(sandTime >= SAND_TIME) IssandeTime = true;
  }
  if(IsTDTime == false)
  {
    TDTime++;
    if(TDTime >= TD_TIME) IsTDTime = true;
  }
}
void setup() {
  Wire.begin(slave_addr); //slave_addr의 주소값을 갖는 slave로 동작
  Wire.onReceive(Receive_Int); //Master에서 보낸 데이터를 수신했을때 호출할 함수를 등록
  Serial.begin(9600);  //아두이노와 통신할 소프트웨어 시리얼 시작
  Timer1.initialize(1000); // 1ms마다 인터럽트 발생
  Timer1.attachInterrupt(mainTimer); // 인터럽트 함수 지정
  AFMS.begin(50); // create with the default frequency 50Hz
  AFMS.begin(50);
  Serial.begin(9600);
}
//write() 함수가 바이트 또는 바이트의 시퀀스를 그대로 보내는 반면, print() 함수는 데이터를 사람이 읽을 수 있는 ASCII 텍스트로 변환하여 보낸다
void loop() {
  if(IssandeTime == true){
    ReadData();
    sandTime = 0;
    IssandeTime = false;
  }
  if (IsTDTime == true){
    currTime += IsTDTime;
    TD(currTime);
    IsTDTime = 0;
    IsTDTime = false;
  }
  while (Serial.available()) {
    cmd = Serial.read();
    if (cmd == '\n') {
      success = true;
      break;
    }
    cmd_arr[inx++] = cmd;
  }
  cmd_arr[inx] = '\0'; // 문자열의 끝을 표시

  if (success) {
    // Serial.println(cmd_arr);
    if (strcmp(cmd_arr, "forward") == 0) {
      // forward(speed);
      backward(speed);
    }
    else if (strcmp(cmd_arr, "turn_left") == 0) {
      turn_left(speed);
      // delay(2300);
      // forward(speed);
      totalTD = 0;
      averageTD = 0;
    }
    else if (strcmp(cmd_arr, "turn_right") == 0) {
      turn_right(speed);
      // delay(2300);
      // forward(speed);
      totalTD = 0;
      averageTD = 0;
    }
    else if (strcmp(cmd_arr, "go_left") == 0) {
      move_left(speed);
    }
    else if (strcmp(cmd_arr, "go_right") == 0) {
      move_right(speed);
    }
    else if (strcmp(cmd_arr, "backward") == 0) {
      forward(speed);
    }
    else if (strcmp(cmd_arr, "stop") == 0) {
      stop(0);
    }
    else {
      stop(0);
    }
    success = false;
    inx = 0;
    memset(cmd_arr,NULL, sizeof(cmd_arr));
  }
}
void Receive_Int() { //Master에서 보낸 데이터가 수신되면 호출되는 함수
  byte m;
  
  while(Wire.available()){ //읽어올 데이터가 있다면
    m = Wire.read(); 
    if(pos < sizeof(MsgBuf)){
      MsgBuf[pos++] = m; //데이터를 버퍼에 저장합니다.
    }
    if(m =='\n'){ // 개행 문자 인식 -> 데이터 정상 수신 확인.
      Check_Data = true; 
    }
  } 
}
void ReadData(){
  delay(1);
  
  if(Check_Data == true){
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, MsgBuf);
    // 파싱 성공한 경우에만 직렬화하고 출력합니다.
    if (!error) {
      
      // JSON 객체를 직렬화하여 문자열로 변환합니다.
      String jsonString;
      serializeJson(doc, jsonString);

      // 추가된 부분: `TD()` 함수에서 계산한 값 추가
      doc["TD"] = td;               // 이동 거리 값 추가
      doc["averageTD"] = averageTD;  // 평균 이동 거리 값 추가
      doc["rpm1"] = rpm1;            // 각 엔코더의 RPM 값 추가
      doc["rpm2"] = rpm2;            // 각 엔코더의 RPM 값 추가
      doc["rpm3"] = rpm3;            // 각 엔코더의 RPM 값 추가
      doc["rpm4"] = rpm4;            // 각 엔코더의 RPM 값 추가

      // JSON 데이터를 문자열로 변환하여 출력
      serializeJson(doc, jsonString);
      Serial.println(jsonString); // 시리얼 포트를 통해 JSON 문자열 출력
    }
    
    // 데이터 초기화
    MsgBuf[pos] = 0;
    pos = 0;
    Check_Data = false;
  }
}

//RC카 제어
void forward(int speed) { 
  DCMotor_1->setSpeed(speed);
  DCMotor_1->run(FORWARD);
  DCMotor_2->setSpeed(speed);
  DCMotor_2->run(FORWARD);
  DCMotor_3->setSpeed(speed);
  DCMotor_3->run(FORWARD);
  DCMotor_4->setSpeed(speed);
  DCMotor_4->run(FORWARD);
}
void backward(int speed) { 
  DCMotor_1->setSpeed(speed);
  DCMotor_1->run(BACKWARD);
  DCMotor_2->setSpeed(speed);
  DCMotor_2->run(BACKWARD);
  DCMotor_3->setSpeed(speed);
  DCMotor_3->run(BACKWARD);
  DCMotor_4->setSpeed(speed);
  DCMotor_4->run(BACKWARD);
}
void move_left(int speed) { 
  DCMotor_1->setSpeed(speed);
  DCMotor_1->run(BACKWARD);
  DCMotor_2->setSpeed(speed);
  DCMotor_2->run(FORWARD);
  DCMotor_3->setSpeed(speed);
  DCMotor_3->run(BACKWARD);
  DCMotor_4->setSpeed(speed);
  DCMotor_4->run(FORWARD);
}
void move_right(int speed) { 
  DCMotor_1->setSpeed(speed);
  DCMotor_1->run(FORWARD);
  DCMotor_2->setSpeed(speed);
  DCMotor_2->run(BACKWARD);
  DCMotor_3->setSpeed(speed);
  DCMotor_3->run(FORWARD);
  DCMotor_4->setSpeed(speed);
  DCMotor_4->run(BACKWARD);
}
void turn_left(int speed) { 
  DCMotor_1->setSpeed(speed);
  DCMotor_1->run(BACKWARD);
  DCMotor_2->setSpeed(speed);
  DCMotor_2->run(BACKWARD);
  DCMotor_3->setSpeed(speed);
  DCMotor_3->run(FORWARD);
  DCMotor_4->setSpeed(speed);
  DCMotor_4->run(FORWARD);
}
void turn_right(int speed) { 
  DCMotor_1->setSpeed(speed);
  DCMotor_1->run(FORWARD);
  DCMotor_2->setSpeed(speed);
  DCMotor_2->run(FORWARD);
  DCMotor_3->setSpeed(speed);
  DCMotor_3->run(BACKWARD);
  DCMotor_4->setSpeed(speed);
  DCMotor_4->run(BACKWARD);
}
void stop(int delay_time) {
  DCMotor_1->setSpeed(0);
  DCMotor_1->run(RELEASE);
  DCMotor_2->setSpeed(0);
  DCMotor_2->run(RELEASE);
  DCMotor_3->setSpeed(0);
  DCMotor_3->run(RELEASE);
  DCMotor_4->setSpeed(0);
  DCMotor_4->run(RELEASE);
  delay(delay_time);
}
void TD(unsigned long time) {
  float totalTD = 0.0;  // 최종 TD 값을 저장할 변수

  // 각 엔코더의 RPM 값을 가져옵니다.
  rpm1 = Encoder1.getRPM();
  rpm2 = Encoder2.getRPM();
  rpm3 = Encoder3.getRPM();
  rpm4 = Encoder4.getRPM();


  // 모든 RPM 값이 양수일 때만 이동 거리 계산
  if (rpm1 < 0 && rpm2 < 0 && rpm3 > 0 && rpm4 > 0) {
    // 각 엔코더에서 이동 거리를 계산하여 totalTD에 더함
    td1 = (rpm1 / 60.0) * 0.25 * time * -1;
    totalTD += td1;

    td2 = (rpm2 / 60.0) * 0.25 * time * -1;
    totalTD += td2;

    td3 = (rpm3 / 60.0) * 0.25 * time;
    totalTD += td3;

    td4 = (rpm4 / 60.0) * 0.25 * time;
    totalTD += td4;

    // 평균 이동 거리 계산 및 출력
    averageTD = totalTD / 4;
  }
  /*else if ((rpm1 > 0 && rpm2 > 0 && rpm3 > 0 && rpm4 > 0) or (rpm1 < 0 && rpm2 < 0 && rpm3 < 0 && rpm4 < 0)) {
    averageTD = 0;
    totalTD = 0;
  }*/
}
