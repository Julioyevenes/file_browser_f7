/**
  ******************************************************************************
  * @file    ${file_name} 
  * @author  ${user}
  * @version 
  * @date    ${date}
  * @brief   
  ******************************************************************************
  * @attention
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "jpeg_hard_dec.h"
#include "jpeg_utils_conf.h"

/* Private types ------------------------------------------------------------*/
typedef struct
{
  uint8_t State;  
  uint8_t *DataBuffer;
  uint32_t DataBufferSize;

}JPEG_Data_BufferTypeDef;

/* Private constants --------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
#define CHUNK_SIZE_OUT ((uint32_t)(768 * 4))

#define JPEG_BUFFER_EMPTY 0
#define JPEG_BUFFER_FULL  1

#define NB_OUTPUT_DATA_BUFFERS      2
#define NB_INPUT_DATA_BUFFERS       1

/* Private variables --------------------------------------------------------*/
JPEG_YCbCrToRGB_Convert_Function pConvert_Function;

uint8_t MCU_Data_OutBuffer0[CHUNK_SIZE_OUT];
uint8_t MCU_Data_OutBuffer1[CHUNK_SIZE_OUT];

JPEG_Data_BufferTypeDef Jpeg_OUT_BufferTab[NB_OUTPUT_DATA_BUFFERS] =
{
  {JPEG_BUFFER_EMPTY , MCU_Data_OutBuffer0 , 0},
  {JPEG_BUFFER_EMPTY , MCU_Data_OutBuffer1, 0}
};

JPEG_Data_BufferTypeDef Jpeg_IN_BufferTab[NB_INPUT_DATA_BUFFERS] =
{
  {JPEG_BUFFER_EMPTY , 0, 0}
};

uint32_t MCU_TotalNb = 0;
uint32_t MCU_BlockIndex = 0;

uint32_t JPEG_OUT_Read_BufferIndex = 0;
uint32_t JPEG_OUT_Write_BufferIndex = 0;
__IO uint32_t Output_Is_Paused = 0;

DMA2D_HandleTypeDef DMA2D_Handle;

/* Private function prototypes ----------------------------------------------*/

/**
  * @brief  
  * @param  
  * @retval 
  */
BYTE JPEG_Decode_DMA(JPEG_HandleTypeDef *hjpeg, FIL *pFile, uint32_t *ReadBufferPtr, uint32_t ReadBufferSize)
{
	uint32_t i;

	MCU_TotalNb = 0;
	MCU_BlockIndex = 0;

	JPEG_OUT_Read_BufferIndex = 0;
	JPEG_OUT_Write_BufferIndex = 0;
	Output_Is_Paused = 0;

	/* Init buffers */
	Jpeg_IN_BufferTab[0].State = JPEG_BUFFER_EMPTY;
	Jpeg_IN_BufferTab[0].DataBuffer = (uint8_t *) ReadBufferPtr;
	Jpeg_IN_BufferTab[0].DataBufferSize = 0;

	for(i = 0; i < NB_OUTPUT_DATA_BUFFERS; i++)
	{
		Jpeg_OUT_BufferTab[i].State = JPEG_BUFFER_EMPTY;
		Jpeg_OUT_BufferTab[i].DataBufferSize = 0;
	}

	/* Read and fill input buffers */
	for(i = 0; i < NB_INPUT_DATA_BUFFERS; i++)
	{
		if(JMVRead(pFile, ReadBufferSize, 0, Jpeg_IN_BufferTab[i].DataBuffer))
			return(100); // read error
		else
			Jpeg_IN_BufferTab[i].DataBufferSize = ReadBufferSize;
			Jpeg_IN_BufferTab[i].State = JPEG_BUFFER_FULL;
	}

	/* Start JPEG decoding with DMA method */
  	HAL_JPEG_Decode_DMA(hjpeg ,
			    Jpeg_IN_BufferTab[0].DataBuffer ,
			    Jpeg_IN_BufferTab[0].DataBufferSize ,
			    Jpeg_OUT_BufferTab[0].DataBuffer ,CHUNK_SIZE_OUT);

	return(0); // success
}

/**
  * @brief  
  * @param  
  * @retval 
  */
