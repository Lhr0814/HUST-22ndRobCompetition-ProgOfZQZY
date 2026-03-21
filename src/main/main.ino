#include <PS2X_lib.h>
#include <Servo.h>
#include <Emakefun_MotorDriver.h>

/******************** 以下是定义引脚 ********************/
#define PS2_DAT     12 // PS2引脚定义
#define PS2_CMD     11 // PS2引脚定义
#define PS2_SEL     10 // PS2引脚定义
#define PS2_CLK     13 // PS2引脚定义
#define RELAY_VCC   53 // 继电器引脚定义
/* 如若需要更改接线引脚，只需要在此处更改宏定义就足够了! */
/******************** 以上是定义引脚 ********************/

/******************** 以下是对象声明 ********************/
PS2X ps2x; // PS2对象定义
Emakefun_MotorDriver mMotorDriver = Emakefun_MotorDriver(0x60); // 电机对象定义
  Emakefun_DCMotor *RB = mMotorDriver.getMotor(M1); //1号 右后
  Emakefun_DCMotor *RF = mMotorDriver.getMotor(M2); //2号 右前
  Emakefun_DCMotor *LF = mMotorDriver.getMotor(M3); //3号 左前
  Emakefun_DCMotor *LB = mMotorDriver.getMotor(M4); //4号 左后
  // Emakefun_EncoderMotor *N20Motor = mMotorDriver.getEncoderMotor(E2); //2号 N20
  // Emakefun_EncoderMotor *ClampMotor = mMotorDriver.getEncoderMotor(E3); //3号 机械臂
  Emakefun_Servo *Servo1 = mMotorDriver.getServo(1); // 1号 舵机
/******************** 以上是对象声明 ********************/

/******************** 以下是控制函数 ********************/
void RelayControl() { // 继电器控制
  if (ps2x.NewButtonState()) {
    if(ps2x.Button(PSB_TRIANGLE)) {
      digitalWrite(RELAY_VCC, HIGH);
      delay(500);
      digitalWrite(RELAY_VCC, LOW);
    }
  }
} // 继电器控制

// void N20Control() { // N20电机控制
//   if (ps2x.NewButtonState()) {
//     if(ps2x.Button(PSB_TRIANGLE)) {//向上
//       Serial.println("N20开始向上...");
//       N20Motor->run(FORWARD, 255, 0);
//       }
//     if(ps2x.Button(PSB_CROSS)) {//叉向下
//       Serial.println("N20开始向下...");
//       N20Motor->run(BACKWARD, 255, 0);
//     }
//     if(ps2x.ButtonReleased(PSB_TRIANGLE)) {//归零
//       Serial.println("N20停！");
//       N20Motor->run(RELEASE);
//     }
//     if(ps2x.ButtonReleased(PSB_CROSS)) {//归零
//       Serial.println("N20停！");
//       N20Motor->run(RELEASE);
//     }
//   }
// } // N20电机控制
//
// void ClampControl() { // 机械臂电机控制
//   if (ps2x.NewButtonState()) {
//     if(ps2x.Button(PSB_CIRCLE)) {//向上
//       Serial.println("机械臂向上...");
//       ClampMotor->run(FORWARD, 255, 0);
//     }
//     if(ps2x.Button(PSB_SQUARE)) {//向下
//       Serial.println("机械臂向下...");
//       ClampMotor->run(BACKWARD, 255, 0);
//     }
//     if(ps2x.ButtonReleased(PSB_CIRCLE)) {//归零
//       Serial.println("机械爪上停！");
//       ClampMotor->run(RELEASE);
//     }
//     if(ps2x.ButtonReleased(PSB_SQUARE)) {//归零
//       Serial.println("机械爪下停！");
//       ClampMotor->run(RELEASE);
//     }
//   }
// } // 机械臂电机控制

