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
  
#ifdef SUPPORT_JMV

/* Includes ------------------------------------------------------------------*/
#include "jmvplayer.h"
#include "stm32f769i_discovery_audio.h"
#include "diskio.h"
#include "jpeg_hard_dec.h"

/* Private types ------------------------------------------------------------*/
 typedef struct _FRAME_HEADER
 {
 	/* video */
 	uint32_t frame_size;

 	/* padding */
 	uint8_t  pad[508];
 } __attribute__ ((packed)) FRAME_HEADER;

/* Private constants --------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
#define FRAME_BUFFER_PTR   			0xC0000000
#define AUDIO_SIZE         			0xFFFFFFFF

#define JPEG_OUTPUT_DATA_BUFFER  	0xC0200000
#define JPEG_PROCESS_TIMEOUT 	 	0xFFFFFFFF
#define JPEG_PROCESS_TIMEOUT_TRIM 	0x400

/* Private variables --------------------------------------------------------*/
__IO AUDIO_OUT_BufferTypeDef  jmvBufferCtl;
BYTE blVideoPlaying = FALSE;

JMV_HEADER jmvHeader;
FRAME_HEADER FrameHeader;
uint32_t frame_buffer_size;
uint32_t audio_buffer_size;

uint32_t *JPEG_ReadBufferPtr;
uint32_t JPEG_ReadBufferSize;

JPEG_HandleTypeDef    	JPEG_Handle;
JPEG_ConfTypeDef      	JPEG_Info;
uint32_t 			  	JPEG_Timeout;
uint32_t 				JPEG_Opt_Timeout;

extern AUDIO_PLAYBACK_StateTypeDef AudioState;
extern __IO uint8_t volumeAudio;
extern uint8_t error;

/* Private function prototypes ----------------------------------------------*/
BYTE JMV_PLAYER_Init(uint8_t Volume, uint32_t SampleRate);
BYTE JMV_PLAYER_Start(FIL *pFile);
BYTE JMV_PLAYER_Process(FIL *pFile);
void JMV_PLAYER_FreeMemory(void);
BYTE JMVRead(FIL *fp, DWORD buffSize, DWORD paddingBytes, DWORD *pBuff);
WORD ReadChunk(FIL *fileStream, WORD numSectors, WORD paddingSectors, DWORD *add);

/**
  * @brief  
  * @param  
  * @retval 
  */
BYTE JMVPlayBack(FIL *pFile)
{
	/* Set playing flag */
  	blVideoPlaying = TRUE;

	error = JMV_PLAYER_Start(pFile);

	if( error != 0 )
	{
		JMV_PLAYER_FreeMemory();

		/* Clear playing flag */
  		blVideoPlaying = FALSE;

		return(error);
	}

    while((jmvBufferCtl.fptr < AUDIO_SIZE) && (BSP_SD_IsDetected() || !disk_status (1)))
	{
		error = JMV_PLAYER_Process(pFile);

		if( error != 0 )
		{
			BSP_AUDIO_OUT_Stop(CODEC_PDWN_HW);
			
			JMV_PLAYER_FreeMemory();

			/* Clear playing flag */
  			blVideoPlaying = FALSE;

			return(error);
		}
	}

	BSP_AUDIO_OUT_Stop(CODEC_PDWN_HW);

	JMV_PLAYER_FreeMemory();

	/* Clear playing flag */
  	blVideoPlaying = FALSE;

	return(0);
}

/**
  * @brief  Initializes Audio Interface.
  * @param  None
  * @retval Audio error
  */
BYTE JMV_PLAYER_Init(uint8_t Volume, uint32_t SampleRate)
{
  if(BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_BOTH, Volume, SampleRate) == 0)
  {
	BSP_AUDIO_OUT_SetAudioFrameSlot(CODEC_AUDIOFRAME_SLOT_02);

    	return(0);
  }
  else
  {
    	return(100);
  }
}

/**
  * @brief  Starts Audio streaming.    
  * @param  None
  * @retval Audio error
  */ 
