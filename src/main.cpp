/*        Your Name & E-mail: Jun Luo / jluo087@ucr.edu

*         Discussion Section: 023

*         Assignment: Final Project

*         I acknowledge all content contained herein, excluding template or example code, is my own original work.

*         Demo Link: https://youtu.be/lziGZteuPm0

*/
#include "timerISR.h"
#include "helper.h"
#include "periph.h"
#include "serialATmega.h"
#include "spi.h"
// serial_println(ADC_read(3));
// PORTD = SetBit(PORTD, 5, 0);
// PORTB = SetBit(PORTB, 0, 0);

#define NUM_TASKS 3 // TODO: Change to the number of tasks being used

// Task struct for concurrent synchSMs implmentations
typedef struct _task
{
    signed char state;         // Task's current state
    unsigned long period;      // Task period
    unsigned long elapsedTime; // Time elapsed since last task tick
    int (*TickFct)(int);       // Task tick function
} task;

// TODO: Define Periods for each task
//  e.g. const unsined long TASK1_PERIOD = <PERIOD>
const unsigned long GCD_PERIOD = 1; // TODO:Set the GCD Period

task tasks[NUM_TASKS]; // declared task array with 5 tasks

void TimerISR()
{
    for (unsigned int i = 0; i < NUM_TASKS; i++)
    { // Iterate through each task in the task array
        if (tasks[i].elapsedTime == tasks[i].period)
        {                                                      // Check if the task is ready to tick
            tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
            tasks[i].elapsedTime = 0;                          // Reset the elapsed time for the next tick
        }
        tasks[i].elapsedTime += GCD_PERIOD; // Increment the elapsed time by GCD_PERIOD
    }
}

enum Music_States
{
    Music_OFF,
    Music_1,
    Music_2
};
int TickFct_Music(int state);

enum LED_States
{
    LED_OFF,
    LED_ON
};
int TickFct_LED(int state);

enum Game_States
{
    Game_OFF,
    Game_ON
};
int TickFct_Game(int state);

int test_i = 0;
int test_j = 0;

int startMelody[12] = {262, 294, 330, 349, 392, 440, 494, 523, 523, 494, 440, 392};
int endMelody[12] = {392, 349, 330, 294, 262, 294, 330, 349, 392, 349, 330, 294};
int noteFrequency = 0;

const int dataPin = 2;
const int clockPin = 3;
const int latchPin = 4;
int led_i = 0;

volatile uint8_t leds = 0b00000001;
volatile uint8_t led_position = 0;

uint8_t x = 0, y = 0;
uint8_t player = 0; // 0 for red, 1 for blue
bool normal = true;
uint8_t board[10][10] = {0};

bool game_over = false;

void highlightCell(uint8_t x, uint8_t y)
{
    fillRect(x * 10 + 10, y * 10 + 10, 10, 10, ST77XX_GREEN);
}

void unhighlightCell(uint8_t x, uint8_t y, uint8_t board[10][10])
{
    uint16_t color = ST77XX_WHITE;
    if (board[y][x] == 1)
    {
        color = ST77XX_RED;
    }
    else if (board[y][x] == 2)
    {
        color = ST77XX_BLUE;
    }
    fillRect(x * 10 + 10, y * 10 + 10, 10, 10, color);
    drawLine(10, 10 + y * 10, 110, 10 + y * 10, ST77XX_BLACK); // Redraw horizontal line
    drawLine(10 + x * 10, 10, 10 + x * 10, 110, ST77XX_BLACK); // Redraw vertical line
}

