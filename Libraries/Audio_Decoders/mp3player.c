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
  
#ifdef SUPPORT_MP3

/* Includes ------------------------------------------------------------------*/
#include "mp3player.h"
#include "coder.h"
#include "mp3dec.h"
#include "AudioDecoders.h"
#include "stm32f769i_discovery_audio.h"

/* Private types ------------------------------------------------------------*/
 enum {
 	AUDIOBUF0 = 0, AUDIOBUF1 = 1, AUDIOBUF2
 };
 
/* Private constants --------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
FIL *fileR;
HMP3Decoder hMP3Decoder;
MP3FrameInfo mp3FrameInfo;
uint8_t *readBuf, *readPtr;
int bytesLeft, outOfData, eofReached;
uint32_t pcmframe_size, *AudioBufPtr;

extern uint8_t error;

/**
  * @brief  Play from audio buf 0, then move to the next buf 
  */
volatile uint8_t PlayingBufIdx = AUDIOBUF0;
volatile uint8_t DecodeBufIdx = AUDIOBUF0;

AUDIOBUF audiobuf[MAX_AUDIOBUF_NUM] = { { NULL, -1, 1 }, 
                                        { NULL, -1, 1 } };
										
extern BYTE blAudioPlaying;
extern __IO uint8_t volumeAudio;

extern USBH_HandleTypeDef hUSBHost;

/* Private function prototypes ----------------------------------------------*/
BYTE MP3_PLAYER_Init(uint8_t Volume, uint32_t SampleRate);
BYTE MP3_PLAYER_Start(FIL *pFile);
BYTE MP3_PLAYER_Process(FIL *pFile);
void MP3_PLAYER_FreeMemory(void);
uint8_t MP3_DecodeFrame();
int MP3_FillReadBuffer(uint8_t *readBuf, uint8_t *readPtr, uint32_t bytesLeft, uint32_t bytesAlign);
void MP3_AddAudioBuf (uint32_t len);

/**
  * @brief  Play mp3 from file
  * @param  pFile: Pointer to file object
  * @retval error: 0 no error
  *                1 error
  */
BYTE MP3PlayBack(FIL *pFile)
{
	/* Set playing flag */
	blAudioPlaying = TRUE;

	error = MP3_PLAYER_Start(pFile);

	if( error != 0 )
	{
		MP3_PLAYER_FreeMemory();

		/* Clear playing flag */
		blAudioPlaying = FALSE;

		return(error);
	}

    while((outOfData == 0) && (eofReached ==0) && (BSP_SD_IsDetected() || !disk_status (1)))
	{
		error = MP3_PLAYER_Process(pFile);

		if( error != 0 )
		{
			BSP_AUDIO_OUT_Stop(CODEC_PDWN_HW);

			MP3_PLAYER_FreeMemory();

			/* Clear playing flag */
			blAudioPlaying = FALSE;

			return(error);
		}
	}

	BSP_AUDIO_OUT_Stop(CODEC_PDWN_HW);

	MP3_PLAYER_FreeMemory();

	/* Clear playing flag */
	blAudioPlaying = FALSE;

	return(0);
}

/**
  * @brief  Initializes Audio Interface.
  * @param  None
  * @retval Audio error
  */
