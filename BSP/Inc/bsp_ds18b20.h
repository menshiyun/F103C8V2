
#ifndef _BSP_DS18B20_H_
#define _BSP_DS18B20_H_

typedef struct _DS18B20_OBJ {
    void  (*Start)(void);
    float (*ReadTemp)(void);
} DS18B20_OBJ;

void *BSP_DS18B20_OBJ(void);

#endif