BYTE JPEG_OutputHandler(JPEG_HandleTypeDef *hjpeg, uint32_t FrameBufferAddress)
{
	uint32_t ConvertedDataCount;

	if(Jpeg_OUT_BufferTab[JPEG_OUT_Read_BufferIndex].State == JPEG_BUFFER_FULL)
	{
 		MCU_BlockIndex += pConvert_Function(Jpeg_OUT_BufferTab[JPEG_OUT_Read_BufferIndex].DataBuffer, 
						    (uint8_t *)FrameBufferAddress, 
						    MCU_BlockIndex, 
						    Jpeg_OUT_BufferTab[JPEG_OUT_Read_BufferIndex].DataBufferSize, 
						    &ConvertedDataCount);   
    
    		Jpeg_OUT_BufferTab[JPEG_OUT_Read_BufferIndex].State = JPEG_BUFFER_EMPTY;
    		Jpeg_OUT_BufferTab[JPEG_OUT_Read_BufferIndex].DataBufferSize = 0;
    
    		JPEG_OUT_Read_BufferIndex++;
    		if(JPEG_OUT_Read_BufferIndex >= NB_OUTPUT_DATA_BUFFERS)
    		{
      			JPEG_OUT_Read_BufferIndex = 0;
    		}
    
    		if(MCU_BlockIndex == MCU_TotalNb)
    		{
      			return 1;
    		}
	}
	else if((Output_Is_Paused == 1) && \
          	(JPEG_OUT_Write_BufferIndex == JPEG_OUT_Read_BufferIndex) && \
          	(Jpeg_OUT_BufferTab[JPEG_OUT_Read_BufferIndex].State == JPEG_BUFFER_EMPTY))
	{
    		Output_Is_Paused = 0;
    		HAL_JPEG_Resume(hjpeg, JPEG_PAUSE_RESUME_OUTPUT);
	}	

	return 0;
}

/**
  * @brief  
  * @param  
  * @retval 
  */
void HAL_JPEG_InfoReadyCallback(JPEG_HandleTypeDef *hjpeg, JPEG_ConfTypeDef *pInfo)
{
	if(JPEG_GetDecodeColorConvertFunc(pInfo, &pConvert_Function, &MCU_TotalNb) != HAL_OK)
  	{
    		while(1);
  	} 
}

/**
  * @brief  
  * @param  
  * @retval 
  */
void HAL_JPEG_GetDataCallback(JPEG_HandleTypeDef *hjpeg, uint32_t NbDecodedData)
{
	if(NbDecodedData == Jpeg_IN_BufferTab[0].DataBufferSize)
	{
		Jpeg_IN_BufferTab[0].State = JPEG_BUFFER_EMPTY;
		Jpeg_IN_BufferTab[0].DataBufferSize = 0;

		HAL_JPEG_Pause(hjpeg, JPEG_PAUSE_RESUME_INPUT);
	}
}

/**
  * @brief  
  * @param  
  * @retval 
  */
void HAL_JPEG_DataReadyCallback (JPEG_HandleTypeDef *hjpeg, uint8_t *pDataOut, uint32_t OutDataLength)
{
  	Jpeg_OUT_BufferTab[JPEG_OUT_Write_BufferIndex].State = JPEG_BUFFER_FULL;
  	Jpeg_OUT_BufferTab[JPEG_OUT_Write_BufferIndex].DataBufferSize = OutDataLength;
    
  	JPEG_OUT_Write_BufferIndex++;
  	if(JPEG_OUT_Write_BufferIndex >= NB_OUTPUT_DATA_BUFFERS)
  	{
    		JPEG_OUT_Write_BufferIndex = 0;        
  	}

  	if(Jpeg_OUT_BufferTab[JPEG_OUT_Write_BufferIndex].State != JPEG_BUFFER_EMPTY)
  	{
    		HAL_JPEG_Pause(hjpeg, JPEG_PAUSE_RESUME_OUTPUT);
    		Output_Is_Paused = 1;
  	}
  	HAL_JPEG_ConfigOutputBuffer(hjpeg, 
				    Jpeg_OUT_BufferTab[JPEG_OUT_Write_BufferIndex].DataBuffer, 
				    CHUNK_SIZE_OUT); 
}

/**
  * @brief  
  * @param  
  * @retval 
  */
void HAL_JPEG_ErrorCallback(JPEG_HandleTypeDef *hjpeg)
{
	while(1);
}

/**
  * @brief  
  * @param  
  * @retval 
  */
void HAL_JPEG_DecodeCpltCallback(JPEG_HandleTypeDef *hjpeg)
{
}

/**
  * @brief  
  * @param  
  * @retval 
  */