BYTE MP3_PLAYER_Init(uint8_t Volume, uint32_t SampleRate)
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
BYTE MP3_PLAYER_Start(FIL *pFile)
{
	fileR = pFile;

	hMP3Decoder = MP3InitDecoder();
	if(hMP3Decoder == 0)
	{
		/* The MP3 Decoder could not be initialized
		 * Stop here. */

		return(200);
	}

	/* Init variables */
	bytesLeft = 0;
	outOfData = 0;
	eofReached = 0;
	pcmframe_size = 0;

	readPtr = readBuf = (uint8_t *) malloc(READBUF_SIZE);

	if(!readPtr)
		return(200); /* Memory allocation error */

	AudioBufPtr = (uint32_t *) malloc(AUDIOBUF_SIZE);

	if(!AudioBufPtr)
		return(200); /* Memory allocation error */

	audiobuf[AUDIOBUF0].BaseAddr = AudioBufPtr;
	audiobuf[AUDIOBUF1].BaseAddr = AudioBufPtr + (AUDIOBUF_SIZE / 8);

	/* Decode and playback from audio buf 0 */
    	DecodeBufIdx = PlayingBufIdx = AUDIOBUF0;
    	audiobuf[AUDIOBUF0].Empty = audiobuf[AUDIOBUF1].Empty = 1;

	/* Decode the first frame to get the frame format */
	if (MP3_DecodeFrame() == 0)
    	{
        	pcmframe_size = (mp3FrameInfo.bitsPerSample / 8) * mp3FrameInfo.outputSamps;
        	MP3_AddAudioBuf (pcmframe_size);
    	}
    	else
    	{
    		return(100); /* fail to decode the first frame */
    	}

	error = MP3_PLAYER_Init(volumeAudio, mp3FrameInfo.samprate);
	if( error != 0 )
		return(error);

	AudioState = AUDIO_STATE_PLAY;
	BSP_AUDIO_OUT_Play((uint32_t *)audiobuf[AUDIOBUF0].BaseAddr, AUDIOBUF_SIZE);

	return(0);
}

/**
  * @brief  Manages Audio process. 
  * @param  None
  * @retval Audio error
  */
