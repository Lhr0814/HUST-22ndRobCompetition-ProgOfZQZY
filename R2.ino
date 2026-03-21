#include <PS2X_lib.h>
#include <Emakefun_MotorDriver.h>
/******************** 以下是引脚定义 ********************/
#define PS2_DAT         12 // PS2引脚定义
#define PS2_CMD         11 // PS2引脚定义
#define PS2_SEL         10 // PS2引脚定义
#define PS2_CLK         13 // PS2引脚定义
#define STEP_PULSE      22 // 步进电机 接 TB6600 PUL-
#define STEP_DIR        23 // 步进电机 接 TB6600 DIR-
#define ClampIN3        42 // 机械臂电机IN3
#define ClampIN4        40 // 机械臂电机IN4
#define FRICTION1_IN1   26 // 摩擦轮1 IN1引脚
#define FRICTION1_IN2   28 // 摩擦轮1 IN2引脚2
#define FRICTION2_IN3   27 // 摩擦轮2 IN3
#define FRICTION2_IN4   29 // 摩擦轮2 IN4
/* 如若需要更改接线引脚，只需要在此处更改宏定义就足够了! */
/******************** 以上是引脚定义 ********************/

/******************** 以下是对象声明 ********************/
PS2X ps2x; // PS2对象定义
Emakefun_MotorDriver mMotorDriver = Emakefun_MotorDriver(0x60); // 电机对象定义
  Emakefun_DCMotor *RB = mMotorDriver.getMotor(M1); //1号 右后
  Emakefun_DCMotor *RF = mMotorDriver.getMotor(M2); //2号 右前
  Emakefun_DCMotor *LB = mMotorDriver.getMotor(M3); //3号 左后
  Emakefun_DCMotor *LF = mMotorDriver.getMotor(M4); //4号 左前
  Emakefun_Servo *Servo = mMotorDriver.getServo(1); // 1号 舵机
/******************** 以上是对象声明 ********************/

/********************* 以下是初始化 *********************/
void initL298N() { //L298N电机初始化
  pinMode(ClampIN3, OUTPUT);
  pinMode(ClampIN4, OUTPUT);

  pinMode(FRICTION1_IN1, OUTPUT);
  pinMode(FRICTION1_IN2, OUTPUT);
  pinMode(FRICTION2_IN3, OUTPUT);
  pinMode(FRICTION2_IN4, OUTPUT);
  // 初始状态：摩擦轮关闭
  digitalWrite(FRICTION1_IN1, LOW);  // 固定方向
  digitalWrite(FRICTION1_IN2, LOW);
  digitalWrite(FRICTION2_IN3, LOW);  // 固定方向
  digitalWrite(FRICTION2_IN4, LOW);
} // L298N电机引脚初始化
void initStepper() { // 步进电机引脚初始化
  pinMode(STEP_DIR, OUTPUT);
  pinMode(STEP_PULSE, OUTPUT);
} // 步进电机引脚初始化
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
} // 自定义电机控制函数
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
    if(ps2x.Button(PSB_TRIANGLE)) {//向上
      Serial.println("机械臂向上...");
      DCMotorRun(ClampIN3, ClampIN4, FORWARD);
    }
    if(ps2x.Button(PSB_CROSS)) {//向下
      Serial.println("机械臂向下...");
      DCMotorRun(ClampIN3, ClampIN4,BACKWARD);
    }
    if(ps2x.ButtonReleased(PSB_TRIANGLE) || ps2x.ButtonReleased(PSB_CROSS)) {//归零
      Serial.println("机械爪停！");
      DCMotorRun(ClampIN3, ClampIN4, BRAKE);
    }
  }
} // 机械臂电机控制

void FrictionControl() { // 摩擦轮控制
  static bool frictionsOn = false;
  // 使用R1键同时控制两个摩擦轮
  if (ps2x.NewButtonState() && ps2x.Button(PSB_R1)) {
    frictionsOn = !frictionsOn;  // 切换状态
    if (frictionsOn) {
      DCMotorRun(FRICTION1_IN1, FRICTION1_IN2, BACKWARD);
      DCMotorRun(FRICTION2_IN3, FRICTION2_IN4, FORWARD);
      Serial.println("摩擦轮开启（两个同时运行）");
    } else {
      DCMotorRun(FRICTION1_IN1, FRICTION1_IN2, RELEASE);
      DCMotorRun(FRICTION2_IN3, FRICTION2_IN4, RELEASE);
      Serial.println("摩擦轮关闭");
    }
  }
} // 摩擦轮控制