bool checkWin(uint8_t board[10][10], uint8_t player)
{
    for (uint8_t y = 0; y < 10; y++)
    {
        for (uint8_t x = 0; x < 10; x++)
        {
            if (board[y][x] == player)
            {
                // Check horizontal
                if (x <= 5 && board[y][x + 1] == player && board[y][x + 2] == player && board[y][x + 3] == player && board[y][x + 4] == player)
                    return true;
                // Check vertical
                if (y <= 5 && board[y + 1][x] == player && board[y + 2][x] == player && board[y + 3][x] == player && board[y + 4][x] == player)
                    return true;
                // Check diagonal (\)
                if (x <= 5 && y <= 5 && board[y + 1][x + 1] == player && board[y + 2][x + 2] == player && board[y + 3][x + 3] == player && board[y + 4][x + 4] == player)
                    return true;
                // Check diagonal (/)
                if (x >= 4 && y <= 5 && board[y + 1][x - 1] == player && board[y + 2][x - 2] == player && board[y + 3][x - 3] == player && board[y + 4][x - 4] == player)
                    return true;
            }
        }
    }
    return false;
}

void resetBoard(uint8_t board[10][10])
{
    fillRect(0, 0, 128, 128, ST77XX_WHITE); // Clear the screen
    drawGrid(10, 10, 10, 10, ST77XX_BLACK); // Redraw the grid
    for (uint8_t y = 0; y < 10; y++)
    {
        for (uint8_t x = 0; x < 10; x++)
        {
            board[y][x] = 0;
        }
    }
}

void updateShiftRegister(uint8_t data)
{
    PORTD = SetBit(PORTD, 4, 0);

    for (int i = 0; i < 8; i++)
    {
        if (data & 0b10000000)
        {
            PORTD = SetBit(PORTD, 2, 1);
        }
        else
        {
            PORTD = SetBit(PORTD, 2, 0);
        }

        data = data << 1;

        PORTD = SetBit(PORTD, 3, 1);
        PORTD = SetBit(PORTD, 3, 0);
    }
    PORTD = SetBit(PORTD, 4, 1);
}
void clearShiftRegister()
{
    updateShiftRegister(0x00);
}
// TODO: Create your tick functions for each task
int TickFct_Music(int state)
{
    switch (state)
    {
    case Music_OFF:
        OCR1A = 0;
        // serial_println(ADC_read(4));
        if (ADC_read(4) > 900)
        {
            state = Music_1;
        }
        else if (game_over)
        {
            state = Music_2;
        }
        break;
    case Music_1:
        if (test_i > 6000)
        {
            state = Music_OFF;
            test_j = 0;
            test_i = 0;
        }
        break;
    case Music_2:
        if (test_i > 6000)
        {
            state = Music_OFF;
            test_j = 0;
            test_i = 0;
            game_over = false;
        }
        break;
    default:
        break;
    }

    switch (state)
    {
    case Music_OFF:
        break;
    case Music_1:
        test_i++;
        if (test_i % 250 == 0)
        {
            test_j++;
            if (test_j > 12)
            {
                test_j = 0;
            }
        }
        noteFrequency = startMelody[test_j];
        ICR1 = 16000000 / (2 * 8 * noteFrequency) - 1;
        OCR1A = ICR1 / 2;
        break;
    case Music_2:
        test_i++;
        if (test_i % 500 == 0)
        {
            test_j++;
            if (test_j > 12)
            {
                test_j = 0;
            }
        }
        noteFrequency = endMelody[test_j];
        ICR1 = 16000000 / (2 * 8 * noteFrequency) - 1;
        OCR1A = ICR1 / 2;
        break;
    default:
        break;
    }
    return state;
}

int TickFct_LED(int state)
{
    switch (state)
    {
    case LED_OFF:
        if (ADC_read(4) > 900 || game_over)
        {
            state = LED_ON;
            leds = 0b00000001;
            led_position = 0;
        }
        break;
    case LED_ON:
        if (led_i > 600)
        {
            state = LED_OFF;
            led_i = 0;
            game_over = false;
        }
        break;
    default:
        break;
    }

    switch (state)
    {
    case LED_OFF:
        clearShiftRegister();
        break;
    case LED_ON:
        led_i++;
        if (led_i % 15 == 0)
        {
            leds = 1 << led_position;
            updateShiftRegister(leds);
            led_position++;
            if (led_position >= 8)
            {
                led_position = 0;
            }
        }
        break;
    default:
        break;
    }
    return state;
}

