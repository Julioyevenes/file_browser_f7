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
#ifndef __FILEBROWSER_H
#define __FILEBROWSER_H

#ifdef __cplusplus
 extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
#include "GenericTypeDefs.h"
#include "Graphics.h"

/* Exported types ------------------------------------------------------------*/
/**
  * @brief   Supported file extensions
  */
 typedef enum _FILE_FORMAT
 {
 	OTHER = 0,
 	BMP_IMG,		/* Bitmap image */
 	JPEG_IMG,		/* JPEG image */
 	GIF_IMG,		/* GIF image */
 	WAV,			/* WAV audio */
 	MP3,			/* MP3 audio */
 	JMV,			/* JMV video */
 	BIN,            /* BIN executable */
 } FILE_FORMAT;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
extern char Path[];

/* Exported functions ------------------------------------------------------- */
WORD    Create_fileBrowser(void);
WORD    fileBrowser_MsgCallback(WORD objMsg, OBJ_HEADER *pObj, GOL_MSG *pMsg);
WORD    fileBrowser_DrawCallback(void);
void    MonitorDriveMedia(void);

#ifdef __cplusplus
}
#endif

#endif /* __FILEBROWSER_H */
