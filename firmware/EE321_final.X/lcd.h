#ifndef LCD_H
#define LCD_H

#include <xc.h>
#include <stdint.h>

// ============================================================
//  LCD Pin Tanımları  (radar_main.c ile uyumlu)
//  Değiştirmek istersen sadece bu bölümü düzenle
// ============================================================
#define LCD_RS      RB2
#define LCD_EN      RB3
#define LCD_D4      RB4
#define LCD_D5      RB5
#define LCD_D6      RB6
#define LCD_D7      RB7

#define LCD_RS_DIR  TRISB2
#define LCD_EN_DIR  TRISB3
#define LCD_D4_DIR  TRISB4
#define LCD_D5_DIR  TRISB5
#define LCD_D6_DIR  TRISB6
#define LCD_D7_DIR  TRISB7

// ============================================================
//  Komut Sabitleri  (MikroC uyumlu isimler)
// ============================================================
#define _LCD_CLEAR       0x01
#define _LCD_RETURN_HOME 0x02
#define _LCD_CURSOR_OFF  0x0C   // Display ON, cursor OFF, blink OFF
#define _LCD_CURSOR_ON   0x0E
#define _LCD_SHIFT_LEFT  0x18
#define _LCD_SHIFT_RIGHT 0x1C

// ============================================================
//  Fonksiyon Prototipleri
// ============================================================
void Lcd_Init(void);
void Lcd_Cmd(uint8_t cmd);
void Lcd_Chr(uint8_t row, uint8_t col, char ch);
void Lcd_Out(uint8_t row, uint8_t col, const char *str);

#endif // LCD_H
