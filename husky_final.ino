#include "HUSKYLENS.h"
#include "SoftwareSerial.h"
#include <Servo.h>
#include <Stepper.h>

#define TransX 2.285714286 // 1도당 HUSKYLENS의 가로 해상도는 2.285714286
#define TransY 2.4 // 1도당 HUSKYLENS의 세로 해상도는 2.4
int DegreeY = 90;
#define MaxDegreeY 160 //서보모터의 최대 각도 (위) 
#define MinDegreeY 60 //서보모터의 최소 각도 (아래)
Servo MotorUpDown;  // 상하 움직임 제어를 위한 서보 모터 객체 선언
//※※※※※※※※※※※※※※Servo MotorLeftRight; // 좌우 움직임 제어를 위한 서보 모터
/*
  모터 드라이브(ULN2003)
  2048:한바퀴(360도), 1024:반바퀴(180도)...
  const int stepsPerRevolution = 2048;
  약 5.68을 dgree에 곱해야 함.
*/
#define IN1 2
#define IN2 3
#define IN3 4
#define IN4 5
Stepper myStepper(2048, IN4 , IN2, IN3 , IN1); //스텝 모터 
#define step_speed 2
#define convert_dgree 1
int MotorDegreeX = 90, MotorDegreeY = 90;   // 서보 모터 (X는 가로, Y는 세로) 초기 위치
int RotateDegreeX, RotateDegreeY; // 모터가 회전할 각도, 초기 값 설정 불필요
int Degree_AMX = 90, Degree_AMY = 90; // 이미 회전한 각도, 두 서보 모터가 제한을 넘는지 확인하기 위함 (모터 초기 위치는 90도)

HUSKYLENS huskylens; //HUSKYLENS 객체 선언
// HUSKYLENS 연결: 녹선 >> SDA; 파란선 >> SCL

unsigned long previousMillis = 0;
const long interval = 1000;

void printResult(HUSKYLENSResult result);
void MotorMove(HUSKYLENSResult result, int MotorDegreeX, int MotorDegreeY); // 서보 모터 회전 함수

void setup() { // 설정
  MotorUpDown.attach(6);    // Arduino nano D8에 연결
  //※※※※※※※※※※※※※※MotorLeftRight.attach(3);  // Arduino nano D9에 연결
  myStepper.setSpeed(step_speed);

  MotorUpDown.write(DegreeY);
  Serial.begin(115200); // 시리얼 모니터 설정
  Wire.begin(); //HUSKYLENS 통신용
  while (!huskylens.begin(Wire)) //HUSKYLENS 설정 절차
  {
    Serial.println(F("Begin failed!"));
    Serial.println(F("1. HUSKYLENS의 \"프로토콜 유형\"을 확인하세요 (일반 설정 >> 프로토콜 유형 >> I2C)"));
    Serial.println(F("2. 연결 상태를 확인하세요."));
    delay(100);
  }
}

void loop() { // 주 프로그램
  unsigned long currentMillis = millis(); //현재시간 
  if (currentMillis - previousMillis >= interval) { //현재시간 - 이전시간 이 1000보다 크거나 같은 경우
    previousMillis = currentMillis;  // 이전 시간을 업데이트

    if (!huskylens.request()) Serial.println(F("HUSKYLENS에서 데이터를 요청하는 데 실패했습니다. 연결을 확인하세요!"));
    else if (!huskylens.isLearned()) Serial.println(F("학습된 내용이 없습니다. HUSKYLENS의 학습 버튼을 눌러 하나 학습하세요!"));
    else if (!huskylens.available()) Serial.println(F("화면에 블록이나 화살표가 나타나지 않았습니다!"));
    //isLearned(): HUSKYLENS가 무엇인가 배웠는지 여부 반환
    //available(): 읽을 수 있는 블록과 화살표의 개수를 반환
    else
    {
      Serial.println(F("###########"));
      while (huskylens.available()) // 메시지를 수신한 경우
      {
        HUSKYLENSResult result = huskylens.read(); //read(): 블록이나 화살표를 읽어 반환
        printResult(result);
        MotorMove(result, MotorDegreeX, MotorDegreeY); // 정보와 모터 위치를 전달하여 모터 회전
        MotorMove(result, MotorDegreeX, MotorDegreeY);
      }
    }
  }
}

