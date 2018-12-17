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
#include "bootloader.h"

/* Private types ------------------------------------------------------------*/
typedef void (*pFunction)(void);

/* Private constants --------------------------------------------------------*/
#if defined(DUAL_BANK)
#define FLASH_SIZE              (uint32_t)(ADDR_FLASH_SECTOR_23 - ADDR_FLASH_SECTOR_0)
#else
#define FLASH_SIZE              (uint32_t)(ADDR_FLASH_SECTOR_11 - ADDR_FLASH_SECTOR_0)
#endif

#define RAM_SIZE        		(uint32_t)0x0007D000

/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
static uint32_t flash_ptr = FLASH_USER_START_ADDR;

/* Private function prototypes ----------------------------------------------*/
static uint32_t GetSector(uint32_t Address);

/**
  * @brief  
  * @param  
  * @retval 
  */
void Bootloader_Init(void)
{
    /* Clear flash flags */
    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
    HAL_FLASH_Lock();
}

/**
  * @brief  
  * @param  
  * @retval 
  */
uint8_t Bootloader_Erase(void)
{
	uint32_t FirstSector = 0, NbOfSectors = 0, SECTORError = 0;
	static FLASH_EraseInitTypeDef EraseInitStruct;
	HAL_StatusTypeDef status = HAL_OK;

	HAL_FLASH_Unlock();

	/* Get the 1st sector to erase */
	FirstSector = GetSector(FLASH_USER_START_ADDR);

	/* Get the number of sector to erase from 1st sector*/
	NbOfSectors = GetSector(FLASH_USER_END_ADDR) - FirstSector + 1;

  	EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
  	EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
  	EraseInitStruct.Sector        = FirstSector;
  	EraseInitStruct.NbSectors     = NbOfSectors;

	status = HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError);

	HAL_FLASH_Lock();

	return (status == HAL_OK) ? BL_OK : BL_ERASE_ERROR;
}

/**
  * @brief  
  * @param  
  * @retval 
  */
void Bootloader_FlashBegin(void)
{    
    /* Reset flash destination address */
    flash_ptr = FLASH_USER_START_ADDR;
    /* Unlock flash */
    HAL_FLASH_Unlock();
}

/**
  * @brief  
  * @param  
  * @retval 
  */
uint8_t Bootloader_FlashNext(uint32_t data)
{
    if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, flash_ptr, data) == HAL_OK)
    {
        /* Check the written value */
        if(*(uint32_t*)flash_ptr != data)
        {
            /* Flash content doesn't match source content */
            HAL_FLASH_Lock();
            return BL_WRITE_ERROR;
        }   
        /* Increment Flash destination address */
        flash_ptr += 4;
    }
    else
    {
        /* Error occurred while writing data into Flash */
        HAL_FLASH_Lock();
        return BL_WRITE_ERROR;
    }
    
    return BL_OK;
}

/**
  * @brief  
  * @param  
  * @retval 
  */
void Bootloader_FlashEnd(void)
{   
    /* Lock flash */
    HAL_FLASH_Lock();
}

/**
  * @brief  
  * @param  
  * @retval 
  */
uint8_t Bootloader_CheckSize(uint32_t appsize)
{
    return ((FLASH_BASE + FLASH_SIZE - FLASH_USER_START_ADDR) >= appsize) ? BL_OK : BL_SIZE_ERROR;
}

/**
  * @brief
  * @param
  * @retval
  */
uint8_t Bootloader_CheckForApplication(void)
{
    return ( (*(__IO uint32_t*)FLASH_USER_START_ADDR) != 0xFFFFFFFF ) ? BL_OK : BL_NO_APP;
}

/**
  * @brief
  * @param
  * @retval
  */
void Bootloader_JumpToApplication(void)
{
    uint32_t  JumpAddress = *(__IO uint32_t*)(FLASH_USER_START_ADDR + 4);
    pFunction Jump = (pFunction)JumpAddress;
    
    HAL_RCC_DeInit();
    HAL_DeInit();
    
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;
    
#if (SET_VECTOR_TABLE)
    SCB->VTOR = FLASH_USER_START_ADDR;
#endif
    
    __set_MSP(*(__IO uint32_t*)FLASH_USER_START_ADDR);
    Jump();
}

/**
  * @brief  Gets the sector of a given address
  * @param  None
  * @retval The sector of a given address
  */
static uint32_t GetSector(uint32_t Address)
{
  uint32_t sector = 0;

  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_SECTOR_0;
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_SECTOR_1;
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_SECTOR_2;
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_SECTOR_3;
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_SECTOR_4;
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = FLASH_SECTOR_5;
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sector = FLASH_SECTOR_6;
  }
  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
  {
    sector = FLASH_SECTOR_7;
  }
  else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
  {
    sector = FLASH_SECTOR_8;
  }
  else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
  {
    sector = FLASH_SECTOR_9;
  }
  else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
  {
    sector = FLASH_SECTOR_10;
  }
#if defined(DUAL_BANK)
  else if((Address < ADDR_FLASH_SECTOR_12) && (Address >= ADDR_FLASH_SECTOR_11))
  {
    sector = FLASH_SECTOR_11;
  }
  else if((Address < ADDR_FLASH_SECTOR_13) && (Address >= ADDR_FLASH_SECTOR_12))
  {
    sector = FLASH_SECTOR_12;
  }
  else if((Address < ADDR_FLASH_SECTOR_14) && (Address >= ADDR_FLASH_SECTOR_13))
  {
    sector = FLASH_SECTOR_13;
  }
  else if((Address < ADDR_FLASH_SECTOR_15) && (Address >= ADDR_FLASH_SECTOR_14))
  {
    sector = FLASH_SECTOR_14;
  }
  else if((Address < ADDR_FLASH_SECTOR_16) && (Address >= ADDR_FLASH_SECTOR_15))
  {
    sector = FLASH_SECTOR_15;
  }
  else if((Address < ADDR_FLASH_SECTOR_17) && (Address >= ADDR_FLASH_SECTOR_16))
  {
    sector = FLASH_SECTOR_16;
  }
  else if((Address < ADDR_FLASH_SECTOR_18) && (Address >= ADDR_FLASH_SECTOR_17))
  {
    sector = FLASH_SECTOR_17;
  }
  else if((Address < ADDR_FLASH_SECTOR_19) && (Address >= ADDR_FLASH_SECTOR_18))
  {
    sector = FLASH_SECTOR_18;
  }
  else if((Address < ADDR_FLASH_SECTOR_20) && (Address >= ADDR_FLASH_SECTOR_19))
  {
    sector = FLASH_SECTOR_19;
  }
  else if((Address < ADDR_FLASH_SECTOR_21) && (Address >= ADDR_FLASH_SECTOR_20))
  {
    sector = FLASH_SECTOR_20;
  }
  else if((Address < ADDR_FLASH_SECTOR_22) && (Address >= ADDR_FLASH_SECTOR_21))
  {
    sector = FLASH_SECTOR_21;
  }
  else if((Address < ADDR_FLASH_SECTOR_23) && (Address >= ADDR_FLASH_SECTOR_22))
  {
    sector = FLASH_SECTOR_22;
  }
  else /* (Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_23) */
  {
    sector = FLASH_SECTOR_23;
  }
#else
  else /* (Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_11) */
  {
    sector = FLASH_SECTOR_11;
  }
#endif /* DUAL_BANK */
  return sector;
}
