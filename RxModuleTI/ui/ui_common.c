/*
 * ui_common.c
 *
 *  Created on: 25 mar 2017
 *      Author: igbt6
 */

#include "ui_common.h"
#include "ui_message_box.h"
#include "ui_keyboard.h"

#define UI_TIMER_BASE TIMER2_BASE
#define UI_TIMER_TYPE TIMER_A
#define UI_TIMER_SYSCTL_TIMER_TYPE SYSCTL_PERIPH_TIMER2
#define UI_TIMER_CFG TIMER_CFG_32_BIT_PER_UP
#define UI_TIMER_INT_FLAGS (TIMER_TIMA_TIMEOUT)
#define UI_TIMER_INT_MODE (INT_TIMER2A)

#define UI_TIMER_CB_NUM 3

static tContext* m_drawingCtx = NULL;

static volatile uint32_t m_msCounter = 0;

typedef struct
{
	uint16_t periodTime;
	void (*timerCb)(void);
}TimerCallback;
static uint8_t m_timerCbNum = 0;
static TimerCallback* m_timerCb[UI_TIMER_CB_NUM];

//@brief Fires up/down the timer
static void uiTimerEnable(bool enable)
{
	if(enable)
	{
		TimerEnable(UI_TIMER_BASE, UI_TIMER_TYPE);
		return;
	}
    TimerDisable(UI_TIMER_BASE, UI_TIMER_TYPE);
}

//@brief Configures Timer2A as a 32-bit periodic timer
static void uiTimerInit()
{
    SysCtlPeripheralEnable(UI_TIMER_SYSCTL_TIMER_TYPE);
    TimerConfigure(UI_TIMER_BASE, UI_TIMER_CFG);
    // Set the Timer2A load value to 1ms.
    TimerLoadSet(UI_TIMER_BASE,  UI_TIMER_TYPE, SysCtlClockGet() / 1000);//1[ms]
    // Configure the Timer2A interrupt for timer timeout.
    TimerIntEnable(UI_TIMER_BASE, UI_TIMER_INT_FLAGS);
    // Enable the Timer2A interrupt on the processor (NVIC).
    IntEnable(UI_TIMER_INT_MODE);
    m_msCounter = 0;
    // Enable Timer2A.
    uiTimerEnable(true);
    //enable all interrupts in case of they are disabled
    ENABLE_ALL_INTERRUPTS();
}

//@brief Timer2A interrupt handler
void UiTimer2AIntHandler(void)
{
	TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
	++m_msCounter;
	for(uint8_t i = 0; i < m_timerCbNum; i++)
	{
		if(m_timerCb[i]->timerCb != NULL)
		{
			if((m_msCounter % m_timerCb[i]->periodTime) == 0)
			{
				(*m_timerCb[i]->timerCb)();
			}
		}
	}
}

//@brief Delay for all UI methods
void uiDelay(uint32_t msDelay)
{
	uint32_t start = m_msCounter;
	while((m_msCounter - start) < msDelay)
	{
	}
}

//@brief Returns current ms Timer2A counter value
uint32_t uiDelayCounterMsVal()
{
	return m_msCounter;
}


/*@brief Main init method which initializes all the UI components*/
void uiInit(tContext* mainDrawingContext)
{
	if(mainDrawingContext == NULL)
	{
		abort(); //crash the APP if NULL
	}
	m_drawingCtx = mainDrawingContext;
	uiTimerInit();
	uiMessageBoxInit(); //msgBox
	uiKeyboardInit(); //keyboard
}

tContext* uiGetMainDrawingContext()
{
	return m_drawingCtx;
}

bool uiRegisterTimerCb(void(*cb)(void), uint16_t period)
{
	if(m_timerCbNum >= UI_TIMER_CB_NUM)
	{
		return false;
	}
	TimerCallback* newCb = (TimerCallback*)malloc(sizeof(TimerCallback));
	m_timerCb[m_timerCbNum++] = newCb;
	return true;
}

bool uiUnRegisterTimerCb(void(*cb)(void))
{
	uint8_t i = 0;
	for(i = 0; i < m_timerCbNum; i++)
	{
		if(m_timerCb[i]->timerCb == cb)
		{
			free(m_timerCb[i]);
			m_timerCbNum--;
			return true;
		}
	}
	return false;
}
