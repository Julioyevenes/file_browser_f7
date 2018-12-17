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
#include "DisplayDriver.h"
#include "stm32f769i_discovery_lcd.h"

/* Private types ------------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
// Color
GFX_COLOR   _color;
GFX_COLOR   _chrcolor;
#ifdef USE_TRANSPARENT_COLOR
	GFX_COLOR   _colorTransparent;
	SHORT       _colorTransparentEnable;
#endif
// Clipping region control
SHORT       _clipRgn;
// Clipping region borders
SHORT       _clipLeft;
SHORT       _clipTop;
SHORT       _clipRight;
SHORT       _clipBottom;
// Active Page
BYTE  _activePage;
// Visual Page
BYTE  _visualPage;

/* Private function prototypes ----------------------------------------------*/

#ifdef USE_TRANSPARENT_COLOR
/**
  * @brief
  * @param
  * @retval
  */
void TransparentColorEnable(GFX_COLOR color) {
    _colorTransparent = color;    
    _colorTransparentEnable = TRANSPARENT_COLOR_ENABLE;
}
#endif

/**
  * @brief
  * @param
  * @retval
  */
void ResetDevice(void) {
  /* LCD DSI initialization in mode Video Burst */
  /* Initialize DSI LCD */
#if(DISP_ORIENTATION == 0 || DISP_ORIENTATION == 180)
  while(BSP_LCD_InitEx(LCD_ORIENTATION_LANDSCAPE) != LCD_OK);
#endif
#if(DISP_ORIENTATION == 90 || DISP_ORIENTATION == 270)
  while(BSP_LCD_InitEx(LCD_ORIENTATION_PORTRAIT) != LCD_OK);
#endif

  BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);

  BSP_LCD_LayerDefaultInit(1, LCD_FB_START_ADDRESS + 0x500000);
  
  BSP_LCD_SetTransparency(1, 0x7F);

  BSP_LCD_SelectLayer(1);
  BSP_LCD_Clear(0);

  BSP_LCD_SelectLayer(0);
}

/**
  * @brief
  * @param
  * @retval
  */
void PutPixel(SHORT x, SHORT y) {
    if(_clipRgn)
    {
        if(x < _clipLeft)
            return;
        if(x > _clipRight)
            return;
        if(y < _clipTop)
            return;
        if(y > _clipBottom)
            return;
    }

#if (COLOR_DEPTH == 16)
	BSP_LCD_DrawPixel(x, y, _color);
#elif (COLOR_DEPTH == 24)
	BSP_LCD_DrawPixel(x, y, _color | 0xFF000000);
#endif
}

/**
  * @brief
  * @param
  * @retval
  */
GFX_COLOR GetPixel(SHORT x, SHORT y) {
	if(x < 0)
		return(0);
	if(x > GetMaxX())
		return(0);
	if(y < 0)
		return(0);
	if(y > GetMaxY())
		return(0);

#if (COLOR_DEPTH == 16)
	return( BSP_LCD_ReadPixel(x, y) );
#elif (COLOR_DEPTH == 24)
	return( BSP_LCD_ReadPixel(x, y) & 0x00FFFFFF );
#endif
}

/**
  * @brief
  * @param
  * @retval
  */
WORD IsDeviceBusy() {
	return(0);
}

#if (COLOR_DEPTH == 24)
/**
  * @brief
  * @param
  * @retval
  */
void ClearDevice(void) {
	BSP_LCD_Clear(_color | 0xFF000000);
}
#endif
