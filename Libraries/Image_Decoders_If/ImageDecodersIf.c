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
#include "ImageDecodersIf.h"
#include "ff.h"

/* Private types ------------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
size_t ff_read(void *ptr, size_t size, size_t n, void *stream);
int    ff_seek(void *stream, long offset, int whence);
long   ff_tell(void *fo);
int    ff_eof (void *stream);

IMG_FILE_SYSTEM_API ff_if =
{
	ff_read,
	ff_seek,
	ff_tell,
	ff_eof
};

/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/

/**
  * @brief  
  * @param  
  * @retval 
  */
size_t ff_read(void *ptr, size_t size, size_t n, void *stream)
{
	size_t btr, br;

	if(size > n)
		btr = size;
	else
		btr = n;

	f_read (stream, ptr, btr, &br);

	return(br);
}

/**
  * @brief  
  * @param  
  * @retval 
  */
int    ff_seek(void *stream, long offset, int whence)
{
	FIL *fp;
	long temp_offset;

	fp = (FIL *) stream;

	switch(whence)
	{
		case 0:
			temp_offset = offset;
			break;
		case 1:
			temp_offset = fp->fptr + offset;
			break;
		case 2:
			temp_offset = fp->fsize - offset;
			break;
	}

	if( f_lseek (fp, temp_offset) == FR_OK )
		return (0);
	else
		return (-1);
}

/**
  * @brief  
  * @param  
  * @retval 
  */
long   ff_tell(void *fo)
{
	FIL *fp;

	fp = (FIL *) fo;
	return (f_tell(fp));
}

/**
  * @brief  
  * @param  
  * @retval 
  */
int    ff_eof (void *stream)
{
	FIL *fp;

	fp = (FIL *) stream;
	return (f_eof(fp));
}
