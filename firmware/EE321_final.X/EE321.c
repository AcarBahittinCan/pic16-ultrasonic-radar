#include <xc.h>
/*
 * EE321 Radar - MPLAB X + XC8 Versiyonu
 * PIC16F877A, 20MHz, HD44780 LCD (4-bit), HC-SR04, SG90 Servo
 *
 * LCD Pinleri: RB2=RS, RB3=EN, RB4-RB7=D4-D7
 * TRIGGER: RD0, ECHO: RC2, SERVO: RC1
 * STOP_BTN: RB0, LED_BLUE: RD1, BUZZER: RC0
 * LED_GREEN: RC3, LED_YELLOW: RC4, LED_RED: RC5
 */

// CONFIG BITS - 20MHz kristal, watchdog kapali
#pragma config FOSC  = HS
#pragma config WDTE  = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP   = OFF
#pragma config CPD   = OFF
#pragma config WRT   = OFF
#pragma config CP    = OFF

#include <stdint.h>
#include "lcd.h"
#include "servo.h"      // SG90 servo kutuphanesi

#define _XTAL_FREQ 20000000UL

// ---------- Pin Tanimlari ----------
#define TRIGGER     RD0
#define ECHO        RC2
#define STOP_BTN    RB0
#define LED_BLUE    RD1
#define BUZZER      RC0
#define LED_GREEN   RC3
#define LED_YELLOW  RC4
#define LED_RED     RC5
// SERVO = RC1, servo.h icinde SERVO_PIN olarak tanimli

// ---------- Global Degiskenler ----------
static uint8_t  system_stopped = 0;

// Forward declarations
void update_alerts(uint16_t distance);
void reset_alerts(void);

// ================================================================
//  UART
// ================================================================
void uart_init(void) {
    TRISC6 = 0;
    TRISC7 = 1;
    SPBRG  = 129;
    TXSTA  = 0x24;
    RCSTA  = 0x90;
}

void uart_putc(uint8_t c) {
    while (!TXIF);
    TXREG = c;
}

void uart_puts(const char *s) {
    while (*s) uart_putc((uint8_t)*s++);
}

void uart_put_num3(uint16_t num) {
    if (num >= 1000) num = 999;
    uart_putc((num / 100)       + '0');
    uart_putc(((num / 10) % 10) + '0');
    uart_putc((num % 10)        + '0');
}

void send_telemetry(uint16_t angle, uint16_t distance) {
    uart_puts("A:");
    uart_put_num3(angle);
    uart_puts(",D:");
    if (distance == 999) uart_puts("---");
    else                 uart_put_num3(distance);
    uart_putc('\r');
    uart_putc('\n');
}

// ================================================================
//  LCD
// ================================================================
void lcd_print_num(uint8_t row, uint8_t col, uint16_t num) {
    uint8_t h, t, o;
    if (num >= 1000) num = 999;
    h = num / 100;
    t = (num / 10) % 10;
    o =  num % 10;
    Lcd_Chr(row, col,   (h == 0) ? ' ' : (h + '0'));
    Lcd_Chr(row, col+1, (h == 0 && t == 0) ? ' ' : (t + '0'));
    Lcd_Chr(row, col+2, o + '0');
}

void lcd_draw_static(void) {
    Lcd_Cmd(_LCD_CLEAR);
    Lcd_Out(1, 1, "ANG:");
    Lcd_Out(1, 9, "DIST:");
    Lcd_Out(2, 2, "EE321  PROJECT");
}

void update_lcd(uint16_t angle, uint16_t distance) {
    lcd_print_num(1, 5, angle);
    if (distance == 999) Lcd_Out(1, 14, "---");
    else                 lcd_print_num(1, 14, distance);
}

// ================================================================
//  Alert LEDleri + Buzzer (State Tracking)
// ================================================================
void reset_alerts(void) {
    LED_GREEN = 0; LED_YELLOW = 0; LED_RED = 0; BUZZER = 1;
    update_alerts(0xFFFF);
}

void update_alerts(uint16_t distance) {
    static uint8_t current_state = 255;
    uint8_t target_state;

    if (distance == 0xFFFF) {
        current_state = 255;
        return;
    }

    if      (distance < 10) target_state = 0;
    else if (distance < 25) target_state = 1;
    else                    target_state = 2;

    if (current_state == target_state) return;
    current_state = target_state;

    if (current_state == 0) {
        LED_GREEN = 0; LED_YELLOW = 0; LED_RED = 1; BUZZER = 0;
    } else if (current_state == 1) {
        LED_GREEN = 0; LED_YELLOW = 1; LED_RED = 0; BUZZER = 1;
    } else {
        LED_GREEN = 1; LED_YELLOW = 0; LED_RED = 0; BUZZER = 1;
    }
}

