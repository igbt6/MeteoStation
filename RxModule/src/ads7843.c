/*
 * ads7843.c
 *
 *  Created on: 08-08-2016
 *      Author: igbt6
 *      based on coocox library
 */
#include "spi.h"
#include "ADS7843.h"
#include "stdio.h"
#include "../emdrv/ustimer/ustimer.h"
#include "../emdrv/gpiointerrupt/gpiointerrupt.h"
#include "bsp.h"
#include "em_cmu.h"
#include "../emdrv/spidrv/spidrv.h"

//privates
static SPIDRV_HandleData_t handleData;
static SPIDRV_Handle_t handle = &handleData;

static TouchInfo touchInfoData;
extern volatile bool mADS7843ScreenTouched;
#define ADS7843_ENABLE_TOUCH_INT

static void ADS7843PenIRQCallback(uint8_t pin);
//*****************************************************************************
// \brief Initialize ADS7843
// \param None.
// This function initialize ADS7843's SPI interface and .
//return None.
//*****************************************************************************
void ADS7843Init(void) {
	/// Configuration data for SPI master using USART1.
#define SPIDRV_MASTER_USART1_LOC1                                         \
	{                                                                         \
	  USART1,                       /* USART port                       */    \
	  _USART_ROUTE_LOCATION_LOC1,   /* USART pins location number       */    \
	  50000,                      /* Bitrate                          */    \
	  8,                            /* Frame length                     */    \
	  0,                            /* Dummy tx value for rx only funcs */    \
	  spidrvMaster,                 /* SPI mode                         */    \
	  spidrvBitOrderMsbFirst,       /* Bit order on bus                 */    \
	  spidrvClockMode0,             /* SPI clock/phase mode             */    \
	  spidrvCsControlApplication,   /* CS controlled by the driver      */    \
	  spidrvSlaveStartImmediate     /* Slave start transfers immediately*/    \
	}

	CMU_ClockEnable(cmuClock_HFPER, true);
	// Enable clock for USART1
	CMU_ClockEnable(cmuClock_USART1, true);
	CMU_ClockEnable(cmuClock_GPIO, true);
	ADS7843_MOSI_OUTPUT();
	ADS7843_MISO_INPUT();
	ADS7843_CLK_OUTPUT();
	ADS7843_CS_OUTPUT();
	SPIDRV_Init_t initData = SPIDRV_MASTER_USART1_LOC1;
	// Initialize a SPI driver instance
	SPIDRV_Init(handle, &initData);

#ifdef ADS7843_USE_PIN_BUSY
	ADS7843_BUSY_INPUT();
	//ADS7843_BUSY_PULLED_INPUT();
#endif

#ifdef ADS7843_ENABLE_TOUCH_INT

	// Initialize GPIO interrupt dispatcher
	GPIOINT_Init();
	ADS7843_INT_INPUT();

	GPIOINT_CallbackRegister(ADS7843_PIN_INT, ADS7843PenIRQCallback);
	ADS7843_INT_IRQ_CONFIG_FALLING(true); //falling edge

#endif

	// assign default values
	touchInfoData.thAdLeft = TOUCH_AD_X_MIN;
	touchInfoData.thAdRight = TOUCH_AD_X_MAX;
	touchInfoData.thAdUp = TOUCH_AD_Y_MAX;
	touchInfoData.thAdDown = TOUCH_AD_Y_MIN;
	touchInfoData.touchStatus = 0;

	ADS7843SetIrqAndPowerDown();
}

void ADS7843SetIrqAndPowerDown(void) {
	uint8_t buf[3] = { ADS7843_READ_X | ADS7843_DFR, 0, 0 };
	ADS7843_CS_LOW();
	SPIDRV_MTransmitB(handle, buf, 1);
	SPIDRV_MTransmitB(handle, &buf[1], 2);
	ADS7843_CS_HIGH();
}

static void ADS7843PenIRQCallback(uint8_t pin) {
	if (pin == ADS7843_PIN_INT && !mADS7843ScreenTouched) {
		ADS7843_INT_IRQ_CONFIG_PIN_DISABLE();
		mADS7843ScreenTouched = true;
	}
}

uint16_t ADS7843PenInq(void) {
	return (uint16_t) ADS7843_GET_INT_PIN();
}

void TransferComplete(SPIDRV_Handle_t handle, Ecode_t transferStatus,
		int itemsTransferred) {
	if (transferStatus == ECODE_EMDRV_SPIDRV_OK) {
		return;
	}
}

static uint16_t ADS7843SpiReadData(uint8_t reg) {
	uint8_t txBuf = reg;
	uint8_t rxBuf[2] = { 0, 0 };
	uint16_t cur = 0x0000;
	//SPIDRV_MTransmit( handle, txBuf, 8,TransferComplete );
	ADS7843_CS_LOW();
	SPIDRV_MTransmitB(handle, &txBuf, 1);
	SPIDRV_MReceiveB(handle, rxBuf, 2);
	cur = ((rxBuf[0] << 4) | (rxBuf[1] >> 4)) & 0x0FFF;
	ADS7843_CS_HIGH();
	return cur;
}

