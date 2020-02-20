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
#ifndef __FLAC_PLAYER_H
#define __FLAC_PLAYER_H

#ifdef __cplusplus
 extern "C" {
#endif

#ifdef SUPPORT_FLAC 

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"
#include "ff.h"
#include "FLAC/stream_decoder.h"

/* Exported types ------------------------------------------------------------*/
typedef struct {
	uint32_t *addr;
	int32_t size;
	uint8_t empty;
}FLAC_BufferTypeDef;

typedef struct {
	uint32_t blocksize;
	uint32_t channels;
	uint32_t bps;
}FLAC_FrameTypeDef;

typedef struct {
	FLAC__uint64 total_samples;
	uint32_t sample_rate;
	uint32_t channels;
	uint32_t bps;
}FLAC_MetadataTypeDef;

#define MAX_AUDIOBUF_NUM	2

typedef struct {
	FIL *fp;
	FLAC__StreamDecoder *decoder;
	FLAC_FrameTypeDef frame;
	FLAC_MetadataTypeDef metadata;
	FLAC_BufferTypeDef buffer[MAX_AUDIOBUF_NUM];

	uint8_t PlayingBufIdx;
	uint8_t DecodeBufIdx;
}FLAC_HandleTypeDef;

/* Exported constants --------------------------------------------------------*/
#define FLAC_AUDIOBUF_SIZE     	(0x9000)	/* 36864 byte */

/* Exported macro ------------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
extern FLAC_HandleTypeDef hflac;

/* Exported functions ------------------------------------------------------- */
BYTE FlacPlayBack(FIL *pFile);

#endif /* SUPPORT_FLAC */

#ifdef __cplusplus
}
#endif

#endif /* __FLAC_PLAYER_H */
