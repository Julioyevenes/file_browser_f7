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
#include "fileBrowser.h"
#include "stm32f7xx_hal.h"
#include "ImageDecoder.h"
#include "ImageDecodersIf.h"
#include "AudioDecoders.h"
#include "ff_gen_drv.h"
#include "main.h"
#ifdef SUPPORT_JMV
	#include "jmvplayer.h"
#endif /* SUPPORT_JMV */

#include "bootloader.h"

/**
  * @brief   Resources
  */
#include "Icons.h"
#include "ArialUnicode.h"

/* Private types ------------------------------------------------------------*/
/**
  * @brief   Structure for thumb drive information
  */
typedef struct _VOLUME_INFO
{
    char    label[12];
    BYTE    valid;
} VOLUME_INFO;

/**
  * @brief   Structure for folder/directory information
  */
typedef struct _FolderElement
{
    char            Name[13];
    XCHAR           XName[13];
    char            LFN[255];
    XCHAR           XLFN[255];
    BYTE            blFolder : 1;
    BYTE            blVolume : 1;
    FILE_FORMAT     FmtType;
} FolderElement;

/**
  * @brief   File browser errors
  */
typedef enum _FB_ERROR
{
	NO_ERROR = 0,
	READ_ERROR = 100,
	MEMORY_ERROR = 200,
	WRITE_ERROR = 300,
	DELETE_ERROR = 400,
	FORMAT_ERROR = 500,
	FILE_ALREADY_EXISTS = 600,
	UNSUPPORTED_FORMAT = 255
} FB_ERROR;

/* Private constants --------------------------------------------------------*/
/**
  * @brief   Strings
  */
const XCHAR                 DetectingStr[] = {'D','e','t','e','c','t','i','n','g',0};
const XCHAR                 PleaseWaitStr[] = {'P','l','e','a','s','e',' ','W','a','i','t','.','.','.',0};
const XCHAR                 UpArrowStr[] = {'+',0};
const XCHAR                 DownArrowStr[] = {'-',0};
const XCHAR                 Exiting1Str[] = {'N','o',' ','D','r','i','v','e',' ','D','e','t','e','c','t','e','d',0};
const XCHAR                 UnsupportedStr[] = {'U','n','s','u','p','p','o','r','t','e','d',0};
const XCHAR                 FormatStr[] = {'F','o','r','m','a','t',0};
const XCHAR 				StopStr[] = {'S','t','o','p',0};
const XCHAR 				ResumeStr[] = {'R','e','s','u','m','e',0};
const XCHAR 				PauseStr[] = {'P','a','u','s','e',0};
const XCHAR 				ReadStr[] = {'R','e','a','d',0};
const XCHAR 				ErrorStr[] = {'E','r','r','o','r',0};
const XCHAR 				MemoryAllocationStr[] = {'M','e','m','o','r','y',' ','a','l','l','o','c','a','t','i','o','n',0};
const XCHAR 				WriteStr[] = {'W','r','i','t','e',0};
const XCHAR 				OpenStr[] = {'O','p','e','n',0};
const XCHAR 				CopyStr[] = {'C','o','p','y',0};
const XCHAR 				PasteStr[] = {'P','a','s','t','e',0};
const XCHAR 				DeleteStr[] = {'D','e','l','e','t','e',0};
const XCHAR 				YesStr[] = {'Y','e','s',0};
const XCHAR 				NoStr[] = {'N','o',0};
const XCHAR 				FileStr[] = {'F','i','l','e',0};
const XCHAR 				AlreadyExistsStr[] = {'A','l','r','e','a','d','y',' ','E','x','i','s','t','s',0};

/* Private macro ------------------------------------------------------------*/
#define MAX_ELEMENTS            100
#define SLIDERSCROLLDELAY   	50
#define AUTOPLAYDELAY   		100

#define SLOW_BUFF_SIZE 			512
#define FAST_BUFF_SIZE 			(1024 * 32)

#define WAIT_UNTIL_FINISH(x)    while(!x)

/**
  * @brief  Object IDs 
  */
#define ID_JPGLISTBOX   0xFC10                              // List Box ID
#define ID_BTNUP4LB     0xFC11                              // Up Button ID
#define ID_BTNDN4LB     0xFC12                              // Down Button ID
#define ID_SLD4LB       0xFC13                              // Slider ID
#define ID_SLDVOL       0xFC14                          	// SliderVol ID

#define ID_BUTTON_A		101
#define ID_BUTTON_B		102
#define ID_BUTTON_C		103
#define ID_BUTTON_D		104

/**
  * @brief  Object dimensions 
  */
#define SCROLLBTNWIDTH  35
#define SCROLLBTNHEIGHT 35

#define LBJPGXPOS       (0)
#define LBJPGYPOS       (0)
#define LBJPGWIDTH      (BSP_LCD_GetXSize() - SCROLLBTNWIDTH - 1)    // width
#define LBJPGHEIGHT     (BSP_LCD_GetYSize() - 46)                    // height (36 is taken from the dimension of the navigation control buttons)
#define BTNUP4LBXPOS    (LBJPGXPOS + LBJPGWIDTH + 1)
#define BTNUP4LBYPOS    (LBJPGYPOS)
#define BTNUP4LBWIDTH   (SCROLLBTNWIDTH)
#define BTNUP4LBHEIGHT  (SCROLLBTNHEIGHT)
#define SLD4LBXPOS      (BTNUP4LBXPOS)
#define SLD4LBYPOS      (BTNUP4LBYPOS + SCROLLBTNHEIGHT + 1)
#define SLD4LBWIDTH     (SCROLLBTNWIDTH)
#define SLD4LBHEIGHT    (LBJPGHEIGHT - ((SCROLLBTNHEIGHT + 1) << 1))
#define BTNDN4LBXPOS    (BTNUP4LBXPOS)
#define BTNDN4LBYPOS	(LBJPGHEIGHT-SCROLLBTNHEIGHT)    
#define BTNDN4LBWIDTH   (SCROLLBTNWIDTH)
#define BTNDN4LBHEIGHT  (SCROLLBTNHEIGHT)

#define CTRLBTN_XINDENT         0
#define CTRLBTN_HEIGHT          45
#define CTRLBTN_WIDTH           (((BSP_LCD_GetXSize() + 1) - (CTRLBTN_XINDENT * 2)) / 4)
#define CtrlBtnTop()            (BSP_LCD_GetYSize() - CTRLBTN_HEIGHT)
#define CtrlBtnBottom()         BSP_LCD_GetYSize()
#define CtrlBtnLeft(column)     (((column + 1) * CTRLBTN_XINDENT) + (column * CTRLBTN_WIDTH))
#define CtrlBtnRight(column)    ((column + 1) * (CTRLBTN_XINDENT + CTRLBTN_WIDTH))

#define ID_SLDVOL_X0     CtrlBtnLeft(1)
#define ID_SLDVOL_Y0     CtrlBtnTop()
#define ID_SLDVOL_X      CtrlBtnRight(2)
#define ID_SLDVOL_Y      CtrlBtnBottom()

#define MOUSE_WINDOW_X                  0
#define MOUSE_WINDOW_Y                  0
#define MOUSE_WINDOW_HEIGHT             BSP_LCD_GetYSize()
#define MOUSE_WINDOW_WIDTH              BSP_LCD_GetXSize()

/* Private variables --------------------------------------------------------*/
/**
  * @brief   Audio player variables
  */
extern BYTE                 		blAudioPlaying;
extern AUDIO_PLAYBACK_StateTypeDef 	AudioState;

#ifdef SUPPORT_JMV
/**
  * @brief   Video player variables
  */
	extern JMV_HEADER 	jmvHeader;
	extern BYTE 		blVideoPlaying;
	BYTE 				bVideoPauseMenu = FALSE;
#endif /* SUPPORT_JMV */

/**
  * @brief   File system variables
  */
BYTE                        mediaPresent[3] = {FALSE, FALSE, FALSE};
VOLUME_INFO                 volume[3];
FILINFO 					finfo;
FIL 						fSrc, fDst;
FATFS 						FatFs[3];
char 						Path1[4], Path2[4], Path3[4];

FolderElement               aFolderElement[MAX_ELEMENTS];
FolderElement 				SelectElement;
FolderElement 				CopyElement;
BYTE                        bStartingElement;
WORD                        bNumElementsInFolder;
BYTE                        blImageOnScreen;
extern __IO DWORD           tick;
BYTE        				bAutoPlay;
BYTE						NextFile;
BYTE        				bFileProcess;
BYTE          				bFileCtrls;
BYTE 						bFileCopy;
BYTE 						bConfirmCtrls;
BYTE 						bShowVolumes = TRUE;
CHAR 						lfnbuff[256], SrcPath[256], DstPath[256], *SrcPathptr = SrcPath;

int16_t  prev_x = 5, prev_y = 1;
uint8_t  mouse_button = 0;

extern USBH_HandleTypeDef hUSBHost[];
extern uint8_t host_state;

extern Disk_drvTypeDef  	disk;