//HUSKYLENS의 결과값을 시리얼에 출력하는 함수
void printResult(HUSKYLENSResult result) { 
  if (result.command == COMMAND_RETURN_BLOCK) { //HUSKYLENS의 블록값
    Serial.println(String() + F("블록: 가로 중심=") + result.xCenter + F(",세로 중심=") + result.yCenter + F(",너비=") + result.width + F(",높이=") + result.height + F(",ID=") + result.ID);
  }
  else if (result.command == COMMAND_RETURN_ARROW) { //HUSKYLENS의 화살표
    Serial.println(String() + F("화살표: 원점 x=") + result.xOrigin + F(",원점 y=") + result.yOrigin + F(",목표 x=") + result.xTarget + F(",목표 y=") + result.yTarget + F(",ID=") + result.ID);
  }
  else { //결과값이 없는 경우 
    Serial.println("알 수 없는 개체!");
  }
}

void MotorMove(HUSKYLENSResult result, int MotorDegreeX, int MotorDegreeY) {
  // 값을 추출하고 변수에 저장
  int xNow = result.xCenter; // 가로 (블록의 중심 X축)
  int yNow = result.yCenter; // 세로 (블록의 중심 Y축)

  Serial.print(F("xNow: "));
  Serial.println(xNow);
  Serial.print(F("yNow: "));
  Serial.println(yNow);

  if (0 < xNow && xNow < 130) { // xNow가 가로 해상도 범위인 0~160에 속하는 경우 
    RotateDegreeX = MotorDegreeX - round(float((160 - xNow)) / TransX); // 회전할 각도 계산 (x축)

    if (Degree_AMX - round(float((160 - xNow)) / TransX) < 20) { // 이미 회전한 각도에서 더해야 하는 각도가 20도 미만인 경우
      RotateDegreeX = 20; // 계산 결과가 제한을 넘지 않도록 최소값 20 설정
      //MotorLeftRight.write(RotateDegreeX);
      myStepper.step((RotateDegreeX - 90)*convert_dgree);
      Degree_AMX = 20; // 이미 회전한 각도 최솟값은 20
    }
    else {
      //MotorLeftRight.write(RotateDegreeX); // 제한 각도를 넘지 않는 경우 정상적으로 모터 회전
      myStepper.step((RotateDegreeX - 90)*convert_dgree);
      Degree_AMX = Degree_AMX - RotateDegreeX; // 이미 회전한 각도 업데이트
    }
    MotorDegreeX = RotateDegreeX; // 모터에 대한 새로운 값으로 업데이트

  }
  else if (190 < xNow && xNow < 320) {  
    RotateDegreeX = MotorDegreeX + round(float((xNow - 160)) / TransX); // 90도 이상
    if (Degree_AMX + round(float((xNow - 160)) / TransX) > 160) {
      RotateDegreeX = 160;
      //MotorLeftRight.write(RotateDegreeX);
      myStepper.step((RotateDegreeX - 90)*convert_dgree);
      Degree_AMX = 160;
    }
    else {
      //MotorLeftRight.write(RotateDegreeX);
      myStepper.step((RotateDegreeX - 90)*convert_dgree);
      Degree_AMX = Degree_AMX + RotateDegreeX;
    }
    MotorDegreeX = RotateDegreeX;

  }

  if (0 < yNow && yNow < 110) { // 좌상단
    DegreeY -= 2;
    if (DegreeY < MinDegreeY)
      DegreeY = MinDegreeY;
    MotorUpDown.write(DegreeY);
  }
  else if (130 < yNow && yNow < 240) {
    DegreeY += 2;
    if (DegreeY > MaxDegreeY)
      DegreeY = MaxDegreeY;
    MotorUpDown.write(DegreeY);
  }
}