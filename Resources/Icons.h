/*****************************************************************************
 * FileName:        Icons_16.h
 * Processor:       PIC32MX
 * Compiler:        MPLAB C32/XC32 (see release notes for tested revision)
 * Linker:          MPLAB LINK32/XC32
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * Copyright(c) 2012 Microchip Technology Inc.  All rights reserved.
 * Microchip licenses to you the right to use, modify, copy and distribute
 * Software only when embedded on a Microchip microcontroller or digital
 * signal controller, which is integrated into your product or third party
 * product (pursuant to the sublicense terms in the accompanying license
 * agreement).
 *
 * You should refer to the license agreement accompanying this Software
 * for additional information regarding your rights and obligations.
 *
 * SOFTWARE AND DOCUMENTATION ARE PROVIDED “AS IS” WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY
 * OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR
 * PURPOSE. IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR
 * OBLIGATED UNDER CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION,
 * BREACH OF WARRANTY, OR OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT
 * DAMAGES OR EXPENSES INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL,
 * INDIRECT, PUNITIVE OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA,
 * COST OF PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY
 * CLAIMS BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF),
 * OR OTHER SIMILAR COSTS.
 *
 *
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * AUTO-GENERATED CODE:  Graphics Resource Converter version: 3.28.15
 *****************************************************************************/

#ifndef ICONS_16_H_FILE
#define ICONS_16_H_FILE
/*****************************************************************************
 * SECTION:  Includes
 *****************************************************************************/
#include <Graphics.h>
#include "HardwareProfile.h"

/*****************************************************************************
 * SECTION:  Graphics Library Firmware Check
 *****************************************************************************/
#if(GRAPHICS_LIBRARY_VERSION != 0x0306)
#warning "It is suggested to use Graphics Library version 3.06 with this version of GRC."
#endif



/*****************************************************************************
 * This is an error check for the color depth
 *****************************************************************************/
#if (COLOR_DEPTH > 16)
#error "Color Depth needs to be 16 to correctly use these resources"
#endif



/*****************************************************************************
 * SECTION:  BITMAPS
 *****************************************************************************/

/*********************************
 * Bitmap Structure
 * Label: File
 * Description:  16x16 pixels, 16-bits per pixel
 ***********************************/
extern const IMAGE_FLASH File;
#define File_WIDTH     (16)
#define File_HEIGHT    (16)
#define File_SIZE      (518)
/*********************************
 * Bitmap Structure
 * Label: Folder
 * Description:  16x16 pixels, 16-bits per pixel
 ***********************************/
extern const IMAGE_FLASH Folder;
#define Folder_WIDTH     (16)
#define Folder_HEIGHT    (16)
#define Folder_SIZE      (518)
/*********************************
 * Bitmap Structure
 * Label: Hard_disk
 * Description:  16x16 pixels, 16-bits per pixel
 ***********************************/
extern const IMAGE_FLASH Hard_disk;
#define Hard_disk_WIDTH     (16)
#define Hard_disk_HEIGHT    (16)
#define Hard_disk_SIZE      (518)
/*********************************
 * Bitmap Structure
 * Label: Movie
 * Description:  16x16 pixels, 16-bits per pixel
 ***********************************/
extern const IMAGE_FLASH Movie;
#define Movie_WIDTH     (16)
#define Movie_HEIGHT    (16)
#define Movie_SIZE      (518)
/*********************************
 * Bitmap Structure
 * Label: Music
 * Description:  16x16 pixels, 16-bits per pixel
 ***********************************/
extern const IMAGE_FLASH Music;
#define Music_WIDTH     (16)
#define Music_HEIGHT    (16)
#define Music_SIZE      (518)
/*********************************
 * Bitmap Structure
 * Label: Picture
 * Description:  16x16 pixels, 16-bits per pixel
 ***********************************/
extern const IMAGE_FLASH Picture;
#define Picture_WIDTH     (16)
#define Picture_HEIGHT    (16)
#define Picture_SIZE      (518)
#endif

