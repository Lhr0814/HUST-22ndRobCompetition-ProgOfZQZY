/*
 *  This is the control file of R1 Robot.
 */
#include <Arduino.h>
#include <PS2X_lib.h>
#include <Emakefun_MotorDriver.h>
/******************** 以下是引脚定义 ********************/
#define PS2_DAT     12//37 // PS2引脚定义
#define PS2_CMD     11//39 // PS2引脚定义
#define PS2_SEL     10//41 // PS2引脚定义
#define PS2_CLK     13//43 // PS2引脚定义
#define RELAY_VCC   52 // 继电器引脚定义
#define ClampENA    44 // 机械臂电机ENA
#define ClampIN3    46 // 机械臂电机IN3
#define ClampIN4    48 // 机械臂电机IN4
/* 如若需要更改接线引脚，只需要在此处更改宏定义就足够了! */
/******************** 以上是引脚定义 ********************/

/******************** 以下是对象声明 ********************/
PS2X ps2x; // PS2对象定义
Emakefun_MotorDriver mMotorDriver = Emakefun_MotorDriver(0x60); // 电机对象定义
  Emakefun_DCMotor *RF = mMotorDriver.getMotor(M1); //1号 右前
  Emakefun_DCMotor *LF = mMotorDriver.getMotor(M2); //2号 左前
  Emakefun_DCMotor *RB = mMotorDriver.getMotor(M3); //3号 右后
  Emakefun_DCMotor *LB = mMotorDriver.getMotor(M4); //4号 左后
  Emakefun_Servo *ClampServo = mMotorDriver.getServo(8); // 机械臂舵机
/******************** 以上是对象声明 ********************/

/********************* 以下是初始化 *********************/
void initL298N() {
  pinMode(ClampENA, OUTPUT);
  pinMode(ClampIN3, OUTPUT);
  pinMode(ClampIN4, OUTPUT);
  Serial.println("L298N电机引脚初始化成功！");
  // pinMode(N20ENA, OUTPUT);
  // pinMode(N20IN1, OUTPUT);
  // pinMode(N20IN2, OUTPUT);
} // L298N驱动模块引脚初始化
void initRelay() {
  pinMode(RELAY_VCC, OUTPUT);
  digitalWrite(RELAY_VCC, LOW);
  Serial.println("继电器引脚初始化成功！");
} //继电器引脚初始化
void initServo() { // 舵机初始化
  ClampServo->writeServo(0);
  Serial.println("舵机初始化成功！");
} // 舵机初始化
void initPS2X() {
  delay(300);
  int error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, false, false);
  if(error == 0) {
    Serial.println("PS2手柄连接成功！");
  } else {
    Serial.print("手柄连接失败，错误代码: ");
    Serial.println(error);
  }
} // PS2手柄初始化
/********************* 以上是初始化 *********************/

/******************** 以下是控制函数 ********************/
void MotorRun(const int IN1, const int IN2, const int EN, const int state, const int speed = 0) {
  switch (state) {
    case FORWARD:
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      analogWrite(EN, speed);
      break;
    case BACKWARD:
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      analogWrite(EN, speed);
      break;
    case RELEASE:
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      break;
    case BRAKE:
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, HIGH);
      break;
  }
} // 自定义电机控制函数

// void N20Control() { // N20电机控制
//   if (ps2x.NewButtonState()) {
//     if(ps2x.Button(PSB_PAD_UP)) {//向上
//       Serial.println("N20开始向上...");
//       MotorRun(N20IN1, N20IN2, N20ENA, FORWARD, 255);
//       }
//     if(ps2x.Button(PSB_PAD_DOWN)) {//叉向下
//       Serial.println("N20开始向下...");
//       MotorRun(N20IN1, N20IN2, N20ENA, BACKWARD, 255);
//     }
//     if(ps2x.ButtonReleased(PSB_PAD_UP) || ps2x.ButtonReleased(PSB_PAD_DOWN)) {//归零
//       Serial.println("N20停！");
//       MotorRun(N20IN1, N20IN2, N20ENA, BRAKE);
//     }
//   }
// } // N20电机控制

void ClampControl() { // 机械臂电机控制
  if (ps2x.NewButtonState()) {
    if(ps2x.Button(PSB_TRIANGLE)) {//向上
      Serial.println("机械臂开始向上...");
      MotorRun(ClampIN3, ClampIN4, ClampENA, FORWARD, 255);
    }
    if(ps2x.Button(PSB_CROSS)) {//向下
      Serial.println("机械臂开始向下...");
      MotorRun(ClampIN3, ClampIN4, ClampENA, BACKWARD, 255);
    }
    if(ps2x.ButtonReleased(PSB_TRIANGLE) || ps2x.ButtonReleased(PSB_CROSS)) {//归零
      Serial.println("机械臂停!");
      MotorRun(ClampIN3, ClampIN4, ClampENA, BRAKE);
    }
  }
} // 机械臂电机控制