//region void ChassisControl() { ...底盘控制部分代码... }
int lf_speed = 0, rf_speed = 0, lb_speed = 0, rb_speed = 0; // 电机速度变量
// 运动参数
int maxSpeed = 255;      // 最大速度 (0-255)
int deadZone = 20;       // 摇杆死区
int currentSpeed = 0;    // 当前速度
void controlChassisSpeed() { //速度控制
  if(ps2x.Button(PSB_R1)) { // R1加速
    maxSpeed = min(255, maxSpeed + 5);
    delay(100);
  }
  if(ps2x.Button(PSB_L1)) { // L1减速
    maxSpeed = max(80, maxSpeed - 5);
    delay(100);
  }
}
void calculateChassis(int x, int y, int r) { // 计算麦克纳姆轮运动
  // 麦轮运动学公式
  lf_speed = y + x + r;  // 左前轮
  rf_speed = y - x - r;  // 右前轮
  lb_speed = y - x + r;  // 左后轮
  rb_speed = y + x - r;  // 右后轮
  // 限制最大速度
  int maxCalculated = max(max(abs(lf_speed), abs(rf_speed)), max(abs(lb_speed), abs(rb_speed)));
  if(maxCalculated > maxSpeed) {
    float scale = static_cast<float>(maxSpeed) / maxCalculated;
    lf_speed *= scale;
    rf_speed *= scale;
    lb_speed *= scale;
    rb_speed *= scale;
  }
}
void setChassisMotor(Emakefun_DCMotor *DCMotor, int speed) { // 控制单个电机
  // 设置方向
  if(speed > 0) {
    DCMotor->run(FORWARD);
  } else if(speed < 0) {
    DCMotor->run(BACKWARD);
  } else {
    DCMotor->run(BRAKE);
  }
  // 设置PWM速度（取绝对值）
  DCMotor->setSpeed(abs(speed));
}
// 设置麦轮电机速度和方向
void setChassisMotorSpeeds() {
  setChassisMotor(LF, lf_speed);
  setChassisMotor(RF, rf_speed);
  setChassisMotor(LB, lb_speed);
  setChassisMotor(RB, rb_speed);
}
void ChassisControl() {
  if(ps2x.read_gamepad(false, 0)) { // 如果成功读取手柄数据
    // 读取摇杆值
    int leftX = ps2x.Analog(PSS_LX) - 128;  // -128 到 127
    int leftY = ps2x.Analog(PSS_LY) - 128;
    int rightX = ps2x.Analog(PSS_RX) - 128;
    // 应用死区
    if(abs(leftX) < deadZone) leftX = 0;
    if(abs(leftY) < deadZone) leftY = 0;
    if(abs(rightX) < deadZone) rightX = 0; //不变

    controlChassisSpeed(); // 速度控制
    calculateChassis(leftX, leftY, rightX); // 计算麦轮运动
    setChassisMotorSpeeds(); // 应用电机控制
    // // 紧急停止
    // if(ps2x.Button(PSB_BLUE)) { // ×按钮
    //   emergencyChassisStop();
    // }
    // 调试输出（可选）
    // debugChassisOutput(leftX, leftY, rightX);
  }
}
// 停止所有麦轮电机
void stopChassisMotors() {
    RB->run(BRAKE);
    RF->run(BRAKE);
    LF->run(BRAKE);
    LB->run(BRAKE);
}
// 电机紧急停止
void emergencyChassisStop() {
    stopChassisMotors();
    Serial.println("紧急停止！");
    delay(1000); // 停止1秒
}
//endregion
/******************** 以上是控制函数 ********************/

void debugMotorStates() {
  static unsigned long lastDebugTime = 0;
  if (millis() - lastDebugTime > 2000) { // 每2秒输出一次
    Serial.println("=== Motor States ===");
    Serial.println("Chassis Motors - LF, RF, LB, RB");
    Serial.println("Encoder Motors - N20, Clamp");
    Serial.print("PS2 Buttons - TRI: ");
    Serial.print(ps2x.Button(PSB_TRIANGLE));
    Serial.print(" CRO: ");
    Serial.print(ps2x.Button(PSB_CROSS));
    Serial.print(" CIR: ");
    Serial.print(ps2x.Button(PSB_CIRCLE));
    Serial.print(" SQU: ");
    Serial.println(ps2x.Button(PSB_SQUARE));
    Serial.println("====================");
    lastDebugTime = millis();
  }
}

/******************** 以下是启动函数 ********************/
void setup() {
  Serial.begin(115200);
  mMotorDriver.begin(50);

  //继电器引脚设置
  pinMode(RELAY_VCC, OUTPUT);
  digitalWrite(RELAY_VCC, LOW);

  // 初始化PS2手柄
  delay(300);
  int error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, false, false);
  if(error == 0) {
    Serial.println("PS2手柄连接成功！");
  } else {
    Serial.print("手柄连接失败，错误代码: ");
    Serial.println(error);
  }
}
/******************** 以上是启动函数 ********************/

/******************** 以下是循环函数 ********************/
void loop() {
  ChassisControl();
  // N20Control();
  // ClampControl();
  RelayControl();
  // debugMotorStates();
  delay(20);
}
/******************** 以上是循环函数 ********************/