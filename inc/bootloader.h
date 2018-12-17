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
#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#ifdef __cplusplus
 extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"

/* Exported types ------------------------------------------------------------*/

/* Bootloader Error Codes */
enum
{
    BL_OK = 0,
    BL_NO_APP,
    BL_SIZE_ERROR,
    BL_CHKS_ERROR,
    BL_ERASE_ERROR,
    BL_WRITE_ERROR,
    BL_OBP_ERROR
};

/* Exported constants --------------------------------------------------------*/
#define FLASH_USER_START_ADDR   (uint32_t)0x08080000   /* Start @ of user Flash area */
#define FLASH_USER_END_ADDR     (uint32_t)0x080FFFFB   /* End @ of user Flash area */

/* Exported macro ------------------------------------------------------------*/

#define SET_VECTOR_TABLE        1       /* Automatically set vector table location before launching application */

/* Base address of the Flash sectors */
#if defined(DUAL_BANK)
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base address of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base address of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base address of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base address of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base address of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base address of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base address of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base address of Sector 7, 128 Kbytes */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base address of Sector 8, 128 Kbytes */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base address of Sector 9, 128 Kbytes */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base address of Sector 10, 128 Kbytes */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base address of Sector 11, 128 Kbytes */
#define ADDR_FLASH_SECTOR_12    ((uint32_t)0x08100000) /* Base address of Sector 12, 16 Kbytes */
#define ADDR_FLASH_SECTOR_13    ((uint32_t)0x08104000) /* Base address of Sector 13, 16 Kbytes */
#define ADDR_FLASH_SECTOR_14    ((uint32_t)0x08108000) /* Base address of Sector 14, 16 Kbytes */
#define ADDR_FLASH_SECTOR_15    ((uint32_t)0x0810C000) /* Base address of Sector 15, 16 Kbytes */
#define ADDR_FLASH_SECTOR_16    ((uint32_t)0x08110000) /* Base address of Sector 16, 64 Kbytes */
#define ADDR_FLASH_SECTOR_17    ((uint32_t)0x08120000) /* Base address of Sector 17, 128 Kbytes */
#define ADDR_FLASH_SECTOR_18    ((uint32_t)0x08140000) /* Base address of Sector 18, 128 Kbytes */
#define ADDR_FLASH_SECTOR_19    ((uint32_t)0x08160000) /* Base address of Sector 19, 128 Kbytes */
#define ADDR_FLASH_SECTOR_20    ((uint32_t)0x08180000) /* Base address of Sector 20, 128 Kbytes */
#define ADDR_FLASH_SECTOR_21    ((uint32_t)0x081A0000) /* Base address of Sector 21, 128 Kbytes */
#define ADDR_FLASH_SECTOR_22    ((uint32_t)0x081C0000) /* Base address of Sector 22, 128 Kbytes */
#define ADDR_FLASH_SECTOR_23    ((uint32_t)0x081E0000) /* Base address of Sector 23, 128 Kbytes */
#else
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base address of Sector 0, 32 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08008000) /* Base address of Sector 1, 32 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08010000) /* Base address of Sector 2, 32 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x08018000) /* Base address of Sector 3, 32 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08020000) /* Base address of Sector 4, 128 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08040000) /* Base address of Sector 5, 256 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08080000) /* Base address of Sector 6, 256 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x080C0000) /* Base address of Sector 7, 256 Kbytes */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08100000) /* Base address of Sector 8, 256 Kbytes */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x08140000) /* Base address of Sector 9, 256 Kbytes */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x08180000) /* Base address of Sector 10, 256 Kbytes */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x081C0000) /* Base address of Sector 11, 256 Kbytes */
#endif /* DUAL_BANK */

/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void    Bootloader_Init(void);
uint8_t Bootloader_Erase(void);

void    Bootloader_FlashBegin(void);
uint8_t Bootloader_FlashNext(uint32_t data);
void    Bootloader_FlashEnd(void);

void    Bootloader_JumpToApplication(void);

#ifdef __cplusplus
}
#endif

#endif /* __BOOTLOADER_H */
