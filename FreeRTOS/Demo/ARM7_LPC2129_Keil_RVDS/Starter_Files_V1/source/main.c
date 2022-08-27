/*
 * FreeRTOS Kernel V10.2.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* 
	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used.
*/


/*
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the demo application tasks.
 * 
 * Main.c also creates a task called "Check".  This only executes every three 
 * seconds but has the highest priority so is guaranteed to get processor time.  
 * Its main function is to check that all the other tasks are still operational.
 * Each task (other than the "flash" tasks) maintains a unique count that is 
 * incremented each time the task successfully completes its function.  Should 
 * any error occur within such a task the count is permanently halted.  The 
 * check task inspects the count of each task to ensure it has changed since
 * the last time the check task executed.  If all the count variables have 
 * changed all the tasks are still executing error free, and the check task
 * toggles the onboard LED.  Should any task contain an error at any time 
 * the LED toggle rate will change from 3 seconds to 500ms.
 *
 */

/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "lpc21xx.h"
#include "queue.h"
/* Peripheral includes. */
#include "serial.h"
#include "GPIO.h"


/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 115200 )


BaseType_t xTaskPeriodicCreate( TaskFunction_t pxTaskCode,
                               const char * const pcName, /*lint !e971 Unqualified char types are allowed for strings and single characters only. */
                               const configSTACK_DEPTH_TYPE usStackDepth,
                               void * const pvParameters,
                               UBaseType_t uxPriority,
                               TaskHandle_t * const pxCreatedTask, 
                               TickType_t period);

/*--------------------   Tasks Prototypes   ----------------------*/

void task_Button_1_Monitor(void *pvParameters);
void task_Button_2_Monitor(void *pvParameters);
void task_Periodic_Transmitter(void *pvParameters);
void task_Uart_Receiver(void *pvParameters);


/*--------------------   Global Variables   ----------------------*/

#define FALLING_EDGE	0
#define RISING_EDGE	1
#define NO_EDGE	2

#define TASK_PERIOD__Button_1_Monitor 50 
#define TASK_PERIOD__Button_2_Monitor 50 
#define TASK_PERIOD__Periodic_Transmitter 100 
#define TASK_PERIOD__Uart_Receiver 20 

/*--------------------   Tasks handlers   ----------------------*/

TaskHandle_t taskhandler_Button_1_Monitor = NULL;
TaskHandle_t taskhandler_Button_2_Monitor = NULL;
TaskHandle_t taskhandler_Periodic_Transmitter = NULL;
TaskHandle_t taskhandler_Uart_Receiver = NULL;

QueueHandle_t Queuehandle;

/*-----------------------------------------------------------*/
/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
static void prvSetupHardware( void );
/*-----------------------------------------------------------*/


/*--------------------      Tasks      ----------------------*/

void task_Button_1_Monitor( void * pvParameters )
{	
	unsigned char button_1_state = 1;
	unsigned char previous_button_1_state = 1;
	const char *rising_edge_str1 = "Rising:Button1\n\n"; 
	const char *falling_edge_str1 = "Falling:Button1\n\n";  
	
	TickType_t xLastWakeTime;

	xLastWakeTime = xTaskGetTickCount();

    for( ;; )
    {
		vTaskDelayUntil(&xLastWakeTime,TASK_PERIOD__Button_1_Monitor);	
		
		button_1_state = GPIO_read(PORT_0,PIN0);
		if(button_1_state != previous_button_1_state)
		{
			previous_button_1_state = button_1_state;

			if(button_1_state)
			{
				xQueueSend(Queuehandle, &rising_edge_str1, 0);
			}
			else
			{
				xQueueSend(Queuehandle, &falling_edge_str1, 0);

			}
		}

		
	
    }
}

