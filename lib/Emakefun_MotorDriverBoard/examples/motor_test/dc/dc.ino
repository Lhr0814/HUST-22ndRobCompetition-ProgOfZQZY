#include "Emakefun_MotorDriver.h"

Emakefun_MotorDriver mMotor = Emakefun_MotorDriver(0x60);
Emakefun_DCMotor *DCMotor_1 = mMotor.getMotor(M1);
Emakefun_DCMotor *RF = mMotor.getMotor(M2);
Emakefun_DCMotor *LF = mMotor.getMotor(M3);
Emakefun_DCMotor *DCMotor_4 = mMotor.getMotor(M4);

void setup()
{
    Serial.begin(9600);
    mMotor.begin(50);
}

void loop()
{
  // 前进
  DCMotor_1->setSpeed(200);
  DCMotor_1->run(FORWARD);
  RF->setSpeed(200);
  RF->run(FORWARD);
  LF->setSpeed(200);
  LF->run(FORWARD);
  DCMotor_4->setSpeed(200);
  DCMotor_4->run(FORWARD);
  delay(4000);
  // 后退
  DCMotor_1->setSpeed(100);
  DCMotor_1->run(BACKWARD);
  RF->setSpeed(100);
  RF->run(BACKWARD);
  LF->setSpeed(100);
  LF->run(BACKWARD);
  DCMotor_4->setSpeed(100);
  DCMotor_4->run(BACKWARD);
  delay(4000);
  // 刹车
  DCMotor_1->run(BRAKE);
  RF->run(BRAKE);
  LF->run(BRAKE);
  DCMotor_4->run(BRAKE);
  delay(4000);
}
