#include "main.h"

// PROTOTYPE
void GPIO_Config(void);
void TIM1_PWM_Config(void);
void TIM2_Encoder_Config(void);
void SysTick_Config_1ms(void);
void Motor_Forward(void);
void Motor_Backward(void);          // Thêm: để PID điều khiển 2 chiều
void Motor_Brake(void);             // Thêm: dừng an toàn khi duty = 0
void Delay_ms(uint32_t ms);

// ======================================================
// SysTick counter (tăng mỗi 1ms trong SysTick_Handler)
// ======================================================
volatile uint32_t systick_ms = 0;

// ======================================================
// ENCODER
// ======================================================
volatile int32_t encoder_count = 0;  // Tổng xung tích lũy (có dấu)

int32_t prev_count    = 0;
int32_t current_count = 0;
int32_t delta_count   = 0;

float rpm = 0.0f;

// CPR của GA25: 390 PPR x4 (Encoder Mode 3) = 1560
#define ENCODER_CPR     1560.0f

// Sample time = 100ms = 0.1s
#define SAMPLE_TIME_MS  100u
#define SAMPLE_TIME_S   0.1f

// ======================================================
// PID BEGIN
// ======================================================

// ----------------------------------------------------
// THÔNG SỐ PID — chỉnh tại đây
// Gợi ý ban đầu: bắt đầu với Kp nhỏ, Ki=0, Kd=0
// rồi tăng dần từng bước (Ziegler–Nichols hoặc thực nghiệm)
// ----------------------------------------------------
#define KP   1.5f       // Hệ số tỉ lệ — phản ứng nhanh với sai số
#define KI   0.8f       // Hệ số tích phân — triệt tiêu sai số tĩnh
#define KD   0.05f      // Hệ số vi phân — giảm dao động, cản overshoots

// ----------------------------------------------------
// GIỚI HẠN ĐẦU RA PWM
// TIM1->ARR = 999, nên CCR1 hợp lệ trong [0, 1000]
// ----------------------------------------------------
#define PWM_MIN  0.0f
#define PWM_MAX  1000.0f

// ----------------------------------------------------
// GIỚI HẠN TÍCH PHÂN (anti-windup clamp)
// Tính sao cho Ki * INTEGRAL_MAX <= PWM_MAX
// → INTEGRAL_MAX = PWM_MAX / Ki = 1000 / 0.8 = 1250
// Điều chỉnh nếu đổi Ki
// ----------------------------------------------------
#define INTEGRAL_MAX  1250.0f

// ----------------------------------------------------
// SETPOINT — tốc độ mong muốn (RPM)
// Dương = quay thuận (Motor_Forward)
// Âm   = quay ngược (Motor_Backward)  [mở rộng tương lai]
// ----------------------------------------------------
float setpoint_rpm = 100.0f;   // <<< ĐẶT TỐC ĐỘ MONG MUỐN Ở ĐÂY

// ----------------------------------------------------
// BIẾN NỘI BỘ PID
// ----------------------------------------------------

// Sai số hiện tại: e(k) = setpoint - measured
float pid_error      = 0.0f;

// Sai số chu kỳ trước: dùng để tính đạo hàm xấp xỉ
// derivative = (e(k) - e(k-1)) / Ts
float pid_prev_error = 0.0f;

// Tích phân tích lũy: integral += e(k) * Ts
// Đây là xấp xỉ diện tích dưới đường cong sai số
float pid_integral   = 0.0f;

// Đầu ra PID thô (chưa clamp)
float pid_output     = 0.0f;

// Đầu ra PWM cuối cùng sau khi clamp [PWM_MIN, PWM_MAX]
int32_t pwm_out      = 0;

