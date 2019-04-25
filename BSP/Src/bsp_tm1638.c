
#include "stm32f1xx_hal.h"
#include "tim.h"
#include "bsp_tm1638.h"
#include <stdio.h>
#include <string.h>

#define STB_GPIO (GPIOB)
#define STB_Pin  (GPIO_PIN_5)
#define STB_High (STB_GPIO->BSRR = STB_Pin)
#define STB_Low  (STB_GPIO->BSRR = STB_Pin << 16)

#define CLK_GPIO (GPIOB)
#define CLK_Pin  (GPIO_PIN_6)
#define CLK_High (CLK_GPIO->BSRR = CLK_Pin)
#define CLK_Low  (CLK_GPIO->BSRR = CLK_Pin << 16)

#define DIO_GPIO (GPIOB)
#define DIO_Pin  (GPIO_PIN_7)
#define DIO_High (DIO_GPIO->BSRR = DIO_Pin)
#define DIO_Low  (DIO_GPIO->BSRR = DIO_Pin << 16)
#define DIO_Read (DIO_GPIO->IDR  & DIO_Pin)

static int TM1638_Busy = 0;

static const char DigitConv[] = {
    0x3F, 0x06, 0x5B, 0x4F, //0 1 2 3
    0x66, 0x6D, 0x7D, 0x07, //4 5 6 7
    0x7F, 0x6F, 0x77, 0x7C, //8 9 A b
    0x39, 0x5E, 0x79, 0x71, //C d E F
    0x00, 0x40, // ' '  '-'
};

static void DIO_SetIn(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_InitStruct.Pin   = DIO_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(DIO_GPIO, &GPIO_InitStruct);
}

static void DIO_SetOut(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_InitStruct.Pin   = DIO_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(DIO_GPIO, &GPIO_InitStruct);
}

static unsigned char byte_read(void)
{
    unsigned char byte = 0;

    DIO_SetIn();

    for (int i = 0; i < 8; i++)
    {
        CLK_Low;usDelay(1);
        CLK_High;usDelay(1);
        byte >>= 1;
        if (DIO_Read)
            byte |= 0x80;
    }

    return byte;
}

static void byte_write(unsigned char byte)
{
    DIO_SetOut();

    for (int i = 0; i < 8; i++)
    {
        CLK_Low;usDelay(1);
        if (byte & 0x01)
            DIO_High;
        else
            DIO_Low;
        CLK_High;usDelay(1);
        byte >>= 1;
    }
}

static void gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    STB_High;
    GPIO_InitStruct.Pin = STB_Pin;
    HAL_GPIO_Init(STB_GPIO, &GPIO_InitStruct);

    CLK_High;
    GPIO_InitStruct.Pin = CLK_Pin;
    HAL_GPIO_Init(CLK_GPIO, &GPIO_InitStruct);

    DIO_High;
    GPIO_InitStruct.Pin = DIO_Pin;
    HAL_GPIO_Init(DIO_GPIO, &GPIO_InitStruct);
}

static void TM1638_Init(void)
{
    if (TM1638_Busy)
        return;

    TM1638_Busy = 1;

    gpio_init();

    STB_Low;usDelay(1);
    byte_write(0x40);
    STB_High;usDelay(1);

    STB_Low;usDelay(1);
    byte_write(0xC0);
    for (int i = 0; i < 16; i++)
        byte_write(0x00);
    STB_High;usDelay(1);

    STB_Low;usDelay(1);
    byte_write(0x88);
    STB_High;usDelay(1);

    TM1638_Busy = 0;
}

static int TM1638_ReadKeys(void)
{
    TM1638_Keys keys = {0};

    if (TM1638_Busy)
        return keys.uInt;

    TM1638_Busy = 1;

    STB_Low;usDelay(1);
    byte_write(0x42);
    DIO_High;
    for (int i = 0; i < 4; i++)
        keys.uChar[i] = byte_read();
    STB_High;usDelay(1);

    TM1638_Busy = 0;

    return keys.uInt;
}