// ================================================================
//  HC-SR04
//  T1CON = 0x31: ON, 1:8 prescaler, Fosc/4
//  1 tick = 1.6 us  ->  dist(cm) = ticks / 36 (~ 58 / 1.6)
// ================================================================
uint16_t read_distance(void) {
    uint16_t timeout;
    uint32_t ticks;

    TRIGGER = 0;
    __delay_us(2);
    TRIGGER = 1;
    __delay_us(10);
    TRIGGER = 0;

    timeout = 0;
    while (!ECHO) {
        if (++timeout > 30000) return 999;
    }

    TMR1H = 0;
    TMR1L = 0;
    T1CON = 0x31;   // ON, 1:8 prescaler, internal

    timeout = 0;
    while (ECHO) {
        if (++timeout > 30000) { T1CON = 0x00; return 999; }
    }
    T1CON = 0x00;

    ticks = ((uint32_t)TMR1H << 8) | TMR1L;
    uint16_t dist = (uint16_t)(ticks / 36);

    if (dist > 400) return 999;
    if (dist < 2)   return 0;
    return dist;
}

// ================================================================
//  Stop Butonu
// ================================================================
void check_stop_toggle(void) {
    if (!STOP_BTN) {
        __delay_ms(30);
        if (!STOP_BTN) {
            system_stopped ^= 1;

            if (system_stopped) {
                reset_alerts();
                SERVO_PIN = 0; TRIGGER = 0;
                LED_BLUE = 1;
                Lcd_Cmd(_LCD_CLEAR);
                Lcd_Out(1, 1, "STOPPED");
                Lcd_Out(2, 1, "PRESS TO RUN");
            } else {
                LED_BLUE = 0;
                Lcd_Cmd(_LCD_CLEAR);
                Lcd_Out(1, 1, "RUNNING");
                __delay_ms(500);
                lcd_draw_static();
            }
            while (!STOP_BTN);
            __delay_ms(30);
        }
    }
}

void wait_if_stopped(void) {
    while (system_stopped) {
        check_stop_toggle();
        __delay_ms(20);
    }
}

// ================================================================
//  MAIN
// ================================================================
void main(void) {
    ADCON1  = 0x06;  // tum portlar dijital
    CMCON   = 0x07;  // komparator kapali
    CCP1CON = 0x00;  // RC2 (ECHO)  - CCP1 kapat
    CCP2CON = 0x00;  // RC1 (SERVO) - CCP2 kapat
    SSPCON  = 0x00;  // RC3 (LED_GREEN) - SSP kapat
    TRISE   = 0x00;  // RD0/RD1 - PSP kapat

    TRISB  = 0xFF;   // LCD kutuphanesi kendi pinlerini ayarlar
    TRISD  = 0x00;
    TRISC  = 0x84;   // RC2(ECHO)=giris, RC7(RX)=giris, gerisi cikti

    PORTC  = 0x00;
    PORTD  = 0x00;

    OPTION_REGbits.nRBPU = 0;  // RB pull-up aktif (STOP_BTN)
    system_stopped = 0;

    // Servo baslatma - pin cikti yap, merkeze al
    servo_attach();
    servo_write(90);
    __delay_ms(500);

    // LCD
    Lcd_Init();
    Lcd_Cmd(_LCD_CLEAR);
    Lcd_Cmd(_LCD_CURSOR_OFF);
    Lcd_Out(1, 1, "EE321 RADAR");
    Lcd_Out(2, 1, "Starting...");
    __delay_ms(1000);
    lcd_draw_static();

    // UART
    uart_init();
    __delay_ms(100);
    uart_puts("EE321 RADAR Started\r\n");

    uint8_t angle     = 0;
    uint8_t direction = 1;
    uint16_t distance;

    while (1) {
        if (direction) {
            if (angle < 180) angle += 5;
            else             direction = 0;
        } else {
            if (angle > 0)   angle -= 5;
            else             direction = 1;
        }

        check_stop_toggle();
        wait_if_stopped();

        servo_write(angle);
        __delay_ms(30);
        distance = read_distance();

        update_alerts(distance);
        update_lcd(angle, distance);
        send_telemetry(angle, distance);
    }
}