// ----------------------------------------------------
// HÀM PID — gọi mỗi SAMPLE_TIME_S giây
//
// Công thức:
//   P = Kp * e(k)
//   I = Ki * integral           (integral = Σ e(k)*Ts, bị clamp)
//   D = Kd * (e(k) - e(k-1)) / Ts
//   output = P + I + D
//   pwm    = clamp(output, PWM_MIN, PWM_MAX)
// ----------------------------------------------------
int32_t PID_Compute(float setpoint, float measured)
{
    // --------------------------------------------------
    // 1. Tính sai số hiện tại
    //    e(k) = setpoint - measured
    //    Dương: đang chậm hơn mục tiêu → cần tăng duty
    //    Âm  : đang nhanh hơn mục tiêu → cần giảm duty
    // --------------------------------------------------
    pid_error = setpoint - measured;

    // --------------------------------------------------
    // 2. Tích phân (Integral term)
    //    integral += e(k) * Ts
    //    Xấp xỉ diện tích hình chữ nhật (Euler forward)
    //    Sau đó clamp để tránh windup:
    //    Nếu output bão hòa (đã ở PWM_MAX), integral sẽ
    //    tiếp tục tăng vô tội vạ nếu không clamp →
    //    gây overshoots nghiêm trọng khi setpoint thay đổi
    // --------------------------------------------------
    pid_integral += pid_error * SAMPLE_TIME_S;

    // Anti-windup: kẹp tích phân trong [-INTEGRAL_MAX, +INTEGRAL_MAX]
    if (pid_integral >  INTEGRAL_MAX) pid_integral =  INTEGRAL_MAX;
    if (pid_integral < -INTEGRAL_MAX) pid_integral = -INTEGRAL_MAX;

    // --------------------------------------------------
    // 3. Đạo hàm (Derivative term)
    //    derivative = (e(k) - e(k-1)) / Ts
    //    Xấp xỉ tốc độ thay đổi sai số (backward difference)
    //    Dương: sai số đang tăng → cần thêm lực cản
    //    Âm  : sai số đang giảm → đang tiến gần setpoint
    //    Lưu ý: nhạy với noise → giữ Kd nhỏ
    // --------------------------------------------------
    float pid_derivative = (pid_error - pid_prev_error) / SAMPLE_TIME_S;

    // --------------------------------------------------
    // 4. Tổng hợp đầu ra PID
    //    output = P + I + D
    // --------------------------------------------------
    pid_output = (KP * pid_error)
               + (KI * pid_integral)
               + (KD * pid_derivative);

    // --------------------------------------------------
    // 5. Lưu sai số hiện tại cho chu kỳ sau
    // --------------------------------------------------
    pid_prev_error = pid_error;

    // --------------------------------------------------
    // 6. Giới hạn đầu ra PWM (Output saturation)
    //    TIM1->CCR1 phải trong [0, 1000]
    //    Nếu output âm: có thể đổi chiều quay qua GPIO
    //    Ở đây clamp về 0 (chỉ quay 1 chiều)
    //    → Xem phần Motor_Direction bên dưới để mở rộng
    // --------------------------------------------------
    if (pid_output >  PWM_MAX) pid_output =  PWM_MAX;
    if (pid_output <  PWM_MIN) pid_output =  PWM_MIN;

    return (int32_t)pid_output;
}

// PID END
// ======================================================


// ======================================================
// SYSTICK HANDLER
// ======================================================
void SysTick_Handler(void)
{
    systick_ms++;
}


// ======================================================
// MAIN
// ======================================================
int main(void)
{
    SysTick_Config_1ms();

    GPIO_Config();
    TIM1_PWM_Config();
    TIM2_Encoder_Config();

    // Khởi động motor quay thuận
    Motor_Forward();

    // Khởi tạo prev_count để tránh spike RPM ở vòng đầu
    prev_count = (int32_t)TIM2->CNT;

    // PID BEGIN — Reset trạng thái PID trước khi vào vòng lặp
    pid_integral   = 0.0f;
    pid_prev_error = 0.0f;
    // PID END

    uint32_t last_sample = systick_ms;

    while (1)
    {
        // Chờ đúng 100ms (non-blocking wait)
        if ((systick_ms - last_sample) < SAMPLE_TIME_MS)
            continue;

        last_sample += SAMPLE_TIME_MS;

        // ==========================================
        // ĐỌC ENCODER
        // ==========================================
        current_count = (int32_t)TIM2->CNT;

        // Phép trừ int32_t tự xử lý wrap-around
        // TIM2->ARR = 0xFFFFFFFF (32-bit), overflow hiếm gặp
        // nhưng int32_t subtraction vẫn đúng nhờ 2's complement
        delta_count = current_count - prev_count;

        // Tích lũy tổng xung có dấu (odometry, nếu cần)
        encoder_count += delta_count;

        prev_count = current_count;

        // ==========================================
        // TÍNH RPM
        // rpm = (delta_count / CPR) / Ts * 60
        //     = delta_count * 60 / (CPR * Ts)
        // Dương = quay thuận, Âm = quay ngược
        // ==========================================
        rpm = ((float)delta_count * 60.0f) /
              (ENCODER_CPR * SAMPLE_TIME_S);

        // ==========================================
        // PID BEGIN — Tính toán và áp dụng điều khiển
        // ==========================================

        // Tính PWM mới từ PID
        pwm_out = PID_Compute(setpoint_rpm, rpm);

        // Ghi giá trị PWM vào thanh ghi so sánh TIM1 CH1
        // CCR1 ∈ [0, 1000] → duty = CCR1 / ARR = CCR1 / 1000
        TIM1->CCR1 = (uint32_t)pwm_out;

        // PID END
        // ==========================================
    }
}


// ======================================================
// SYSTICK CONFIG — 1ms tick
// ======================================================
void SysTick_Config_1ms(void)
{
    SysTick->LOAD = (SystemCoreClock / 1000u) - 1u;
    SysTick->VAL  = 0u;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk    |
                    SysTick_CTRL_ENABLE_Msk;
}


