#include "system.h"
#include "stm32f411xe.h"

// dinh nghia chu ky lay mau he thong (10ms)
#define SYSTEM_SAMPLE_TIME_MS  10

// bien static volatile chi co the truy cap trong file system.c
static volatile uint32_t tick_ms = 0;

// cau hinh clock 100MHz tu nguon noi HSI 16MHz
void SystemClock_Config_100MHz(void)
{
    // bat nguon noi HSI
    RCC->CR |= (1 << 0);
    while (!(RCC->CR & (1 << 1))); // cho HSI on dinh

    // bat clock cho power interface de cau hinh dien ap loi
    RCC->APB1ENR |= (1 << 28);

    // Flash Latency: 3 wait states cho vung chay 100MHz, bat prefetch va cache
    FLASH->ACR = (1 << 10) | (1 << 9) | (1 << 8) | (3 << 0);

    // cau hinh bo chia Prescalers: AHB = /1 (100MHz); APB1 = /2 (50MHz); APB2 = /1 (100MHz)
    RCC->CFGR &= ~((0xF << 4) | (0x7 << 10) | (0x7 << 13));
    RCC->CFGR |=  (0 << 4) | (4 << 10) | (0 << 13);

    // cau hinh tham so PLL: M=8, N=100, P=2 (P=00 trong thanh ghi), SRC=HSI (SRC=0)
    RCC->PLLCFGR = (8 << 0) | (100 << 6) | (0 << 16) | (0 << 22);

    // bat bo nhan tan PLL
    RCC->CR |= (1 << 24);
    while (!(RCC->CR & (1 << 25))); // cho PLL khoa (locked) on dinh

    // chuyen nguon System Clock tong sang dung dau ra PLL
    RCC->CFGR &= ~(3 << 0);
    RCC->CFGR |=  (2 << 0);
    while ((RCC->CFGR & (3 << 2)) != (2 << 2)); // cho phan cung xac nhan chuyen doi thanh cong
}

// cau hinh SysTick tao ngat moi 1ms
void SysTick_Init(void)
{
    // Clock he thong = 100MHz -> 1ms can 100,000 thoi quet (ticks)
    SysTick->LOAD = 100000 - 1;
    SysTick->VAL  = 0;

    // bit 2: chon nguon System Clock; bit 1: cho phep ngat; bit 0: bat bo dem
    SysTick->CTRL = (1 << 2) | (1 << 1) | (1 << 0);
}

// ham phuc vu ngat SysTick (He thong tu dong goi moi 1ms)
void SysTick_Handler(void)
{
    tick_ms++;
}

// ham kiem tra dinh thoi khong block (Non-blocking sample check)
uint8_t Check_Sample_Time(uint32_t sample_time_ms)
{
    static uint32_t last_tick = 0;
    if ((tick_ms - last_tick) >= sample_time_ms)
    {
        last_tick += sample_time_ms;
        return 1;
    }
    return 0;
}

uint32_t Get_Tick(void) {
    return tick_ms;
}

void Delay_ms(uint32_t ms) {
    uint32_t start_time = Get_Tick();
    while ((Get_Tick() - start_time) < ms);
}
