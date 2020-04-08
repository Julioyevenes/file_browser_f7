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
#ifndef __JPEG_HARD_DEC_H
#define __JPEG_HARD_DEC_H

#ifdef __cplusplus
 extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
#include "jpeg_utils.h"
#include "ff.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
BYTE JPEG_Decode_DMA(JPEG_HandleTypeDef *hjpeg, FIL *pFile, uint32_t *ReadBufferPtr, uint32_t ReadBufferSize);
BYTE JPEG_OutputHandler(JPEG_HandleTypeDef *hjpeg, uint32_t FrameBufferAddress);

void DMA2D_Init(uint32_t ImageWidth, uint32_t ImageHeight);
void DMA2D_CopyBuffer(uint32_t *pSrc, uint32_t *pDst, uint16_t xPos, uint16_t yPos, uint16_t ImageWidth, uint16_t ImageHeight);

#ifdef __cplusplus
}
#endif

#endif /* __JPEG_HARD_DEC_H */