void HAL_JPEG_MspInit(JPEG_HandleTypeDef *hjpeg)
{
  static DMA_HandleTypeDef   hdmaIn;
  static DMA_HandleTypeDef   hdmaOut;

  /* Enable JPEG clock */
  __HAL_RCC_JPEG_CLK_ENABLE();

    /* Enable DMA clock */
  __HAL_RCC_DMA2_CLK_ENABLE();

  HAL_NVIC_SetPriority(JPEG_IRQn, 0x06, 0x0F);
  HAL_NVIC_EnableIRQ(JPEG_IRQn);

  /* Input DMA */
  /* Set the parameters to be configured */
  hdmaIn.Init.Channel = DMA_CHANNEL_9;
  hdmaIn.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdmaIn.Init.PeriphInc = DMA_PINC_DISABLE;
  hdmaIn.Init.MemInc = DMA_MINC_ENABLE;
  hdmaIn.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  hdmaIn.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
  hdmaIn.Init.Mode = DMA_NORMAL;
  hdmaIn.Init.Priority = DMA_PRIORITY_HIGH;
  hdmaIn.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
  hdmaIn.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
  hdmaIn.Init.MemBurst = DMA_MBURST_INC4;
  hdmaIn.Init.PeriphBurst = DMA_PBURST_INC4;

  hdmaIn.Instance = DMA2_Stream3;

  /* Associate the DMA handle */
  __HAL_LINKDMA(hjpeg, hdmain, hdmaIn);

  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0x07, 0x0F);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);

  /* DeInitialize the DMA Stream */
  HAL_DMA_DeInit(&hdmaIn);
  /* Initialize the DMA stream */
  HAL_DMA_Init(&hdmaIn);


  /* Output DMA */
  /* Set the parameters to be configured */
  hdmaOut.Init.Channel = DMA_CHANNEL_9;
  hdmaOut.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdmaOut.Init.PeriphInc = DMA_PINC_DISABLE;
  hdmaOut.Init.MemInc = DMA_MINC_ENABLE;
  hdmaOut.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  hdmaOut.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
  hdmaOut.Init.Mode = DMA_NORMAL;
  hdmaOut.Init.Priority = DMA_PRIORITY_VERY_HIGH;
  hdmaOut.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
  hdmaOut.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
  hdmaOut.Init.MemBurst = DMA_MBURST_INC4;
  hdmaOut.Init.PeriphBurst = DMA_PBURST_INC4;


  hdmaOut.Instance = DMA2_Stream4;
  /* DeInitialize the DMA Stream */
  HAL_DMA_DeInit(&hdmaOut);
  /* Initialize the DMA stream */
  HAL_DMA_Init(&hdmaOut);

  /* Associate the DMA handle */
  __HAL_LINKDMA(hjpeg, hdmaout, hdmaOut);

  HAL_NVIC_SetPriority(DMA2_Stream4_IRQn, 0x07, 0x0F);
  HAL_NVIC_EnableIRQ(DMA2_Stream4_IRQn);

}

/**
  * @brief  Initialize the DMA2D in memory to memory with PFC.
  * @param  ImageWidth: image width
  * @param  ImageHeight: image Height 
  * @retval None
  */
void DMA2D_Init(uint32_t ImageWidth, uint32_t ImageHeight)
{  
  /* Init DMA2D */
  /*##-1- Configure the DMA2D Mode, Color Mode and output offset #############*/ 
  DMA2D_Handle.Init.Mode          = DMA2D_M2M_PFC;
  DMA2D_Handle.Init.ColorMode     = DMA2D_OUTPUT_RGB565;
  DMA2D_Handle.Init.OutputOffset  = BSP_LCD_GetXSize() - ImageWidth;
  DMA2D_Handle.Init.AlphaInverted = DMA2D_REGULAR_ALPHA;  /* No Output Alpha Inversion*/  
  DMA2D_Handle.Init.RedBlueSwap   = DMA2D_RB_REGULAR;     /* No Output Red & Blue swap */   
  
  /*##-2- DMA2D Callbacks Configuration ######################################*/
  DMA2D_Handle.XferCpltCallback  = NULL;
  
  /*##-3- Foreground Configuration ###########################################*/
  DMA2D_Handle.LayerCfg[1].AlphaMode = DMA2D_REPLACE_ALPHA;
  DMA2D_Handle.LayerCfg[1].InputAlpha = 0xFF;

#if (JPEG_RGB_FORMAT == JPEG_ARGB8888)
  DMA2D_Handle.LayerCfg[1].InputColorMode = DMA2D_INPUT_ARGB8888;
  
#elif (JPEG_RGB_FORMAT == JPEG_RGB888)  
  DMA2D_Handle.LayerCfg[1].InputColorMode = DMA2D_INPUT_RGB888;
  
#elif (JPEG_RGB_FORMAT == JPEG_RGB565)  
  DMA2D_Handle.LayerCfg[1].InputColorMode = DMA2D_INPUT_RGB565;

#endif /* JPEG_RGB_FORMAT * */
  
  DMA2D_Handle.LayerCfg[1].InputOffset = 0;
  DMA2D_Handle.LayerCfg[1].RedBlueSwap = DMA2D_RB_REGULAR; /* No ForeGround Red/Blue swap */
  DMA2D_Handle.LayerCfg[1].AlphaInverted = DMA2D_REGULAR_ALPHA; /* No ForeGround Alpha inversion */
  
  DMA2D_Handle.Instance          = DMA2D; 

  /*##-4- DMA2D Initialization     ###########################################*/
  HAL_DMA2D_Init(&DMA2D_Handle);
  HAL_DMA2D_ConfigLayer(&DMA2D_Handle, 1);
}

/**
  * @brief  Copy the Decoded image to the display Frame buffer.
  * @param  pSrc: Pointer to source buffer
  * @param  pDst: Pointer to destination buffer
  * @param  ImageWidth: image width
  * @param  ImageHeight: image Height 
  * @retval None
  */
void DMA2D_CopyBuffer(uint32_t *pSrc, uint32_t *pDst, uint16_t xPos, uint16_t yPos, uint16_t ImageWidth, uint16_t ImageHeight)
{
  uint32_t destination;
  
  destination = (uint32_t)pDst + ((yPos * BSP_LCD_GetXSize()) + xPos) * 2;
  
  HAL_DMA2D_Start_IT(&DMA2D_Handle, (uint32_t)pSrc, destination, ImageWidth, ImageHeight);
}