//****************************************************************************
// \brief Read the x, y axis ADC convert value once from ADS7843
// \param x To save the x axis ADC convert value.
// \param y To save the y axis ADC convert value.
// \return None.
//*****************************************************************************
void ADS7843ReadRawXY(uint16_t *x, uint16_t *y) {
	*x = ADS7843SpiReadData(ADS7843_READ_X | ADS7843_DFR);
#ifdef ADS7843_USE_PIN_BUSY
	// wait for conversion complete
	while(ADS7843_GET_BUSY_PIN());
#else
	//for (volatile int i = 0; i < 1000; i++);
#endif
	*y = ADS7843SpiReadData(ADS7843_READ_Y | ADS7843_DFR);
}

//*****************************************************************************
//brief read the x, y axis ADC convert value from ADS7843(with software filter)
// \param x To save the x axis ADC convert value.
// \param y To save the y axis ADC convert value.
// This function Reads the x,y axis's ADC value from ADS7843 with software
// filter. The function first read out TOUCH_SMAPLE_LEN samples. Then discard
// the first data and the last data and sort the remained data. Then discard
// the first and the last one of the remained data and compute the average
// value of the final remained data.
// \return None.
//*****************************************************************************
void ADS7843ReadXY(uint16_t *x, uint16_t *y) {
	uint16_t xyDataBuf[2][TOUCH_MAX_NUM_OF_SAMPLES];
	uint8_t i, j;
	uint16_t temp;

	for (i = 0; i < TOUCH_MAX_NUM_OF_SAMPLES; i++) {
		ADS7843ReadRawXY(&xyDataBuf[0][i], &xyDataBuf[1][i]);
	}
// Discard the first and the last one of the data and sort remained data
	for (i = 1; i < TOUCH_MAX_NUM_OF_SAMPLES - 2; i++) {
		for (j = i + 1; j < TOUCH_MAX_NUM_OF_SAMPLES - 1; j++) {
			if (xyDataBuf[0][i] > xyDataBuf[0][j]) {
				temp = xyDataBuf[0][i];
				xyDataBuf[0][i] = xyDataBuf[0][j];
				xyDataBuf[0][j] = temp;
			}

			if (xyDataBuf[1][i] > xyDataBuf[1][j]) {
				temp = xyDataBuf[1][i];
				xyDataBuf[1][i] = xyDataBuf[1][j];
				xyDataBuf[1][j] = temp;
			}
		}
	}
	xyDataBuf[0][0] = 0;
	xyDataBuf[1][0] = 0;

// Discard the first and the last one of the sorted data
// and compute the average value of the remained data.
	for (i = 2; i < TOUCH_MAX_NUM_OF_SAMPLES - 2; i++) {
		xyDataBuf[0][0] += xyDataBuf[0][i];
		xyDataBuf[1][0] += xyDataBuf[1][i];
	}
	*x = xyDataBuf[0][0] / (TOUCH_MAX_NUM_OF_SAMPLES - 4);
	*y = xyDataBuf[1][0] / (TOUCH_MAX_NUM_OF_SAMPLES - 4);
}

//*****************************************************************************
// \brief Read the XY coordinate of touch point.
// \param x To save the x coordinate
// \param y To save the y coordinate
// This function is to read current touch point. The x,y coordinates will be
// read out from the input parameters. If the screen is not touched, it will
// return last value, after the last value is read out, additional read will
// return fail information and nothing will be read out.
// \return None.
//*****************************************************************************
uint8_t ADS7843ReadPointXY(uint16_t *x, uint16_t *y) {
	uint16_t adX;
	uint16_t adY;
	uint32_t diff = 0;

	if (!ADS7843_GET_INT_PIN()) {
		//  If pen down
		ADS7843ReadXY(&adX, &adY);
		//x
		diff = touchInfoData.thAdRight - touchInfoData.thAdLeft;
		//  limit the edges
		if (adX < (touchInfoData.thAdLeft - TOUCH_AD_CALIB_ERROR)
				|| adX > (touchInfoData.thAdRight + TOUCH_AD_CALIB_ERROR)) {
			adX = 0;
		}
		//  calculate the x coordinate
		*x = (adX * TOUCH_SCREEN_WIDTH) / diff;
		touchInfoData.lastX = touchInfoData.curX;
		touchInfoData.curX = *x;

		//y
		diff = touchInfoData.thAdUp - touchInfoData.thAdDown;
		//  limit the edges
		if (adY < (touchInfoData.thAdDown - TOUCH_AD_CALIB_ERROR)
				|| adY > (touchInfoData.thAdUp + TOUCH_AD_CALIB_ERROR)) {
			adY = 0;
		}
		//  calculate the y coordinate
		*y = (adY * TOUCH_SCREEN_HEIGHT) / diff;
		touchInfoData.lastY = touchInfoData.curY;
		touchInfoData.curY = *y;
		return 0;
	}
	else
	{
		//  if pen up
		if (touchInfoData.touchStatus & TOUCH_STATUS_PENUP) {
			touchInfoData.lastX = touchInfoData.curX;
			*x = touchInfoData.curX;
			touchInfoData.lastY = touchInfoData.curY;
			*y = touchInfoData.curY;
			touchInfoData.touchStatus = 0;
			return 0;
		}
		return 1;
	}
}

