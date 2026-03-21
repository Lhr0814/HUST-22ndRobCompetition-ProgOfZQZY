#include <Arduino.h>
#include <PS2X_lib.h>
#include <Servo.h>

// PS2接收器引脚定义
#define PS2_DAT 51
#define PS2_CMD 50

#define PS2_SEL 53
#define PS2_CLK 52

// 电机引脚定义
// 左前轮 (LF)
#define LF_ENA 9
#define LF_IN1 8
#define LF_IN2 7
// 右前轮 (RF)
#define RF_ENB 6
#define RF_IN3 5
#define RF_IN4 4

// 左后轮 (LB)
#define LB_ENA 3
#define LB_IN1 2
#define LB_IN2 22
// 右后轮 (RB)
#define RB_ENB 44
#define RB_IN3 24
#define RB_IN4 25

// N20电机
#define N20_ENA 10
#define N20_IN1 43
#define N20_IN2 42
// 夹爪电机
#define Clamp_ENB 11
#define Clamp_IN3 41
#define Clamp_IN4 40

//电磁阀
#define Relay_VCC 0

//舵机
#define Servo_PIN 0


PS2X ps2x;

// 运动参数
int maxSpeed = 180;      // 最大速度 (0-255)
int deadZone = 20;       // 摇杆死区
int currentSpeed = 0;    // 当前速度

// 电机速度变量
int lf_speed = 0, rf_speed = 0, lb_speed = 0, rb_speed = 0;

// N20电机控制
void N20Control() {
      if (ps2x.NewButtonState()) {
        if(ps2x.Button(PSB_TRIANGLE)) {//向上
          Serial.println("三角形向上...");
          digitalWrite(N20_IN1, LOW);
          digitalWrite(N20_IN2, HIGH);
          analogWrite(N20_ENA, 255);
        }
        if(ps2x.Button(PSB_CROSS)) {//叉向下
          Serial.println("叉子向下...");
          digitalWrite(N20_IN1, HIGH);
          digitalWrite(N20_IN2, LOW);
          analogWrite(N20_ENA, 255);
        }
      if(ps2x.ButtonReleased(PSB_TRIANGLE)) {//归零
        Serial.println("三角形结束！");
        digitalWrite(N20_IN1, LOW);
        digitalWrite(N20_IN2, LOW);
        analogWrite(N20_ENA, 0);
      }
      if(ps2x.ButtonReleased(PSB_CROSS)) {//归零
        Serial.println("叉子结束！");
        digitalWrite(N20_IN1, LOW);
        digitalWrite(N20_IN2, LOW);
        analogWrite(N20_ENA, 0);
      }
    }
}
// 机械臂电机控制
void ClampControl() {
      if (ps2x.NewButtonState()) {
        if(ps2x.Button(PSB_CIRCLE)) {//向上
          Serial.println("圆圈向上...");
          digitalWrite(Clamp_IN3, LOW);
          digitalWrite(Clamp_IN4, HIGH);
          analogWrite(Clamp_ENB, 255);
        }
        if(ps2x.Button(PSB_SQUARE)) {//向下
          Serial.println("正方形向下...");
          digitalWrite(Clamp_IN3, HIGH);
          digitalWrite(Clamp_IN4, LOW);
          analogWrite(Clamp_ENB, 255);
        }
      if(ps2x.ButtonReleased(PSB_CIRCLE)) {//归零
        Serial.println("圆圈结束！");
        digitalWrite(Clamp_IN3, LOW);
        digitalWrite(Clamp_IN4, LOW);
        analogWrite(Clamp_ENB, 0);
      }
      if(ps2x.ButtonReleased(PSB_SQUARE)) {//归零
        Serial.println("正方形结束！");
        digitalWrite(Clamp_IN3, LOW);
        digitalWrite(Clamp_IN4, LOW);
        analogWrite(Clamp_ENB, 0);
      }
    }
}

// region void caculateMecanum() { ...底盘控制部分代码... }
void controlSpeed();
void calculateMecanum(int left_x, int left_y, int right_x);
void setMotorSpeeds();
void MecanumControl() {
  if(ps2x.read_gamepad(false, 0)) { // 如果成功读取手柄数据
    // 读取摇杆值
    int leftX = ps2x.Analog(PSS_LX) - 128;  // -128 到 127
    int leftY = ps2x.Analog(PSS_LY) - 128;
    int rightX = ps2x.Analog(PSS_RX) - 128;

    // 应用死区
    if(abs(leftX) < deadZone) leftX = 0;
    if(abs(leftY) < deadZone) leftY = 0;
    if(abs(rightX) < deadZone) rightX = 0;

    // 速度控制
    controlSpeed();

    // 计算麦轮运动
    calculateMecanum(leftX, leftY, rightX);

    // 应用电机控制
    setMotorSpeeds();

    // // 紧急停止
    // if(ps2x.Button(PSB_BLUE)) { // ×按钮
    //   emergencyStop();
    // }

    // 调试输出（可选）
    // debugOutput(leftX, leftY, rightX);
  }

}