BYTE JMV_PLAYER_Start(FIL *pFile)
{
	if(JMVRead(pFile, sizeof(jmvHeader), 0, &jmvHeader))
		return(100); // read error

	if(!jmvHeader.frame_jpeg)
	{
		BSP_LCD_SetLayerWindow(	0,
								(BSP_LCD_GetXSize() - jmvHeader.frame_width) / 2,
								(BSP_LCD_GetYSize() - jmvHeader.frame_height) / 2,
								jmvHeader.frame_width, jmvHeader.frame_height );
	}

	frame_buffer_size = jmvHeader.frame_width * \
						jmvHeader.frame_height * \
						jmvHeader.frame_bitdepth;

	audio_buffer_size = (jmvHeader.audio_rate * \
						 jmvHeader.audio_bitdepth * \
						 jmvHeader.audio_numchannels) / jmvHeader.frame_rate;

	jmvBufferCtl.pbuff = (uint8_t *) malloc(audio_buffer_size * 2);

	if(!jmvBufferCtl.pbuff)
		return(200); /* Memory allocation error */

	error = JMV_PLAYER_Init(volumeAudio, jmvHeader.audio_rate);
	if( error != 0 )
		return(error);

	jmvBufferCtl.state = BUFFER_OFFSET_NONE;

	if(jmvHeader.frame_jpeg)
	{
		JPEG_ReadBufferPtr = (uint32_t *) malloc(jmvHeader.frame_max_size);
		if(!JPEG_ReadBufferPtr)
			return(200); /* Memory allocation error */
		JPEG_ReadBufferSize = jmvHeader.frame_max_size;

		if(JMVRead(pFile, audio_buffer_size, 0, jmvBufferCtl.pbuff))
			return(100); // read error

		memcpy(jmvBufferCtl.pbuff + audio_buffer_size, jmvBufferCtl.pbuff, audio_buffer_size);

		if(JMVRead(pFile, sizeof(FrameHeader), 0, &FrameHeader))
			return(100); // read error

		if(FrameHeader.frame_size > JPEG_ReadBufferSize)
			return(200); /* Memory allocation error */

		/* Init The JPEG Look Up Tables used for YCbCr to RGB conversion */
		JPEG_InitColorTables();

		/* Init the HAL JPEG driver */
		JPEG_Handle.Instance = JPEG;
		HAL_JPEG_Init(&JPEG_Handle);

		/* JPEG decoding with DMA (Not Blocking) Method */
		if(JPEG_Decode_DMA(&JPEG_Handle, pFile, JPEG_ReadBufferPtr, FrameHeader.frame_size))
			return(100); // read error
		JPEG_Timeout = JPEG_PROCESS_TIMEOUT;

		/* Wait till end of JPEG decoding and perfom Input/Output Processing in BackGround */
		do
		{
			JPEG_Timeout--;
		}while(JPEG_OutputHandler(&JPEG_Handle, JPEG_OUTPUT_DATA_BUFFER) == 0 && \
		       JPEG_Timeout != 0);

		/* Save the optimal timeout for JPEG decoding */
		if(JPEG_Timeout != 0)
			JPEG_Opt_Timeout = JPEG_PROCESS_TIMEOUT - JPEG_Timeout + JPEG_PROCESS_TIMEOUT_TRIM;

		/* Get JPEG Info */
		HAL_JPEG_GetInfo(&JPEG_Handle, &JPEG_Info);

		/* Initialize the DMA2D */
		DMA2D_Init(JPEG_Info.ImageWidth, JPEG_Info.ImageHeight);

		/* Copy the Decoded frame to the display frame buffer using the DMA2D */
		DMA2D_CopyBuffer((uint32_t *)JPEG_OUTPUT_DATA_BUFFER,
				 (uint32_t *)FRAME_BUFFER_PTR,
				 (BSP_LCD_GetXSize() - JPEG_Info.ImageWidth)/2,
				 (BSP_LCD_GetYSize() - JPEG_Info.ImageHeight)/2,
				 JPEG_Info.ImageWidth,
				 JPEG_Info.ImageHeight);
	}
	else
	{
		if(JMVRead(pFile, audio_buffer_size, 0, jmvBufferCtl.pbuff))
			return(100); // read error

		memcpy(jmvBufferCtl.pbuff + audio_buffer_size, jmvBufferCtl.pbuff, audio_buffer_size);

		if(JMVRead(pFile, frame_buffer_size, 0, FRAME_BUFFER_PTR))
			return(100); // read error
	}

	AudioState = AUDIO_STATE_PLAY;

	BSP_AUDIO_OUT_Play(jmvBufferCtl.pbuff, audio_buffer_size * 2);
	jmvBufferCtl.fptr = audio_buffer_size * 2;

	return(0);
}

/**
  * @brief  Manages Audio process. 
  * @param  None
  * @retval Audio error
  */
