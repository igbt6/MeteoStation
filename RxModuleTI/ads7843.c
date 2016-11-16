/*
 * ads7843.c
 *
 *  Created on: 08-08-2016
 *      Author: igbt6
 *
 */

#include "ads7843.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
// Configure the SoftSSI module.  The size of the FIFO buffers can be
// Module SSI0, pins are assigned as follows:
//      PA2 - SoftSSICLK
//      PA3 - SoftSSIFss
//      PA4 - SoftSSIRx
//      PA5 - SoftSSITx
static void spiInit(void)
{

	uint32_t rxBuf[1];
	SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	GPIOPinConfigure(GPIO_PA2_SSI0CLK);
	GPIOPinConfigure(GPIO_PA3_SSI0FSS);
	GPIOPinConfigure(GPIO_PA4_SSI0RX);
	GPIOPinConfigure(GPIO_PA5_SSI0TX);

	GPIOPinTypeSSI(GPIO_PORTA_BASE,
			GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_3 | GPIO_PIN_2);

	// Configure and enable the SSI port for SPI master mode.  Use SSI0,
	// system clock supply, idle clock level low and active low clock in
	// freescale SPI mode, master mode, 1MHz SSI frequency, and 8-bit data.
	SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0,
	SSI_MODE_MASTER, 1000000, 8);

	// Enable the SSI0 module.
	SSIEnable(SSI0_BASE);

	//flush the receives fifos
	while (SSIDataGetNonBlocking(SSI0_BASE, &rxBuf[0]))
	{
	}
}

static uint16_t spiReadWord(uint8_t reg) {
	uint8_t txBuf = reg;
	uint32_t rxBuf[2] = { 0, 0 };
	uint16_t cur = 0x0000;
	//ADS7843_CS_LOW();
	SSIDataPut(SSI0_BASE, txBuf);
	while (SSIBusy(SSI0_BASE)) {
	}
	//read 2 ytes of data
	for (int idx = 0; idx < 2; ++idx)
	{
		SSIDataGet(SSI0_BASE, &rxBuf[idx]);
		rxBuf[idx]&= 0x00FF;
	}
	cur = ((rxBuf[0] << 4) | (rxBuf[1] >> 4)) & 0x0FFF;
	//ADS7843_CS_HIGH();
	return cur;
}

#define ADS7843_ENABLE_TOUCH_INT

void ADS7843init(void) {

	spiInit();

#ifdef ADS7843_USE_PIN_BUSY
	ADS7843_BUSY_INPUT();
#endif

#ifdef ADS7843_ENABLE_TOUCH_INT
	// Initialize GPIO interrupt dispatcher
	GPIOINT_Init();
	ADS7843_INT_INPUT();
	m_pinIrqCb = new
	Callback<void(uint8_t)>(this,&ADS7843penIRQCallback);
	GPIOINT_CallbackRegister(ADS7843_PIN_INT, m_pinIrqCb);
	ADS7843_INT_IRQ_CONFIG_FALLING(true); //falling edge - reacts on PENDOWN touch
#endif
	// assign default values
	m_touchInfoData.touchStatus = TOUCH_STATUS_PENUP;
	setIrqAndPowerDown();
}

bool ADS7843read() {
	return readPointXY(m_currentTouchedPoint, false);
}

bool ADS7843dataAvailable() {
	if (getTouchStatus() == ADS7843TOUCH_STATUS_TOUCHING) {
		return true;
	}
	return false;
}

TouchPoint ADS7843getTouchedPoint() {
	return m_currentTouchedPoint;
}

void ADS7843setIrqAndPowerDown(void) {
	uint8_t buf[3] = { ADS7843_READ_X | ADS7843_DFR, 0, 0 };
	ADS7843_CS_LOW();
	SPIDRV_MTransmitB(handle, buf, 1);
	SPIDRV_MTransmitB(handle, &buf[1], 2);
	ADS7843_CS_HIGH();
}

void ADS7843penIRQCallback(uint8_t pin) {
	if (pin == ADS7843_PIN_INT) {
		if (!getIrqPinState()) //if pendown
		{
			//ADS7843_INT_IRQ_CONFIG_PIN_DISABLE();
			ADS7843_INT_IRQ_CONFIG_FALLING(false);
			ADS7843_INT_IRQ_CONFIG_RISING(true);
			m_touchInfoData.touchStatus = TOUCH_STATUS_TOUCHING;
		} else {
			ADS7843_INT_IRQ_CONFIG_RISING(false);
			ADS7843_INT_IRQ_CONFIG_FALLING(true);
			m_touchInfoData.touchStatus = TOUCH_STATUS_PENUP;
		}
	}
}