// 计算麦克纳姆轮运动
void calculateMecanum(int x, int y, int r) {
  // 麦轮运动学公式
  lf_speed = y + x + r;  // 左前轮
  rf_speed = y - x - r;  // 右前轮
  lb_speed = y - x + r;  // 左后轮
  rb_speed = y + x - r;  // 右后轮

  // 限制最大速度
  int maxCalculated = max(max(abs(lf_speed), abs(rf_speed)),
                         max(abs(lb_speed), abs(rb_speed)));

  if(maxCalculated > maxSpeed) {
    float scale = (float)maxSpeed / maxCalculated;
    lf_speed *= scale;
    rf_speed *= scale;
    lb_speed *= scale;
    rb_speed *= scale;
  }
}

// 控制单个电机
void setMotor(int enPin, int in1, int in2, int speed) {
  // 设置方向
  if(speed > 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  } else if(speed < 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  } else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }

  // 设置PWM速度（取绝对值）
  analogWrite(enPin, abs(speed));
}

// 设置麦轮电机速度和方向
void setMotorSpeeds() {
  setMotor(LF_ENA, LF_IN1, LF_IN2, lf_speed);
  setMotor(RF_ENB, RF_IN3, RF_IN4, rf_speed);
  setMotor(LB_ENA, LB_IN1, LB_IN2, lb_speed);
  setMotor(RB_ENB, RB_IN3, RB_IN4, rb_speed);
}

// 停止所有麦轮电机
void stopAllMotors() {
  analogWrite(LF_ENA, 0);
  analogWrite(RF_ENB, 0);
  analogWrite(LB_ENA, 0);
  analogWrite(RB_ENB, 0);

  digitalWrite(LF_IN1, LOW);
  digitalWrite(LF_IN2, LOW);
  digitalWrite(RF_IN3, LOW);
  digitalWrite(RF_IN4, LOW);
  digitalWrite(LB_IN1, LOW);
  digitalWrite(LB_IN2, LOW);
  digitalWrite(RB_IN3, LOW);
  digitalWrite(RB_IN4, LOW);
}

// 电机紧急停止
void emergencyStop() {
  stopAllMotors();
  Serial.println("紧急停止！");
  delay(1000); // 停止1秒
}

// 调试输出
void debugOutput(int x, int y, int r) {
  Serial.print("X:");
  Serial.print(x);
  Serial.print(" Y:");
  Serial.print(y);
  Serial.print(" R:");
  Serial.print(r);
  Serial.print(" LF:");
  Serial.print(lf_speed);
  Serial.print(" RF:");
  Serial.print(rf_speed);
  Serial.print(" LB:");
  Serial.print(lb_speed);
  Serial.print(" RB:");
  Serial.print(rb_speed);
  Serial.print(" Max:");
  Serial.println(maxSpeed);
} // endregion

// SETUP 函数
void setup() {
  Serial.begin(115200);

  // 初始化电机引脚
  pinMode(LF_ENA, OUTPUT);
  pinMode(LF_IN1, OUTPUT);
  pinMode(LF_IN2, OUTPUT);

  pinMode(RF_ENB, OUTPUT);
  pinMode(RF_IN3, OUTPUT);
  pinMode(RF_IN4, OUTPUT);

  pinMode(LB_ENA, OUTPUT);
  pinMode(LB_IN1, OUTPUT);
  pinMode(LB_IN2, OUTPUT);

  pinMode(RB_ENB, OUTPUT);
  pinMode(RB_IN3, OUTPUT);
  pinMode(RB_IN4, OUTPUT);

  pinMode(N20_ENA, OUTPUT);
  pinMode(N20_IN1, OUTPUT);
  pinMode(N20_IN2, OUTPUT);

  pinMode(Clamp_ENB, OUTPUT);
  pinMode(Clamp_IN3, OUTPUT);
  pinMode(Clamp_IN4, OUTPUT);

  // 初始化PS2手柄
  delay(300);
  int error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, false, false);

  if(error == 0) {
    Serial.println("PS2手柄连接成功！");
    Serial.println("控制说明:");
    Serial.println("左摇杆 - 前后左右移动");
    Serial.println("右摇杆左右 - 旋转");
    Serial.println("R1/L1 - 加速/减速");
    Serial.println("×按钮 - 紧急停止");
  } else {
    Serial.print("手柄连接失败，错误代码: ");
    Serial.println(error);
  }
}
// LOOP 函数
void loop() {
  MecanumControl();
  N20Control();
  ClampControl();
  delay(20);
}