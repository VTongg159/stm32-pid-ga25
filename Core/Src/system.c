#include "system.h"
#include "stm32f411xe.h"
#define SYSTEM_SAMPLE_TIME_MS  10 //chu ki lay mau he thong
static volatile uint32_t tick_ms = 0;

// cau hinh clock 100MHz tu nguon noi HSI 16MHz
void SystemClock_Config_100MHz(void)
{
    RCC->CR |= (1 << 0); // bat bo dao dong noi toc do cao
    while (!(RCC->CR & (1 << 1))); // cho HSI on dinh

    // bat clock cho power interface de cau hinh dien ap loi cpu
    RCC->APB1ENR |= (1 << 28);

    // cau hinh do tre la 3 chu ky cho 100MHz
    FLASH->ACR &= ~(0xF << 0);
    FLASH->ACR |= (3 << 0);
    FLASH->ACR |= (1 << 8); //Prefetch enable
    FLASH->ACR |= (1 << 9); // Instruction cache enable
    FLASH->ACR |= (1 << 10); // Data cache enable

    // cau hinh bo chia cho  AHB 100MHz
    RCC->CFGR &= ~(0xF << 4);
    RCC->CFGR |=  (0 << 4); // khong chia xung clock
    // cau hinh bo chi cho APB1 50MHz
    RCC->CFGR &= ~(0x7 << 10);
    RCC->CFGR |=  (4 << 10); // chia 2
    // cau hinh bo chia cho APB2 100MHz
    RCC->CFGR &= ~(0x7 << 13);
    RCC->CFGR |=  (0 << 13); // khong chia

    // cau hinh PPL
    RCC->PLLCFGR &= ~(0x3F << 0);
    RCC->PLLCFGR |= (8 << 0); // PLLM = 8
    RCC->PLLCFGR &= ~(0x1FF << 6);
    RCC->PLLCFGR |= (100 << 6); // PLLN = 100
    RCC->PLLCFGR &= ~(0x3 << 16);
    RCC->PLLCFGR |= (0 << 16);  // PLLP = 0
    RCC->PLLCFGR &= ~(1 << 22);
    RCC->PLLCFGR |= (0 << 22);   // PLLSRC = 0
    RCC->PLLCFGR &= ~(0xF << 24);
    RCC->PLLCFGR |= (4 << 24);   //PLLQ = 4

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
    SysTick->LOAD = 100000 - 1;// Clock he thong = 100MHz -> 1ms can 100,000 ticks
    SysTick->VAL  = 0;
    SysTick->CTRL = (1 << 2) | (1 << 1) | (1 << 0);
}

// ham phuc vu ngat SysTick
void SysTick_Handler(void)
{
    tick_ms++;
}

// ham kiem tra dinh thoi khong block
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
