#include <delay.h>
#include <gpio.h>
#include <stm32.h>
#include <stdbool.h>

// ------------------ PRZYCISKI ------------------
// ------------------ AKTYWNY 0 ------------------

#define USER_BTN_GPIO GPIOC
#define USER_BTN_PIN 13

#define LEFT_BTN_GPIO GPIOB
#define LEFT_BTN_PIN 3
#define RIGHT_BTN_GPIO GPIOB
#define RIGHT_BTN_PIN 4
#define UP_BTN_GPIO GPIOB
#define UP_BTN_PIN 5
#define DOWN_BTN_GPIO GPIOB
#define DOWN_BTN_PIN 6
#define ACTION_BTN_GPIO GPIOB
#define ACTION_BTN_PIN 10

// ------------------ AKTYWNY 1 ------------------

#define AT_BTN_GPIO GPIOA
#define AT_BTN_PIN 0

// ------------------ USART ------------------

#define USART_Mode_Rx_Tx (USART_CR1_RE | USART_CR1_TE)
#define USART_Enable USART_CR1_UE
#define USART_WordLength_8b 0x0000
#define USART_Parity_No 0x0000
#define USART_FlowControl_None 0x0000
#define USART_StopBits_1 0x0000     // Liczba bitów przerwy między komunikatami.
#define HSI_HZ 16000000U
#define PCLK1_HZ HSI_HZ             // Zmienne do ustalenia prędkości przesyłania danych.
#define BAUD 9600U

// ------------------ USART ------------------


#define BUFFOR_SIZE 10000

struct cyclic_buffer {
    char* start;
    char* end;
    char* head;
    char* tail;
    unsigned size;
};

typedef struct cyclic_buffer cyclic_buffer;


cyclic_buffer c;

void init(cyclic_buffer* c) {
    static char storage[BUFFOR_SIZE];  
    c->size = BUFFOR_SIZE;
    c->start = storage;
    c->end = c->start + c->size;
    c->head = c->start;
    c->tail = c->start;
}

bool is_empty(cyclic_buffer* c) {
    if(c->tail == c->head) {
        return true;
    }
    return false;
}

char* next_in_buffer(cyclic_buffer* c, char* pointer) {
    if(pointer + 1 == c->end) {
        return c->start;
    }
    return pointer + 1;
}

void push_back(cyclic_buffer* c, const char* s) {
    while(*s != '\0') {
        *(c->head) = *s;
        s++;
        char* next = next_in_buffer(c, c->head);
        if(next == c->tail) {
            break;
        }
        c->head = next;
    }
}

char pop_front(cyclic_buffer* c) {
    char rez = *(c->tail);
    c->tail = next_in_buffer(c, c->tail);
    return rez;
}

char* get_tail(cyclic_buffer* c) {
    return c->tail;
}

unsigned get_first_message_length(cyclic_buffer* c) {
    char* find_end = c->tail;
    unsigned length = 1;
    while(*find_end != '\n') {
        find_end = next_in_buffer(c, find_end);
        length++;
    }
    return length;
}

void pop_front_x_bytes(cyclic_buffer* c, unsigned pop_n) {
    for(unsigned i = 0; i < pop_n; i++) {
        pop_front(c);
    }
}

void left_pressed(cyclic_buffer* c) {
    if((LEFT_BTN_GPIO->IDR >> LEFT_BTN_PIN) & 1) {
        push_back(c, "LEFT RELEASED\r\n");
    }
    else {
        push_back(c, "LEFT PRESSED\r\n");
    }
}

void right_pressed(cyclic_buffer* c) {
    if((RIGHT_BTN_GPIO->IDR >> RIGHT_BTN_PIN) & 1) {
        push_back(c, "RIGHT RELEASED\r\n");
    }
    else {
        push_back(c, "RIGHT PRESSED\r\n");
    }
}

void up_pressed(cyclic_buffer* c) {
    if((UP_BTN_GPIO->IDR >> UP_BTN_PIN) & 1) {
        push_back(c, "UP RELEASED\r\n");
    }
    else {
        push_back(c, "UP PRESSED\r\n");
    }
}

