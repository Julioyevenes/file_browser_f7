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

#ifdef SUPPORT_WAV
  
/* Includes ------------------------------------------------------------------*/
#include "waveplayer.h"
#include "stm32f769i_discovery_audio.h"

/* Private types ------------------------------------------------------------*/
typedef struct {
  uint32_t ChunkID;       /* 0 */
  uint32_t FileSize;      /* 4 */
  uint32_t FileFormat;    /* 8 */
  uint32_t SubChunk1ID;   /* 12 */
  uint32_t SubChunk1Size; /* 16*/
  uint16_t AudioFormat;   /* 20 */
  uint16_t NbrChannels;   /* 22 */
  uint32_t SampleRate;    /* 24 */

  uint32_t ByteRate;      /* 28 */
  uint16_t BlockAlign;    /* 32 */
  uint16_t BitPerSample;  /* 34 */
  uint32_t SubChunk2ID;   /* 36 */
  uint32_t SubChunk2Size; /* 40 */
}WAVE_FormatTypeDef;

/* Private constants --------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
__IO AUDIO_OUT_BufferTypeDef  BufferCtl;
WAVE_FormatTypeDef WaveFormat;

extern BYTE blAudioPlaying;
extern __IO uint8_t volumeAudio;
extern AUDIO_PLAYBACK_StateTypeDef AudioState;
extern uint8_t error;

extern USBH_HandleTypeDef hUSBHost;

/* Private function prototypes ----------------------------------------------*/
BYTE WAV_PLAYER_Init(uint8_t Volume, uint32_t SampleRate);
BYTE WAV_PLAYER_Start(FIL *pFile);
BYTE WAV_PLAYER_Process(FIL *pFile);
void WAV_PLAYER_FreeMemory(void);
static BYTE GetFileInfo(FIL *pFile, WAVE_FormatTypeDef *info);

/**
  * @brief  Play wave from file
  * @param  pFile: Pointer to file object
  * @retval error: 0 no error
  *                1 error
  */
BYTE WavePlayBack(FIL *pFile)
{
	/* Set playing flag */
	blAudioPlaying = TRUE;

	error = WAV_PLAYER_Start(pFile);

	if( error != 0 )
	{
		WAV_PLAYER_FreeMemory();

		/* Clear playing flag */
	  	blAudioPlaying = FALSE;

		return(error);
	}

    while((BufferCtl.fptr < WaveFormat.FileSize) && (BSP_SD_IsDetected() || !disk_status (1)))
	{
		error = WAV_PLAYER_Process(pFile);

		if( error != 0 )
		{
			BSP_AUDIO_OUT_Stop(CODEC_PDWN_HW);

			WAV_PLAYER_FreeMemory();

			/* Clear playing flag */
	  		blAudioPlaying = FALSE;

			return(error);
		}
	}

	BSP_AUDIO_OUT_Stop(CODEC_PDWN_HW);

	WAV_PLAYER_FreeMemory();

	/* Clear playing flag */
	blAudioPlaying = FALSE;

	return(0);
}

/**
  * @brief  Initializes Audio Interface.
  * @param  None
  * @retval Audio error
  */
BYTE WAV_PLAYER_Init(uint8_t Volume, uint32_t SampleRate)
{
  if(BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_BOTH, Volume, SampleRate) == 0)
  {
	BSP_AUDIO_OUT_SetAudioFrameSlot(CODEC_AUDIOFRAME_SLOT_02);

    	return(0); // success
  }
  else
  {
    	return(100); // read error
  }
}

/**
  * @brief  Starts Audio streaming.    
  * @param  None
  * @retval Audio error
  */ 
BYTE WAV_PLAYER_Start(FIL *pFile)
{
	uint32_t bytesread;

	BufferCtl.pbuff = (uint8_t *) malloc(AUDIO_OUT_BUFFER_SIZE);

	if(!BufferCtl.pbuff)
		return(200); /* Memory allocation error */

	error = GetFileInfo(pFile, &WaveFormat);
	if( error != 0 )
		return(error);

	error = WAV_PLAYER_Init(volumeAudio, WaveFormat.SampleRate);
	if( error != 0 )
		return(error);

	BufferCtl.state = BUFFER_OFFSET_NONE;

	f_read (pFile, BufferCtl.pbuff, AUDIO_OUT_BUFFER_SIZE, &bytesread);

	if(bytesread)
	{
		AudioState = AUDIO_STATE_PLAY;

		BSP_AUDIO_OUT_Play(BufferCtl.pbuff, AUDIO_OUT_BUFFER_SIZE);
          	BufferCtl.fptr = bytesread;
          	return(0); // success
	}

	return(100); // read error
}

/**
  * @brief  Manages Audio process. 
  * @param  None
  * @retval Audio error
  */
BYTE WAV_PLAYER_Process(FIL *pFile)
{
	uint32_t bytesread;

	switch(AudioState)
  	{
		case AUDIO_STATE_PLAY:
			if(BufferCtl.state == BUFFER_OFFSET_HALF)
			{
				f_read (pFile, BufferCtl.pbuff, AUDIO_OUT_BUFFER_SIZE/2, &bytesread);
				if(!bytesread)
				{ 
					BSP_AUDIO_OUT_Stop(CODEC_PDWN_HW);
					return(100); // read error
				} 
				BufferCtl.state = BUFFER_OFFSET_NONE;
			}

			if(BufferCtl.state == BUFFER_OFFSET_FULL)
			{
				f_read (pFile, BufferCtl.pbuff + (AUDIO_OUT_BUFFER_SIZE / 2), AUDIO_OUT_BUFFER_SIZE/2, &bytesread);
				if(!bytesread)
				{ 
					BSP_AUDIO_OUT_Stop(CODEC_PDWN_HW);
					return(100); // read error
				} 
				BufferCtl.state = BUFFER_OFFSET_NONE;
			}

			/* USB Host Background task */
			USBH_Process(&hUSBHost);

#ifdef GFX_LIB
			/* Graphic user interface */
			GOL_Procedures(); //Polling mode
#endif
			break;

		case AUDIO_STATE_STOP:
			BSP_AUDIO_OUT_Stop(CODEC_PDWN_HW);
			BufferCtl.fptr = WaveFormat.FileSize; 
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
			USBH_Process(&hUSBHost);

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
void WAV_PLAYER_FreeMemory(void)
{
	free(BufferCtl.pbuff);
	BufferCtl.pbuff = NULL;
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
    BufferCtl.state = BUFFER_OFFSET_FULL;

    BufferCtl.fptr += AUDIO_OUT_BUFFER_SIZE/2;
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
    BufferCtl.state = BUFFER_OFFSET_HALF;

    BufferCtl.fptr += AUDIO_OUT_BUFFER_SIZE/2;
  }
}
#endif /* USE_AUDIO_DECODERS */

/**
  * @brief  
  * @param  
  * @retval 
  */
static BYTE GetFileInfo(FIL *pFile, WAVE_FormatTypeDef *info)
{
	uint32_t bytesread;

	if( f_read (pFile, info, sizeof(WaveFormat), &bytesread) == FR_OK )
		return(0); // success

	return(100); // read error
}

#endif /* SUPPORT_WAV */
	
