
#ifndef _BSP_TM1638_H_
#define _BSP_TM1638_H_

#define TM1638_LED1 0x01
#define TM1638_LED2 0x02
#define TM1638_LED3 0x04
#define TM1638_LED4 0x08
#define TM1638_LED5 0x10
#define TM1638_LED6 0x20
#define TM1638_LED7 0x40
#define TM1638_LED8 0x80

typedef union _TM1638_Keys {
    struct {
        unsigned char S1:4;
        unsigned char S5:4;
        unsigned char S2:4;
        unsigned char S6:4;
        unsigned char S3:4;
        unsigned char S7:4;
        unsigned char S4:4;
        unsigned char S8:4;
    } Key;
    unsigned char uChar[4];
    unsigned int  uInt;
} TM1638_Keys;

typedef struct _TM1638_OBJ {
    void (*Init)(void);
    int  (*ReadKeys)(void);
    void (*BrightSet)(int);
    void (*LedSet)(int);
    void (*TubeSet)(int, int, int);
    void (*DisInt)(int);
    void (*DisFloat)(float);
} TM1638_OBJ;

void *BSP_TM1638_OBJ(void);

#endif
