/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2019 STMicroelectronics International N.V.
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice,
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other
  *    contributors to this software may be used to endorse or promote products
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under
  *    this license is void and will automatically terminate your rights under
  *    this license.
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32f1xx_hal.h"
#include "iwdg.h"
#include "tim.h"
#include "bsp_ds18b20.h"
#include "bsp_tm1638.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LIMIT_HIGH 80
#define LIMIT_LOW  20

#define STEP_1     1
#define STEP_5     5

#define TEMP_L1    0.5
#define TEMP_L2    1

#define HEAT_ON  (GPIOB->BSRR = GPIO_PIN_8)
#define HEAT_OFF (GPIOB->BSRR = GPIO_PIN_8 << 16)

#define FAN_ON   (GPIOB->BSRR = GPIO_PIN_9)
#define FAN_OFF  (GPIOB->BSRR = GPIO_PIN_9 << 16)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
static float ds18b20_temp = 0;
/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId scanTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartScanTask(void const * argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of scanTask */
  osThreadDef(scanTask, StartScanTask, osPriorityNormal, 0, 128);
  scanTaskHandle = osThreadCreate(osThread(scanTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();

  /* USER CODE BEGIN StartDefaultTask */
  DS18B20_OBJ *ds18b20 = BSP_DS18B20_OBJ();
  /* Infinite loop */
  for(;;)
  {
    HAL_IWDG_Refresh(&hiwdg);
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    ds18b20->Start();
    osDelay(750);
    ds18b20_temp = ds18b20->ReadTemp();
    osDelay(250);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartScanTask */
/**
* @brief Function implementing the scanTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartScanTask */
void StartScanTask(void const * argument)
{
    /* USER CODE BEGIN StartScanTask */
    int func = 1;
    int sTemp = 25;
    TM1638_OBJ *tm1638 = BSP_TM1638_OBJ();
    TM1638_Keys tm1638_press = {0};

    tm1638->Init();
    tm1638->BrightSet(3);
    tm1638->LedSet(TM1638_LED1);
    /* Infinite loop */
    for(;;)
    {
        if (func != 2)
            tm1638->DisFloat(ds18b20_temp);

        if (func == 3)
        {
            if (ds18b20_temp <= sTemp - TEMP_L2)
            {
                HEAT_ON;
                FAN_OFF;
            }
            else if ((ds18b20_temp >= sTemp - TEMP_L1)&&(ds18b20_temp <= sTemp + TEMP_L1))
            {
                HEAT_OFF;
                FAN_OFF;
            }
            else if (ds18b20_temp >= sTemp + TEMP_L2)
            {
                HEAT_OFF;
                FAN_ON;
            }
        }

        osDelay(20);

        tm1638_press.uInt = tm1638->ReadKeys();
        if (tm1638_press.uInt)
        {
            osDelay(20);
            tm1638_press.uInt &= tm1638->ReadKeys();
            osDelay(200);
            tm1638_press.uInt ^= tm1638->ReadKeys();

            if (tm1638_press.uInt)
            {
                if (tm1638_press.Key.S1)
                {
                    func = 1;
                    tm1638->LedSet(TM1638_LED1);
                    HEAT_OFF;
                    FAN_OFF;
                }
                else if (tm1638_press.Key.S2)
                {
                    func = 2;
                    tm1638->LedSet(TM1638_LED2);
                    tm1638->DisInt(sTemp);
                    HEAT_OFF;
                    FAN_OFF;
                }
                else if (tm1638_press.Key.S3)
                {
                    func = 3;
                    tm1638->LedSet(TM1638_LED3);
                }
                else if (func == 2)
                {
                    if (tm1638_press.Key.S4)
                        continue;
                    else if (tm1638_press.Key.S5)
                        sTemp += STEP_1;
                    else if (tm1638_press.Key.S6)
                        sTemp -= STEP_1;
                    else if (tm1638_press.Key.S7)
                        sTemp += STEP_5;
                    else if (tm1638_press.Key.S8)
                        sTemp -= STEP_5;

                    if (sTemp < LIMIT_LOW)
                        sTemp = LIMIT_LOW;

                    if (sTemp > LIMIT_HIGH)
                        sTemp = LIMIT_HIGH;

                    tm1638->DisInt(sTemp);
                }
            }
        }
    }
    /* USER CODE END StartScanTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
