/**
  ******************************************************************************
  * @file    Templates/Src/main.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    22-April-2016
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/** @addtogroup STM32F7xx_HAL_Examples
  * @{
  */

/** @addtogroup Templates
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
__IO DWORD tick = 0;

USBH_HandleTypeDef hUSBHost[5], *phUSBHost = NULL;
uint8_t host_state;

/**
  * @brief   File system variables
  */
extern char Path1[];
extern char Path2[];
extern char Path3[];

/**
  * @brief   Audio codec variables
  */
FlagStatus volTask = RESET;

/**
  * @brief   GOL variables
  */
SCREEN_STATES screenState = CREATE_FILEBROWSER; 	// current state of main state machine

extern I2C_HandleTypeDef hI2cAudioHandler;
extern int16_t  prev_x, prev_y;
extern uint8_t  mouse_button;

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void Error_Handler(void);
static void MPU_Config(void);
static void CPU_CACHE_Enable(void);

void TouchGetMsg(GOL_MSG *pMsg);
void HUB_Process(void);
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* This project template calls firstly two functions in order to configure MPU feature 
     and to enable the CPU Cache, respectively MPU_Config() and CPU_CACHE_Enable().
     These functions are provided as template implementation that User may integrate 
     in his application, to enhance the performance in case of use of AXI interface 
     with several masters. */ 
  
  /* Configure the MPU attributes as Write Through */
  MPU_Config();

  /* Enable the CPU Cache */
  CPU_CACHE_Enable();

  /* STM32F7xx HAL library initialization:
       - Configure the Flash ART accelerator on ITCM interface
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Low Level Initialization
     */
  HAL_Init();

  /* Configure the system clock to 216 MHz */
  SystemClock_Config();

  /* Init routines */
  BSP_TS_Init(GetMaxX(), GetMaxY());
  GOLInit(); // initialize graphics library

  /* Setup SD GPIO */
  FATFS_LinkDriver(&SD_Driver, Path1);
  disk_initialize (0);

  memset(&hUSBHost[0], 0, sizeof(USBH_HandleTypeDef));

  hUSBHost[0].valid   = 1;
  hUSBHost[0].address = USBH_DEVICE_ADDRESS;
  hUSBHost[0].Pipes   = USBH_malloc(sizeof(uint32_t) * USBH_MAX_PIPES_NBR);

  /* Init Host Library */
  USBH_Init(&hUSBHost[0], USBH_UserProcess, 0);

  /* Add Supported Class */
  USBH_RegisterClass(&hUSBHost[0], USBH_MSC_CLASS);
  USBH_RegisterClass(&hUSBHost[0], USBH_HID_CLASS);
  USBH_RegisterClass(&hUSBHost[0], USBH_HUB_CLASS);

  /* Start Host Process */
  USBH_Start(&hUSBHost[0]);

  FATFS_LinkDriver(&USBH_Driver, Path2);
  FATFS_LinkDriver(&USBH_2_Driver, Path3);

#ifdef USE_TRANSPARENT_COLOR
  TransparentColorEnable(WHITE); // Define transparent color to be used
#endif

  /* Infinite loop */
  while (1)
  {
	  /* USB Host Background task */
	  HUB_Process();

      /* Graphic user interface */
      GOL_Procedures();
  }
}

/**
  * @brief
  * @param
  * @retval
  */
void HUB_Process(void)
{
	static uint8_t current_port = -1;

	if(phUSBHost != NULL && phUSBHost->valid == 1)
	{
		USBH_Process(phUSBHost);

		if(phUSBHost->busy)
			return;
	}

	for( ;; )
	{
		current_port++;

		if(current_port > MAX_HUB_PORTS)
			current_port = 0;

		if(hUSBHost[current_port].valid)
		{
			phUSBHost = &hUSBHost[current_port];
			USBH_LL_SetupEP0(phUSBHost);

			if(phUSBHost->valid == 3)
			{
				phUSBHost->valid = 1;
				phUSBHost->busy  = 1;
			}

			break;
		}
	}
}

/**
  * @brief
  * @param
  * @retval
  */
uint8_t BSP_USB_IsDetected(void)
{
	if(USBH_GetActiveClass(phUSBHost) == USB_MSC_CLASS)
		return !disk_status (1);
	else
		return 0;
}

/**
  * @brief  This function must be called periodically to manage
  *         graphic interface and user interactions.
  * @param  None
  * @retval None
  */
void GOL_Procedures(void)
{
	GOL_MSG msg; // GOL message structure to interact with GOL

	if(GOLDraw())
	{                               // Draw GOL objects
		// Drawing is finished, we can now process new message
		TouchGetMsg(&msg);          // Get message from touch screen

		GOLMsg(&msg);               // Process message
	}
}

/**
  * @brief  The user MUST implement this function. GOLMsg() calls
  *         this function when a valid message for an object in the
  *         active list is received. User action for the message should
  *         be implemented here. If this function returns non-zero,
  *         the message for the object will be processed by default.
  *         If zero is returned, GOL will not perform any action.
  * @param  objMsg - Translated message for the object or the action ID response from the object.
  * @param  pObj   - Pointer to the object that processed the message.
  * @param  pMsg   - Pointer to the GOL message from user.
  * @retval Return a non-zero if the message will be processed by default.
  *         If a zero is returned, the message will not be processed by GOL.
  */
WORD GOLMsgCallback(WORD objMsg, OBJ_HEADER* pObj, GOL_MSG* pMsg)
{
	switch(screenState) {
		case DISPLAY_FILEBROWSER:
			return (fileBrowser_MsgCallback(objMsg, pObj, pMsg));
		default:
			return (1); // process message by default
	}
}

