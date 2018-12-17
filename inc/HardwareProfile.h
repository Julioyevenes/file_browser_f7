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
#ifndef __HARDWARE_PROFILE_H
#define __HARDWARE_PROFILE_H

#ifdef __cplusplus
 extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/

/* Horizontal and vertical display resolution */
#define DISP_HOR_RESOLUTION		800
#define DISP_VER_RESOLUTION		480

/* Image orientation (can be 0, 90, 180, 270 degrees). */
#define DISP_ORIENTATION		0

/* Panel Data Width */
#define DISP_DATA_WIDTH         24

/* Horizontal synchronization timing in pixels */
#define DISP_HOR_PULSE_WIDTH	63  
#define DISP_HOR_BACK_PORCH		120
#define DISP_HOR_FRONT_PORCH	120	
	
/* Vertical synchronization timing in lines */
#define DISP_VER_PULSE_WIDTH	12
#define DISP_VER_BACK_PORCH		12	
#define DISP_VER_FRONT_PORCH	12

/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __HARDWARE_PROFILE_H */