bool ADS7843getIrqPinState(void) {
	return (bool) ADS7843_GET_INT_PIN();
}

void ADS7843readRawXY(uint16_t *x, uint16_t *y) {
	*x = spiReadData(ADS7843_READ_X | ADS7843_DFR);
#ifdef ADS7843_USE_PIN_BUSY
	// wait for conversion complete
	while(ADS7843_GET_BUSY_PIN());
#else
	//for (volatile int i = 0; i < 1000; i++);
#endif
	*y = spiReadData(ADS7843_READ_Y | ADS7843_DFR);
}

void ADS7843readXY(uint16_t *x, uint16_t *y) {
	uint16_t xyDataBuf[2][TOUCH_MAX_NUM_OF_SAMPLES];
	uint8_t i, j;
	uint16_t temp;

	for (i = 0; i < 7; i++) {
		readRawXY(&xyDataBuf[0][i], &xyDataBuf[1][i]);
	}
	*x = fastMedian(xyDataBuf[0]);
	*y = fastMedian(xyDataBuf[1]);
	return;
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

uint16_t ADS7843fastMedian(uint16_t *samples)
const
{
  // do a fast median selection  - code stolen from https://github.com/andysworkshop/stm32plus library
  #define PIX_SORT(a,b) { if ((a)>(b)) PIX_SWAP((a),(b)); }
  #define PIX_SWAP(a,b) { uint16_t temp=(a);(a)=(b);(b)=temp; }
  PIX_SORT(samples[0], samples[5]);PIX_SORT(samples[0], samples[3]);PIX_SORT(
		samples[1], samples[6]);PIX_SORT(samples[2], samples[4]);PIX_SORT(
		samples[0], samples[1]);PIX_SORT(samples[3], samples[5]);PIX_SORT(
		samples[2], samples[6]);PIX_SORT(samples[2], samples[3]);PIX_SORT(
		samples[3], samples[6]);PIX_SORT(samples[4], samples[5]);PIX_SORT(
		samples[1], samples[4]);PIX_SORT(samples[1], samples[3]);PIX_SORT(
		samples[3], samples[4]); return samples[3];
}

TouchPoint ADS7843translateCoordinates(const TouchPoint* rawPoint) {
TouchPoint p;
p.x = m_ax * rawPoint.x + m_bx * rawPoint.y + m_dx;
p.y = m_ay * rawPoint.x + m_by * rawPoint.y + m_dy;
return p;
}

bool ADS7843readPointXY(TouchPoint* touchPoint, bool calibrationEnabled) {
if (m_touchInfoData.touchStatus == TOUCH_STATUS_TOUCHING) {
	uint16_t xyDataBuf[2][7]; //7 samples
	uint8_t i, j;
	uint16_t temp;
	TouchPoint p;
	for (i = 0; i < 7; i++) {
		readRawXY(&xyDataBuf[0][i], &xyDataBuf[1][i]);
	}
	p.x = fastMedian(xyDataBuf[0]);
	p.y = fastMedian(xyDataBuf[1]);
	if (!calibrationEnabled) {
		touchPoint = translateCoordinates(p);
	} else {
		touchPoint = p;
	}
	return true;
}
return false;
}

TouchStatus ADS7843getTouchStatus() {
return m_touchInfoData.touchStatus;
}

bool ADS7843readOnePointRawCoordinates(TouchPoint* point) {
TouchPoint tempP;
const uint16_t samplesNum = 100;
uint16_t idx = 0;
uint32_t xSum = 0;
uint32_t ySum = 0;

while (getIrqPinState())
	; //wait till we pen down the panel
for (;;) {
	if (!getIrqPinState()) {
		readPointXY(tempP, true);
		xSum += tempP.x;
		ySum += tempP.y;
		++idx;
		if (idx == samplesNum)
			break;
	}
}
point.x = xSum / samplesNum;
point.y = ySum / samplesNum;
return true;
}

/*
 uint8_t ADS7843performThreePointCalibration(ILI9320& lcd)
 {
 TouchPoint p1,p2,p3;
 TouchPoint t1,t2,t3;
 // point 1 is at 25%,50%, 2 is at 75%,25% and 3 is at 75%,75%
 ILI9320DispalySize size_ = lcd.getSize();
 p1.x= (size_.x *25) /100;
 p1.y= (size_.y *50) /100;
 p2.x= (size_.x *75) /100;
 p2.y= (size_.y *25) /100;
 p3.x= (size_.x *75) /100;
 p3.y= (size_.y *75) /100;
 ADS7843_INT_IRQ_CONFIG_FALLING(false); //disable ext interrupts
 ADS7843_INT_IRQ_CONFIG_RISING(false);
 //1st point
 lcd.print("Tap & hold the point", 5, 10, 0, ILI9320ColorsRED);
 lcd.drawCircle(p1.x,p1.y,30, ILI9320ColorsBLACK);
 lcd.fillCircle(p1.x,p1.y,10, ILI9320ColorsBLACK);
 readOnePointRawCoordinates(t1);
 lcd.drawCircle(p1.x,p1.y,30, ILI9320ColorsGREEN);
 lcd.fillCircle(p1.x,p1.y,10, ILI9320ColorsGREEN);
 Delay(1000000); //1s
 //2nd point
 lcd.drawCircle(p2.x,p2.y,30, ILI9320ColorsBLACK);
 lcd.fillCircle(p2.x,p2.y,10, ILI9320ColorsBLACK);
 readOnePointRawCoordinates(t2);
 lcd.drawCircle(p2.x,p2.y,30, ILI9320ColorsGREEN);
 lcd.fillCircle(p2.x,p2.y,10, ILI9320ColorsGREEN);
 Delay(1000000); //1s
 //3rd point
 lcd.drawCircle(p3.x,p3.y,30, ILI9320ColorsBLACK);
 lcd.fillCircle(p3.x,p3.y,10, ILI9320ColorsBLACK);
 readOnePointRawCoordinates(t3);
 lcd.drawCircle(p3.x,p3.y,30, ILI9320ColorsGREEN);
 lcd.fillCircle(p3.x,p3.y,10, ILI9320ColorsGREEN);


 //final computation based on:
 //https://www.maximintegrated.com/en/app-notes/index.mvp/id/5296
 // and
 //http://www.ti.com/lit/an/slyt277/slyt277.pdf
 int32_t delta,deltaX1,deltaX2,deltaX3,deltaY1,deltaY2,deltaY3;
 // intermediate values for the calculation
 delta=((int32_t)(t1.x-t3.x)*(int32_t)(t2.y-t3.y))-((int32_t)(t2.x-t3.x)*(int32_t)(t1.y-t3.y));

 deltaX1=((int32_t)(p1.x-p3.x)*(int32_t)(t2.y-t3.y))-((int32_t)(p2.x-p3.x)*(int32_t)(t1.y-t3.y));
 deltaX2=((int32_t)(t1.x-t3.x)*(int32_t)(p2.x-p3.x))-((int32_t)(t2.x-t3.x)*(int32_t)(p1.x-p3.x));
 deltaX3=p1.x*((int32_t)t2.x*(int32_t)t3.y - (int32_t)t3.x*(int32_t)t2.y) -
 p2.x*(t1.x*(int32_t)t3.y - (int32_t)t3.x*(int32_t)t1.y) +
 p3.x*((int32_t)t1.x*(int32_t)t2.y - (int32_t)t2.x*(int32_t)t1.y);

 deltaY1=((int32_t)(p1.y-p3.y)*(int32_t)(t2.y-t3.y))-((int32_t)(p2.y-p3.y)*(int32_t)(t1.y-t3.y));
 deltaY2=((int32_t)(t1.x-t3.x)*(int32_t)(p2.y-p3.y))-((int32_t)(t2.x-t3.x)*(int32_t)(p1.y-p3.y));
 deltaY3=p1.y*((int32_t)t2.x*(int32_t)t3.y - (int32_t)t3.x*(int32_t)t2.y) -
 p2.y*((int32_t)t1.x*(int32_t)t3.y - (int32_t)t3.x*(int32_t)t1.y) +
 p3.y*((int32_t)t1.x*(int32_t)t2.y - (int32_t)t2.x*t1.y);

 // final values
 m_ax=(float)deltaX1/(float)delta;
 m_bx=(float)deltaX2/(float)delta;
 m_dx=(float)deltaX3/(float)delta;

 m_ay=(float)deltaY1/(float)delta;
 m_by=(float)deltaY2/(float)delta;
 m_dy=(float)deltaY3/(float)delta;
 lcd.print("Success - storing data...", 0, 100, 0, ILI9320ColorsRED);
 Delay(1000000); //1s
 lcd.clrScr();
 ADS7843_INT_IRQ_CONFIG_FALLING(true);

 return 0;
 }
 */