// ======================================================
// DELAY — dùng SysTick
// ======================================================
void Delay_ms(uint32_t ms)
{
    uint32_t start = systick_ms;
    while ((systick_ms - start) < ms) { }
}


// ======================================================
// GPIO CONFIG
// ======================================================
void GPIO_Config(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // PA8 → TIM1_CH1 PWM (AF1)
    GPIOA->MODER &= ~(3u << (8 * 2));
    GPIOA->MODER |=  (2u << (8 * 2));
    GPIOA->AFR[1] &= ~(0xFu << ((8 - 8) * 4));
    GPIOA->AFR[1] |=  (1u   << ((8 - 8) * 4));

    // PA9, PA10 → OUTPUT (IN1, IN2 motor driver)
    GPIOA->MODER &= ~(3u << (9  * 2));
    GPIOA->MODER |=  (1u << (9  * 2));
    GPIOA->MODER &= ~(3u << (10 * 2));
    GPIOA->MODER |=  (1u << (10 * 2));

    // PA0, PA1 → PULL-UP (encoder A, B)
    GPIOA->PUPDR &= ~(3u << (0 * 2));
    GPIOA->PUPDR |=  (1u << (0 * 2));
    GPIOA->PUPDR &= ~(3u << (1 * 2));
    GPIOA->PUPDR |=  (1u << (1 * 2));
}


// ======================================================
// TIM1 PWM CONFIG — 1kHz, PA8
// APB2 = 84MHz, PSC=83, ARR=999 → f=1kHz
// ======================================================
void TIM1_PWM_Config(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    TIM1->PSC = 84 - 1;
    TIM1->ARR = 1000 - 1;

    TIM1->CCMR1 &= ~(7u << 4);
    TIM1->CCMR1 |=  (6u << 4);         // OC1M = PWM Mode 1
    TIM1->CCMR1 |= TIM_CCMR1_OC1PE;

    TIM1->CCER |= TIM_CCER_CC1E;
    TIM1->CCR1  = 0;                    // Bắt đầu với duty = 0%
    TIM1->BDTR |= TIM_BDTR_MOE;
    TIM1->CR1  |= TIM_CR1_ARPE;
    TIM1->CR1  |= TIM_CR1_CEN;
}


// ======================================================
// TIM2 ENCODER CONFIG — PA0 (CH1), PA1 (CH2)
// Mode 3 (TI1 + TI2) = x4 counting
// ======================================================
void TIM2_Encoder_Config(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // PA0, PA1 → AF1 (TIM2_CH1, TIM2_CH2)
    GPIOA->MODER &= ~(3u << (0 * 2));
    GPIOA->MODER |=  (2u << (0 * 2));
    GPIOA->MODER &= ~(3u << (1 * 2));
    GPIOA->MODER |=  (2u << (1 * 2));

    GPIOA->AFR[0] &= ~(0xFu << (0 * 4));
    GPIOA->AFR[0] |=  (1u   << (0 * 4));
    GPIOA->AFR[0] &= ~(0xFu << (1 * 4));
    GPIOA->AFR[0] |=  (1u   << (1 * 4));

    // SMS = 011 → Encoder Mode 3 (đếm cả 2 cạnh của CH1 và CH2)
    TIM2->SMCR &= ~TIM_SMCR_SMS;
    TIM2->SMCR |= (TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1);

    // CC1S = 01 (IC1 mapped to TI1), CC2S = 01 (IC2 mapped to TI2)
    TIM2->CCMR1 &= ~(TIM_CCMR1_CC1S | TIM_CCMR1_CC2S);
    TIM2->CCMR1 |=  (TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_0);

    // ARR max 32-bit để không bao giờ tràn trong phạm vi thực tế
    TIM2->ARR = 0xFFFFFFFFu;
    TIM2->CNT = 0u;
    TIM2->CR1 |= TIM_CR1_CEN;
}


// ======================================================
// MOTOR DIRECTION CONTROL
// IN1=1, IN2=0 → Forward
// IN1=0, IN2=1 → Backward
// IN1=0, IN2=0 → Brake (hoặc Coast tùy driver)
// ======================================================

void Motor_Forward(void)
{
    GPIOA->BSRR = GPIO_BSRR_BS9;   // IN1 = 1
    GPIOA->BSRR = GPIO_BSRR_BR10;  // IN2 = 0
}

void Motor_Backward(void)
{
    GPIOA->BSRR = GPIO_BSRR_BR9;   // IN1 = 0
    GPIOA->BSRR = GPIO_BSRR_BS10;  // IN2 = 1
}

void Motor_Brake(void)
{
    GPIOA->BSRR = GPIO_BSRR_BR9;   // IN1 = 0
    GPIOA->BSRR = GPIO_BSRR_BR10;  // IN2 = 0
}
