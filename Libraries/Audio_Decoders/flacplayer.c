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
  
#ifdef SUPPORT_FLAC  

/* Includes ------------------------------------------------------------------*/
#include "flacplayer.h"
#include "AudioDecoders.h"
#include "stm32f769i_discovery_audio.h"

/* Private types ------------------------------------------------------------*/
enum {
	AUDIOBUF0 = 0, AUDIOBUF1 = 1, AUDIOBUF2
};

/* Private constants --------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
uint32_t pcmframe_size, eofReached, *AudioBufPtr;
FLAC_HandleTypeDef hflac;

extern uint8_t error;
extern BYTE blAudioPlaying;
extern __IO uint8_t volumeAudio;

/* Private function prototypes ----------------------------------------------*/
BYTE FLAC_PLAYER_Init(uint8_t Volume, uint32_t SampleRate);
BYTE FLAC_PLAYER_Start(FIL *pFile);
BYTE FLAC_PLAYER_Process(FIL *pFile);
void FLAC_PLAYER_FreeMemory(void);

void FLAC_AddAudioBuf(uint32_t len);

static FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data);
static FLAC__StreamDecoderSeekStatus seek_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data);
static FLAC__StreamDecoderTellStatus tell_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
static FLAC__StreamDecoderLengthStatus length_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);
static FLAC__bool eof_callback(const FLAC__StreamDecoder *decoder, void *client_data);
static FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

/**
  * @brief  Play wave from file
  * @param  pFile: Pointer to file object
  * @retval error: 0 no error
  *                1 error
  */
BYTE FlacPlayBack(FIL *pFile)
{
	/* Set playing flag */
	blAudioPlaying = TRUE;

	error = FLAC_PLAYER_Start(pFile);

	if( error != 0 )
	{
		FLAC_PLAYER_FreeMemory();

		/* Clear playing flag */
	  	blAudioPlaying = FALSE;

		return(error);
	}

    while((eofReached == 0) && (BSP_SD_IsDetected() || !disk_status (1)))
	{
		error = FLAC_PLAYER_Process(pFile);

		if( error != 0 )
		{
			BSP_AUDIO_OUT_Stop(CODEC_PDWN_HW);

			FLAC_PLAYER_FreeMemory();

			/* Clear playing flag */
	  		blAudioPlaying = FALSE;

			return(error);
		}
	}

	BSP_AUDIO_OUT_Stop(CODEC_PDWN_HW);

	FLAC_PLAYER_FreeMemory();

	/* Clear playing flag */
	blAudioPlaying = FALSE;

	return(0);	
}

/**
  * @brief  Initializes Audio Interface.
  * @param  None
  * @retval Audio error
  */
BYTE FLAC_PLAYER_Init(uint8_t Volume, uint32_t SampleRate)
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
BYTE FLAC_PLAYER_Start(FIL *pFile)
{
	FLAC__StreamDecoderInitStatus init_status;
	
	/* Save file pointer for internal use */
	hflac.fp = pFile;

	hflac.decoder = FLAC__stream_decoder_new();
	if(hflac.decoder == 0)
	{
		/* The FLAC Decoder could not be initialized
		 * Stop here. */

		return(200);
	}
	
	init_status = FLAC__stream_decoder_init_stream(	hflac.decoder, 
													read_callback,
													seek_callback,
													tell_callback,
													length_callback,
													eof_callback,
													write_callback, 
													metadata_callback,
													error_callback,
													0x00);	

	/* Init variables */
	eofReached = 0;
	pcmframe_size = 0;
	
	AudioBufPtr = (uint32_t *) malloc(FLAC_AUDIOBUF_SIZE);
	if(!AudioBufPtr)
		return(200); /* Memory allocation error */
	
	/* Decode and playback from audio buf 0 */
	hflac.DecodeBufIdx = hflac.PlayingBufIdx = AUDIOBUF0;
	hflac.buffer[AUDIOBUF0].empty = hflac.buffer[AUDIOBUF1].empty = 1;	
	
	/* Decode the first metadata block */
	if (FLAC__stream_decoder_process_until_end_of_metadata(hflac.decoder) == true)
	{
		/* Then decode the first audio frame */
		FLAC__stream_decoder_process_single(hflac.decoder);

		/* Then decode the second audio frame */
		FLAC__stream_decoder_process_single(hflac.decoder);

		pcmframe_size = hflac.frame.blocksize * hflac.frame.channels * (hflac.frame.bps / 8);

		/* Init pcm buffer */
		hflac.buffer[AUDIOBUF0].addr = AudioBufPtr;
		hflac.buffer[AUDIOBUF1].addr = AudioBufPtr + (pcmframe_size / 4);
	}
	else
	{
		return(100); /* fail to decode the first frame */
	}

	error = FLAC_PLAYER_Init(volumeAudio, hflac.metadata.sample_rate);
	if( error != 0 )
		return(error);

	AudioState = AUDIO_STATE_PLAY;
	BSP_AUDIO_OUT_Play((uint32_t *)hflac.buffer[AUDIOBUF0].addr, pcmframe_size * 2);

	return(0);	
}