void ServoControl() { // 舵机控制
  if (ps2x.NewButtonState()) {
    if(ps2x.Button(PSB_CIRCLE)) {
      Servo->writeServo(0);
    }
    if(ps2x.Button(PSB_SQUARE)) {
      Servo->writeServo(135);
    }
  }
} // 舵机控制

void StepperControl() { // 步进电机控制
  static bool stepperRunning = false;
  static bool stepperDirection = true; // true=正转，false=反转
  static unsigned long lastStepTime = 0;
  const unsigned long stepDelay = 100; // 100微秒，对应10000步/秒

  if (ps2x.NewButtonState()) { // 检查按键状态
    if(ps2x.Button(PSB_PAD_UP)) { // R2键正转
      stepperRunning = true;
      stepperDirection = true;
      digitalWrite(STEP_DIR, LOW); // 假设LOW为正向
      Serial.println("步进电机正转");
    }
    else if(ps2x.Button(PSB_PAD_DOWN)) { // L2键反转
      stepperRunning = true;
      stepperDirection = false;
      digitalWrite(STEP_DIR, HIGH); // 假设HIGH为反向
      Serial.println("步进电机反转");
    }
  }
  if (ps2x.ButtonReleased(PSB_PAD_UP) || ps2x.ButtonReleased(PSB_PAD_DOWN)) {
  // 检查按键释放
    if(stepperRunning && ((ps2x.ButtonReleased(PSB_PAD_UP) && stepperDirection) ||
                          (ps2x.ButtonReleased(PSB_PAD_DOWN) && !stepperDirection))) {
      stepperRunning = false;
      Serial.println("步进电机停止");
                          }
  }
  if (stepperRunning) { // 产生脉冲（非阻塞方式）
    unsigned long currentTime = micros();
    if (currentTime - lastStepTime >= stepDelay) {
      // 产生一个脉冲
      digitalWrite(STEP_PULSE, HIGH);
      delayMicroseconds(2); // 脉冲宽度2微秒
      digitalWrite(STEP_PULSE, LOW);
      lastStepTime = currentTime;
    }
  }
} // 步进电机控制

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
void calculateChassis(int x, int y, int r) { // 计算麦克纳姆轮运动
  // 麦轮运动学公式
  lf_speed = y + x + r;  // 左前轮
  rf_speed = y - x - r;  // 右前轮
  lb_speed = y - x + r;  // 左后轮
  rb_speed = y + x - r;  // 右后轮
  // 限制最大速度
  int maxChassisCalculated = max(max(abs(lf_speed), abs(rf_speed)), max(abs(lb_speed), abs(rb_speed)));
  if(maxChassisCalculated > maxChassisSpeed) {
    float scale = static_cast<float>(maxChassisSpeed) / maxChassisCalculated;
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
unsigned long lastClampTime = 0;
unsigned long lastFrictTime = 0;
unsigned long lastServoTime = 0;
unsigned long lastStepTime = 0; // endregion
// region // 循环间隔设置
constexpr unsigned long CHASSIS_INTERVAL = 10;  // 10ms - 底盘控制
constexpr unsigned long CLAMP_INTERVAL = 10;    // 10ms - 机械臂
constexpr unsigned long FRICT_INTERVAL = 10;    // 10ms - 摩擦轮
constexpr unsigned long SERVO_INTERVAL = 10;    // 10ms - 舵机
constexpr unsigned long STEP_INTERVAL = 0;      // 0    - 步进电机
// endregion
/******************* 以上是非阻塞定义 *******************/

/******************** 以下是启动函数 ********************/
void setup() {
  Serial.begin(115200);
  mMotorDriver.begin(50);

  initL298N();
  initStepper();
  initPS2X(); // 请置于setup最后
  tone(A0, 392, 300); // 音高E4
}
/******************** 以上是启动函数 ********************/

/******************** 以下是循环函数 ********************/
void loop() {
  unsigned long currentTime = millis();
  unsigned long currentMicroTime = micros();
  if (currentMicroTime - lastStepTime >= STEP_INTERVAL) {
    StepperControl();
    lastStepTime = currentMicroTime;
  } // 摩擦轮电机控制
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
  if (currentTime - lastFrictTime >= FRICT_INTERVAL) {
    FrictionControl();
    lastFrictTime = currentTime;
  } // 摩擦轮电机控制
}
/******************** 以上是循环函数 ********************/

/******************** 以下是区域模板 ********************/
/******************** 以上是区域模板 ********************/