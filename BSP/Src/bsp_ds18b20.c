
#include "stm32f1xx_hal.h"
#include "tim.h"
#include "bsp_ds18b20.h"

#define DQ_GPIO (GPIOB)
#define DQ_Pin  (GPIO_PIN_4)
#define DQ_High (DQ_GPIO->BSRR = DQ_Pin)
#define DQ_Low  (DQ_GPIO->BSRR = DQ_Pin << 16)
#define DQ_Read (DQ_GPIO->IDR  & DQ_Pin)

static void DQ_SetIn(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_InitStruct.Pin   = DQ_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(DQ_GPIO, &GPIO_InitStruct);
}

static void DQ_SetOut(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_InitStruct.Pin   = DQ_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(DQ_GPIO, &GPIO_InitStruct);
}

static unsigned char byte_read(void)
{
    unsigned char byte = 0;

    DQ_SetOut();

    for (int i = 0; i < 8; i++)
    {
        DQ_Low;usDelay(1);
        DQ_SetIn();usDelay(14);
        byte >>= 1;
        if (DQ_Read)
            byte |= 0x80;
        usDelay(45);
        DQ_SetOut();
        DQ_High;usDelay(1);
    }

    return byte;
}

static void byte_write(unsigned char byte)
{
    DQ_SetOut();

    for (int i = 0; i < 8; i++)
    {
        DQ_Low;usDelay(1);
        if (byte & 0x01)
            DQ_High;
        else
            DQ_Low;
        usDelay(30);
        DQ_High;usDelay(1);
        byte >>= 1;
    }
}

static void reset(void)
{
    DQ_SetOut();
    DQ_Low;
    usDelay(500);
    DQ_SetIn();
    usDelay(500);
}

static void DS18B20_Start(void)
{
    reset();
    byte_write(0xCC);
    byte_write(0x44);
}

typedef struct _DataStruct {
    float Value;
    int   Sign;
} DataStruct;

#define DATA_TOTAL 10

static float DS18B20_CalcAvg(float value)
{
    static int Pos = 0;
    static DataStruct Data[DATA_TOTAL] = {0};

    float Temp  = 0;
    int   Count = 0;

    Data[Pos].Value = value;
    Data[Pos].Sign  = 1;

    Pos++;
    if (Pos >= DATA_TOTAL)
        Pos = 0;

    for (int i = 0; i < DATA_TOTAL; i++)
    {
        if (Data[i].Sign)
        {
            Temp += Data[i].Value;
            Count++;
        }
    }

    Temp /= Count;

    return Temp;
}

static float DS18B20_ReadTemp(void)
{
    union {
        char  Byte[2];
        short Short;
    } Temp = {0};

    reset();
    byte_write(0xCC);
    byte_write(0xBE);
    Temp.Byte[0] = byte_read();
    Temp.Byte[1] = byte_read();

    return Temp.Short * 0.0625;
}

void *BSP_DS18B20_OBJ(void)
{
    static DS18B20_OBJ obj = {
        .Start    = DS18B20_Start,
        .ReadTemp = DS18B20_ReadTemp,
    };

    return &obj;
}