/**
  * @brief  GOLDrawCallback() function MUST BE implemented by
  *         the user. This is called inside the GOLDraw()
  *         function when the drawing of objects in the active
  *         list is completed. User drawing must be done here.
  *         Drawing color, line type, clipping region, graphic
  *         cursor position and current font will not be changed
  *         by GOL if this function returns a zero. To pass
  *         drawing control to GOL this function must return
  *         a non-zero value. If GOL messaging is not using
  *         the active link list, it is safe to modify the
  *         list here.
  * @param  None
  * @retval Return a one if GOLDraw() will have drawing control
  *         on the active list. Return a zero if user wants to
  *         keep the drawing control.
  */
WORD GOLDrawCallback()
{
	static DWORD    prevTick = 0; // keeps previous value of tick

	switch(screenState) {
		case CREATE_FILEBROWSER:
            if(BSP_SD_IsDetected() || BSP_USB_IsDetected()) {
				if(Create_fileBrowser() == 1)
					screenState = DISPLAY_FILEBROWSER; // switch to next state
				else
					screenState = CREATE_FILEBROWSER;
			}
			return (1);

		case DISPLAY_FILEBROWSER:
			fileBrowser_DrawCallback();
			return (1);
		default:
			break;
	}

	return (1); // release drawing control to GOL
}

/**
  * @brief  Manages the interactions with touch screen
  *         and populates GOL message structure.
  * @param  GOL_MSG* pMsg: Pointer to the message structure to be populated.
  * @retval None
  */
void TouchGetMsg(GOL_MSG* pMsg)
{
   static SHORT prevX = -1;
   static SHORT prevY = -1;
   SHORT x, y;
   TS_StateTypeDef TS_State;

   if (mouse_button == 0)
   {
      x = -1;
      y = -1;
   }
   else
   {
      x = prev_x;
      y = prev_y;
   }

   if( HAL_I2C_GetState(&hI2cAudioHandler) == HAL_I2C_STATE_READY )
   {
	   if(ts_driver != 0x00)
		   BSP_TS_GetState(&TS_State);
	   else
		   TS_State.touchDetected = 0;
   }
   else
   {
	   TS_State.touchDetected = 0;
   }

   if (TS_State.touchDetected == 0 && mouse_button == 0)
   {
      x = -1;
      y = -1;
   }
   else if(mouse_button == 0)
   {
      x = TS_State.touchX[0];
      y = TS_State.touchY[0];
   }

   pMsg->type = TYPE_TOUCHSCREEN;
   pMsg->uiEvent = EVENT_INVALID;

   if ((prevX == x) && (prevY == y) && (x != -1) && (y != -1))
   {
      pMsg->uiEvent = EVENT_STILLPRESS;
      pMsg->param1 = x;
      pMsg->param2 = y;
      return;
   }

   if ((prevX != -1) || (prevY != -1))
   {

      if ((x != -1) && (y != -1))
      {
         pMsg->uiEvent = EVENT_MOVE;
      }
      else
      {
         pMsg->uiEvent = EVENT_RELEASE;
         pMsg->param1 = prevX;
         pMsg->param2 = prevY;
         prevX = x;
         prevY = y;

         return;
      }
   }
   else
   {
      if ((x != -1) && (y != -1))
      {
         pMsg->uiEvent = EVENT_PRESS;
      }
      else
      {
         pMsg->uiEvent = EVENT_INVALID;
      }

   }

   pMsg->param1 = x;
   pMsg->param2 = y;
   prevX = x;
   prevY = y;
}

/**
  * @brief  User Process
  * @param  phost: Host Handle
  * @param  id: Host Library user message ID
  * @retval None
  */
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id)
{
  switch(id)
  {
  	case HOST_USER_SELECT_CONFIGURATION:
    		break;

  	case HOST_USER_DISCONNECTION:
  			host_state = HOST_USER_DISCONNECTION;
    		break;

  	case HOST_USER_CLASS_ACTIVE:
  			HAL_Delay(100);
  			host_state = HOST_USER_CLASS_ACTIVE;
    		break;

  	case HOST_USER_CONNECTION:
  			host_state = HOST_USER_CONNECTION;
    		break;

  	default:
    		break;
  }
}

/**
  * @brief
  * @param
  * @retval
  */
void HAL_Delay(__IO uint32_t Delay)
{
	uint32_t tick;

	tick = (SystemCoreClock/1000) * Delay;
	while(tick--)
	{
	}
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 216000000
  *            HCLK(Hz)                       = 216000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 25
  *            PLL_N                          = 432
  *            PLL_P                          = 2
  *            PLL_Q                          = 9
  *            PLL_R                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 7
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;
  
  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();
  
  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 480;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 10;
  RCC_OscInitStruct.PLL.PLLR = 8;
  
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }
  
  /* Activate the OverDrive to reach the 216 MHz Frequency */  
  ret = HAL_PWREx_EnableOverDrive();
  if(ret != HAL_OK)
  {
    Error_Handler();
  }
  
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2; 
  
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* User may add here some code to deal with this error */
  while(1)
  {
  }
}

/**
  * @brief  Configure the MPU attributes as Write Through for Internal SRAM1/2.
  * @note   The Base Address is 0x20020000 since this memory interface is the AXI.
  *         The Configured Region Size is 512KB because the internal SRAM1/2 
  *         memory size is 384KB.
  * @param  None
  * @retval None
  */
static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;
  
  /* Disable the MPU */
  HAL_MPU_Disable();

  /* Configure the MPU attributes as WT for SRAM */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x20020000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