int TickFct_Game(int state)
{
    uint16_t adc0 = ADC_read(0);
    uint16_t adc1 = ADC_read(1);
    uint16_t adc2 = ADC_read(2);

    switch (state)
    {
    case Game_OFF:
        // serial_println(ADC_read(0)); // >900 up  <200 down 500-700 mid
        // serial_println(ADC_read(1)); // >900 right  <200 left 500-700 mid
        // serial_println(ADC_read(2)); // >800 normal  <200 pressed
        if (ADC_read(4) > 900)
        {
            state = Game_ON;
            x = 0, y = 0;
            player = 0; // 0 for red, 1 for blue
            normal = true;
            highlightCell(x, y);
            // board[10][10] = {0};
        }
        break;
    case Game_ON:
        if (ADC_read(5) > 900)
        {
            state = Game_OFF;
            resetBoard(board);
            PORTD = SetBit(PORTD, 6, 0);
            PORTD = SetBit(PORTD, 5, 0);
        }
        break;
    default:
        break;
    }

    switch (state)
    {
    case Game_OFF:
        break;
    case Game_ON:
        if ((adc0 < 700) && (adc0 > 500) && (adc1 > 500) && (adc1 < 700) && (adc2 > 800))
        {
            normal = true;
        }

        if (normal)
        {
            if (adc0 > 900 && y > 0)
            {
                unhighlightCell(x, y, board);
                y--;
                normal = false;
                highlightCell(x, y);
            }
            else if (adc0 < 200 && y < 9)
            {
                unhighlightCell(x, y, board);
                y++;
                normal = false;
                highlightCell(x, y);
            }

            if (adc1 > 900 && x < 9)
            {
                unhighlightCell(x, y, board);
                x++;
                normal = false;
                highlightCell(x, y);
            }
            else if (adc1 < 200 && x > 0)
            {
                unhighlightCell(x, y, board);
                x--;
                normal = false;
                highlightCell(x, y);
            }
        }

        if (adc2 < 200)
        {
            if (board[y][x] == 0)
            {
                if (player == 0)
                {
                    drawPiece(x, y, ST77XX_RED); // draw red piece
                    player = 1;
                    board[y][x] = 1;
                    normal = false;
                    if (checkWin(board, 1))
                    {
                        game_over = true;
                        PORTD = SetBit(PORTD, 5, 1);
                    }
                }
                else
                {
                    drawPiece(x, y, ST77XX_BLUE); // draw blue piece
                    player = 0;
                    board[y][x] = 2;
                    normal = false;
                    if (checkWin(board, 2))
                    {
                        game_over = true;
                        PORTD = SetBit(PORTD, 6, 1);
                    }
                }
            }
        }
        break;
    default:
        break;
    }
    return state;
}

int main(void)
{
    serial_init(9600); // only for debugging
    // TODO: initialize all your inputs and ouputs
    DDRD = 0xFF;
    PORTD = 0x00;
    DDRB = 0xFF;
    PORTB = 0x00;

    DDRC = 0x00;
    PORTC = 0xFF;

    ADC_init(); // initializes ADC

    TCCR1A |= (1 << WGM11) | (1 << COM1A1);
    TCCR1B |= (1 << WGM12) | (1 << WGM13) | (1 << CS11);

    SPI_init();
    TFT_init();
    drawGrid(10, 10, 10, 10, ST77XX_BLACK);

    // TODO: Initialize tasks here
    //  e.g.
    //  tasks[0].period = ;
    //  tasks[0].state = ;
    //  tasks[0].elapsedTime = ;
    //  tasks[0].TickFct = ;
    unsigned char i = 0;
    tasks[i].state = Music_OFF;
    tasks[i].period = 1;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &TickFct_Music;
    ++i;
    tasks[i].state = LED_OFF;
    tasks[i].period = 10;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &TickFct_LED;
    ++i;
    tasks[i].state = Game_OFF;
    tasks[i].period = 1;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &TickFct_Game;

    TimerSet(GCD_PERIOD);
    TimerOn();

    while (1)
    {
    }

    return 0;
}