BYTE JMV_PLAYER_Process(FIL *pFile)
{
	switch(AudioState)
	{
		case AUDIO_STATE_PLAY:
			if(jmvBufferCtl.state == BUFFER_OFFSET_HALF)
			{
				if(jmvHeader.frame_jpeg)
				{
					if(JMVRead(pFile, audio_buffer_size, 0, jmvBufferCtl.pbuff))
						return(100); // read error

					if(JMVRead(pFile, sizeof(FrameHeader), 0, &FrameHeader))
						return(100); // read error

					if(FrameHeader.frame_size > JPEG_ReadBufferSize)
						return(200); /* Memory allocation error */

					/* JPEG decoding with DMA (Not Blocking) Method */
					if(JPEG_Decode_DMA(&JPEG_Handle, pFile, JPEG_ReadBufferPtr, FrameHeader.frame_size))
						return(100); // read error
					JPEG_Timeout = JPEG_Opt_Timeout;

					/* Wait till end of JPEG decoding and perfom Input/Output Processing in BackGround */
					do
					{
						JPEG_Timeout--;
					}while(JPEG_OutputHandler(&JPEG_Handle, JPEG_OUTPUT_DATA_BUFFER) == 0 && \
					       JPEG_Timeout != 0);

					if(JPEG_Timeout == 0)
					{
						HAL_JPEG_Abort(&JPEG_Handle);
					}
					else
					{
						/* Copy the Decoded frame to the display frame buffer using the DMA2D */
						DMA2D_CopyBuffer((uint32_t *)JPEG_OUTPUT_DATA_BUFFER,
								 (uint32_t *)FRAME_BUFFER_PTR,
								 (BSP_LCD_GetXSize() - JPEG_Info.ImageWidth)/2,
								 (BSP_LCD_GetYSize() - JPEG_Info.ImageHeight)/2,
								 JPEG_Info.ImageWidth,
								 JPEG_Info.ImageHeight);
					}
				}
				else
				{
					if(JMVRead(pFile, audio_buffer_size, 0, jmvBufferCtl.pbuff))
						return(100); // read error
					if(JMVRead(pFile, frame_buffer_size, 0, FRAME_BUFFER_PTR))
						return(100); // read error
				}

				jmvBufferCtl.state = BUFFER_OFFSET_NONE;
			}

			if(jmvBufferCtl.state == BUFFER_OFFSET_FULL)
			{
				if(jmvHeader.frame_jpeg)
				{
					if(JMVRead(pFile, audio_buffer_size, 0, jmvBufferCtl.pbuff + audio_buffer_size))
						return(100); // read error

					if(JMVRead(pFile, sizeof(FrameHeader), 0, &FrameHeader))
						return(100); // read error

					if(FrameHeader.frame_size > JPEG_ReadBufferSize)
						return(200); /* Memory allocation error */

					/* JPEG decoding with DMA (Not Blocking) Method */
					if(JPEG_Decode_DMA(&JPEG_Handle, pFile, JPEG_ReadBufferPtr, FrameHeader.frame_size))
						return(100); // read error
					JPEG_Timeout = JPEG_Opt_Timeout;

					/* Wait till end of JPEG decoding and perfom Input/Output Processing in BackGround */
					do
					{
						JPEG_Timeout--;
					}while(JPEG_OutputHandler(&JPEG_Handle, JPEG_OUTPUT_DATA_BUFFER) == 0 && \
					       JPEG_Timeout != 0);

					if(JPEG_Timeout == 0)
					{
						HAL_JPEG_Abort(&JPEG_Handle);
					}
					else
					{
						/* Copy the Decoded frame to the display frame buffer using the DMA2D */
						DMA2D_CopyBuffer((uint32_t *)JPEG_OUTPUT_DATA_BUFFER,
								 (uint32_t *)FRAME_BUFFER_PTR,
								 (BSP_LCD_GetXSize() - JPEG_Info.ImageWidth)/2,
								 (BSP_LCD_GetYSize() - JPEG_Info.ImageHeight)/2,
								 JPEG_Info.ImageWidth,
								 JPEG_Info.ImageHeight);
					}
				}
				else
				{
					if(JMVRead(pFile, audio_buffer_size, 0, jmvBufferCtl.pbuff + audio_buffer_size))
						return(100); // read error
					if(JMVRead(pFile, frame_buffer_size, 0, FRAME_BUFFER_PTR))
						return(100); // read error
				}

				jmvBufferCtl.state = BUFFER_OFFSET_NONE;
			}

			/* USB Host Background task */
			HUB_Process();

#ifdef GFX_LIB
			/* Graphic user interface */
			GOL_Procedures(); //Polling mode
#endif
			break;

		case AUDIO_STATE_STOP:
			BSP_AUDIO_OUT_Stop(CODEC_PDWN_HW);
			jmvBufferCtl.fptr = AUDIO_SIZE;
			break;

		case AUDIO_STATE_PAUSE:
			BSP_AUDIO_OUT_Pause();
			AudioState = AUDIO_STATE_WAIT;
			break;

		case AUDIO_STATE_RESUME:
			BSP_AUDIO_OUT_Resume();
			AudioState = AUDIO_STATE_PLAY;
			break;

		case AUDIO_STATE_WAIT:
  		case AUDIO_STATE_IDLE:
  		case AUDIO_STATE_INIT:
		default:
			/* USB Host Background task */
			HUB_Process();

#ifdef GFX_LIB
			/* Graphic user interface */
			GOL_Procedures(); //Polling mode
#endif
			break;
	}

	return(0); // success
}

/**
  * @brief
  * @param
  * @retval
  */
