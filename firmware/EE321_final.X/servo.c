/*
 * servo.c - SG90 Servo Kutuphanesi (XC8 / PIC16F877A)
 *
 * Orijinal: ServoC.cpp - Infineon Technologies AG (Arduino)
 * XC8 portunu yapan: EE321 Radar Projesi
 *
 * Timer1 kullanimi:
 *   Fosc/4 = 5 MHz, 1:1 prescaler -> 1 tick = 0.2 us
 *   540  us =  2700 tick  (0   derece)
 *   2450 us = 12250 tick  (180 derece)
 *   timer_load = 65536 - ticks
 */

#include "servo.h"

// ============================================================
//  servo_attach()
//  Arduino: pinMode(servo_pin, OUTPUT)
// ============================================================
void servo_attach(void) {
    CCP2CON      = 0x00;   // RC1 uzerindeki CCP2 peripheral'i kapat
    SERVO_PIN_DIR = 0;     // RC1 cikti
    SERVO_PIN     = 0;     // baslangicta dusuk
}

// ============================================================
//  servo_delay_us()
//  Arduino: delayUs() -> micros() ile busy-wait
//  XC8    : Timer1 ile donanim tabanli bekleme
//           Safety timeout: Timer1 donerse dongudan cik
// ============================================================
void servo_delay_us(uint16_t us) {
    uint16_t ticks     = (uint16_t)((uint32_t)us * 5UL);  // 1 us = 5 tick (5MHz)
    uint16_t load      = (uint16_t)(65536UL - ticks);
    uint16_t safety    = 0;

    TMR1H = (uint8_t)(load >> 8);
    TMR1L = (uint8_t)(load & 0xFF);
    PIR1bits.TMR1IF = 0;
    T1CON = 0x01;   // Timer1 ON, 1:1 prescaler, Fosc/4

    while (!PIR1bits.TMR1IF) {
        if (++safety > 60000) break;   // ~12ms max guvenlik
    }

    //T1CON = 0x00;
}

// ============================================================
//  servo_write()
//  Arduino: write(angle)
//
//  map(angle, 0, 180, 540, 2450):
//    pulse_us = 540 + (angle * (2450 - 540)) / 180
//             = 540 + (angle * 1910) / 180
//
//  constrain(angle, 0, 180):
//    manuel sinir kontrolu
// ============================================================
void servo_write(uint8_t angle) {
    uint16_t pulse_us;
    uint16_t rest_us;

    /* constrain */
    if (angle < SERVO_MIN_ANGLE) angle = SERVO_MIN_ANGLE;
    if (angle > SERVO_MAX_ANGLE) angle = SERVO_MAX_ANGLE;

    /* map: 0 derece -> 540us, 180 derece -> 2450us */
    pulse_us = SERVO_MIN_PULSE_US +
               (uint16_t)(((uint32_t)angle *
               (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US)) / SERVO_MAX_ANGLE);

    rest_us = 20000U - pulse_us;   // 20ms period tamamla

    /* digitalWrite(HIGH) */
    SERVO_PIN = 1;
    servo_delay_us(pulse_us);

    /* digitalWrite(LOW) */
    SERVO_PIN = 0;
    servo_delay_us(rest_us);
}
