#include <Arduino.h>
#include <PS2X_lib.h>
#include <Emakefun_MotorDriver.h>
/******************** 以下是引脚定义 ********************/
#define PS2_DAT         12 // PS2引脚定义
#define PS2_CMD         11 // PS2引脚定义
#define PS2_SEL         10 // PS2引脚定义
#define PS2_CLK         13 // PS2引脚定义
#define ClampIN3        28 // 机械臂电机 IN3 引脚 28
#define ClampIN4        30 // 机械臂电机 IN4 引脚 30
#define T8_IN1          24 // 丝杆电机 IN1 引脚 24
#define T8_IN2          26 // 丝杆电机 IN2 引脚 26
#define FRICTION1_IN1   38 // 摩擦轮1 IN1 引脚
#define FRICTION1_IN2   40 // 摩擦轮1 IN2 引脚
#define FRICTION2_IN3   42 // 摩擦轮2 IN3 引脚
#define FRICTION2_IN4   44 // 摩擦轮2 IN4 引脚
// #define LF_IN3          48 // 左前轮 IN1 引脚
// #define LF_IN4          46 // 左前轮 IN2 引脚
// #define LF_ENB          44 // 左前轮 ENB 引脚
/* 如若需要更改接线引脚，只需要在此处更改宏定义就足够了! */
/******************** 以上是引脚定义 ********************/

/******************** 以下是对象声明 ********************/
PS2X ps2x; // PS2对象定义
Emakefun_MotorDriver mMotorDriver = Emakefun_MotorDriver(0x60); // 电机对象定义
  Emakefun_DCMotor *LB = mMotorDriver.getMotor(M1); //1号 左后
  Emakefun_DCMotor *RB = mMotorDriver.getMotor(M2); //2号 右后
  Emakefun_DCMotor *LF = mMotorDriver.getMotor(M3); //3号 左前
  Emakefun_DCMotor *RF = mMotorDriver.getMotor(M4); //4号 右前
  Emakefun_Servo *Servo = mMotorDriver.getServo(8); // 8号 舵机
/******************** 以上是对象声明 ********************/

/********************* 以下是初始化 *********************/
void initL298N() { //L298N电机初始化
  pinMode(ClampIN3, OUTPUT);
  pinMode(ClampIN4, OUTPUT);      // 机械臂引脚
  pinMode(T8_IN1, OUTPUT);
  pinMode(T8_IN2, OUTPUT);        // 丝杆电机引脚

  pinMode(FRICTION1_IN1, OUTPUT);
  pinMode(FRICTION1_IN2, OUTPUT);
  pinMode(FRICTION2_IN3, OUTPUT);
  pinMode(FRICTION2_IN4, OUTPUT); // 摩擦轮引脚
  Serial.println("L298N电机引脚初始化成功!");
  // 初始状态：摩擦轮关闭
  digitalWrite(FRICTION1_IN1, LOW);  // 固定方向
  digitalWrite(FRICTION1_IN2, LOW);
  digitalWrite(FRICTION2_IN3, LOW);  // 固定方向
  digitalWrite(FRICTION2_IN4, LOW);
} // L298N电机引脚初始化
void initServo() { // 舵机初始化
  Servo->writeServo(0);
  Serial.println("舵机初始化成功!");
} // 舵机初始化
void initPS2X() {
  delay(300);
  int error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, false, false);
  if(error == 0) {
    Serial.println("PS2手柄连接成功!");
  } else {
    Serial.print("手柄连接失败，错误代码: ");
    Serial.println(error);
  }
} // PS2手柄初始化
/********************* 以上是初始化 *********************/

