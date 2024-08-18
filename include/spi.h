#ifndef SPI_H
#define SPI_H

#include <avr/io.h>
#include <util/delay.h>

#define TFT_CS     10
#define TFT_RST    7
#define TFT_DC     8
#define TFT_SCK    13
#define TFT_MOSI   11

#define ST77XX_BLACK   0x0000
#define ST77XX_WHITE   0xFFFF
#define ST77XX_RED     0xF800
#define ST77XX_BLUE    0x001F
#define ST77XX_GREEN   0x07E0

int16_t abs(int16_t x) {
    return x < 0 ? -x : x;
}

void SPI_init() {
    DDRB |= (1 << DDB3) | (1 << DDB5) | (1 << DDB2); // MOSI, SCK, SS as output
    SPCR |= (1 << SPE) | (1 << MSTR) | (1 << SPR0);  // Enable SPI, Master mode, fosc/16
}

void SPI_send(uint8_t data) {
    SPDR = data;
    while (!(SPSR & (1 << SPIF)));
}

void sendCommand(uint8_t command) {
    PORTB &= ~(1 << PB2); // CS low
    PORTB &= ~(1 << PB0); // DC low
    SPI_send(command);
    PORTB |= (1 << PB2);  // CS high
}

void sendData(uint8_t data) {
    PORTB &= ~(1 << PB2); // CS low
    PORTB |= (1 << PB0);  // DC high
    SPI_send(data);
    PORTB |= (1 << PB2);  // CS high
}

void TFT_reset() {
    PORTD &= ~(1 << PD7);
    _delay_ms(50);
    PORTD |= (1 << PD7);
    _delay_ms(50);
}

void setAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    sendCommand(0x2A); // Column 
    sendData(0x00);
    sendData(x0);
    sendData(0x00);
    sendData(x1);

    sendCommand(0x2B); // Row 
    sendData(0x00);
    sendData(y0);
    sendData(0x00);
    sendData(y1);

    sendCommand(0x2C); // Write to RAM
}


void fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color) {
    setAddrWindow(x, y, x + w - 1, y + h - 1);
    for (uint16_t i = 0; i < w * h; i++) {
        sendData(color >> 8);
        sendData(color);
    }
}

void TFT_init() {
    TFT_reset();

    sendCommand(0x01); // Software reset
    _delay_ms(150);

    sendCommand(0x11); // Sleep out
    _delay_ms(500);

    sendCommand(0x36); // Memory Data Access Control
    sendData(0xC8); // Row/column address, RGB order

    sendCommand(0x3A); // Interface Pixel Format
    sendData(0x05); // 16-bit/pixel

    sendCommand(0x29); // Display ON
    _delay_ms(100);

    // Clear the screen
    fillRect(0, 0, 128, 128, ST77XX_WHITE); 
}

void drawPixel(uint8_t x, uint8_t y, uint16_t color) {
    setAddrWindow(x, y, x, y);
    sendData(color >> 8);
    sendData(color);
}

void drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color) {
    int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy, e2; // error value e_xy

    while (1) {
        drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void drawGrid(uint8_t x, uint8_t y, uint8_t size, uint8_t cellSize, uint16_t color) {
    for (uint8_t i = 0; i <= size; i++) {
        // Draw horizontal lines
        drawLine(x, y + i * cellSize, x + size * cellSize, y + i * cellSize, color);
        // Draw vertical lines
        drawLine(x + i * cellSize, y, x + i * cellSize, y + size * cellSize, color);
    }
}

void drawPiece(uint8_t x, uint8_t y, uint16_t color) {
    fillRect(x * 10 + 11, y * 10 + 11, 8, 8, color);
}

#endif 