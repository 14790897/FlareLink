# FlareLink - 远程点火装置

基于ESP32-C3的无线点火控制系统，通过UART触发信号控制高压发生器的开关。

## 🎯 系统概述

FlareLink是一个双ESP32-C3系统：
- **发送端**（ESPHome）：通过GPIO9物理按钮或Web界面触发，发送控制信号
- **接收端**（PlatformIO/Arduino）：接收UART信号，驱动GPIO12控制MOSFET开关

### 工作原理

```
[发送端] GPIO9按钮 → UART TX(GPIO0)
           ↓ (无线传输，如LoRa)
[接收端] UART RX(GPIO1) → 检测5x0xFF → GPIO12高电平(2秒) → MOSFET → 高压发生器
```

## 📋 硬件需求

### 接收端（点火控制器）
- ESP32-C3开发板（AirM2M CORE ESP32C3）
- MOSFET模块（用于开关控制）
- 高压发生器
- LoRa模块（可选，用于无线通信）

### 发送端（遥控器）
- ESP32-C3开发板
- 按钮（连接到GPIO9和GND）
- LoRa模块（可选）

## 🔌 接线说明

### 接收端
```
ESP32-C3          连接
━━━━━━━━━━━━━━━━━━━━━━━━━━━━
GPIO0  (TX)   →   LoRa模块 RX
GPIO1  (RX)   ←   LoRa模块 TX
GPIO12        →   MOSFET栅极（G）

MOSFET接线:
  S (源极)    →   GND（与ESP32-C3共地）
  D (漏极)    →   高压发生器负极
  G (栅极)    ←   GPIO12

⚠️ 重要：高压发生器电源负极必须与ESP32-C3的GND连接（共地）
        否则MOSFET无法正常工作！
```

### 发送端
```
ESP32-C3          连接
━━━━━━━━━━━━━━━━━━━━━━━━━━━━
GPIO0  (TX)   →   LoRa模块 TX
GPIO1  (RX)   ←   LoRa模块 RX
GPIO9         →   按钮一端
GND           →   按钮另一端
```

## 🚀 软件功能

### 接收端（`src/main.cpp`）
- **触发条件**：连续接收5个`0xFF`字节
- **输出行为**：GPIO12输出2秒高电平信号
- **安全特性**：
  - 触发期间忽略后续UART数据，防止误触发
  - 2秒后自动关闭，防止过热
  - 串口日志监控所有事件
- **波特率**：9600 bps

### 发送端（`lora-esp32c3.yaml`）
- **手动触发**：按下GPIO9物理按钮发送5x0xFF
- **自动模式**：每秒发送测试包（可通过Web界面开关）
- **Web控制**：通过浏览器访问设备IP控制
- **按钮功能**：
  - "Send High Signal (FF)"：发送5x0xFF触发信号
  - "Send Pattern (AABB)"：发送测试数据包

## 📦 编译与上传

### 接收端（PlatformIO）
```powershell
# 编译
pio run

# 上传固件
pio run -t upload

# 查看日志
pio device monitor -b 115200
```

### 发送端（ESPHome）
1. 编辑`secrets.yaml`，填写Wi-Fi凭据：
```yaml
wifi_ssid: "你的WiFi名称"
wifi_password: "你的WiFi密码"
```

2. 编译并上传：
```powershell
esphome run lora-esp32c3.yaml
```

## 📊 日志示例

### 正常触发流程
```
=== ESP32-C3 UART Trigger Started ===
GPIO12 initialized as OUTPUT (LOW)
UART initialized: TX=GPIO0, RX=GPIO1, Baud=9600
Waiting for 5x 0xFF trigger...

[RX] 0xFF (count: 1/5)
[RX] 0xFF (count: 2/5)
[RX] 0xFF (count: 3/5)
[RX] 0xFF (count: 4/5)
[RX] 0xFF (count: 5/5)
[TRIGGER] 5x 0xFF detected! GPIO12 -> HIGH for 2000ms

[STATUS] GPIO12=HIGH, elapsed=500ms/2000ms
[STATUS] GPIO12=HIGH, elapsed=1000ms/2000ms
[STATUS] GPIO12=HIGH, elapsed=1500ms/2000ms
[OUTPUT] GPIO12 -> LOW (elapsed=2000ms, ready for next trigger)
```

## ⚙️ 配置参数

### 可调整参数（`src/main.cpp`）
```cpp
constexpr int UART_TX_PIN = 0;              // UART发送引脚
constexpr int UART_RX_PIN = 1;              // UART接收引脚
constexpr int OUTPUT_PIN  = 12;             // 输出控制引脚
constexpr uint32_t UART_BAUD = 9600;        // 波特率
const unsigned long OUTPUT_DURATION = 2000;  // 高电平持续时间(ms)
```

## ⚠️ 安全须知

1. **电气安全**
   - **共地连接**：高压发生器电源负极必须与ESP32-C3的GND连接，否则MOSFET无法正常导通
   - 高压发生器必须正确接地
   - 使用适当额定的MOSFET（建议≥10A，≥60V）
   - 添加续流二极管保护MOSFET
   - 确保所有高压连接绝缘良好

2. **操作安全**
   - 首次测试时不连接高压发生器，使用LED验证功能
   - 使用前检查所有接线
   - 保持安全距离进行远程操作
   - 不要在易燃环境中使用

3. **软件安全**
   - 2秒自动断开防止过载
   - 建议根据实际负载调整`OUTPUT_DURATION`
   - 测试时可将持续时间改为更短（如500ms）

## 🔧 故障排查

| 问题 | 可能原因 | 解决方案 |
|------|---------|---------|
| GPIO12持续高电平 | 代码崩溃/看门狗重启 | 检查串口日志，确认固件正常运行 |
| 无法触发 | UART接线错误 | 检查TX/RX交叉连接 |
| 触发后立即关闭 | UART缓冲区干扰 | 已在代码中修复（清空缓冲区） |
| ESP32频繁重启 | 电源不足 | 使用独立5V电源供电 |

## 📁 项目结构

```
FlareLink/
├── src/
│   └── main.cpp              # 接收端主程序
├── lora-esp32c3.yaml         # 发送端ESPHome配置
├── secrets.yaml              # Wi-Fi凭据（需手动创建）
├── platformio.ini            # PlatformIO配置
└── README.md                 # 本文档
```

## 📝 技术规格

- **MCU**：ESP32-C3 (RISC-V 160MHz)
- **RAM**：320KB
- **Flash**：4MB
- **UART波特率**：9600 bps
- **触发信号**：5个连续0xFF字节
- **输出时长**：2秒（可配置）
- **输出电平**：3.3V（驱动MOSFET栅极）

## 🔄 版本历史

- **v1.0** - 初始版本
  - UART触发检测
  - 2秒定时输出
  - 完整日志支持
  - ESPHome发送端集成

## 📄 许可证

本项目仅供学习和研究使用。使用者需自行承担使用本设备的所有风险和责任。

## ⚡ 免责声明

本设备涉及高压电路，操作不当可能造成人身伤害或财产损失。使用前请：
- 充分了解电气安全知识
- 遵守当地法律法规
- 在专业人员指导下使用
- 严禁用于非法用途

**使用本项目即表示您已知晓并接受上述风险。**
