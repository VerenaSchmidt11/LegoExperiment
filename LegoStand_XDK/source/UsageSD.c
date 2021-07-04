/*
 * UsageSD.c
 *
 *  Created on: 25.05.2021
 *      Author: verena
 */


/* system header files */
#include <stdio.h>

#include "XdkAppInfo.h"

/* additional interface header
 *  files */

//#include "XDK_Utils.h"
//#include "ff.h"
//#include "BSP_BoardType.h"
//#include "BCDS_Assert.h"

#include "BCDS_CmdProcessor.h"
#include "FreeRTOS.h"
#include "BCDS_SDCard_Driver.h"
#include "ff.h"

/* constants */

#define DEFAULT_LOGICAL_DRIVE ""
#define DRIVE_ZERO UINT8_C(0)
#define FORCE_MOUNT UINT8_C(1)
#define FIRST_LOCATION UINT8_C(0)

static FIL fileObject;

/**************************************
	 *    Writing to SD Card              *
	 *************************************/


void InitSdCard(void)
{
	Retcode_T retVal = RETCODE_FAILURE;
	FRESULT FileSystemResult = FR_OK;
	static FATFS FatFileSystemObject;
	SDCardDriver_Initialize();
	if (SDCARD_INSERTED == SDCardDriver_GetDetectStatus())
	{
		retVal = SDCardDriver_DiskInitialize(DRIVE_ZERO);
		if (RETCODE_OK == retVal)
		{
			printf("SD Card Disk initialize succeeded \n\r");
			FileSystemResult = f_mount(&FatFileSystemObject, DEFAULT_LOGICAL_DRIVE,
									   FORCE_MOUNT);
			if (FR_OK != FileSystemResult)
			{
				printf("Mounting SD card failed \n\r");
			}
		}
	}
}

Retcode_T searchForFileOnSdCard(const char *filename, FILINFO *fileData)
{
	if (FR_OK == f_stat(filename, fileData))
	{
		printf("File %s found on SD card. \n\r", filename);
		return RETCODE_OK;
	}
	else
	{
		printf("File %s does not exist. \n\r", filename);
		return RETCODE_FAILURE;
	}
}

void createFileOnSdCard(const char *filename)
{
	if (FR_OK == f_open(&fileObject, filename, FA_CREATE_NEW))
	{
		printf("File %s was created successfully \n\r", filename);
	}
}

void writeDataIntoFileOnSdCard(const char *filename, const char *dataBuffer)
{
	FRESULT fileSystemResult;
	char ramBufferWrite[UINT16_C(1000)]; // Temporay buffer for write file
	uint16_t fileSize;
	UINT bytesWritten;
	fileSize = (uint16_t)strlen(dataBuffer);

	for (uint32_t index = 0; index < fileSize; index++)
	{
		ramBufferWrite[index] = dataBuffer[index];
	}
	f_open(&fileObject, filename, FA_OPEN_EXISTING | FA_WRITE);
	f_lseek(&fileObject, f_size(&fileObject));
	fileSystemResult = f_write(&fileObject, ramBufferWrite, fileSize, &bytesWritten);

	if ((fileSystemResult != FR_OK) || (fileSize != bytesWritten))
	{
		printf("Error: Cannot write to file %s \n\r", filename);
	}
	fileSystemResult = f_close(&fileObject);
}