/**
  * @brief   Pointers to screen objects
  */
BUTTON                      *pBtnUp, *pBtnDn;
BUTTON 						*pBtnA, *pBtnB, *pBtnC, *pBtnD;
LISTBOX                     *pListBox;
SLIDER                      *pSlider;
XCHAR                       *pFBItemList = NULL;
SLIDER 						*pSliderVol;
  
/* Private function prototypes ----------------------------------------------*/
void                        FillNewElements(void);
void                        DisplayErrorInfo(FB_ERROR nError);
WORD 						CreateCtrlButtons(XCHAR *pTextA, XCHAR *pTextB, XCHAR *pTextC, XCHAR *pTextD);
FILE_FORMAT 				getFileExt(FILINFO *finfo);
BOOL 						GOLDeleteObj(OBJ_HEADER *object);
BYTE						AutoPlay(void);
void 						SetAudioCtrls(BOOL Set);
void 						SetFileCtrls(BOOL Set);
void 						SetConfirmCtrls(BOOL Set);
void 						CopyFolderElement(FolderElement *Src, FolderElement *Dst);
BYTE 						CopyFile(FIL *Src, FIL *Dst);
BYTE 						CopyFolder(const char *pathSrc, const char *pathDst, const char *SrcName);
BYTE 						DeleteFolder(const char *path);
void 						Bootloader_Process(FolderElement *obj);
void 						USR_MOUSE_ProcessData(HID_MOUSE_Info_TypeDef *data);
static void 				HID_MOUSE_UpdatePosition(int8_t x, int8_t y);

/**
  * @brief  
  * @param  
  * @retval 
  */
WORD    Create_fileBrowser(void) {
    BYTE    TextHeight;
    WORD    TextX;

    // Free memory for the objects in the previous linked list and start new list to display
    // the files seen on the media
    GOLFree();

    // initialize the image decoder
    ImageDecoderInit();

    // initialize the screen	
    SetColor(WHITE);
    ClearDevice();

    SetFont((void *) &ARIALUNI_18);
    TextHeight = GetTextHeight((void *) &ARIALUNI_18);

    SetColor(BLACK);
    TextX = (BSP_LCD_GetXSize() - GetTextWidth((XCHAR *)DetectingStr, (void *) &ARIALUNI_18)) / 2;
    WAIT_UNTIL_FINISH(OutTextXY(TextX, 3 * TextHeight, (XCHAR *)DetectingStr));
    TextX = (BSP_LCD_GetXSize() - GetTextWidth((XCHAR *)PleaseWaitStr, (void *) &ARIALUNI_18)) / 2;
    WAIT_UNTIL_FINISH(OutTextXY(TextX, 6 * TextHeight, (XCHAR *)PleaseWaitStr));

    blImageOnScreen = 0;
    blAudioPlaying = FALSE;
    bAutoPlay = FALSE;
    bFileProcess = FALSE;
    NextFile = 0xFF;

    // create the listbox, slider and buttons that will emulate a
    // list box with controls.
    pListBox = LbCreate
        (
            ID_JPGLISTBOX,
            LBJPGXPOS,
            LBJPGYPOS,
            LBJPGXPOS + LBJPGWIDTH,
            LBJPGYPOS + LBJPGHEIGHT,
            LB_DRAW | LB_SINGLE_SEL,
            pFBItemList,
            NULL
        );

    if(pListBox == NULL)
        return (0);

    pSlider = SldCreate
        (
            ID_SLD4LB,
            SLD4LBXPOS,
            SLD4LBYPOS,
            SLD4LBXPOS + SLD4LBWIDTH,
            SLD4LBYPOS + SLD4LBHEIGHT,
            SLD_DRAW | SLD_VERTICAL | SLD_SCROLLBAR,
            2,
            1,
            1,  // these are temporary fill items will set to proper values
            NULL
        );
    if(pSlider == NULL)
        return (0);

    pBtnUp = BtnCreate
        (
            ID_BTNUP4LB,
            BTNUP4LBXPOS,
            BTNUP4LBYPOS,
            BTNUP4LBXPOS + BTNUP4LBWIDTH,
            BTNUP4LBYPOS + BTNUP4LBHEIGHT,
            0,
            BTN_DRAW,
            NULL,
            (XCHAR *)UpArrowStr,
            NULL
        );

    if(pBtnUp == NULL)
        return (0);

    pBtnDn = BtnCreate
        (
            ID_BTNDN4LB,
            BTNDN4LBXPOS,
            BTNDN4LBYPOS,
            BTNDN4LBXPOS + BTNDN4LBWIDTH,
            BTNDN4LBYPOS + BTNDN4LBHEIGHT,
            0,
            BTN_DRAW,
            NULL,
            (XCHAR *)DownArrowStr,
            NULL
        );

    if(pBtnDn == NULL)
        return (0);
		
    pSliderVol = SldCreate
        (
			ID_SLDVOL,                                              // object’s ID
			ID_SLDVOL_X0, ID_SLDVOL_Y0, ID_SLDVOL_X, ID_SLDVOL_Y,   // object’s dimension
			SLD_DRAW,                                				// object state after creation
			70,                                                     // range
			1,                                                      // page
			volumeAudio,                                            // initial position
			NULL                                                    // use default style scheme
        );
    if(pSliderVol == NULL)
        return (0);

    // create the control buttons at the bottom of the screen
    if(!CreateCtrlButtons(NULL, NULL, NULL, NULL))
		return (0);
	
    pBtnA = (BUTTON *)GOLFindObject(ID_BUTTON_A);
    pBtnB = (BUTTON *)GOLFindObject(ID_BUTTON_B);
    pBtnC = (BUTTON *)GOLFindObject(ID_BUTTON_C);
    pBtnD = (BUTTON *)GOLFindObject(ID_BUTTON_D);

    GOLDeleteObj(pSliderVol);

    MonitorDriveMedia();

    // set the first item to be focused
    LbSetFocusedItem(pListBox, 0);

    // successful creation of the JPEG demo screen
    return (1);
}

/**
  * @brief  
  * @param  
  * @retval 
  */
