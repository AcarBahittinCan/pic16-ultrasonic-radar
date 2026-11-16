/*
 * servo.h - SG90 Servo Kutuphanesi (XC8 / PIC16F877A)
 *
 * Orijinal: ServoC.cpp - Infineon Technologies AG (Arduino)
 * XC8 portunu yapan: EE321 Radar Projesi
 *
 * Kullanim:
 *   servo_attach();          // pin cikti yap, CCP2 kapat
 *   servo_write(90);         // 90 dereceye git
 */

#ifndef SERVO_H
#define SERVO_H

#include <xc.h>
#include <stdint.h>

#ifndef _XTAL_FREQ
  #define _XTAL_FREQ 20000000UL
#endif

// ============================================================
//  Servo Pin Tanimi  (degistirmek istersen buraya doku)
// ============================================================
#define SERVO_PIN       RC1
#define SERVO_PIN_DIR   TRISC1

// ============================================================
//  Pulse Genislik Sinirlari  (SG90 datasheet degerleri)
// ============================================================
#define SERVO_MIN_ANGLE        0     // minimum aci (derece)
#define SERVO_MAX_ANGLE        180   // maksimum aci (derece)
#define SERVO_MIN_PULSE_US     540   // 0   derece icin pulse genisligi (us)
#define SERVO_MAX_PULSE_US     2450  // 180 derece icin pulse genisligi (us)

// ============================================================
//  Fonksiyon Prototipleri
// ============================================================

/* Servo pinini cikti yap, CCP2'yi kapat */
void servo_attach(void);

/* Servo'yu verilen aciya gonder (0-180 derece) */
void servo_write(uint8_t angle);

/* Dahili: us cinsinden bekleme (Timer1 tabanli) */
void servo_delay_us(uint16_t us);

#endif // SERVO_H