/******************** 以下是控制函数 ********************/
void DCMotorRun(const int IN1, const int IN2, const int EN, const int state, const int speed = 0) {
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
} // 自定义电机控制函数(方向+状态)
void DCMotorRun(const int IN1, const int IN2, const int state) {
  switch (state) {
    case FORWARD:
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      break;
    case BACKWARD:
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
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
} // 自定义电机控制函数(PWM不使用)

void ClampControl() { // 机械臂电机控制
  if (ps2x.NewButtonState()) {
    if(ps2x.Button(PSB_TRIANGLE)) { // 向上
      Serial.println("机械臂向上...");
      DCMotorRun(ClampIN3, ClampIN4, BACKWARD);
    }
    if(ps2x.Button(PSB_CROSS)) { // 向下
      Serial.println("机械臂向下...");
      DCMotorRun(ClampIN3, ClampIN4,FORWARD);
    }
    if(ps2x.ButtonReleased(PSB_TRIANGLE) || ps2x.ButtonReleased(PSB_CROSS)) { // 归零
      Serial.println("机械爪停！");
      DCMotorRun(ClampIN3, ClampIN4, BRAKE);
    }
  }
} // 机械臂电机控制

void ShootControl(const uint32_t startTime) {
  static bool isShooting = false;
  if (!isShooting && ps2x.NewButtonState() && ps2x.Button(PSB_R1)) {

    constexpr int T8MOTOR_RUN_TIME = 2000; // 丝杆电机的运动时间 - 动态调整
    constexpr int T8MOTOR_DELAY_TIME = 200; // 丝杆电机的顶端等待时间 - 动态调整
    isShooting = true; // 发射过程使按键不可用

    Serial.println("开始发射过程...");

    Serial.println("摩擦轮开始转动...");
    DCMotorRun(FRICTION1_IN1, FRICTION1_IN2, BACKWARD);
    DCMotorRun(FRICTION2_IN3, FRICTION2_IN4, FORWARD);

    Serial.println("丝杆开始上推...");
    DCMotorRun(T8_IN1, T8_IN2, FORWARD);
    while (millis() - startTime <= T8MOTOR_RUN_TIME) {} // 丝杆电机的运动等待
    uint32_t lastActionTime = millis(); // lastActionTime存储上一动作结束时的时间戳

    Serial.println("丝杆电机到达顶部!");
    DCMotorRun(T8_IN1, T8_IN2, BRAKE);
    while (millis() - lastActionTime <= T8MOTOR_DELAY_TIME) {} // 丝杆电机在顶部的停留等待
    lastActionTime = millis();

    Serial.println("摩擦轮释放!");
    DCMotorRun(FRICTION1_IN1, FRICTION1_IN2, RELEASE);
    DCMotorRun(FRICTION2_IN3, FRICTION2_IN4, RELEASE);

    Serial.println("丝杆开始下推...");
    DCMotorRun(T8_IN1, T8_IN2, BACKWARD);
    while (millis() - lastActionTime <= T8MOTOR_RUN_TIME) {}

    Serial.println("丝杆电机到达底部! 发射过程结束!");
    DCMotorRun(T8_IN1, T8_IN2, BRAKE);

    isShooting = false;
  }
} // 发射连招控制

void ServoControl() { // 舵机控制
  static bool isClampServoOpen = false;
  if (ps2x.NewButtonState() && (ps2x.Button(PSB_SQUARE) || ps2x.Button(PSB_CIRCLE))) {
    isClampServoOpen = !isClampServoOpen;
    if (isClampServoOpen) {
      Serial.println("舵机夹爪开!");
      Servo->writeServo(75);
    } else {
      Serial.println("舵机夹爪合!");
      Servo->writeServo(0);
    }
  }
} // 舵机控制

//region void ChassisControl() { ...底盘控制部分代码... }
static int lf_speed = 0, rf_speed = 0, lb_speed = 0, rb_speed = 0; // 电机速度变量
// 运动参数
int maxChassisSpeed = 255;      // 最大速度 (0-255)
static int deadZone = 20;       // 摇杆死区
int currentChassisSpeed = 0;    // 当前速度
void controlChassisSpeed() { //速度控制
  if(ps2x.Button(PSB_R1)) { // R1加速
    maxChassisSpeed = min(255, maxChassisSpeed + 5);
    delay(100);
  }
  if(ps2x.Button(PSB_L1)) { // L1减速
    maxChassisSpeed = max(80, maxChassisSpeed - 5);
    delay(100);
  }
}
void calculateChassis(const int x, const int y, const int r) { // 计算麦克纳姆轮运动
  // 麦轮运动学公式
  lf_speed = -y + x + r;  // 左前轮
  rf_speed = -y - x - r;  // 右前轮
  lb_speed = 1*(-y - x + r);  // 左后轮
  rb_speed = 1*(-y + x - r);  // 右后轮
  // 限制最大速度
  int maxChassisCalculated = max(max(abs(lf_speed), abs(rf_speed)), max(abs(lb_speed), abs(rb_speed)));
  if(maxChassisCalculated > maxChassisSpeed) {
    const float scale = static_cast<float>(maxChassisSpeed) / maxChassisCalculated;
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

    //controlChassisSpeed(); // 速度控制
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
uint32_t lastChassisTime = 0;
uint32_t lastClampTime = 0;
uint32_t lastShootTime = 0;
uint32_t lastServoTime = 0;
uint32_t lastStepTime = 0; // endregion
// region // 循环间隔设置
constexpr uint32_t CHASSIS_INTERVAL = 10;  // 10ms - 底盘控制
constexpr uint32_t CLAMP_INTERVAL = 10;    // 10ms - 机械臂
constexpr uint32_t SERVO_INTERVAL = 10;    // 10ms - 舵机
constexpr uint32_t SHOOT_INTERVAL = 10;    // 10ms - 发射
// endregion
/******************* 以上是非阻塞定义 *******************/

/******************** 以下是启动函数 ********************/
void setup() {
  Serial.begin(115200);
  mMotorDriver.begin(50);

  initL298N();
  initServo();
  initPS2X(); // 请置于所有init的最后
    Serial.println("\n=== === === ===");
    Serial.println("所有部分均已初始化完成!");
    Serial.println("=== === === ===\n");
  tone(A0, 392, 300); // 音高E4
}
/******************** 以上是启动函数 ********************/

/******************** 以下是循环函数 ********************/
void loop() {
  const uint32_t currentTime = millis();
  if (currentTime - lastChassisTime >= CHASSIS_INTERVAL) {
    ChassisControl();
    lastChassisTime = currentTime;
  } // 底盘控制
  if (currentTime - lastClampTime >= CLAMP_INTERVAL) {
    ClampControl();
    lastClampTime = currentTime;
  } // 舵机控制
  if (currentTime - lastServoTime >= SERVO_INTERVAL) {
    ServoControl();
    lastServoTime = currentTime;
  } // 机械臂控制
  if (currentTime - lastShootTime >= SHOOT_INTERVAL) {
    ShootControl(currentTime);
    lastShootTime = currentTime;
  } // 发射连招控制
}
/******************** 以上是循环函数 ********************/

/******************** 以下是区域模板 ********************/
/******************** 以上是区域模板 ********************/