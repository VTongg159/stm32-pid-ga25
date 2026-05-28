# Hệ thống Điều khiển Vòng kín Vận tốc Động cơ DC sử dụng STM32 và C# HMI

Dự án xây dựng bộ điều khiển vận tốc vòng kín (Closed-loop Speed Control) cho động cơ DC GA25 bằng thuật toán PID vị trí tiêu chuẩn, thực thi thời gian thực trên vi điều khiển STM32F411CEU6 (Bare-metal) và giám sát đồ thị trực quan qua ứng dụng C# Windows Forms trên máy tính.

---

## 📌 Các Tính Năng Cốt Lõi

* **Lập trình cấu hình thanh ghi:** Tối ưu hóa hiệu suất CPU, giảm dung lượng bộ nhớ, đạt Jitter cực thấp so với dùng thư viện HAL/LL.
* **Thời gian thực đa nhiệm không chặn (Non-blocking Time-slicing):** Vòng lặp PID chạy cứng chu kỳ 10ms (100Hz), cập nhật giao diện LCD mỗi 200ms và xử lý còi báo (Buzzer) độc lập dựa trên máy trạng thái phần mềm.
* **Đếm xung Encoder x4 phần cứng:** Cấu hình Hardware Encoder Mode trên bộ đếm 32-bit (TIM2), loại bỏ hoàn toàn rủi ro tràn số và mất mát xung khi động cơ quay tốc độ cao.
* **Thuật toán Anti-windup:** Áp dụng phương pháp Clamping (Kẹp dữ liệu và tính ngược) giúp thuật toán PID không bị bão hòa tích phân khi thay đổi Setpoint đột ngột, triệt tiêu độ vọt lố.
* **Ứng dụng Giám sát đa luồng (C# HMI):** Sử dụng `SerialPort` bất đồng bộ trên luồng nền để thu thập dữ liệu Telemetry tốc độ cao (115200 bps), vẽ đồ thị đáp ứng thời độ (Setpoint, Actual Speed, PWM) theo thời gian thực mà không gây treo giao diện.

---

## 🛠️ Kiến Trúc Hệ Thống

Hệ thống được thiết kế theo cấu trúc điều khiển phản hồi vòng kín tiêu chuẩn:

1. **Tín hiệu đặt (Setpoint):** Được nhận từ giao diện C# hoặc qua các nút bấm vật lý (External Interrupts - EXTI).
2. **Bộ điều khiển (Controller):** Lõi ARM Cortex-M4 (100MHz) thực thi hàm toán học `PID_Compute` có bọc giáp Anti-windup.
3. **Cơ cấu chấp hành (Actuator):** Advanced Timer (TIM1) xuất xung PWM tần số cao 10kHz điều khiển mạch công suất cầu H.
4. **Đối tượng điều khiển (Plant):** Động cơ DC GA25 (tích hợp hộp số).
5. **Cảm biến phản hồi (Sensor):** Quadrature Encoder hồi tiếp số lượng xung để nội suy ra vận tốc vòng/phút (RPM).

---

## 📁 Cấu Trúc Mã Nguồn Dự Án

```text
├── Firmware_STM32F411/          # Thư mục mã nguồn C cho Vi điều khiển
│   ├── Core/
│   │   ├── system.c/.h          # Cấu hình Clock 100MHz, định thời SysTick 1ms
│   │   ├── timer_pwm.c/.h       # Khởi tạo TIM1 xuất xung PWM 10kHz
│   │   ├── encoder.c/.h         # Cấu hình TIM2 Hardware Encoder Mode 32-bit
│   │   ├── pid.c/.h             # Thuật toán PID toán học & Anti-windup
│   │   └── uart.c/.h            # Giao tiếp UART 115200bps (Ngắt RX, Polling TX)
│   ├── HMI/
│   │   ├── i2c_lcd.c/.h         # Driver LCD I2C 400kHz tối ưu không delay
│   │   ├── buttons.c/.h         # Cấu hình ngắt nút bấm vật lý (EXTI)
│   │   └── hmi_gpio.c/.h        # Quản lý đèn LED, còi Buzzer trạng thái
│   ├── globals.h                # Định nghĩa các biến toàn cục dùng chung
│   └── main.c                   # Vòng lặp Super-loop phân mảng thời gian
│
└── Software_CSharp_HMI/         # Thư mục dự án Windows Forms App (C#)
    ├── PID_Tuner_App.sln        # File Solution của Visual Studio
    ├── Form1.cs                 # Xử lý logic giao diện, xử lý luồng nhận dữ liệu
    └── Form1.Designer.cs        # Định nghĩa các thành phần đồ họa (Chart, Buttons)