void JMV_PLAYER_FreeMemory(void)
{
	free(jmvBufferCtl.pbuff);
	jmvBufferCtl.pbuff = NULL;

	free(JPEG_ReadBufferPtr);
	JPEG_ReadBufferPtr = NULL;
}

#ifndef USE_AUDIO_DECODERS
/**
  * @brief  Calculates the remaining file size and new position of the pointer.
  * @param  None
  * @retval None
  */
void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
  	if(AudioState == AUDIO_STATE_PLAY)
  	{
    		jmvBufferCtl.state = BUFFER_OFFSET_FULL;

    		jmvBufferCtl.fptr += audio_buffer_size;
  	}
}

/**
  * @brief  Manages the DMA Half Transfer complete interrupt.
  * @param  None
  * @retval None
  */
void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{
  	if(AudioState == AUDIO_STATE_PLAY)
  	{
    		jmvBufferCtl.state = BUFFER_OFFSET_HALF;

    		jmvBufferCtl.fptr += audio_buffer_size;
  	}
}
#endif /* USE_AUDIO_DECODERS */

/**
  * @brief
  * @param
  * @retval
  */
BYTE JMVRead(FIL *fp, DWORD buffSize, DWORD paddingBytes, DWORD *pBuff)
{
	WORD sectorsToWrite, excludedSectors, result;

	sectorsToWrite = buffSize / _MAX_SS;
	excludedSectors = paddingBytes / _MAX_SS;

	while (sectorsToWrite)
	{
		result = ReadChunk(fp, sectorsToWrite, excludedSectors, pBuff);

		if (result == 0xFFFF)
			return(100); // read error

		sectorsToWrite -= result;
	}

	return(0); // no error
}

/**
  * @brief
  * @param
  * @retval
  */
WORD ReadChunk(FIL *fileStream, WORD numSectors, WORD paddingSectors, DWORD *add)
{
	BYTE 	csect;
	DWORD 	clst, sec_sel;
	WORD 	sectorsToRead;
	DWORD 	currentCluster;
	DWORD 	prevousCluster;
	DRESULT status = RES_OK;

	if (fileStream->fptr == 0)
		fileStream->clust = fileStream->sclust;

	sec_sel = clust2sect(fileStream->fs, fileStream->clust);
	csect = (BYTE)(fileStream->fptr / _MAX_SS & (fileStream->fs->csize - 1));
	sec_sel += csect;

	currentCluster = fileStream->clust;
	prevousCluster = currentCluster;

	// This will be the minimum sectors that are available by the card to be read
	sectorsToRead = fileStream->fs->csize - csect;

	// get as many sectors from clusters that are contiguous
	while (sectorsToRead < numSectors) {
		if (fileStream->fptr == 0) {
			clst = fileStream->sclust;
		} else {
#if _USE_FASTSEEK
			if (fileStream->cltbl)
				clst = clmt_clust(fileStream, fileStream->fptr);
			else
#endif
				clst = get_fat(fileStream->fs, fileStream->clust);
		}
		if (clst < 2) return 0xFFFF;
		if (clst == 0xFFFFFFFF) return 0xFFFF;
		fileStream->clust = clst;

		if ((prevousCluster + 1) != fileStream->clust) {
			fileStream->clust = prevousCluster;
			break;
		}

		prevousCluster++;
		sectorsToRead += fileStream->fs->csize;
	}

	// make sure that we are not over the bounds
	if (sectorsToRead > numSectors)
		sectorsToRead = numSectors;

	status = disk_read(fileStream->fs->drv, add, sec_sel, (sectorsToRead - paddingSectors));

	if (status != RES_OK) {
		fileStream->clust = currentCluster;
		return 0;
	} else {
		// update the pointers
		fileStream->fptr += (_MAX_SS * sectorsToRead);

		if (fileStream->fptr > fileStream->fsize) {
			fileStream->fptr = fileStream->fsize;
			return 0xFFFF;
		}
	}

	// get the current sector within the current cluster
	currentCluster = csect + sectorsToRead;
	while (currentCluster > fileStream->fs->csize)
		currentCluster -= fileStream->fs->csize;

	csect = currentCluster;

	// get a new cluster if necessary
	if (csect == fileStream->fs->csize) {
		csect = 0;

		if (fileStream->fptr == 0) {
			clst = fileStream->sclust;
		} else {
#if _USE_FASTSEEK
			if (fileStream->cltbl)
				clst = clmt_clust(fileStream, fileStream->fptr);
			else
#endif
				clst = get_fat(fileStream->fs, fileStream->clust);
		}
		if (clst < 2) return 0xFFFF;
		if (clst == 0xFFFFFFFF) return 0xFFFF;
		fileStream->clust = clst;
	}

	return sectorsToRead;
}

#endif /* SUPPORT_JMV */
