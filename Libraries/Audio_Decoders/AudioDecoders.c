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
#include "AudioDecoders.h"
#ifdef SUPPORT_WAV
	#include "waveplayer.h"
#endif
#ifdef SUPPORT_MP3
	#include "mp3player.h"
#endif
#ifdef SUPPORT_JMV
	#include "jmvplayer.h"
#endif
#ifdef SUPPORT_FLAC
	#include "flacplayer.h"
#endif

/* Private types ------------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
BYTE                        blAudioPlaying = FALSE;
__IO uint8_t 				volumeAudio = 20;
FILE_FORMAT					bCallback;
AUDIO_PLAYBACK_StateTypeDef AudioState;
uint8_t 					error;

extern __IO AUDIO_OUT_BufferTypeDef  BufferCtl;
#ifdef SUPPORT_JMV
	extern __IO AUDIO_OUT_BufferTypeDef  jmvBufferCtl;
	extern uint32_t 					 audio_buffer_size;
#endif /* SUPPORT_JMV */
extern volatile uint8_t PlayingBufIdx;
extern AUDIOBUF audiobuf[];

/* Private function prototypes ----------------------------------------------*/

/**
  * @brief  Play audio from file
  * @param  pFile	  : Pointer to file object
  * @param  fileFormat: File extension
  * @retval error: 0 no error
  *                1 error
  */
BYTE AudioPlayBack(FIL *pFile, FILE_FORMAT fileFormat) {
	BYTE bRetVal = 0;

	bCallback = fileFormat;

	switch(fileFormat)
	{
#ifdef SUPPORT_WAV
		case WAV: 	bRetVal = WavePlayBack(pFile); break;
#endif
#ifdef SUPPORT_MP3
		case MP3: 	bRetVal = MP3PlayBack(pFile); break;
#endif
#ifdef SUPPORT_FLAC
		case FLAC: 	bRetVal = FlacPlayBack(pFile); break;
#endif
		default:    bRetVal = 0xFF;
	}

     return(bRetVal);
}

/**
  * @brief  Calculates the remaining file size and new position of the pointer.
  * @param  None
  * @retval None
  */
void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
#ifdef SUPPORT_WAV
	if(bCallback == WAV) {
		if(AudioState == AUDIO_STATE_PLAY)
		{
			BufferCtl.state = BUFFER_OFFSET_FULL;

			BufferCtl.fptr += AUDIO_OUT_BUFFER_SIZE/2;
		}
	}
#endif /* SUPPORT_WAV */

#ifdef SUPPORT_MP3
	if(bCallback == MP3) {
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
#endif /* SUPPORT_MP3 */

#ifdef SUPPORT_JMV
if(bCallback == JMV) {
  	if(AudioState == AUDIO_STATE_PLAY)
  	{
    		jmvBufferCtl.state = BUFFER_OFFSET_FULL;

    		jmvBufferCtl.fptr += audio_buffer_size;
  	}
}
#endif /* SUPPORT_JMV */

#ifdef SUPPORT_FLAC
if(bCallback == FLAC) {
  	if(AudioState == AUDIO_STATE_PLAY)
  	{
		/* Data in hflac.buffer[hflac.PlayingBufIdx] has been sent out */
		hflac.buffer[hflac.PlayingBufIdx].empty = 1;
		hflac.buffer[hflac.PlayingBufIdx].size = -1;

		/* Send the data in next audio buffer */
		hflac.PlayingBufIdx++;
		if (hflac.PlayingBufIdx == MAX_AUDIOBUF_NUM)
			hflac.PlayingBufIdx = 0;

		if (hflac.buffer[hflac.PlayingBufIdx].empty == 1) {
			/* If empty==1, it means read file+decoder is slower than playback
		 	 (it will cause noise) or playback is over (it is ok). */;
		 	BSP_LED_Toggle(LED1);
		}
  	}
}
#endif /* SUPPORT_FLAC */
}

/**
  * @brief  Manages the DMA Half Transfer complete interrupt.
  * @param  None
  * @retval None
  */
void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{
#ifdef SUPPORT_WAV	
	if(bCallback == WAV) {
		if(AudioState == AUDIO_STATE_PLAY)
		{
			BufferCtl.state = BUFFER_OFFSET_HALF;

			BufferCtl.fptr += AUDIO_OUT_BUFFER_SIZE/2;
		}
	}
#endif /* SUPPORT_WAV */

#ifdef SUPPORT_MP3
	if(bCallback == MP3) {
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
#endif /* SUPPORT_MP3 */

#ifdef SUPPORT_JMV
if(bCallback == JMV) {
  	if(AudioState == AUDIO_STATE_PLAY)
  	{
    		jmvBufferCtl.state = BUFFER_OFFSET_HALF;

    		jmvBufferCtl.fptr += audio_buffer_size;
  	}
}
#endif /* SUPPORT_JMV */

#ifdef SUPPORT_FLAC
if(bCallback == FLAC) {
  	if(AudioState == AUDIO_STATE_PLAY)
  	{
		/* Data in hflac.buffer[hflac.PlayingBufIdx] has been sent out */
		hflac.buffer[hflac.PlayingBufIdx].empty = 1;
		hflac.buffer[hflac.PlayingBufIdx].size = -1;

		/* Send the data in next audio buffer */
		hflac.PlayingBufIdx++;
		if (hflac.PlayingBufIdx == MAX_AUDIOBUF_NUM)
			hflac.PlayingBufIdx = 0;

		if (hflac.buffer[hflac.PlayingBufIdx].empty == 1) {
			/* If empty==1, it means read file+decoder is slower than playback
		 	 (it will cause noise) or playback is over (it is ok). */;
		 	BSP_LED_Toggle(LED1);
		}
  	}
}
#endif /* SUPPORT_FLAC */
}
