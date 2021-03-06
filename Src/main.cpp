/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2017 STMicroelectronics International N.V. 
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
/* Includes ------------------------------------------------------------------*/

extern "C" {
	#include "stm32f4xx_hal.h"
	#include "main.h"
	#include "adc.h"
	#include "dac.h"
	#include "dma.h"
	#include "rtc.h"
	#include "i2c.h"
	#include "tim.h"
	#include "usb_device.h"
	#include "gpio.h"

	/* USER CODE BEGIN Includes */
	#include "display.h"
	#include "peripherals.h"
    #include "Settings.h"
    
    #include <math.h>
}

#include "os.h"

#define sign(i) ((i > 0) - (i < 0))


/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
OS os;
/* Private variables ---------------------------------------------------------*/
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/
void fuseButton(void) {
	HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_5);
	//Output ausschalten
}

void outputButton(void) {
	HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
	HAL_GPIO_WritePin(OUTPUT_FET_GPIO_Port, OUTPUT_FET_Pin, HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == 1 ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void encoderATurned(int8_t steps) {
	if(TIM7->CNT<((abs(steps) * 500 / FAST_STEP_FREQUENCY)))
		OS::targetI += -steps * AMPS_PER_STEP * FAST_MULTIPLICATOR;
	else
		OS::targetI += -steps * AMPS_PER_STEP / 2;
	
	TIM7->CNT = 0;
	HAL_TIM_Base_Start(&htim7);
	
	if(OS::targetI < 0.0f) OS::targetI = 0.0f;
	else if(OS::targetI > IOUTMAX) OS::targetI = IOUTMAX;
}

void encoderBTurned(int8_t steps) {    
	if(TIM7->CNT < ((abs(steps) * 500 / FAST_STEP_FREQUENCY)))
		OS::targetU += -steps * VOLTS_PER_STEP * FAST_MULTIPLICATOR;
	else
		OS::targetU += -steps * VOLTS_PER_STEP / 2;
	
	TIM7->CNT = 0;
	HAL_TIM_Base_Start(&htim7);
	
	if(os.targetU < 0.0f) OS::targetU = 0.0f;
	else if(os.targetU > UOUTMAX) OS::targetU = UOUTMAX;
}

void encoderAButton(void) {
	
}

void encoderBButton(void) {
	
}
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

int main(void) {
    static uint32_t adcReadCnt = 0;
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_DAC_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USB_DEVICE_Init();
  MX_RTC_Init();
  MX_TIM6_Init();
  MX_I2C3_Init();
  MX_TIM7_Init();

  /* USER CODE BEGIN 2 */
  initCallbacks();
  initUsDelay();
  initDisplay();
  initADC(&hadc1);
  initDAC(&hdac, &htim6);
  initEncoder(&htim2, &htim3);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  setBrightness(100);
  setContrast(95);
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
  
  addOutputButtonCallback(&outputButton);
  addFuseButtonCallback(&fuseButton);
  addEncoderATurnedCallback(&encoderATurned);
  addEncoderBTurnedCallback(&encoderBTurned);
  addEncoderAButtonCallback(&encoderAButton);
  addEncoderBButtonCallback(&encoderBButton);
  
  os.onInit();
  
  while(os.running) {
    if(adcReadCnt++ >= 10) {
        OS::actualU = getADCValue(U_IST);
        OS::actualI = getADCValue(I_IST);
        
        if(OS::actualU < 0)
            OS::actualU = 0;
        if(OS::actualI < 0)
            OS::actualI = 0;
        if(OS::actualU > 99.9)
            OS::actualU = 99.9;
        if(OS::actualI > 1999)
            OS::actualI = 1999;
        
        adcReadCnt = 0;
    }
    setDACValue(U_SOLL, OS::targetU);
    setDACValue(I_SOLL, OS::targetI);
    
    handleEncoderCallbacks();   
	os.mainEntry();
  }
  
  os.onShutdown();
  
  /* USER CODE END 3 */

}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

    /**Configure the main internal regulator output voltage 
    */
  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 160;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.PLLI2S.PLLI2SN = 192;
  PeriphClkInitStruct.PLLI2S.PLLI2SM = 16;
  PeriphClkInitStruct.PLLI2S.PLLI2SR = 2;
  PeriphClkInitStruct.PLLI2S.PLLI2SQ = 4;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48CLKSOURCE_PLLI2SQ;
  PeriphClkInitStruct.PLLI2SSelection = RCC_PLLI2SCLKSOURCE_PLLSRC;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void _Error_Handler(char * file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
  {
  }
  /* USER CODE END Error_Handler_Debug */ 
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
  * @}
  */ 

/**
  * @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
