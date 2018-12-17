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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AUDIO_DECODERS_H
#define __AUDIO_DECODERS_H

#ifdef __cplusplus
 extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"
#include "ff.h"
#include "fileBrowser.h"

/* Exported types ------------------------------------------------------------*/
typedef enum {
  AUDIO_STATE_IDLE = 0,
  AUDIO_STATE_WAIT,    
  AUDIO_STATE_INIT,    
  AUDIO_STATE_PLAY,
  AUDIO_STATE_PRERECORD,
  AUDIO_STATE_RECORD,  
  AUDIO_STATE_NEXT,  
  AUDIO_STATE_PREVIOUS,
  AUDIO_STATE_FORWARD,   
  AUDIO_STATE_BACKWARD,
  AUDIO_STATE_STOP,   
  AUDIO_STATE_PAUSE,
  AUDIO_STATE_RESUME,
  AUDIO_STATE_VOLUME_UP,
  AUDIO_STATE_VOLUME_DOWN,
  AUDIO_STATE_ERROR,  
}AUDIO_PLAYBACK_StateTypeDef;

typedef enum {
   BUFFER_OFFSET_NONE = 0,
   BUFFER_OFFSET_HALF,
   BUFFER_OFFSET_FULL,
}BUFFER_StateTypeDef;

typedef struct {
  uint8_t *pbuff;
  BUFFER_StateTypeDef state;
  uint32_t fptr;
}AUDIO_OUT_BufferTypeDef;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
extern BYTE                 		blAudioPlaying;
extern __IO uint8_t 				volumeAudio;
extern FILE_FORMAT					bCallback;
extern AUDIO_PLAYBACK_StateTypeDef 	AudioState;
extern uint8_t 						error;

/* Exported functions ------------------------------------------------------- */
BYTE AudioPlayBack(FIL *pFile, FILE_FORMAT fileFormat);

#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_DECODERS_H */