void ClampServoControl() { // 舵机控制
  static bool isClampServoOn = false;
  if (ps2x.NewButtonState() && ps2x.Button(PSB_SQUARE)) {
    isClampServoOn = !isClampServoOn;
    if (isClampServoOn) {
      Serial.println("舵机夹爪开!");
      ClampServo->writeServo(90);
    } else {
      Serial.println("舵机夹爪合!");
      ClampServo->writeServo(0);
    }
  }
} // 舵机控制

// void FrontServoControl() { // 舵机控制
//   static unsigned long FrontServoState;
//   if (ps2x.NewButtonState() && ps2x.Button(PSB_PAD_RIGHT)) {
//     FrontServoState++;
//   }
//   if (FrontServoState % 2 == 1) {
//     FrontServo->writeServo(120);
//   } else {
//     FrontServo->writeServo(0);
//   }
// } // 舵机控制

void RelayControl() { // 继电器控制
  if (ps2x.NewButtonState()) {
    if(ps2x.Button(PSB_R2)) {
      tone(A0, 440, 200);
      delay(100);
      Serial.println("气缸推出！");
      digitalWrite(RELAY_VCC, HIGH);
      delay(500);
      Serial.println("气缸收回！");
      digitalWrite(RELAY_VCC, LOW);
    }
  }
} // 继电器控制

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
  lf_speed = y - x + r;  // 左前轮
  rf_speed = y + x - r;  // 右前轮
  lb_speed = y + x + r;  // 左后轮
  rb_speed = y - x - r;  // 右后轮
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
  setChassisMotor(RF, -rf_speed);
  setChassisMotor(LB, lb_speed);
  setChassisMotor(RB, -rb_speed);
}
void ChassisControl() {
  if(ps2x.read_gamepad(false, 0)) { // 如果成功读取手柄数据
    // 读取摇杆值
    int leftX = ps2x.Analog(PSS_LX) - 128;  // -128 到 127
    int leftY = ps2x.Analog(PSS_LY) - 128;
    int rightX = - ps2x.Analog(PSS_RX) + 128;
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

/******************* 以下是非阻塞定义 *******************/
// region // 全局定时变量设置
unsigned long lastChassisTime = 0;
unsigned long lastN20Time = 0;
unsigned long lastClampTime = 0;
unsigned long lastClampServoTime = 0;
unsigned long lastFrontServoTime = 0;
unsigned long lastRelayTime = 0; // endregion
// region // 循环间隔设置
constexpr uint8_t CHASSIS_INTERVAL = 10;        // 10ms - 底盘控制
constexpr uint8_t N20_INTERVAL = 10;            // 10ms - N20电机
constexpr uint8_t CLAMP_INTERVAL = 10;          // 10ms - 机械臂
constexpr uint8_t CLAMP_SERVO_INTERVAL = 10;    // 10ms - 夹爪舵机
constexpr uint8_t RELAY_INTERVAL = 10;          // 10ms - 继电器
// endregion
/******************* 以上是非阻塞定义 *******************/

/******************** 以下是启动函数 ********************/
void setup() {
  Serial.begin(115200);
  mMotorDriver.begin(50);

  initL298N();
  initRelay();
  initServo();
  initPS2X(); // 请置于setup最后
  Serial.println("\n=== === === ===");
  Serial.println("所有部分均已初始化完成!");
  Serial.println("=== === === ===\n");
  tone(A0, 262, 300);
}
/******************** 以上是启动函数 ********************/

/******************** 以下是循环函数 ********************/
void loop() {
  uint8_t currentTime = millis();
  if (currentTime - lastChassisTime >= CHASSIS_INTERVAL) {
    ChassisControl();
    lastChassisTime = currentTime;
  } // 底盘控制
  if (currentTime - lastClampTime >= CLAMP_INTERVAL) {
    ClampControl();
    lastClampTime = currentTime;
  } // 机械臂电机控制
  if (currentTime - lastClampServoTime >= CLAMP_SERVO_INTERVAL) {
    ClampServoControl();
    lastClampServoTime = currentTime;
  } // 机械臂舵机控制
  if (currentTime - lastRelayTime >= RELAY_INTERVAL) {
    RelayControl();
    lastRelayTime = currentTime;
  } // 继电器控制
}
/******************** 以上是循环函数 ********************/

/******************** 以下是区域模板 ********************/
/******************** 以上是区域模板 ********************/
