#include <delay.h>
#include <gpio.h>
#include <stm32.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

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

// ------------------ DIODY ------------------

#define RED_LED_GPIO GPIOA
#define GREEN_LED_GPIO GPIOA
#define BLUE_LED_GPIO GPIOB
#define GREEN2_LED_GPIO GPIOA
#define RED_LED_PIN 6
#define GREEN_LED_PIN 7
#define BLUE_LED_PIN 0
#define GREEN2_LED_PIN 5

#define RedLEDon() \
    RED_LED_GPIO->BSRR = 1 << (RED_LED_PIN + 16)
#define RedLEDoff() \
    RED_LED_GPIO->BSRR = 1 << RED_LED_PIN

#define GreenLEDon() \
    GREEN_LED_GPIO->BSRR = 1 << (GREEN_LED_PIN + 16)
#define GreenLEDoff() \
    GREEN_LED_GPIO->BSRR = 1 << GREEN_LED_PIN

#define BlueLEDon() \
    BLUE_LED_GPIO->BSRR = 1 << (BLUE_LED_PIN + 16)
#define BlueLEDoff() \
    BLUE_LED_GPIO->BSRR = 1 << BLUE_LED_PIN

#define Green2LEDon() \
    GREEN2_LED_GPIO->BSRR = 1 << GREEN2_LED_PIN
#define Green2LEDoff() \
    GREEN2_LED_GPIO->BSRR = 1 << (GREEN2_LED_PIN + 16)


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
    //return 15;
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

bool left_pressed(cyclic_buffer* c, bool was_pressed) {
    if((LEFT_BTN_GPIO->IDR >> LEFT_BTN_PIN) & 1) {
        if(was_pressed) {
            push_back(c, "LEFT RELEASED\r\n");
        }
        return false;
    }
    if(!was_pressed) {
        push_back(c, "LEFT PRESSED\r\n");
    }
    return true;
}

bool right_pressed(cyclic_buffer* c, bool was_pressed) {
    if((RIGHT_BTN_GPIO->IDR >> RIGHT_BTN_PIN) & 1) {
        if(was_pressed) {
            push_back(c, "RIGHT RELEASED\r\n");
        }
        return false;
    }
    if(!was_pressed) {
        push_back(c, "RIGHT PRESSED\r\n");
    }
    return true;
}

bool up_pressed(cyclic_buffer* c, bool was_pressed) {
    if((UP_BTN_GPIO->IDR >> UP_BTN_PIN) & 1) {
        if(was_pressed) {
            push_back(c, "UP RELEASED\r\n");
        }
        return false;
    }
    if(!was_pressed) {
        push_back(c, "UP PRESSED\r\n");
    }
    return true;
}

bool down_pressed(cyclic_buffer* c, bool was_pressed) {
    if((DOWN_BTN_GPIO->IDR >> DOWN_BTN_PIN) & 1) {
        if(was_pressed) {
            push_back(c, "DOWN RELEASED\r\n");
        }
        return false;
    }
    if(!was_pressed) {
        push_back(c, "DOWN PRESSED\r\n");
    }
    return true;
}

bool action_pressed(cyclic_buffer* c, bool was_pressed) {
    if((ACTION_BTN_GPIO->IDR >> ACTION_BTN_PIN) & 1) {
        if(was_pressed) {
            push_back(c, "FIRE RELEASED\r\n");
        }
        return false;
    }
    if(!was_pressed) {
        push_back(c, "FIRE PRESSEDD\r\n");
    }
    return true;
}

bool user_pressed(cyclic_buffer* c, bool was_pressed) {
    if((USER_BTN_GPIO->IDR >> USER_BTN_PIN) & 1) {
        if(was_pressed) {
            push_back(c, "USER RELEASED\r\n");
        }
        return false;
    }
    if(!was_pressed) {
        push_back(c, "USER PRESSED\r\n");
    }
    return true;
}

bool mode_pressed(cyclic_buffer* c, bool was_pressed) {
    if(!((AT_BTN_GPIO->IDR >> AT_BTN_PIN) & 1)) {
        if(was_pressed) {
            push_back(c, "MODE RELEASED\r\n");
        }
        return false;
    }
    if(!was_pressed) {
        push_back(c, "MODE PRESSED\r\n");
    }
    return true;
}


void check_buttons(cyclic_buffer* c, bool* b_pressed) {
    if(left_pressed(c, b_pressed[LEFT_BTN_PIN])) { b_pressed[LEFT_BTN_PIN] = true; } else { b_pressed[LEFT_BTN_PIN] = false; }
    if(right_pressed(c, b_pressed[RIGHT_BTN_PIN])) { b_pressed[RIGHT_BTN_PIN] = true; } else { b_pressed[RIGHT_BTN_PIN] = false; }
    if(up_pressed(c, b_pressed[UP_BTN_PIN])) { b_pressed[UP_BTN_PIN] = true; } else { b_pressed[UP_BTN_PIN] = false; }
    if(down_pressed(c, b_pressed[DOWN_BTN_PIN])) { b_pressed[DOWN_BTN_PIN] = true; } else { b_pressed[DOWN_BTN_PIN] = false; }
    if(mode_pressed(c, b_pressed[AT_BTN_PIN])) { b_pressed[AT_BTN_PIN] = true; } else { b_pressed[AT_BTN_PIN] = false; }
    if(user_pressed(c, b_pressed[USER_BTN_PIN])) { b_pressed[USER_BTN_PIN] = true; } else { b_pressed[USER_BTN_PIN] = false; }
    if(action_pressed(c, b_pressed[ACTION_BTN_PIN])) { b_pressed[ACTION_BTN_PIN] = true; } else { b_pressed[ACTION_BTN_PIN] = false; }
}

void send_message(cyclic_buffer* c) {
    if((DMA1_Stream6->CR & DMA_SxCR_EN) == 0 && (DMA1->HISR & DMA_HISR_TCIF6) == 0) {
        DMA1_Stream6->M0AR = (uint32_t)get_tail(c);
        DMA1_Stream6->NDTR = get_first_message_length(c);
        DMA1_Stream6->CR |= DMA_SxCR_EN;
    }
    
}

// Funkcja ta wywołuje się asynchronicznie w stosunku do mojego programu, nie mogą jej wywoływać.
// Funkcja ta wywoluje sie gdy nastepuje przerwanie.
void DMA1_Stream6_IRQHandler(cyclic_buffer* c, bool* transmission_available, bool* queued) { 
    uint32_t isr = DMA1->HISR;          // Interrupt Status Register, ustawiony na 1 jeżeli przesłanie zakończyło się sukcesem
    if (isr & DMA_HISR_TCIF6) {
        pop_front_x_bytes(c, get_first_message_length(c));

        DMA1->HIFCR = DMA_HIFCR_CTCIF6;         //Interrput Fluck Clear Register -> zeruje HISR.
        
        if(!is_empty(c)) {
            send_message(c);
        }
    }
}


int main() {
    
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
    RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_DMA1EN;

    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

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
    NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    NVIC_EnableIRQ(DMA1_Stream5_IRQn);


    USART2->CR1 |= USART_Enable;        // włączenie UARTU na koniec




    cyclic_buffer c;
    bool buttons_pressed[20];
    memset(buttons_pressed, false, sizeof(buttons_pressed));
    bool transmission_available = true;
    bool queued = false;

    bool was_empty = false;

    init(&c);
    for(;;) {
        check_buttons(&c, buttons_pressed);

        if(!is_empty(&c) && was_empty) {
            was_empty = false;
            send_message(&c);
        }

        if(is_empty(&c)) {
            was_empty = true;
        }
    }
        

    return 0;
}