void getCoordinates(uint16_t* x, uint16_t* y) {
	*x=touchInfoData.curX;
	*y=touchInfoData.curY;
}

//*****************************************************************************
// \brief Touch screen calibration.
// \param None.
// This function is to calibrate the touch screen. If the read out coordinate
// is not accurate, you can use this function to calibrate it. After this
// function is called, you must touch the screen in about 10 seconds or the
// function will return with no effect. After you touch the screen, the counter
// will be zeroed and restart counting. If the calibration is not over and the
// touch is always applied on the screen, the counter will not increase, so you
// don't have to worry about the calibrate time.
//     All you need to do with the calibration is to use a touch pen or other
// object which is a little sharp but will not damage the touch screen to touch
// the screen and slide the pen from top edge of the screen to the bottom edge.
// Then slide the pen from left edge to right edge. Make sure that the pen should
// not leave the screen until it slides to the edge or the calibrate may be
// inaccurate. The direction or sequence is optional. If __DEBUG_PRINT__ is defined
// you can see the calibration result. The calibration will calibrate both x axis
// and y axis. Also you can calibration only one axis. the axis which is not
// calibrated will retain its original value.
// \return None.
//
//*****************************************************************************
uint8_t ADS7843Calibration(void) {
	uint16_t adx, ady;
	uint16_t adxmin = TOUCH_AD_X_MAX;
	uint16_t adxmax = TOUCH_AD_X_MIN;
	uint16_t adymin = TOUCH_AD_X_MAX;
	uint16_t adymax = TOUCH_AD_X_MIN;
	uint16_t calibrationFlag = 0;
	uint32_t timeout = 0;

#ifdef ADS7843_ENABLE_TOUCH_INT
//  disable touch interrupt
	ADS7843_INT_IRQ_CONFIG_PIN_DISABLE();
#endif
	while (1) {
		if (!ADS7843_GET_INT_PIN()) {
			//
			// If pen down continuously read the x,y value and record the
			// maximum and minimum value
			//
			while (!ADS7843_GET_INT_PIN()) {
				ADS7843ReadXY(&adx, &ady);
				if (adx < adxmin) {
					adxmin = adx;
				} else if (adx > adxmax) {
					adxmax = adx;
				}

				if (ady < adymin) {
					adymin = ady;
				} else if (ady > adymax) {
					adymax = ady;
				}
			}
			// Counter clear.
			timeout = 0;
		}

		// If x axis calibration is not complete
		if (!(calibrationFlag & 1)) {
			//  If x axis calibrate over
			if ((adxmax > (TOUCH_AD_X_MAX - TOUCH_AD_CALIB_ERROR))
					&& (adxmax < (TOUCH_AD_X_MAX + TOUCH_AD_CALIB_ERROR))
					&& (adxmin > (TOUCH_AD_X_MIN - TOUCH_AD_CALIB_ERROR))
					&& (adxmin < (TOUCH_AD_X_MIN + TOUCH_AD_CALIB_ERROR))) {

				touchInfoData.thAdLeft = adxmin;
				touchInfoData.thAdRight = adxmax;
				calibrationFlag |= 1;
			}
		}

		// If y axis calibration is not complete
		if (!(calibrationFlag & 2)) {
			//  If y axis calibrate over
			if ((adymax > (TOUCH_AD_Y_MAX - TOUCH_AD_CALIB_ERROR))
					&& (adymax < (TOUCH_AD_Y_MAX + TOUCH_AD_CALIB_ERROR))
					&& (adymin > (TOUCH_AD_Y_MIN - TOUCH_AD_CALIB_ERROR))
					&& (adymin < (TOUCH_AD_Y_MIN + TOUCH_AD_CALIB_ERROR))) {
				touchInfoData.thAdUp = adymax;
				touchInfoData.thAdDown = adymin;
				calibrationFlag |= 2;
			}
		}

		// If two coordinates calibrate over or timer out
		if ((calibrationFlag == 3) || (timeout++ > 100000)) {
#ifdef ADS7843_ENABLE_TOUCH_INT
			ADS7843_INT_IRQ_CONFIG_FALLING(true);
#endif
			return calibrationFlag;
		}
		Delay(1000);
	}
}
