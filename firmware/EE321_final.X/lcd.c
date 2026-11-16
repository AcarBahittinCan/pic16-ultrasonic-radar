/*
 * lcd.c  —  HD44780 4-bit LCD sürücüsü
 * PIC16F877A + XC8, 20 MHz
 *
 * Bağlantı:
 *   VSS → GND, VDD → +5V, V0 → 10k pot orta ucu
 *   RS  → RB2, RW  → GND (sadece yazıyoruz)
 *   EN  → RB3
 *   D4  → RB4, D5 → RB5, D6 → RB6, D7 → RB7
 */

#include "lcd.h"

#ifndef _XTAL_FREQ
  #define _XTAL_FREQ 20000000UL
#endif

// ============================================================
//  Dahili yardımcılar
// ============================================================

/* 4 veri bitini porta yaz */
static void lcd_nibble(uint8_t nibble) {
    LCD_D4 = (nibble >> 0) & 1;
    LCD_D5 = (nibble >> 1) & 1;
    LCD_D6 = (nibble >> 2) & 1;
    LCD_D7 = (nibble >> 3) & 1;
}

/* EN darbesi */
static void lcd_pulse(void) {
    LCD_EN = 1;
    __delay_us(1);
    LCD_EN = 0;
    __delay_us(50);   // komut işlenme süresi
}

/* Tam bir byte gönder (RS=0 komut, RS=1 veri) */
static void lcd_send(uint8_t data, uint8_t rs) {
    LCD_RS = rs;
    lcd_nibble(data >> 4);   // üst nibble önce
    lcd_pulse();
    lcd_nibble(data & 0x0F); // alt nibble
    lcd_pulse();
}

// ============================================================
//  Satır başı adresleri  (2x16 LCD standart)
// ============================================================
static uint8_t lcd_row_addr(uint8_t row) {
    switch (row) {
        case 1:  return 0x00;
        case 2:  return 0x40;
        case 3:  return 0x14;   // 4-satırlı LCD için
        case 4:  return 0x54;
        default: return 0x00;
    }
}

// ============================================================
//  Public API
// ============================================================

void Lcd_Init(void) {
    /* Tüm LCD pinlerini çıkış yap */
    LCD_RS_DIR = 0;
    LCD_EN_DIR = 0;
    LCD_D4_DIR = 0;
    LCD_D5_DIR = 0;
    LCD_D6_DIR = 0;
    LCD_D7_DIR = 0;

    LCD_RS = 0;
    LCD_EN = 0;

    __delay_ms(50);   // güç stabilizasyonu

    /* 8-bit moddan 4-bit moda geçiş (datasheet prosedürü) */
    lcd_nibble(0x03); lcd_pulse(); __delay_ms(5);
    lcd_nibble(0x03); lcd_pulse(); __delay_us(150);
    lcd_nibble(0x03); lcd_pulse(); __delay_us(150);
    lcd_nibble(0x02); lcd_pulse(); // 4-bit moda gir

    /* Fonksiyon ayarı: 4-bit, 2 satır, 5x8 font */
    lcd_send(0x28, 0); __delay_us(50);
    /* Display kapalı */
    lcd_send(0x08, 0); __delay_us(50);
    /* Temizle */
    lcd_send(0x01, 0); __delay_ms(2);
    /* Giriş modu: cursor sağa, shift yok */
    lcd_send(0x06, 0); __delay_us(50);
    /* Display açık, cursor kapalı */
    lcd_send(0x0C, 0); __delay_us(50);
}

void Lcd_Cmd(uint8_t cmd) {
    if (cmd == _LCD_CLEAR) {
        lcd_send(0x01, 0);
        __delay_ms(2);          // clear komutu ~1.6ms sürer
    } else {
        lcd_send(cmd, 0);
        __delay_us(50);
    }
}

void Lcd_Chr(uint8_t row, uint8_t col, char ch) {
    /* DDRAM adresini ayarla */
    uint8_t addr = lcd_row_addr(row) + (col - 1);
    lcd_send(0x80 | addr, 0);
    __delay_us(50);
    /* Karakteri yaz */
    lcd_send((uint8_t)ch, 1);
    __delay_us(50);
}

void Lcd_Out(uint8_t row, uint8_t col, const char *str) {
    /* Başlangıç adresini ayarla */
    uint8_t addr = lcd_row_addr(row) + (col - 1);
    lcd_send(0x80 | addr, 0);
    __delay_us(50);
    /* String yaz */
    while (*str) {
        lcd_send((uint8_t)*str++, 1);
        __delay_us(50);
    }
}