/**
  * @brief  Manages Audio process. 
  * @param  None
  * @retval Audio error
  */
BYTE FLAC_PLAYER_Process(FIL *pFile)
{
	switch(AudioState)
	{
		case AUDIO_STATE_PLAY:
			eofReached = f_eof(hflac.fp);
			
			/* wait for DMA transfer */
			if(hflac.buffer[hflac.DecodeBufIdx].empty == 1)
			{
				/* Decoder one frame */
				if (FLAC__stream_decoder_process_single(hflac.decoder) == true)
				{
				}
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

			/* Reset the mp3 player variables */ 
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
void FLAC_PLAYER_FreeMemory(void)
{
	FLAC__stream_decoder_delete(hflac.decoder);
	free(AudioBufPtr);
	AudioBufPtr = NULL;
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
		}
  }
}
#endif /* USE_AUDIO_DECODERS */

/**
  * @brief  Add an PCM frame to audio buf after decoding.
  *
  * @param  len: Specifies the frame size in bytes.
  * @retval None.
  *
  */
void FLAC_AddAudioBuf(uint32_t len)
{
	/* Mark the status to not-empty which means it is available to playback. */
	hflac.buffer[hflac.DecodeBufIdx].empty = 0;
	hflac.buffer[hflac.DecodeBufIdx].size = len;

	/* Point to the next buffer */
	hflac.DecodeBufIdx ++;
	if (hflac.DecodeBufIdx == MAX_AUDIOBUF_NUM)
		hflac.DecodeBufIdx = 0;
}

/**
 * Read data callback. Called when decoder needs more input data.
 *
 * @param decoder       Decoder instance
 * @param buffer        Buffer to store read data in
 * @param bytes         Pointer to size of buffer
 * @param client_data   Client data set at initilisation
 *
 * @return Read status
 */
FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder* decoder, FLAC__byte buffer[], size_t* bytes, void* client_data)
{
	uint32_t bytesread;
	
    if (*bytes > 0) 
    {
        // read data directly into buffer
		f_read (hflac.fp, buffer, *bytes * sizeof(FLAC__byte), &bytesread);
		*bytes = bytesread / sizeof(FLAC__byte);
        if (f_error(hflac.fp)) {
            // read error -> abort
            return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
        }
        else if (*bytes == 0) {
            // EOF
            return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
        }
        else {
            // OK, continue decoding
            return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
        }
    }
    else {
        // decoder called but didn't want ay bytes -> abort
        return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
    }	
}

/**
 * Write callback. Called when decoder has decoded a single audio frame.
 *
 * @param decoder       Decoder instance
 * @param frame         Decoded frame
 * @param buffer        Array of pointers to decoded channels of data
 * @param client_data   Client data set at initilisation
 *
 * @return Read status
 */
FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder* decoder, const FLAC__Frame* frame, 
                                              const FLAC__int32* const buffer[], void* client_data)
{
	uint32_t sample, size;

	if(hflac.metadata.total_samples == 0) {
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	if(hflac.metadata.channels != 2 || hflac.metadata.bps != 16) {
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	hflac.frame.blocksize = frame->header.blocksize;
	hflac.frame.channels = frame->header.channels;
	hflac.frame.bps = frame->header.bits_per_sample;
    size = hflac.frame.blocksize * hflac.frame.channels * (hflac.frame.bps / 8);

    for (sample = 0; sample < hflac.frame.blocksize; sample++) {
		hflac.buffer[hflac.DecodeBufIdx].addr[sample] = (buffer[0][sample] << 16) | (buffer[1][sample] & 0xFFFF);
    }
	FLAC_AddAudioBuf (size);

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

/**
 * Seek callback. Called when decoder needs to seek the stream.
 *
 * @param decoder               Decoder instance
 * @param absolute_byte_offset  Offset from beginning of stream to seek to
 * @param client_data           Client data set at initilisation
 *
 * @return Seek status
 */
FLAC__StreamDecoderSeekStatus seek_callback(const FLAC__StreamDecoder* decoder, FLAC__uint64 absolute_byte_offset, void* client_data)
{
	if (f_lseek(hflac.fp, (off_t)absolute_byte_offset) < 0) {
        // seek failed
        return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
    }
    else {
        return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
    }	
}

/**
 * Tell callback. Called when decoder wants to know current position of stream.
 *
 * @param decoder               Decoder instance
 * @param absolute_byte_offset  Offset from beginning of stream to seek to
 * @param client_data           Client data set at initilisation
 *
 * @return Tell status
 */
FLAC__StreamDecoderTellStatus tell_callback(const FLAC__StreamDecoder* decoder, FLAC__uint64* absolute_byte_offset, void* client_data)
{
    if (f_tell(hflac.fp) < 0) {
        // seek failed
        return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
    }
    else {
        // update offset
        *absolute_byte_offset = (FLAC__uint64) f_tell(hflac.fp);
        return FLAC__STREAM_DECODER_TELL_STATUS_OK;
    }	
}

/**
 * Length callback. Called when decoder wants total length of stream.
 *
 * @param decoder        Decoder instance
 * @param stream_length  Total length of stream in bytes
 * @param client_data    Client data set at initilisation
 *
 * @return Length status
 */
FLAC__StreamDecoderLengthStatus length_callback(const FLAC__StreamDecoder* decoder, FLAC__uint64* stream_length, void* client_data)
{
    if (f_size(hflac.fp) == 0) {
        // failed
        return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
    }
    else {
        // pass on length
        *stream_length = (FLAC__uint64) f_size(hflac.fp);
        return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
    }	
}

/**
 * EOF callback. Called when decoder wants to know if end of stream is reached.
 *
 * @param decoder       Decoder instance
 * @param client_data   Client data set at initilisation
 *
 * @return True if end of stream
 */
FLAC__bool eof_callback(const FLAC__StreamDecoder* decoder, void* client_data)
{
	return f_eof(hflac.fp);
}

/**
 * Metadata callback. Called when decoder has decoded metadata.
 *
 * @param decoder       Decoder instance
 * @param metadata      Decoded metadata block
 * @param client_data   Client data set at initilisation
 */
void metadata_callback(const FLAC__StreamDecoder* decoder, const FLAC__StreamMetadata* metadata, void* client_data)
{
	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
	{
		/* save for later */
		hflac.metadata.total_samples = metadata->data.stream_info.total_samples;
		hflac.metadata.sample_rate = metadata->data.stream_info.sample_rate;
		hflac.metadata.channels = metadata->data.stream_info.channels;
		hflac.metadata.bps = metadata->data.stream_info.bits_per_sample;		
	}
}

/**
 * Error callback. Called when error occured during decoding.
 *
 * @param decoder       Decoder instance
 * @param status        Error
 * @param client_data   Client data set at initilisation
 */
void error_callback(const FLAC__StreamDecoder* decoder, FLAC__StreamDecoderErrorStatus status, void* client_data)
{
}	

#endif /* SUPPORT_FLAC */