void task_Button_2_Monitor( void * pvParameters )
{
	unsigned char button_2_state = 1;
	unsigned char previous_button_2_state = 1;
	const char *rising_edge_str2 = "Rising:Button2\n\n"; 
	const char *falling_edge_str2 = "Falling:Button2\n\n"; 

	TickType_t xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();

    for( ;; )
    {
		vTaskDelayUntil(&xLastWakeTime,TASK_PERIOD__Button_2_Monitor);	

		button_2_state = GPIO_read(PORT_0,PIN1);

		if(button_2_state != previous_button_2_state)
		{
			previous_button_2_state = button_2_state;

			if(button_2_state)
			{
				xQueueSend(Queuehandle, &rising_edge_str2, 0);
			}
			else
			{
				xQueueSend(Queuehandle, &falling_edge_str2, 0);
			}
	
    	}
	}
}

void task_Periodic_Transmitter( void * pvParameters )
{	
	const char *periodic_str = "Periodic String\n\n"; 

	TickType_t xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();

    for( ;; )
    {
		vTaskDelayUntil(&xLastWakeTime,TASK_PERIOD__Periodic_Transmitter);	

		xQueueSend(Queuehandle, &periodic_str, 0);	
	
    }
}

void task_Uart_Receiver( void * pvParameters )
{
    char *recieved_str;
	unsigned char counter = 0;

	TickType_t xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();

    for( ;; )
    {
		vTaskDelayUntil(&xLastWakeTime,TASK_PERIOD__Uart_Receiver);	

		if(xQueueReceive(Queuehandle, &recieved_str, 0) == pdTRUE){
			for(counter = 0;recieved_str[counter] != '\0'; counter++ )
			{
				xSerialPutChar(recieved_str[counter]);
			}
		}
		
    }
}

/*-----------------------------------------------------------*/



/*
 * Application entry point:
 * Starts all the other tasks, then starts the scheduler. 
 */
int main( void )
{
	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();


/*---------------    Queue and Tasks Creation    -----------------*/

Queuehandle = xQueueCreate(3,sizeof(char *));

xTaskPeriodicCreate( task_Button_1_Monitor,
					( const char * ) "Button_1_Monitor",
					50,
					( void * ) 0,
					1,
					NULL,
					TASK_PERIOD__Button_1_Monitor );


xTaskPeriodicCreate( task_Button_2_Monitor,
					( const char * ) "Button_2_Monitor",
					50,
					( void * ) 0,
					1,
					NULL,
					TASK_PERIOD__Button_2_Monitor );

xTaskPeriodicCreate( task_Periodic_Transmitter,
					( const char * ) "Periodic_Transmitter",
					50,
					( void * ) 0,
					1,
					NULL,
					TASK_PERIOD__Periodic_Transmitter );


xTaskPeriodicCreate( task_Uart_Receiver,
					( const char * ) "Uart_Receiver",
					50,
					( void * ) 0,
					1,
					NULL,
					TASK_PERIOD__Uart_Receiver );
/*-----------------------------------------------------------*/

	/* Now all the tasks have been started - start the scheduler.

	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used here. */
										
	vTaskStartScheduler();

	/* Should never reach here!  If you do then there was not enough heap
	available for the idle task to be created. */
	for( ;; );
}
/*-----------------------------------------------------------*/

/* Function to reset timer 1 */
void timer1Reset(void)
{
	T1TCR |= 0x2;
	T1TCR &= ~0x2;
}

/* Function to initialize and start timer 1 */
static void configTimer1(void)
{
	T1PR = 1000;
	T1TCR |= 0x1;
}

static void prvSetupHardware( void )
{
	/* Perform the hardware setup required.  This is minimal as most of the
	setup is managed by the settings in the project file. */

	/* Configure UART */
	xSerialPortInitMinimal(mainCOM_TEST_BAUD_RATE);

	/* Configure GPIO */
	GPIO_init();
	
	/* Config trace timer 1 and read T1TC to get current tick */
	configTimer1();

	/* Setup the peripheral bus to be the same as the PLL output. */
	VPBDIV = mainBUS_CLK_FULL;
}
/*-----------------------------------------------------------*/