void down_pressed(cyclic_buffer* c) {
    if((DOWN_BTN_GPIO->IDR >> DOWN_BTN_PIN) & 1) {
        push_back(c, "DOWN RELEASED\r\n");
    }
    else {
        push_back(c, "DOWN PRESSED\r\n");
    }
  
}

void action_pressed(cyclic_buffer* c) {
    if((ACTION_BTN_GPIO->IDR >> ACTION_BTN_PIN) & 1) {
        push_back(c, "FIRE RELEASED\r\n");
    }
    else {
        push_back(c, "FIRE PRESSEDD\r\n");
    }
}

void user_pressed(cyclic_buffer* c) {
    if((USER_BTN_GPIO->IDR >> USER_BTN_PIN) & 1) {
        push_back(c, "USER RELEASED\r\n");
    }
    else {
        push_back(c, "USER PRESSED\r\n");
    }
}

void mode_pressed(cyclic_buffer* c) {
    if(!((AT_BTN_GPIO->IDR >> AT_BTN_PIN) & 1)) {
        push_back(c, "MODE RELEASED\r\n");
    }
    else {
        push_back(c, "MODE PRESSED\r\n");
    }
}

void send_message(cyclic_buffer* c) {
    DMA1_Stream6->M0AR = (uint32_t)get_tail(c);
    DMA1_Stream6->NDTR = get_first_message_length(c);
    DMA1_Stream6->CR |= DMA_SxCR_EN;
    
}

// Funkcja ta wywołuje się asynchronicznie w stosunku do mojego programu, nie mogą jej wywoływać.
// Funkcja ta wywoluje sie gdy nastepuje przerwanie.
void DMA1_Stream6_IRQHandler() { 
    uint32_t isr = DMA1->HISR;          // Interrupt Status Register, ustawiony na 1 jeżeli przesłanie zakończyło się sukcesem
    if (isr & DMA_HISR_TCIF6) {
        pop_front_x_bytes(&c, get_first_message_length(&c));

        DMA1->HIFCR = DMA_HIFCR_CTCIF6;         //Interrput Fluck Clear Register -> zeruje HISR.
        
        if(!is_empty(&c)) {
            send_message(&c);
        }
    }
}

void check_if_available() {
    // ten warunek sprawdać gdy przycisk zgłasza przerwania i jeżeli DMA nie jest zajęte to od razu
    // dajemy do DMA a w.p.p kolejkujemy.
    if((DMA1_Stream6->CR & DMA_SxCR_EN) == 0 && (DMA1->HISR & DMA_HISR_TCIF6) == 0) {
        send_message(&c);
    }
}


void EXTI0_IRQHandler(void) {
    EXTI->PR = EXTI_PR_PR0;
    mode_pressed(&c);
    check_if_available();
}

void EXTI3_IRQHandler(void) {
    EXTI->PR = EXTI_PR_PR3;
    left_pressed(&c);
    check_if_available();
}

void EXTI4_IRQHandler(void) {
    EXTI->PR = EXTI_PR_PR4;
    right_pressed(&c);
    check_if_available();
}

void EXTI9_5_IRQHandler(void) {
    if(EXTI->PR & EXTI_PR_PR5) {
        EXTI->PR = EXTI_PR_PR5;
        up_pressed(&c);
    }
    else {
        EXTI->PR = EXTI_PR_PR6;
        down_pressed(&c);
    }

    check_if_available();
}

void EXTI15_10_IRQHandler(void) {
    if(EXTI->PR & EXTI_PR_PR13) {
        EXTI->PR = EXTI_PR_PR13;
        user_pressed(&c);
    }
    else {
        EXTI->PR = EXTI_PR_PR10;
        action_pressed(&c);
    }

    check_if_available();
}