BYTE MP3_PLAYER_Process(FIL *pFile)
{
	switch(AudioState)
  	{
		case AUDIO_STATE_PLAY:
			/* wait for DMA transfer */
			if(audiobuf[DecodeBufIdx].Empty == 1)
			{
				/* Decoder one frame */
				if (MP3_DecodeFrame() == 0)
				{
					pcmframe_size = (mp3FrameInfo.bitsPerSample / 8) * mp3FrameInfo.outputSamps;
					MP3_AddAudioBuf (pcmframe_size);
				}
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

			/* Reset the mp3 player variables */ 
			bytesLeft = 0;
			outOfData = 0;
			eofReached = 1;
			pcmframe_size = 0;
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
	
	return(0);
}

/**
  * @brief
  * @param
  * @retval
  */
void MP3_PLAYER_FreeMemory(void)
{
	MP3FreeDecoder(hMP3Decoder);
	free(AudioBufPtr);
	free(readBuf);

	hMP3Decoder = NULL;
	AudioBufPtr = NULL;
	readPtr = NULL;
	readBuf = NULL;
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
		/* Data in audiobuf[PlayingBufIdx] has been sent out */
		audiobuf[PlayingBufIdx].Empty = 1;
		audiobuf[PlayingBufIdx].Size = -1;

		/* Send the data in next audio buffer */
		PlayingBufIdx++;
		if (PlayingBufIdx == MAX_AUDIOBUF_NUM)
			PlayingBufIdx = 0;

		if (audiobuf[PlayingBufIdx].Empty == 1) {
			/* If empty==1, it means read file+decoder is slower than playback
		 	 (it will cause noise) or playback is over (it is ok). */;
		}
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
		/* Data in audiobuf[PlayingBufIdx] has been sent out */
		audiobuf[PlayingBufIdx].Empty = 1;
		audiobuf[PlayingBufIdx].Size = -1;

		/* Send the data in next audio buffer */
		PlayingBufIdx++;
		if (PlayingBufIdx == MAX_AUDIOBUF_NUM)
			PlayingBufIdx = 0;

		if (audiobuf[PlayingBufIdx].Empty == 1) {
			/* If empty==1, it means read file+decoder is slower than playback
		 	 (it will cause noise) or playback is over (it is ok). */;
		}
  }
}
#endif /* USE_AUDIO_DECODERS */

 /**
  * @brief  Decode a frame.
  *
  * @param  None.
  * @retval 0: success, 1: terminated.
  *
  */
 uint8_t MP3_DecodeFrame() {
 	uint8_t word_align, frame_decoded;
 	int nRead, offset, err;

 	frame_decoded = 0;
 	word_align = 0;
 	nRead = 0;
 	offset = 0;

 	do {
 		/* somewhat arbitrary trigger to refill buffer - should always be enough for a full frame */
 		if (bytesLeft < 2 * MAINBUF_SIZE && !eofReached) {
 			/* Align to 4 bytes */
 			word_align = (4 - (bytesLeft & 3)) & 3;

 			/* Fill read buffer */
 			nRead = MP3_FillReadBuffer(readBuf, readPtr, bytesLeft, word_align);

 			bytesLeft += nRead;
 			readPtr = readBuf + word_align;
 			if (nRead == 0) {
 				eofReached = 1; /* end of file */
 				outOfData = 1;
 			}
 		}

 		/* find start of next MP3 frame - assume EOF if no sync found */
 		offset = MP3FindSyncWord(readPtr, bytesLeft);
 		if (offset < 0) {
 			readPtr = readBuf;
 			bytesLeft = 0;
 			continue;
 		}
 		readPtr += offset;
 		bytesLeft -= offset;

 		//simple check for valid header
 		if (((*(readPtr + 1) & 24) == 8) || ((*(readPtr + 1) & 6) != 2) || ((*(readPtr + 2) & 240) == 240) || ((*(readPtr + 2) & 12) == 12)
 				|| ((*(readPtr + 3) & 3) == 2)) {
 			readPtr += 1; //header not valid, try next one
 			bytesLeft -= 1;
 			continue;
 		}

 		err = MP3Decode(hMP3Decoder, &readPtr, &bytesLeft, (short *) audiobuf[DecodeBufIdx].BaseAddr, 0);
 		if (err == -6) {
 			readPtr += 1;
 			bytesLeft -= 1;
 			continue;
 		}

 		if (err) {
 			/* error occurred */
 			switch (err) {
				case ERR_MP3_INDATA_UNDERFLOW:
					/* do nothing - next call to decode will provide more inData */
					break;
				case ERR_MP3_MAINDATA_UNDERFLOW:
					/* do nothing - next call to decode will provide more mainData */
					break;
				case ERR_MP3_INVALID_HUFFCODES:
				 	/* do nothing */
					break;
 				case ERR_MP3_FREE_BITRATE_SYNC:
 				default:
 					outOfData = 1;
 					break;
 			}
 		} else {
 			/* no error */
 			MP3GetLastFrameInfo(hMP3Decoder, &mp3FrameInfo);
 			frame_decoded = 1;
 		}

 	} while (!frame_decoded && !outOfData);

 	if (outOfData == 1)
 		return 0x1; /* Decoder terminated */
 	else return 0x0; /* Decoder success. */

 }

 /**
  * @brief  Read data from MP3 file and fill in the Read Buffer.
  *
  * @param  readBuf: Pointer to the start of the Read Buffer
  * @param  readPtr: Pointer to the start of the left bytes in Read Buffer
  * @param  bytesLeft: Specifies the left bytes number in Read Buffer
  * @param  bytesAlign: Specifies the pad number to make sure it is aligned to 4 bytes
  * @retval Actual number of bytes read from file.
  *
  */
 int MP3_FillReadBuffer(uint8_t *readBuf, uint8_t *readPtr, uint32_t bytesLeft, uint32_t bytesAlign) {
 	uint32_t nRead;

 	/* Move the left bytes from the end to the front */
 	memmove(readBuf + bytesAlign, readPtr, bytesLeft);

 		f_read (fileR, (void *) (readBuf + bytesLeft + bytesAlign), READBUF_SIZE - bytesLeft - bytesAlign, &nRead);
 		/* zero-pad to avoid finding false sync word after last frame (from old data in readBuf) */
 		if (nRead < READBUF_SIZE - bytesLeft - bytesAlign)
 			memset(readBuf + bytesAlign + bytesLeft + nRead, 0, READBUF_SIZE - bytesAlign - bytesLeft - nRead);

 	return nRead;
 }

 /**
   * @brief  Add an PCM frame to audio buf after decoding.
   *
   * @param  len: Specifies the frame size in bytes.
   * @retval None.
   *
   */
 void MP3_AddAudioBuf (uint32_t len)
 {
     /* Mark the status to not-empty which means it is available to playback. */
     audiobuf[DecodeBufIdx].Empty = 0;
     audiobuf[DecodeBufIdx].Size = len;

     /* Point to the next buffer */
     DecodeBufIdx ++;
     if (DecodeBufIdx == MAX_AUDIOBUF_NUM)
         DecodeBufIdx = 0;
 }

#endif /* SUPPORT_MP3 */
