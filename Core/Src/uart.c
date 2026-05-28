#include "uart.h"

static volatile char rx_buffer[50];
static volatile uint8_t rx_index = 0;
static volatile uint8_t rx_ready = 0;

void UART_Init(void) {
    //cap xung clock cho GPIOA va USART2
    RCC->AHB1ENR |= (1 << 0 );
    RCC->APB1ENR |= (1 << 17);

    //cau hinh che do alternate PA2 va PA3
    // PA2: TX
    GPIOA->MODER &= ~(1 << 4);
    GPIOA->MODER |= (1 << 5);

    //PA3: RX
    GPIOA->MODER &= ~(1 << 6);
    GPIOA->MODER |= (1 << 7);

    // cau hinh AF7: USART2
    //PA2
    GPIOA->AFR[0] &= ~(0xF << 8);
    GPIOA->AFR[0] |= (0x7 << 8);

    // PA3
    GPIOA->AFR[0] &= ~(0xF << 12);
    GPIOA->AFR[0] |= (0x7 << 12);

    // cau hinh che do high speed cho PA2 va PA3
    GPIOA->OSPEEDR |= (3 << 4);
    GPIOA->OSPEEDR |= (3 << 6);

    // cau hinh che do pull up cho PA2 va PA3
    GPIOA->PUPDR |= (1 << 4);
    GPIOA->PUPDR &= ~(1 << 5);
    GPIOA->PUPDR |= (1 << 6);
    GPIOA->PUPDR &= ~(1 << 7);

	// cau hinh baudrate = 115200
    USART2->BRR = (27 << 4) | (2 << 0);

    // cau hinh thanh ghi control
    USART2->CR1 |= (1 << 3); // TE
	USART2->CR1 |= (1 << 2); // RE
	USART2->CR1 |= (1 << 13); // UE

}

void UART_SendChar(char c) {
    // Polling: doi cho den khi co TXE lên 1
    while (!(USART2->SR & USART_SR_TXE));

    // Ghi du lieu vao thanh ghi Data Register
    USART2->DR = (c & 0xFF);
}

void UART_SendString(const char* str) {
    while (*str) {
        UART_SendChar(*str++);
    }
}

void UART_SendNumber(int32_t num) {
    char buffer[12]; // chua toi da 10 chu so + dấu trừ + null
    int i = 0;
    int is_negative = 0;

    // xu ly so 0
    if (num == 0) {
        UART_SendChar('0');
        return;
    }

    // xu ly  so am
    if (num < 0) {
        is_negative = 1;
        // Xử ly tran int32_t: -2147483648
        if (num == -2147483648LL) {
            UART_SendString("-2147483648");
            return;
        }
        num = -num;
    }

    // tach tung chu so theo thu tu nguoc
    while (num > 0) {
        buffer[i++] = (num % 10) + '0'; //chuyen thanh ky tu ASCII
        num /= 10;
    }

    if (is_negative) {
        buffer[i++] = '-';
    }

    // In nnguoc mang ra UART
    while (i > 0) {
        UART_SendChar(buffer[--i]);
    }
}

void UART_RX_Interrupt_Enable(void) {
    USART2->CR1 |= (1 << 5);              // RXNEIE = 1
    NVIC_SetPriority(USART2_IRQn, 1);
    NVIC_EnableIRQ(USART2_IRQn);
}

void USART2_IRQHandler(void) {
    if (USART2->SR & (1 << 5)) {
        char c = (char)(USART2->DR & 0xFF);
        if (c == '\n' || c == '\r') {
            if (rx_index > 0) {
                rx_buffer[rx_index] = '\0';
                rx_ready = 1;    // Báo hiệu đã có lệnh
                rx_index = 0;
            }
        } else {
            if (rx_index < 49) rx_buffer[rx_index++] = c;
            else rx_index = 0;
        }
    }
}

uint8_t UART_IsCommandReady(void) {
    return rx_ready;
}

void UART_GetCommand(char *out_buf) {
    if (rx_ready) {
        strncpy(out_buf, (char*)rx_buffer, 50);
        rx_ready = 0; // Xóa cờ sau khi main đã lấy data
    }
}