int main() {

    init(&c);

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
    RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_DMA1EN;

    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    

    GPIOafConfigure(GPIOA,
        2,
        GPIO_OType_PP,
        GPIO_Fast_Speed,
        GPIO_PuPd_NOPULL,
        GPIO_AF_USART2);
    
    GPIOafConfigure(GPIOA,
        3,
        GPIO_OType_PP,
        GPIO_Fast_Speed,
        GPIO_PuPd_UP,
        GPIO_AF_USART2); 
    
    // Trzeba skonfigurować przerwania dla odpowiedniego układu i linii na które jest guzik.
    // Co jeżeli zrobię najpierw falling a potem Rising?
    GPIOinConfigure(USER_BTN_GPIO, USER_BTN_PIN, GPIO_PuPd_UP, EXTI_Mode_Interrupt, EXTI_Trigger_Rising_Falling);
    GPIOinConfigure(LEFT_BTN_GPIO, LEFT_BTN_PIN, GPIO_PuPd_UP, EXTI_Mode_Interrupt, EXTI_Trigger_Rising_Falling);
    GPIOinConfigure(RIGHT_BTN_GPIO, RIGHT_BTN_PIN, GPIO_PuPd_UP, EXTI_Mode_Interrupt, EXTI_Trigger_Rising_Falling);
    GPIOinConfigure(DOWN_BTN_GPIO, DOWN_BTN_PIN, GPIO_PuPd_UP, EXTI_Mode_Interrupt, EXTI_Trigger_Rising_Falling);
    GPIOinConfigure(UP_BTN_GPIO, UP_BTN_PIN, GPIO_PuPd_UP, EXTI_Mode_Interrupt, EXTI_Trigger_Rising_Falling);
    GPIOinConfigure(AT_BTN_GPIO, AT_BTN_PIN, GPIO_PuPd_UP, EXTI_Mode_Interrupt, EXTI_Trigger_Rising_Falling);
    GPIOinConfigure(ACTION_BTN_GPIO, ACTION_BTN_PIN, GPIO_PuPd_UP, EXTI_Mode_Interrupt, EXTI_Trigger_Rising_Falling);
    

    USART2->CR1 = USART_Mode_Rx_Tx | USART_WordLength_8b | USART_Parity_No;
    USART2->CR2 = USART_StopBits_1;
    USART2->CR3 = USART_CR3_DMAT | USART_CR3_DMAR;
    USART2->BRR = (PCLK1_HZ + (BAUD / 2U)) / BAUD;

    DMA1_Stream6->CR = 4U << 25 |
        DMA_SxCR_PL_1 |
        DMA_SxCR_MINC |
        DMA_SxCR_DIR_0 |
        DMA_SxCR_TCIE;

    DMA1_Stream6->PAR = (uint32_t)&USART2->DR;

    DMA1_Stream5->CR = 4U << 25 |
        DMA_SxCR_PL_1 |
        DMA_SxCR_MINC |
        DMA_SxCR_TCIE;
    
    DMA1_Stream5->PAR = (uint32_t)&USART2->DR;

    DMA1->HIFCR = DMA_HIFCR_CTCIF6 | DMA_HIFCR_CTCIF5;

    // Zerujemy bity przerwań, które mogły się zapalić przy odpalaniu programu (ich stan nie jest znany).
    EXTI->PR = EXTI_PR_PR0;
    EXTI->PR = EXTI_PR_PR10;
    EXTI->PR = EXTI_PR_PR13;
    EXTI->PR = EXTI_PR_PR3;
    EXTI->PR = EXTI_PR_PR4;
    EXTI->PR = EXTI_PR_PR5;
    EXTI->PR = EXTI_PR_PR6;

    NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    NVIC_EnableIRQ(DMA1_Stream5_IRQn);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
    NVIC_EnableIRQ(EXTI0_IRQn);
    NVIC_EnableIRQ(EXTI3_IRQn);
    NVIC_EnableIRQ(EXTI4_IRQn);
    NVIC_EnableIRQ(EXTI9_5_IRQn);

    USART2->CR1 |= USART_Enable;        // włączenie UARTU na koniec.

    return 0;
}