WORD    fileBrowser_MsgCallback(WORD objMsg, OBJ_HEADER *pObj, GOL_MSG *pMsg) {
    LISTITEM    *pItemSel;
    FB_ERROR 	bError;
    FRESULT 	fr;
    DIR 		dir;

    // check if an image is being shown
    if(blImageOnScreen == 1)
    {
        // this is the routine to go back and show the list when an
        // image is being shown on the screen or the slide show is
        // currently ongoing
        if(pMsg->uiEvent == EVENT_RELEASE)
        {
            blImageOnScreen = 0;

            GOLRedrawRec(0, 0, BSP_LCD_GetXSize(), BSP_LCD_GetYSize());

            SetFileCtrls(FALSE);
        }

        return (0);
    }

#ifdef SUPPORT_JMV
    // check if an video file is now playing
    if(blVideoPlaying == TRUE && bVideoPauseMenu == FALSE)
    {
    	if(pMsg->uiEvent == EVENT_RELEASE)
    	{
    		BSP_LCD_SetLayerVisible(1, ENABLE);

    		bVideoPauseMenu = TRUE;
    		AudioState = AUDIO_STATE_PAUSE;

    		if(!jmvHeader.frame_jpeg)
    			BSP_LCD_SetLayerWindow(0, 0, 0, BSP_LCD_GetXSize(), BSP_LCD_GetYSize());

    		SetAudioCtrls(TRUE);
    		BtnSetText(pBtnD, ResumeStr);
    	}

    	return (0);
    }
#endif /* SUPPORT_JMV */

    switch(GetObjID(pObj))
    {
        case ID_BUTTON_A:
            if(objMsg == BTN_MSG_RELEASED)
            {   
				// check if button is pressed
                // do not process if user moved the touch away from the button
                // returning 1 will update the button
                if(pMsg->uiEvent == EVENT_MOVE)
                    return (1);

                // check if an audio file is now playing
                if(blAudioPlaying == TRUE)
				{
					/* Disable the continuous playback */
					bAutoPlay = FALSE;

					AudioState = AUDIO_STATE_STOP;

					SetAudioCtrls(FALSE);
					SetFileCtrls(FALSE);

					return (1);
				}

#ifdef SUPPORT_JMV
                // check if an video file is now playing
                if(blVideoPlaying == TRUE)
                {
					/* Disable the continuous playback */
					bAutoPlay = FALSE;

                	bVideoPauseMenu = FALSE;

                	AudioState = AUDIO_STATE_STOP;

                	SetAudioCtrls(FALSE);
                	SetFileCtrls(FALSE);

                	return (1);
                }
#endif /* SUPPORT_JMV */

                if(bFileCtrls == TRUE)
				{
    				if(SelectElement.blVolume == 1 && \
    				   SelectElement.blFolder == 1)
    				{
    					bShowVolumes = TRUE;
    					FillNewElements();
    					return (1);
    				}
    				else if(SelectElement.blVolume == 1 && \
    						SelectElement.blFolder == 0)
    				{
    					bShowVolumes = FALSE;

    					if( f_chdrive (SelectElement.Name) == FR_OK )
    					{
    						FillNewElements();
    						return (1);
    					}
    				}
    				else if(SelectElement.blVolume == 0 && \
    						SelectElement.blFolder == 1)
    				{
    					if(bFileCopy == FALSE)
    						SetFileCtrls(FALSE);

    					if( f_chdir (SelectElement.Name) == FR_OK )
    					{
    						FillNewElements();
    						return (1);
    					}
    				}
    				else if(SelectElement.FmtType == BMP_IMG  || \
    						SelectElement.FmtType == JPEG_IMG || \
							SelectElement.FmtType == GIF_IMG)
					{
						if( f_open (&fSrc, SelectElement.Name, FA_READ) == FR_OK )
						{
							blImageOnScreen = 1;

							// initialize the screen
							SetColor(BLACK);
							ClearDevice();

							bError = ImageDecode
									(
									&fSrc,
									SelectElement.FmtType,
									0,
									0,
									BSP_LCD_GetXSize(),
									BSP_LCD_GetYSize(),
									(IMG_DOWN_SCALE | IMG_ALIGN_CENTER),
									&ff_if,
									NULL
									);

							if(bError)
								DisplayErrorInfo(bError);

							f_close (&fSrc);
						}
					}
					else if(SelectElement.FmtType == WAV || \
							SelectElement.FmtType == MP3 || \
							SelectElement.FmtType == FLAC)
					{
						/* Enable the continuous playback */
						bAutoPlay = TRUE;

						SetAudioCtrls(TRUE);
					}
#ifdef SUPPORT_JMV
					else if(SelectElement.FmtType == JMV)
					{
						/* Enable the continuous playback */
						bAutoPlay = TRUE;
					}
#endif /* SUPPORT_JMV */
					else if(SelectElement.FmtType == BIN)
					{
						Bootloader_Process(SelectElement.Name);

					  	/* Launch application */
					  	Bootloader_JumpToApplication();
					}
					else if(SelectElement.FmtType == OTHER)
					{
						DisplayErrorInfo(UNSUPPORTED_FORMAT);
						GOLRedrawRec(0, 0, BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
					}
				}
            }

            return (1);

        case ID_BUTTON_B:
           if(objMsg == BTN_MSG_RELEASED)
           {   // check if button is pressed
               // do not process if user moved the touch away from the button
               // returning 1 wil update the button
               if(pMsg->uiEvent == EVENT_MOVE)
                   return (1);

               if(bConfirmCtrls == TRUE)
               {
            	   if(SelectElement.blVolume == 1 && \
            	      SelectElement.blFolder == 0)
            	   {
            		   if( f_mkfs (SelectElement.Name, 0, 0) != FR_OK)
            			   DisplayErrorInfo(FORMAT_ERROR);
            	   }
            	   else if(SelectElement.blFolder == 1)
            	   {
            		   DisplayErrorInfo(DeleteFolder(SelectElement.Name));
            	   }
            	   else
            	   {
					   if( f_unlink (SelectElement.Name) != FR_OK)
						   DisplayErrorInfo(DELETE_ERROR);
            	   }

            	   FillNewElements();

            	   SetConfirmCtrls(FALSE);
            	   SetFileCtrls(FALSE);
            	   bConfirmCtrls = FALSE;

            	   return (1);
               }

               if(bFileCtrls == TRUE)
               {
            	   if(SelectElement.blFolder == 1)
            	   {
            		   SrcPathptr = &SrcPath;
            		   f_getcwd (&SrcPath, sizeof(SrcPath));
            		   if(SrcPath[3] == 0)
            		   {
            			   SrcPathptr += strlen(SrcPath);
            			   strcpy(SrcPathptr, SelectElement.Name);
            		   }
            		   else
            		   {
						   SrcPathptr += strlen(SrcPath);
						   strcpy(SrcPathptr, "/");
						   SrcPathptr += strlen("/");
						   strcpy(SrcPathptr, SelectElement.Name);
            		   }
            	   }
            	   else
            	   {
            		   if( f_open (&fSrc, SelectElement.Name, FA_READ) != FR_OK )
            			   DisplayErrorInfo(READ_ERROR);
            	   }

            	   CopyFolderElement(&SelectElement, &CopyElement);

            	   SetState(pBtnB, BTN_DISABLED);
            	   ClrState(pBtnC, BTN_DISABLED);
            	   GOLRedrawRec(0, 0, BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
            	   bFileCopy = TRUE;
               }
           }

           return (1);

        case ID_BUTTON_C:
           if(objMsg == BTN_MSG_RELEASED)
           {   // check if button is pressed
               // do not process if user moved the touch away from the button
               // returning 1 wil update the button
               if(pMsg->uiEvent == EVENT_MOVE)
                   return (1);

               if(bConfirmCtrls == TRUE)
               {
            	   SetConfirmCtrls(FALSE);
            	   SetFileCtrls(TRUE);
            	   bConfirmCtrls = FALSE;

            	   return (1);
               }

               if(bFileCtrls == TRUE)
               {
            	   if(CopyElement.blFolder == 1)
            	   {
            		   f_getcwd (&DstPath, sizeof(DstPath));

            		   DisplayErrorInfo(CopyFolder(SrcPath, DstPath, CopyElement.Name));
            	   }
            	   else
            	   {
            		   fr = f_findfirst (&dir, &finfo, "", "*.*");
            		   while(fr == FR_OK)
            		   {
            			   if(finfo.fname[0] == 0)
            				   break;

            			   if(strcmp(finfo.fname, CopyElement.Name) == 0)
            			   {
            				   DisplayErrorInfo(FILE_ALREADY_EXISTS);
            				   f_close (&fSrc);

            				   SetFileCtrls(FALSE);
            				   bFileCtrls = FALSE;
            				   bFileCopy = FALSE;

            				   return (1);
            			   }

            			   fr = f_findnext(&dir, &finfo);
            		   }

            		   if( f_open (&fDst, CopyElement.Name, 	FA_CREATE_NEW | FA_WRITE) == FR_OK )
            		   {
            			   DisplayErrorInfo(CopyFile(&fSrc, &fDst));

            			   f_close (&fSrc);
            			   f_close (&fDst);
            		   }
            		   else
            		   {
            			   DisplayErrorInfo(READ_ERROR);
            		   }
            	   }

            	   FillNewElements();

            	   SetFileCtrls(FALSE);
            	   bFileCtrls = FALSE;
            	   bFileCopy = FALSE;
               }
           }

           return (1);

        case ID_BUTTON_D:
            if(objMsg == BTN_MSG_RELEASED)
            {   // check if button is pressed
                // do not process if user moved the touch away from the button
                // returning 1 wil update the button
                if(pMsg->uiEvent == EVENT_MOVE)
                    return (1);

                // check if an audio file is now playing
                if(blAudioPlaying == TRUE)
                {
                	if(AudioState == AUDIO_STATE_PLAY)
                	{
                		AudioState = AUDIO_STATE_PAUSE;
                		BtnSetText(pBtnD, ResumeStr);
                		SetState(pBtnD, BTN_DRAW);
                	}
                	else if(AudioState == AUDIO_STATE_WAIT)
                	{
                		AudioState = AUDIO_STATE_RESUME;
                		BtnSetText(pBtnD, PauseStr);
                		SetState(pBtnD, BTN_DRAW);
                	}

                	return (1);
                }

#ifdef SUPPORT_JMV
                // check if an video file is now playing
                if(blVideoPlaying == TRUE)
                {
                	BSP_LCD_SetLayerVisible(1, DISABLE);

                	bVideoPauseMenu = FALSE;
                	AudioState = AUDIO_STATE_RESUME;

                	if(!jmvHeader.frame_jpeg)
                	{
						BSP_LCD_SetLayerWindow(	0,
												(BSP_LCD_GetXSize() - jmvHeader.frame_width) / 2,
												(BSP_LCD_GetYSize() - jmvHeader.frame_height) / 2,
												jmvHeader.frame_width, jmvHeader.frame_height );
                	}

                	SetAudioCtrls(FALSE);

                	return (1);
                }
#endif /* SUPPORT_JMV */

                if(bFileCtrls == TRUE)
                {
                	SetConfirmCtrls(TRUE);
                	bConfirmCtrls = TRUE;
                }
            }

            return (1);

        case ID_JPGLISTBOX:
            if(pMsg->uiEvent == EVENT_MOVE)
            {
                pMsg->uiEvent = EVENT_PRESS;            // change event for listbox

                // Process message by default
                LbMsgDefault(objMsg, (LISTBOX *)pObj, pMsg);

                // Set new list box position
                SldSetPos(pSlider, LbGetCount(pListBox) - LbGetFocusedItem(pListBox) - 1);
                SetState(pSlider, SLD_DRAW_THUMB);
                pMsg->uiEvent = EVENT_MOVE;             // restore event for listbox
            }
            else if(pMsg->uiEvent == EVENT_PRESS)
            {
                // call the message default processing of the list box to select the item
                LbMsgDefault(objMsg, (LISTBOX *)pObj, pMsg);
            }
            else if(pMsg->uiEvent == EVENT_RELEASE)
            {
                // check which item was selected and display appropriate screen
                pItemSel = LbGetSel(pListBox, NULL);    // get the selected item

                /* Sets the starting file for AutoPlay function */
				NextFile = pItemSel->data;

                CopyFolderElement(&aFolderElement[pItemSel->data], &SelectElement);

                SetFileCtrls(TRUE);

                if(aFolderElement[pItemSel->data].blVolume == 1 && \
                   aFolderElement[pItemSel->data].blFolder == 0)
                	BtnSetText(pBtnD, FormatStr);

                bFileCtrls = TRUE;
            }

            // The message was processed. To avoid other objects processing the 
            // processed message reset the message.
            pMsg->uiEvent = EVENT_INVALID;
            return (0);

        case ID_SLD4LB:
        	
        	if((objMsg == SLD_MSG_INC) || (objMsg == SLD_MSG_DEC)) 
            {   // check slider was touched.

	            // Process message by default
	            SldMsgDefault(objMsg, (SLIDER *)pObj, pMsg);
	
	            // Set new list box position
	            if(LbGetFocusedItem(pListBox) != LbGetCount(pListBox) - SldGetPos(pSlider))
	            {
	                LbSetFocusedItem(pListBox, LbGetCount(pListBox) - SldGetPos(pSlider));
	                SetState(pListBox, LB_DRAW_ITEMS);
	            }
	            
	        }

            // The message was processed
            return (0);

        case ID_BTNUP4LB:                               // slider up button was pressed
            if(objMsg == BTN_MSG_RELEASED)
            {
				// check if we have reached the very first then stay there.
	            if (LbGetFocusedItem(pListBox) == 0)
	                LbSetFocusedItem(pListBox,0);
	            else    
                	LbSetFocusedItem(pListBox,LbGetFocusedItem(pListBox)-1);                
                SetState(pListBox, LB_DRAW_ITEMS);
                SldSetPos(pSlider, SldGetPos(pSlider) + 1);
                SetState(pSlider, SLD_DRAW_THUMB);
            }

            return (1);

        case ID_BTNDN4LB:                               // slider down button was pressed
            if(objMsg == BTN_MSG_RELEASED)
            {
	            // set all items to be not displayed
                pItemSel = pListBox->pItemList;
                while(pItemSel != NULL) {
                	pItemSel->status = 0;
                	pItemSel = pItemSel->pNextItem;
                }	
                LbSetFocusedItem(pListBox, LbGetFocusedItem(pListBox) + 1);
                SetState(pListBox, LB_DRAW_ITEMS);
                SldSetPos(pSlider, SldGetPos(pSlider) - 1);
                SetState(pSlider, SLD_DRAW_THUMB);
            }

            return (1);
			
        case ID_SLDVOL:

        	if((objMsg == SLD_MSG_INC) || (objMsg == SLD_MSG_DEC))
                {   // check slider was touched.

	            	// Process message by default
	            	SldMsgDefault(objMsg, (SLIDER *)pObj, pMsg);

	            	// check if an audio/video file is now playing
#ifdef SUPPORT_JMV
                    if(blAudioPlaying == TRUE || blVideoPlaying == TRUE)
#else
		            if(blAudioPlaying == TRUE)
#endif /* SUPPORT_JMV */
                    {
                    	volumeAudio = SldGetPos(pSliderVol);
                    	volTask = SET;
                    }
                }

            // The message was processed
            return (0);
    }

    return (1);
}

/**
  * @brief  
  * @param  
  * @retval 
  */
WORD    fileBrowser_DrawCallback(void) {
    static DWORD    prevTick = 0;   // keeps previous value of tick	
    SHORT           item;
    HID_MOUSE_Info_TypeDef *m_pinfo;

	if(host_state == HOST_USER_CLASS_ACTIVE && \
	   USBH_GetActiveClass(&hUSBHost[4]) == USB_HID_CLASS)
	{
		m_pinfo = USBH_HID_GetMouseInfo(&hUSBHost[4]);
		if(m_pinfo != NULL)
		{
			mouse_button = m_pinfo->buttons[0];

			/* Handle Mouse data position */
    		USR_MOUSE_ProcessData(m_pinfo);
		}

		if( (blVideoPlaying == TRUE && bVideoPauseMenu == TRUE) ||\
		    (blVideoPlaying == FALSE) )
		{
			BSP_LCD_SetLayerVisible(1, ENABLE);
		}
	}
	else
	{
		BSP_LCD_SetLayerVisible(1, DISABLE);
	}

	MonitorDriveMedia();

    // check if media is still present, if not return to main menu
	if(mediaPresent[0] == FALSE)
		f_mount(NULL, (TCHAR const*)Path1, 1);

	if(mediaPresent[1] == FALSE)
		f_mount(NULL, (TCHAR const*)Path2, 1);

	if(mediaPresent[2] == FALSE)
		f_mount(NULL, (TCHAR const*)Path3, 1);

	if(mediaPresent[0] == FALSE && mediaPresent[1] == FALSE && mediaPresent[2] == FALSE)
	{
		// Free memory used by graphic objects
		GOLFree();

		SetColor(WHITE);
		ClearDevice();
		screenState = CREATE_FILEBROWSER;
		return (1);
	}
	
    // this implements the automatic playback of the multimedia list box contents
    if(bAutoPlay && !bFileProcess)
    {
    	if(aFolderElement[NextFile].FmtType == WAV || \
    	   aFolderElement[NextFile].FmtType == MP3 || \
		   aFolderElement[NextFile].FmtType == FLAC)
    	{
    		if((tick - prevTick) > AUTOPLAYDELAY)
    		{
				AutoPlay();
    			prevTick = tick;
    		}
    	}
#ifdef SUPPORT_JMV
    	else
    	{
    		AutoPlay();
    	}
#endif /* SUPPORT_JMV */
    }
    else
    {
    	// this implements the automatic scrolling of the list box contents
    	// when the up or down buttons are continuously pressed.
    	if((tick - prevTick) > SLIDERSCROLLDELAY)
    	{
    		item = LbGetFocusedItem(pListBox);
    		if(GetState(pBtnUp, BTN_PRESSED))
    		{

    			// redraw only the listbox when the item that is focused has changed
    			LbSetFocusedItem(pListBox, LbGetFocusedItem(pListBox) - 1);
    			if(item != LbGetFocusedItem(pListBox))
    			{
    				SetState(pListBox, LB_DRAW_ITEMS);
    			}

    			SldSetPos(pSlider, SldGetPos(pSlider) + 1);
    			SetState(pSlider, SLD_DRAW_THUMB);
    		}

    		if(GetState(pBtnDn, BTN_PRESSED))
    		{
    			LbSetFocusedItem(pListBox, LbGetFocusedItem(pListBox) + 1);
    			if(item != LbGetFocusedItem(pListBox))
    			{
    				SetState(pListBox, LB_DRAW_ITEMS);
    			}

    			SldSetPos(pSlider, SldGetPos(pSlider) - 1);
    			SetState(pSlider, SLD_DRAW_THUMB);
    		}

    		prevTick = tick;
    	}
    }
	
	return (1);
}

/**
  * @brief  
  * @param  
  * @retval 
  */
void FillNewElements(void) {
	CHAR 		PathStr[256];
    WORD        bCounter;
    FRESULT 	fr;
    DIR 		dir;
    void        *pBitmap;

    bNumElementsInFolder = 0;
    finfo.lfname = &lfnbuff[0]; // Init FATFS LFN buffer
    finfo.lfsize = sizeof(lfnbuff);

    if(bShowVolumes)
    {
    	for(bCounter = 0; bCounter < _VOLUMES; bCounter++)
    	{
    		if(volume[bCounter].valid)
    		{
    			aFolderElement[bNumElementsInFolder].blVolume = 1;
    			aFolderElement[bNumElementsInFolder].blFolder = 0;

    			strcpy(aFolderElement[bNumElementsInFolder].Name, volume[bCounter].label);
    			strcpy(aFolderElement[bNumElementsInFolder].XName, volume[bCounter].label);

    			strcpy(aFolderElement[bNumElementsInFolder].LFN, volume[bCounter].label);
    			strcpy(aFolderElement[bNumElementsInFolder].XLFN, volume[bCounter].label);

    			bNumElementsInFolder++;
    		}
    	}
    }
    else
    {
    	f_getcwd (&PathStr, sizeof(PathStr));

    	if( (strcmp(PathStr, volume[0].label) == 0) || \
    		(strcmp(PathStr, volume[1].label) == 0) || \
			(strcmp(PathStr, volume[2].label) == 0) )
    	{
    		aFolderElement[bNumElementsInFolder].blVolume = 1;
    		aFolderElement[bNumElementsInFolder].blFolder = 1;

    		strcpy(aFolderElement[bNumElementsInFolder].Name, "..");
    		strcpy(aFolderElement[bNumElementsInFolder].XName, "..");

    		strcpy(aFolderElement[bNumElementsInFolder].LFN, "..");
    		strcpy(aFolderElement[bNumElementsInFolder].XLFN, "..");

    		bNumElementsInFolder++;
    	}

    	fr = f_findfirst (&dir, &finfo, "", "*");
    	for(bCounter = 0; bCounter < MAX_ELEMENTS; bCounter++)
    	{
    		if(fr == FR_OK)
    		{
    			if(finfo.fattrib == AM_DIR)
    			{
                	BYTE    i;
                	if( (finfo.fname[0] == '.' && finfo.fname[1] == '\0') || \
                	     finfo.fname[0] == '\0' )
                	{
                    	fr = f_findnext(&dir, &finfo);
                    	continue;
                	}

                	aFolderElement[bNumElementsInFolder].blVolume = 0;
                	aFolderElement[bNumElementsInFolder].blFolder = 1;
                	for(i = 0; i < 13; i++)
                	{
                		aFolderElement[bNumElementsInFolder].Name[i] = finfo.fname[i];
                		aFolderElement[bNumElementsInFolder].XName[i] = finfo.fname[i];
                	}
                	if(dir.lfn_idx != 0xFFFF) // If LFN is available
                	{
                		for(i = 0; i < 255; i++)
                		{
                			aFolderElement[bNumElementsInFolder].LFN[i] = finfo.lfname[i];
                			aFolderElement[bNumElementsInFolder].XLFN[i] = finfo.lfname[i];
                		}
                	}
                	else
                	{
                		for(i = 0; i < 13; i++)
                		{
                			aFolderElement[bNumElementsInFolder].LFN[i] = finfo.fname[i];
                			aFolderElement[bNumElementsInFolder].XLFN[i] = finfo.fname[i];
                		}
                	}

                	bNumElementsInFolder++;
    			}
    		}
            else
            {
                	break;
            }

    		fr = f_findnext(&dir, &finfo);
    	}

    	fr = f_findfirst (&dir, &finfo, "", "*.*");
    	for(bCounter = 0; bCounter < MAX_ELEMENTS && bNumElementsInFolder < MAX_ELEMENTS; bCounter++)
    	{
            if(fr == FR_OK)
            {
            	if( (finfo.fattrib == AM_ARC) || (finfo.fattrib == (AM_ARC | AM_RDO)) )
            	{
                	BYTE    i;
                	if( (finfo.fname[0] == '.' || finfo.fname[0] == '\0') )
                	{
                		fr = f_findnext(&dir, &finfo);
                		continue;
                	}

                	aFolderElement[bNumElementsInFolder].blVolume = 0;
                	aFolderElement[bNumElementsInFolder].blFolder = 0;
                	aFolderElement[bNumElementsInFolder].FmtType = getFileExt(&finfo);
                	for(i = 0; i < 13; i++)
                	{
                		aFolderElement[bNumElementsInFolder].Name[i] = finfo.fname[i];
                		aFolderElement[bNumElementsInFolder].XName[i] = finfo.fname[i];
                	}
                	if(dir.lfn_idx != 0xFFFF) // If LFN is available
                	{
                		for(i = 0; i < 255; i++)
                		{
                			aFolderElement[bNumElementsInFolder].LFN[i] = finfo.lfname[i];
                			aFolderElement[bNumElementsInFolder].XLFN[i] = finfo.lfname[i];
                		}
                	}
                	else
                	{
                		for(i = 0; i < 13; i++)
                		{
                			aFolderElement[bNumElementsInFolder].LFN[i] = finfo.fname[i];
                			aFolderElement[bNumElementsInFolder].XLFN[i] = finfo.fname[i];
                		}
                	}

                	bNumElementsInFolder++;
            	}
            }
            else
            {
                	break;
            }

            fr = f_findnext(&dir, &finfo);
    	}
    }

    // fill the list box items
    bStartingElement = 0;

    // clear the list first
    LbDelItemsList(pListBox);

    for(bCounter = 0; (bStartingElement + bCounter) < bNumElementsInFolder; bCounter++)
    {

        // set the proper bitmap icon
    	if(aFolderElement[bStartingElement + bCounter].blVolume == 1 && \
    	   aFolderElement[bStartingElement + bCounter].blFolder == 0)
    	{
    		pBitmap = (void *) &Hard_disk;
    	}
    	else if(aFolderElement[bStartingElement + bCounter].blFolder == 1)
        {
            pBitmap = (void *) &Folder;
        }
        else if(aFolderElement[bStartingElement + bCounter].FmtType == JPEG_IMG)
        {
            pBitmap = (void *) &Picture;
        }
        else if(aFolderElement[bStartingElement + bCounter].FmtType == BMP_IMG)
        {
            pBitmap = (void *) &Picture;
        }
        else if(aFolderElement[bStartingElement + bCounter].FmtType == FLAC)
        {
            pBitmap = (void *) &Music;
        }
        else if(aFolderElement[bStartingElement + bCounter].FmtType == MP3)
        {
            pBitmap = (void *) &Music;
        }
		else if(aFolderElement[bStartingElement + bCounter].FmtType == WAV)
        {
            pBitmap = (void *) &Music;
        }
#ifdef SUPPORT_JMV
		else if(aFolderElement[bStartingElement + bCounter].FmtType == JMV)
		{
			pBitmap = (void *) &Movie;
		}
#endif /* SUPPORT_JMV */
        else
        {
            pBitmap = (void *) &File;
        }

        // assign the item to the list
        if(NULL == LbAddItem(pListBox, NULL, aFolderElement[bStartingElement + bCounter].XLFN, pBitmap, 0, bCounter))
            break;
        else
        {

            // adjust the slider page and range
            SldSetRange(pSlider, bNumElementsInFolder);
            SldSetPage(pSlider, bNumElementsInFolder >> 1);

            // to completely redraw the object when GOLDraw() is executed.
            SetState(pSlider, DRAW);
            SetState(pListBox, DRAW);
        }
    }
}

/**
  * @brief  
  * @param  
  * @retval 
  */
void DisplayErrorInfo(FB_ERROR nError) {
	WORD    TextX, TextY, TextHeight;

    	SetColor(WHITE);
    	ClearDevice();

    	SetColor(BRIGHTRED);
    	SetFont((void *) &ARIALUNI_18);

    	TextHeight = GetTextHeight((void *) &ARIALUNI_18);
    	TextY = (BSP_LCD_GetYSize() - 3 * TextHeight) / 2;

    	switch(nError) {
		case READ_ERROR:
    			TextX = (BSP_LCD_GetXSize() - GetTextWidth((XCHAR *)ReadStr, (void *) &ARIALUNI_18)) / 2;
    			WAIT_UNTIL_FINISH(OutTextXY(TextX, TextY, (XCHAR *)ReadStr));
    			TextX = (BSP_LCD_GetXSize() - GetTextWidth((XCHAR *)ErrorStr, (void *) &ARIALUNI_18)) / 2;
    			WAIT_UNTIL_FINISH(OutTextXY(TextX, TextY + 2 * TextHeight, (XCHAR *)ErrorStr));
			break;

		case MEMORY_ERROR:
    			TextX = (BSP_LCD_GetXSize() - GetTextWidth((XCHAR *)MemoryAllocationStr, (void *) &ARIALUNI_18)) / 2;
    			WAIT_UNTIL_FINISH(OutTextXY(TextX, TextY, (XCHAR *)MemoryAllocationStr));
    			TextX = (BSP_LCD_GetXSize() - GetTextWidth((XCHAR *)ErrorStr, (void *) &ARIALUNI_18)) / 2;
    			WAIT_UNTIL_FINISH(OutTextXY(TextX, TextY + 2 * TextHeight, (XCHAR *)ErrorStr));
			break;

		case WRITE_ERROR:
				TextX = (BSP_LCD_GetXSize() - GetTextWidth((XCHAR *)WriteStr, (void *) &ARIALUNI_18)) / 2;
    			WAIT_UNTIL_FINISH(OutTextXY(TextX, TextY, (XCHAR *)WriteStr));
    			TextX = (BSP_LCD_GetXSize() - GetTextWidth((XCHAR *)ErrorStr, (void *) &ARIALUNI_18)) / 2;
    			WAIT_UNTIL_FINISH(OutTextXY(TextX, TextY + 2 * TextHeight, (XCHAR *)ErrorStr));
			break;

		case DELETE_ERROR:
				TextX = (BSP_LCD_GetXSize() - GetTextWidth((XCHAR *)DeleteStr, (void *) &ARIALUNI_18)) / 2;
    			WAIT_UNTIL_FINISH(OutTextXY(TextX, TextY, (XCHAR *)DeleteStr));
    			TextX = (BSP_LCD_GetXSize() - GetTextWidth((XCHAR *)ErrorStr, (void *) &ARIALUNI_18)) / 2;
    			WAIT_UNTIL_FINISH(OutTextXY(TextX, TextY + 2 * TextHeight, (XCHAR *)ErrorStr));
			break;

		case FORMAT_ERROR:
				TextX = (BSP_LCD_GetXSize() - GetTextWidth((XCHAR *)FormatStr, (void *) &ARIALUNI_18)) / 2;
				WAIT_UNTIL_FINISH(OutTextXY(TextX, TextY, (XCHAR *)FormatStr));
				TextX = (BSP_LCD_GetXSize() - GetTextWidth((XCHAR *)ErrorStr, (void *) &ARIALUNI_18)) / 2;
				WAIT_UNTIL_FINISH(OutTextXY(TextX, TextY + 2 * TextHeight, (XCHAR *)ErrorStr));
			break;

		case FILE_ALREADY_EXISTS:
				TextX = (BSP_LCD_GetXSize() - GetTextWidth((XCHAR *)FileStr, (void *) &ARIALUNI_18)) / 2;
		    	WAIT_UNTIL_FINISH(OutTextXY(TextX, TextY, (XCHAR *)FileStr));
		    	TextX = (BSP_LCD_GetXSize() - GetTextWidth((XCHAR *)AlreadyExistsStr, (void *) &ARIALUNI_18)) / 2;
		    	WAIT_UNTIL_FINISH(OutTextXY(TextX, TextY + 2 * TextHeight, (XCHAR *)AlreadyExistsStr));
			break;

		case UNSUPPORTED_FORMAT:
    			TextX = (BSP_LCD_GetXSize() - GetTextWidth((XCHAR *)UnsupportedStr, (void *) &ARIALUNI_18)) / 2;
    			WAIT_UNTIL_FINISH(OutTextXY(TextX, TextY, (XCHAR *)UnsupportedStr));
    			TextX = (BSP_LCD_GetXSize() - GetTextWidth((XCHAR *)FormatStr, (void *) &ARIALUNI_18)) / 2;
    			WAIT_UNTIL_FINISH(OutTextXY(TextX, TextY + 2 * TextHeight, (XCHAR *)FormatStr));
			break;

		default:
			break;
    	}

    	DelayMs(800);
}

/**
  * @brief  
  * @param  
  * @retval 
  */
void    MonitorDriveMedia(void) {
	BYTE        mediaPresentNow;

	mediaPresentNow = BSP_SD_IsDetected();

	if(mediaPresentNow != mediaPresent[0])
	{
		if(mediaPresentNow)
		{
			if( disk.drv[0]->disk_initialize(disk.lun[0]) != STA_NOINIT )
			{
				if( f_mount(&FatFs[0], (TCHAR const*)Path1, 1) == FR_OK )
				{
					mediaPresent[0] = TRUE;
					strcpy(volume[0].label, Path1);
					volume[0].valid = TRUE;
				}
				else
				{
					mediaPresent[0] = FALSE;
				}
			}
			else
			{
				mediaPresent[0] = FALSE;
			}
		}
		else
		{
			mediaPresent[0] = FALSE;
			volume[0].valid = FALSE;
		}

		bShowVolumes = TRUE;

		FillNewElements();
	}

	if(USBH_GetActiveClass(&hUSBHost[1]) == USB_MSC_CLASS)
		mediaPresentNow = !disk_status (1);
	else
		mediaPresentNow = mediaPresent[1];

	if(mediaPresentNow != mediaPresent[1])
	{
		if(mediaPresentNow)
		{
			if( disk.drv[1]->disk_initialize(disk.lun[1]) != STA_NOINIT )
			{
				if( f_mount(&FatFs[1], (TCHAR const*)Path2, 1) == FR_OK )
				{
					mediaPresent[1] = TRUE;
					strcpy(volume[1].label, Path2);
					volume[1].valid = TRUE;
				}
				else
				{
					mediaPresent[1] = FALSE;
				}
			}
			else
			{
				mediaPresent[1] = FALSE;
			}
		}
		else
		{
			mediaPresent[1] = FALSE;
			volume[1].valid = FALSE;
		}

		bShowVolumes = TRUE;

		FillNewElements();
	}

	if(USBH_GetActiveClass(&hUSBHost[2]) == USB_MSC_CLASS)
		mediaPresentNow = !disk_status (2);
	else
		mediaPresentNow = mediaPresent[2];

	if(mediaPresentNow != mediaPresent[2])
	{
		if(mediaPresentNow)
		{
			if( disk.drv[2]->disk_initialize(disk.lun[2]) != STA_NOINIT )
			{
				if( f_mount(&FatFs[2], (TCHAR const*)Path3, 1) == FR_OK )
				{
					mediaPresent[2] = TRUE;
					strcpy(volume[2].label, Path3);
					volume[2].valid = TRUE;
				}
				else
				{
					mediaPresent[2] = FALSE;
				}
			}
			else
			{
				mediaPresent[2] = FALSE;
			}
		}
		else
		{
			mediaPresent[2] = FALSE;
			volume[2].valid = FALSE;
		}

		bShowVolumes = TRUE;

		FillNewElements();
	}
}

/**
  * @brief  
  * @param  
  * @retval 
  */
WORD CreateCtrlButtons(XCHAR *pTextA, XCHAR *pTextB, XCHAR *pTextC, XCHAR *pTextD) {
	WORD    state;
    BUTTON  *pObj;


    state = BTN_DRAW;
    if(pTextA == NULL)
        state = BTN_DRAW | BTN_DISABLED;
    pObj = BtnCreate
    (
        ID_BUTTON_A,
        CtrlBtnLeft(0),
        CtrlBtnTop(),
        CtrlBtnRight(0),
        CtrlBtnBottom(),
        0,
        state,
        NULL,
        pTextA,
        NULL
    );
    if (pObj == NULL)
        return (0);

    state = BTN_DRAW;
    if(pTextB == NULL)
        state = BTN_DRAW | BTN_DISABLED;
    pObj = BtnCreate
    (
        ID_BUTTON_B,
        CtrlBtnLeft(1),
        CtrlBtnTop(),
        CtrlBtnRight(1),
        CtrlBtnBottom(),
        0,
        state,
        NULL,
        pTextB,
        NULL
    );
    if (pObj == NULL)
        return (0);

    state = BTN_DRAW;
    if(pTextC == NULL)
        state = BTN_DRAW | BTN_DISABLED;
    pObj = BtnCreate
    (
        ID_BUTTON_C,
        CtrlBtnLeft(2),
        CtrlBtnTop(),
        CtrlBtnRight(2),
        CtrlBtnBottom(),
        0,
        state,
        NULL,
        pTextC,
        NULL
    );
    if (pObj == NULL)
        return (0);

    state = BTN_DRAW;
    if(pTextD == NULL)
        state = BTN_DRAW | BTN_DISABLED;
    pObj = BtnCreate
    (
        ID_BUTTON_D,
        CtrlBtnLeft(3),
        CtrlBtnTop(),
        CtrlBtnRight(3),
        CtrlBtnBottom(),
        0,
        state,
        NULL,
        pTextD,
        NULL
    );
    if (pObj == NULL)
        return (0);
}

/**
  * @brief  
  * @param  
  * @retval 
  */
FILE_FORMAT getFileExt(FILINFO *finfo) {
	FILE_FORMAT Ext;

	if     (strstr(finfo->fname, ".BMP"))
		Ext = BMP_IMG;
	else if(strstr(finfo->fname, ".JPG"))
		Ext = JPEG_IMG;
	else if(strstr(finfo->fname, ".GIF"))
		Ext = GIF_IMG;
#ifdef SUPPORT_WAV
	else if(strstr(finfo->fname, ".WAV"))
		Ext = WAV;
#endif /* SUPPORT_WAV */
#ifdef SUPPORT_MP3
	else if(strstr(finfo->fname, ".MP3"))
		Ext = MP3;
#endif /* SUPPORT_MP3 */
#ifdef SUPPORT_JMV
	else if(strstr(finfo->fname, ".JMV"))
		Ext = JMV;
#endif /* SUPPORT_JMV */
#ifdef SUPPORT_FLAC
	else if(strstr(finfo->fname, ".FLA"))
		Ext = FLAC;
#endif /* SUPPORT_FLAC */
	else if(strstr(finfo->fname, ".BIN"))
		Ext = BIN;
	else
		Ext = OTHER;

	return(Ext);
}

/**
  * @brief
  * @param
  * @retval
  */
BOOL GOLDeleteObj(OBJ_HEADER *object)
{
    if(!_pGolObjects)
        return (FALSE);

    if(object == _pGolObjects)
    {
        _pGolObjects = (OBJ_HEADER *)object->pNxtObj;
    }
    else
    {
        OBJ_HEADER  *curr;
        OBJ_HEADER  *prev;

        curr = (OBJ_HEADER *)_pGolObjects->pNxtObj;
        prev = (OBJ_HEADER *)_pGolObjects;

        while(curr)
        {
            if(curr == object)
                break;

            prev = (OBJ_HEADER *)curr;
            curr = (OBJ_HEADER *)curr->pNxtObj;
        }

        if(!curr)
            return (FALSE);

        prev->pNxtObj = curr->pNxtObj;
    }

    return (TRUE);
}

/**
  * @brief
  * @param
  * @retval
  */
BYTE AutoPlay(void)
{
    FB_ERROR 	bMediaError;

    bFileProcess = TRUE;

    if(aFolderElement[NextFile].FmtType == WAV || \
	   aFolderElement[NextFile].FmtType == MP3 || \
	   aFolderElement[NextFile].FmtType == FLAC)
	{
    	if( f_open (&fSrc, aFolderElement[NextFile].Name, FA_READ) == FR_OK )
		{
			bMediaError = AudioPlayBack(&fSrc, aFolderElement[NextFile].FmtType);

			if(bMediaError)
			{
				bAutoPlay = FALSE;

				DisplayErrorInfo(bMediaError);

				SetAudioCtrls(FALSE);
				SetFileCtrls(FALSE);
			}

			f_close (&fSrc);
		}
	}
#ifdef SUPPORT_JMV
	else if(aFolderElement[NextFile].FmtType == JMV)
	{
		if( f_open (&fSrc, aFolderElement[NextFile].Name, FA_READ) == FR_OK )
		{
			bCallback = JMV;

			BSP_LCD_SetLayerVisible(1, DISABLE);

			bMediaError = JMVPlayBack(&fSrc);

			if(bMediaError == MEMORY_ERROR)
				DisplayErrorInfo(bMediaError);

			f_close (&fSrc);
		}
	}
#endif /* SUPPORT_JMV */

	NextFile++;
	if(NextFile >= bNumElementsInFolder)
	{
		BSP_LCD_SetLayerVisible(1, ENABLE);

		NextFile = 0;
		bAutoPlay = FALSE;

		BSP_LCD_SetLayerWindow(0, 0, 0, BSP_LCD_GetXSize(), BSP_LCD_GetYSize());

		SetAudioCtrls(FALSE);
		SetFileCtrls(FALSE);
	}

    bFileProcess = FALSE;
    return (0);
}

/**
  * @brief
  * @param
  * @retval
  */
void SetAudioCtrls(BOOL Set)
{
	if(Set)
	{
		GOLDeleteObj(pBtnB); GOLDeleteObj(pBtnC);
        GOLAddObject(pSliderVol);

        /* Set audio buttons */
        BtnSetText(pBtnA, StopStr); BtnSetText(pBtnD, PauseStr);
        ClrState(pBtnA, BTN_DISABLED); ClrState(pBtnD, BTN_DISABLED);

        /* Disable needless objs */
        SetState(pListBox, LB_DISABLED); SetState(pSlider, SLD_DISABLED);
        SetState(pBtnUp, BTN_DISABLED); SetState(pBtnDn, BTN_DISABLED);

        GOLRedrawRec(0, 0, BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
	}
	else
	{
		GOLDeleteObj(pSliderVol);
		GOLAddObject(pBtnB); GOLAddObject(pBtnC);

		/* Clear audio buttons */
		BtnSetText(pBtnA, NULL); BtnSetText(pBtnD, NULL);
		SetState(pBtnA, BTN_DISABLED); SetState(pBtnD, BTN_DISABLED);

		/* Enable objs */
		ClrState(pListBox, LB_DISABLED); ClrState(pSlider, SLD_DISABLED);
		ClrState(pBtnUp, BTN_DISABLED); ClrState(pBtnDn, BTN_DISABLED);

		GOLRedrawRec(0, 0, BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
	}
}

/**
  * @brief
  * @param
  * @retval
  */
void SetFileCtrls(BOOL Set)
{
	if(Set)
	{
		/* Set file buttons */
		BtnSetText(pBtnA, OpenStr); BtnSetText(pBtnB, CopyStr);
		BtnSetText(pBtnC, PasteStr); BtnSetText(pBtnD, DeleteStr);
		ClrState(pBtnA, BTN_DISABLED); ClrState(pBtnB, BTN_DISABLED);
		ClrState(pBtnD, BTN_DISABLED);

		GOLRedrawRec(0, 0, BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
	}
	else
	{
		/* Clear file buttons */
		BtnSetText(pBtnA, NULL); BtnSetText(pBtnB, NULL);
		BtnSetText(pBtnC, NULL); BtnSetText(pBtnD, NULL);
		SetState(pBtnA, BTN_DISABLED); SetState(pBtnB, BTN_DISABLED);
		SetState(pBtnC, BTN_DISABLED); SetState(pBtnD, BTN_DISABLED);

		GOLRedrawRec(0, 0, BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
	}
}

/**
  * @brief
  * @param
  * @retval
  */
void SetConfirmCtrls(BOOL Set)
{
	if(Set)
	{
		/* Set confirmation buttons */
		BtnSetText(pBtnB, YesStr); BtnSetText(pBtnC, NoStr);
		ClrState(pBtnB, BTN_DISABLED); ClrState(pBtnC, BTN_DISABLED);

		/* Disable needless objs */
		SetState(pBtnA, BTN_DISABLED); SetState(pBtnD, BTN_DISABLED);

		GOLRedrawRec(0, 0, BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
	}
	else
	{
		/* Clear confirmation buttons */
		BtnSetText(pBtnB, NULL); BtnSetText(pBtnC, NULL);
		SetState(pBtnB, BTN_DISABLED); SetState(pBtnC, BTN_DISABLED);

		/* Enable objs */
		ClrState(pBtnA, BTN_DISABLED); ClrState(pBtnD, BTN_DISABLED);

		GOLRedrawRec(0, 0, BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
	}
}

/**
  * @brief
  * @param
  * @retval
  */
void CopyFolderElement(FolderElement *Src, FolderElement *Dst)
{
	BYTE i;

	for(i = 0; i < 13; i++)
	{
		Dst->Name[i] = Src->Name[i];
		Dst->XName[i] = Src->XName[i];
	}
	Dst->blFolder = Src->blFolder;
	Dst->blVolume = Src->blVolume;
	Dst->FmtType = Src->FmtType;
}

/**
  * @brief
  * @param
  * @retval
  */
BYTE CopyFile(FIL *Src, FIL *Dst)
{
	PRIVATE BYTE *buf, str[] = {'I',0}, CopyProgr;
	PRIVATE WORD bytesread, byteswrite, TextX, TextY, TextHeight, copyBuffSize;

	uint8_t String[30] = {0}, LastString[30] = {0};

	if(Dst->fs->drv == 0)
		copyBuffSize = SLOW_BUFF_SIZE;
	else
		copyBuffSize = FAST_BUFF_SIZE;

	buf = (BYTE *) malloc(copyBuffSize);
	if(!buf)
		return(200); /* Memory allocation error */

	SetColor(WHITE);
	ClearDevice();

	SetFont((void *) &ARIALUNI_18);

	TextHeight = GetTextHeight((void *) &ARIALUNI_18);
	TextY = (BSP_LCD_GetYSize() - 3 * TextHeight) / 2;

	do
	{
		if( f_read (Src, buf, copyBuffSize, &bytesread) != FR_OK )
		{
			free(buf);
			return(READ_ERROR); // read error
		}

		if( f_write (Dst, buf, bytesread, &byteswrite) != FR_OK )
		{
			free(buf);
			return(WRITE_ERROR); // write error
		}

		SetColor(WHITE);
		WAIT_UNTIL_FINISH(OutTextXY( 0,
									 BSP_LCD_GetYSize() / 3,
									 (XCHAR *)LastString));
		sprintf(String, "%.2f  /  %.2f  KB", (float)Src->fptr / 1024, (float)Src->fsize / 1024);

		SetColor(BLACK);
		WAIT_UNTIL_FINISH(OutTextXY( 0,
									 BSP_LCD_GetYSize() / 3,
									 (XCHAR *)String));
		memcpy(LastString, String, sizeof(LastString));

		CopyProgr = ( (float)Src->fptr / Src->fsize * 100 );
		TextX = CopyProgr * ( (float)BSP_LCD_GetXSize() / 100 );
		WAIT_UNTIL_FINISH(OutTextXY(TextX, TextY, (XCHAR *)str));
	}
	while(bytesread == copyBuffSize);

	free(buf);
	return(0); // success
}

/**
  * @brief
  * @param
  * @retval
  */
BYTE CopyFolder(const char *pathSrc, const char *pathDst, const char *SrcName)
{
	FRESULT 	fr;
	DIR 		dir;
	FILINFO 	finfo;
	BYTE        err = 0;
	char        path1[256] = { 0 }, path2[256] = { 0 }, *path2ptr = path2;

	finfo.lfname = &lfnbuff[0]; // Init FATFS LFN buffer
	finfo.lfsize = sizeof(lfnbuff);

	if(pathDst[0] == '0')
		f_chdrive ("0:/");
	if(pathDst[0] == '1')
		f_chdrive ("1:/");
	if(pathDst[0] == '2')
		f_chdrive ("2:/");

	if( f_chdir (pathDst) != FR_OK )
		return(100); // read error

	if( f_mkdir (SrcName) != FR_OK )
		return(300); // write error

	if(pathDst[3] == 0)
	{
		strcpy(path2, pathDst);
		path2ptr += strlen(pathDst);
		strcpy(path2ptr, SrcName);
	}
	else
	{
		strcpy(path2, pathDst);
		path2ptr += strlen(pathDst);
		strcpy(path2ptr, "/");
		path2ptr += strlen("/");
		strcpy(path2ptr, SrcName);
	}

	if(pathSrc[0] == '0')
		f_chdrive ("0:/");
	if(pathSrc[0] == '1')
		f_chdrive ("1:/");
	if(pathSrc[0] == '2')
		f_chdrive ("2:/");

	if( f_chdir (pathSrc) != FR_OK )
		return(100); // read error

	fr = f_findfirst (&dir, &finfo, "", "*.*");
	while(fr == FR_OK)
	{
		if(finfo.fname[0] == 0)
            		break;

		if( (finfo.fattrib == AM_ARC) || (finfo.fattrib == (AM_ARC | AM_RDO)) )
		{
			if( finfo.fname[0] == '.' )
			{
				fr = f_findnext(&dir, &finfo);
				continue;
			}

			if( f_open (&fSrc, finfo.fname, FA_READ) != FR_OK )
				return(100); // read error

			if(path2[0] == '0')
				f_chdrive ("0:/");
			if(path2[0] == '1')
				f_chdrive ("1:/");
			if(path2[0] == '2')
				f_chdrive ("2:/");

			if( f_chdir (path2) != FR_OK )
				return(100); // read error

			if( f_open (&fDst, finfo.fname, FA_CREATE_NEW | FA_WRITE) != FR_OK )
			{
				if( f_close (&fSrc) != FR_OK )
					return(300); // write error

				return(300); // write error
			}

			err = CopyFile(&fSrc, &fDst);
			if(err != 0)
			{
				if( f_close (&fSrc) != FR_OK )
					return(300); // write error

				if( f_close (&fDst) != FR_OK )
					return(300); // write error

				return(err);
			}

			if( f_close (&fSrc) != FR_OK )
				return(300); // write error

			if( f_close (&fDst) != FR_OK )
				return(300); // write error

			if(pathSrc[0] == '0')
				f_chdrive ("0:/");
			if(pathSrc[0] == '1')
				f_chdrive ("1:/");
			if(pathSrc[0] == '2')
				f_chdrive ("2:/");

			if( f_chdir (pathSrc) != FR_OK )
				return(100); // read error
		}

		fr = f_findnext(&dir, &finfo);
	}

	fr = f_findfirst (&dir, &finfo, "", "*");
	while(fr == FR_OK)
	{
		if(finfo.fname[0] == 0)
            		break;

		if(finfo.fattrib == AM_DIR)
		{
			if( finfo.fname[0] == '.' || (finfo.fname[0] == '.' && finfo.fname[1] == '.') )
			{
				fr = f_findnext(&dir, &finfo);
				continue;
			}

			if( f_chdir (finfo.fname) != FR_OK )
				return(100); // read error

			if( f_getcwd (&path1, sizeof(path1)) != FR_OK )
				return(100); // read error

			err = CopyFolder(path1, path2, finfo.fname);
			if(err != 0)
				return(err);

			if(pathSrc[0] == '0')
				f_chdrive ("0:/");
			if(pathSrc[0] == '1')
				f_chdrive ("1:/");
			if(pathSrc[0] == '2')
				f_chdrive ("2:/");

			if( f_chdir (pathSrc) != FR_OK )
				return(100); // read error
		}

		fr = f_findnext(&dir, &finfo);
	}

	return(0); // success
}

/**
  * @brief
  * @param
  * @retval
  */
BYTE DeleteFolder(const char *path)
{
	FRESULT 	fr;
	DIR 		dir;
	FILINFO 	finfo;
	BYTE        err = 0;

	finfo.lfname = &lfnbuff[0]; // Init FATFS LFN buffer
	finfo.lfsize = sizeof(lfnbuff);

	if( f_chdir (path) != FR_OK )
		return(100); // read error

	fr = f_findfirst (&dir, &finfo, "", "*.*");
	while(fr == FR_OK)
	{
		if(finfo.fname[0] == 0)
            		break;

		if( (finfo.fattrib == AM_ARC) || (finfo.fattrib == (AM_ARC | AM_RDO)) )
		{
			if( finfo.fname[0] == '.' )
			{
				fr = f_findnext(&dir, &finfo);
				continue;
			}

			if( f_unlink (finfo.fname) != FR_OK)
				return(400); // delete error
		}

		fr = f_findnext(&dir, &finfo);
	}

	fr = f_findfirst (&dir, &finfo, "", "*");
	while(fr == FR_OK)
	{
		if(finfo.fname[0] == 0)
            		break;

		if(finfo.fattrib == AM_DIR)
		{
			if( finfo.fname[0] == '.' || (finfo.fname[0] == '.' && finfo.fname[1] == '.') )
			{
				fr = f_findnext(&dir, &finfo);
				continue;
			}

			err = DeleteFolder(finfo.fname);
			if(err != 0)
				return(err);
		}

		fr = f_findnext(&dir, &finfo);
	}

	if( f_chdir ("..") != FR_OK )
		return(100); // read error

	if( f_unlink (path) != FR_OK)
		return(400); // delete error

	return(0); // success
}

/**
  * @brief
  * @param
  * @retval
  */
void Bootloader_Process(FolderElement *obj)
{
	FRESULT fr;
	UINT bytesread, TextX, TextY, TextHeight;;
	uint32_t data, *UserAddr = FLASH_USER_START_ADDR, i = 0;
	int8_t str[] = {'I',0}, CopyProgr;

	SetColor(WHITE);
	ClearDevice();

	SetColor(BLACK);
	SetFont((void *) &ARIALUNI_18);

	TextHeight = GetTextHeight((void *) &ARIALUNI_18);
	TextY = (BSP_LCD_GetYSize() - 3 * TextHeight) / 2;

	/* Open file */
	if(f_open(&fSrc, obj->Name, FA_READ) == FR_OK)
	{
		/* Check if application exist */
		do
		{
			fr = f_read(&fSrc, &data, 4, &bytesread);
			if(bytesread)
			{
				if( UserAddr[i] != data )
				{
					break;
				}
			}
			else
			{
				return;
			}

			i++;
		} while(fr == FR_OK);

		f_rewind(&fSrc);

		/* Check application size */
		if(Bootloader_CheckSize( f_size(&fSrc) ) == BL_OK)
		{
			/* Init Bootloader and Flash */
            Bootloader_Init();

			/* Erase Flash */
			Bootloader_Erase();

			/* Programming */
			Bootloader_FlashBegin();
			do
			{
				fr = f_read(&fSrc, &data, 4, &bytesread);
				if(bytesread)
				{
					Bootloader_FlashNext(data);
				}

				CopyProgr = ( (float)f_tell(&fSrc) / f_size(&fSrc) * 100 );

				TextX = CopyProgr * ( (float)BSP_LCD_GetXSize() / 100 );
				WAIT_UNTIL_FINISH(OutTextXY(TextX, TextY, (XCHAR *)str));
			} while((fr == FR_OK) && (bytesread > 0));
			Bootloader_FlashEnd();
		}

		f_close(&fSrc);
	}
}

/**
  * @brief  Processes Mouse data.
  * @param  data: Mouse data to be displayed
  * @retval None
  */
void USR_MOUSE_ProcessData(HID_MOUSE_Info_TypeDef *data)
{
  	if((data->x != 0) && (data->y != 0))
  	{
		HID_MOUSE_UpdatePosition(data->x , data->y);
  	}
}

/**
  * @brief  Handles mouse scroll to update the mouse position on display window.
  * @param  x: USB HID Mouse X co-ordinate
  * @param  y: USB HID Mouse Y co-ordinate
  * @retval None
  */
static void HID_MOUSE_UpdatePosition(int8_t x, int8_t y)
{
	static int16_t  x_loc  = 0, y_loc  = 0;

  	if((x != 0) || (y != 0))
  	{
		x_loc += x;
		y_loc += y;

		if(y_loc > MOUSE_WINDOW_HEIGHT - 12)
		{
			y_loc = MOUSE_WINDOW_HEIGHT - 12;
		}
		if(x_loc > MOUSE_WINDOW_WIDTH - 10)
		{
			x_loc = MOUSE_WINDOW_WIDTH - 10;
		}

		if(y_loc < 2)
		{
			y_loc = 2;
		}
		if(x_loc < 2)
		{
			x_loc = 2;
		}
		BSP_LCD_SelectLayer(1);

		BSP_LCD_SetBackColor(0);
		BSP_LCD_SetTextColor(0);
		BSP_LCD_DisplayChar(MOUSE_WINDOW_X + prev_x, MOUSE_WINDOW_Y + prev_y, 'x');

		BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
		BSP_LCD_DisplayChar(MOUSE_WINDOW_X + x_loc, MOUSE_WINDOW_Y + y_loc, 'x');

		BSP_LCD_SelectLayer(0);

		prev_x = x_loc;
		prev_y = y_loc;
  	}
}
