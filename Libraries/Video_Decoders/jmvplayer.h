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
#ifndef __JMV_PLAYER_H
#define __JMV_PLAYER_H

#ifdef __cplusplus
 extern "C" {
#endif 

#ifdef SUPPORT_JMV

/* Includes ------------------------------------------------------------------*/
#include "AudioDecoders.h"
#include "ff.h"

/* Exported types ------------------------------------------------------------*/
 typedef struct _JMV_HEADER
 {
 	/* video */
 	uint16_t frame_width;
 	uint16_t frame_height;
 	uint8_t  frame_bitdepth;
 	uint8_t  frame_rate;

 	/* audio */
 	uint8_t  audio_numchannels;
 	uint8_t  audio_bitdepth;
 	uint16_t audio_rate;

 	/* padding */
 	uint8_t  pad[502];
 } JMV_HEADER;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
extern JMV_HEADER 					 jmvHeader;
extern __IO AUDIO_OUT_BufferTypeDef  jmvBufferCtl;
extern 		BYTE     				 blVideoPlaying;
extern uint32_t 					 audio_buffer_size;

/* Exported functions ------------------------------------------------------- */
BYTE JMVPlayBack(FIL *pFile);

#endif /* SUPPORT_JMV */

#ifdef __cplusplus
}
#endif

#endif /* __JMV_PLAYER_H */