static void TM1638_BrightSet(int pulse)
{
    if ((pulse < 0)||(pulse > 7))
        return;

    if (TM1638_Busy)
        return;

    TM1638_Busy = 1;

    STB_Low;usDelay(1);
    byte_write(0x88|pulse);
    STB_High;usDelay(1);

    TM1638_Busy = 0;
}

static void TM1638_LedSet(int bits)
{
    if (TM1638_Busy)
        return;

    TM1638_Busy = 1;

    STB_Low;usDelay(1);
    byte_write(0x44);
    STB_High;usDelay(1);

    for (int i = 0; i < 8; i++)
    {
        STB_Low;usDelay(1);
        byte_write(0xC0|((i + 1) * 2 - 1));
        byte_write(bits & 0x01 ? 1 : 0);
        STB_High;usDelay(1);
        bits >>= 1;
    }

    TM1638_Busy = 0;
}

static void TM1638_TubeSet(int n, int digit, int dot)
{
    if ((n < 1)||(n > 8))
        return;

    if (TM1638_Busy)
        return;

    TM1638_Busy = 1;

    STB_Low;usDelay(1);
    byte_write(0x44);
    STB_High;usDelay(1);

    STB_Low;usDelay(1);
    byte_write(0xC0|(--n * 2));
    byte_write(DigitConv[digit & 0xFF]|(dot ? 0x80 : 0x00));
    STB_High;usDelay(1);

    TM1638_Busy = 0;
}

static void TM1638_DisInt(int value)
{
    char str[9] = {0};
    int  len    = 0;
    int  digit  = 0;

    if (TM1638_Busy)
        return;

    TM1638_Busy = 1;

    len = snprintf(str, sizeof(str), "%d", value);
    if (len > 8)
        len = 8;

    STB_Low;usDelay(1);
    byte_write(0x44);
    STB_High;usDelay(1);

    for (int i = 0; i < 8; i++)
    {
        STB_Low;usDelay(1);

        byte_write(0xC0|((7 - i) * 2));

        if (len > 0)
        {
            digit = str[len-- - 1];

            if (digit == '-')
                digit = 0x11;
            else
                digit -= 0x30;

            byte_write(DigitConv[digit]);
        }
        else
            byte_write(0x00);

        STB_High;usDelay(1);
    }

    TM1638_Busy = 0;
}

static void TM1638_DisFloat(float value)
{
    char str[9] = {0};
    int  len    = 0;
    int  digit  = 0;
    int  dot    = 0;

    if (TM1638_Busy)
        return;

    TM1638_Busy = 1;

    len = snprintf(str, sizeof(str), "%f", value);
    if (len > 8)
        len = 8;

    STB_Low;usDelay(1);
    byte_write(0x44);
    STB_High;usDelay(1);

    for (int i = 0; i < 8; i++)
    {
        STB_Low;usDelay(1);

        byte_write(0xC0|((7 - i) * 2));

        if (len > 0)
        {
            dot   = 0;
            digit = str[len-- - 1];

            if (digit == '.')
            {
                dot = 1;
                digit = str[len-- - 1];
            }

            if (digit == '-')
                digit = 0x11;
            else
                digit -= 0x30;

            byte_write(DigitConv[digit]|(dot ? 0x80 : 0x00));
        }
        else
            byte_write(0x00);

        STB_High;usDelay(1);
    }

    TM1638_Busy = 0;
}

void *BSP_TM1638_OBJ(void)
{
    static TM1638_OBJ obj = {
        .Init      = TM1638_Init,
        .ReadKeys  = TM1638_ReadKeys,
        .BrightSet = TM1638_BrightSet,
        .LedSet    = TM1638_LedSet,
        .TubeSet   = TM1638_TubeSet,
        .DisInt    = TM1638_DisInt,
        .DisFloat  = TM1638_DisFloat,
    };

    return &obj;
}
