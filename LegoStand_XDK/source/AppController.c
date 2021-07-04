
#include "XdkAppInfo.h"

#undef BCDS_MODULE_ID
#define BCDS_MODULE_ID XDK_APP_MODULE_ID_APP_CONTROLLER

/* own header files */
#include "AppController.h"

/* additional header files for XDK error codes etc.*/
#include "XDK_Utils.h"
#include "BCDS_Assert.h"
#include "BCDS_Retcode.h"
#include "BCDS_CmdProcessor.h"

/* Header files for RTOS, Tasks, Timers */

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "stdio.h"

#include "XdkSensorHandle.h"
#include "XDK_Sensor.h"

/* Header file which includes functions for reading / writing SD-Cards */
#include "UsageSD.h"

/* definitions needed for running XDK application */

static xTaskHandle AppControllerHandle = NULL; /**< OS thread handle for Application controller */

static CmdProcessor_T *AppCmdProcessor; /**< Handle to store the main Command processor handle to be reused by ServalPAL thread */

// global definitions for the FILE
const char FilePrefix[] = "sensordata_";
const char FileType[] = ".csv";
unsigned int FileCounter = 0;
const char Filename[100];

// Sensor correction value

#define APP_TEMPERATURE_OFFSET_CORRECTION (-3459)

static Sensor_Setup_T SensorSetup =
	{
		.CmdProcessorHandle = NULL,
		.Enable =
			{
				.Accel = false,
				.Mag = true, //magnetic sensor needed for experimental setup
				.Gyro = false,
				.Humidity = false,
				.Temp = false,
				.Pressure = false,
				.Light = true, //light sensor needed for experimental setup
				.Noise = false,
			},
		.Config =
			{
				.Accel =
					{
						.Type = SENSOR_ACCEL_BMA280,
						.IsRawData = false,
						.IsInteruptEnabled = false,

					},
				.Gyro =
					{
						.Type = SENSOR_GYRO_BMG160,
						.IsRawData = false,
					},
				.Mag =
					{
						.IsRawData = false,
					},
				.Light =
					{
						.IsInteruptEnabled = false,

					},
				.Temp =
					{
						.OffsetCorrection = APP_TEMPERATURE_OFFSET_CORRECTION,
					},
			},
}; /**< Sensor setup parameters */

/* Function for writing data on the filename using the defined filename and content */
void WriteSDSensorData(char *FileContent)
{

	//Retcode_T retcode = RETCODE_OK;

	writeDataIntoFileOnSdCard(Filename, FileContent); // writing the file and content
}

/* Function which is currently running on the XDK in a never-ending loop */

static void AppControllerFire(void *pvParameters)
{
	BCDS_UNUSED(pvParameters);
	Sensor_Value_T sensorValue; // variable for getting magnetic and light sensor data
	char FileText[100];
	int currentTimer; // variable for the soft-timer

	memset(&sensorValue, 0x00, sizeof(sensorValue)); // resets the variable sensorValue

	while (1) //this loop runs forever
	{

		Sensor_GetData(&sensorValue);		//get the sensor information
		currentTimer = xTaskGetTickCount(); // get the current soft timer

		// store the timer and sensor information in the variable for later wrinting on the SD-card
		sprintf(FileText, "%i, %i, %i, %i, %i \n", currentTimer, (long int)sensorValue.Mag.X, (long int)sensorValue.Mag.Y, (long int)sensorValue.Mag.Z, (long int)sensorValue.Light);
		// store the information
		writeDataIntoFileOnSdCard(Filename, &FileText);

		// write the information on the USB-port, additionally
		printf(FileText);
	}
}

/* Function which enables functions on the XDK - used after AppControllerSetup*/
static void AppControllerEnable(void *param1, uint32_t param2)
{
	BCDS_UNUSED(param1);
	BCDS_UNUSED(param2);
	Retcode_T retcode = RETCODE_OK;

	Sensor_Enable(); // Enable the sensor

	const char FileContent[] = "Timer, Mag_X,Mag_Y,Mag_Z,Light \n"; // header file

	// Initialize the file on the SD-card, first get the next file name
	sprintf(Filename, "%s%i%s", FilePrefix, FileCounter, FileType); //Filename, first trial

	while (RETCODE_OK == searchForFileOnSdCard(Filename, NULL)) //look for the next free filename
	{
		FileCounter = FileCounter + 1;									// increase the file counter
		sprintf(Filename, "%s%i%s", FilePrefix, FileCounter, FileType); // generate the next file name
	}

	createFileOnSdCard(Filename);					  //if free filename is found, generate new file
	writeDataIntoFileOnSdCard(Filename, FileContent); //write the header in the new file

	if (RETCODE_OK == retcode) // for starting the main application
	{
		if (pdPASS != xTaskCreate(AppControllerFire, (const char *const)"AppController", TASK_STACK_SIZE_APP_CONTROLLER, NULL, TASK_PRIO_APP_CONTROLLER, &AppControllerHandle))
		{
			retcode = RETCODE(RETCODE_SEVERITY_ERROR, RETCODE_OUT_OF_RESOURCES);
		}
	}
	if (RETCODE_OK != retcode) // message in case of an error
	{
		printf("AppControllerEnable : Failed \r\n");
		Retcode_RaiseError(retcode);
		assert(0); /* To provide LED indication for the user */
	}
	Utils_PrintResetCause(); //This function prints if last reset was caused due to watchdog time
}

/* Functions for all XDK setups - started after AppController_Init */

static void AppControllerSetup(void *param1, uint32_t param2)
{
	BCDS_UNUSED(param1);
	BCDS_UNUSED(param2);
	Retcode_T retcode = RETCODE_OK;
	// Sensor setups
	Sensor_Setup(&SensorSetup);

	// Init SD-Card
	InitSdCard();

	// next step: AppControllerEnable
	retcode = CmdProcessor_Enqueue(AppCmdProcessor, AppControllerEnable, NULL, UINT32_C(0));
	if (RETCODE_OK != retcode)
	{
		printf("AppControllerSetup : Failed \r\n");
		Retcode_RaiseError(retcode);
		assert(0); /* To provide LED indication for the user */
	}
}

/* First function called */
void AppController_Init(void *cmdProcessorHandle, uint32_t param2)
{
	BCDS_UNUSED(param2);

	Retcode_T retcode = RETCODE_OK;

	if (cmdProcessorHandle == NULL)
	{
		printf("AppController_Init : Command processor handle is NULL \r\n");
		retcode = RETCODE(RETCODE_SEVERITY_ERROR, RETCODE_NULL_POINTER);
	}
	else
	{
		AppCmdProcessor = (CmdProcessor_T *)cmdProcessorHandle;
		retcode = CmdProcessor_Enqueue(AppCmdProcessor, AppControllerSetup, NULL, UINT32_C(0));
	}

	if (RETCODE_OK != retcode)
	{
		Retcode_RaiseError(retcode);
		assert(0); /* To provide LED indication for the user */
	}
}
