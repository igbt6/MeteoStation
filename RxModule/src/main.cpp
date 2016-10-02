/**************************************************************************//**
 * @file
 * @brief Meteo Station
 * @author igbt6
 * @version 0.01
 *
 ******************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "ILI9320.h"
#include "ads7843.h"
#include "bitmaps.h"
#include "../emdrv/ustimer/ustimer.h"



#include "../3rdParty/AWind/TextBoxString.h"
#include "../3rdParty/AWind/TextBoxNumber.h"
#include "../3rdParty/AWind/DecoratorPrimitives.h"
#include "../3rdParty/AWind/WindowsManager.h"
#include "../3rdParty/AWind/DefaultDecorators.h"
#include "../3rdParty/AWind/defaultFonts.h"

class TextExampleWindow : public MainWindow
{
	TextBoxNumber *_textNumber;
public:
	TextExampleWindow(int width,int height):MainWindow(width,height)
	{
		AddDecorator(new DecoratorRectFill(Color::Black));
		AddDecorator(new DecoratorColor(Color::SkyBlue));
		int x=0;
		int y=40;
		TextBoxFString *label=new TextBoxFString(x,y,width/2,25,("This is label: "));
		label->SetFont(BigFont);
		x=width*3.0/4;
		_textNumber=new TextBoxNumber(x,y,width-x,25,0);
		_textNumber->SetDecorators(GetDecorators()); // here we save one decorator beacuse main window and text window have thae same decorator properties: black background
		_textNumber->SetFont(BigFont);
		_textNumber->SetMargins(20,2);
		_textNumber->SetNumber(4);
		_textNumber->SetIsReadOnly(false);

		AddChild(label);
		AddChild(_textNumber);
	}
	void Create()
	{
	}
	void Notify(Window * wnd)
	{
		if(wnd == _textNumber)
		{
			//out<<F("Value changed")<<((TextBoxNumber *)_textNumber)->GetNumber()<<endln;
		}
	}
};

#include "../3rdParty/AWind/TextBoxString.h"
#include "../3rdParty/AWind/TextBoxStrTouch.h"

class wnd_info : public MainWindow
{
  TextBoxStrTouch *edtBox;
public:
  wnd_info(int width,int height):MainWindow(width,height)
  {
    int x=0, y=10, xOff=95, yOff=20, xW=width-xOff, tHgt=13;
    AddDecorator(new DecoratorRectFill(Color::Black));
    AddDecorator(new DecoratorColor(Color::SkyBlue));

    TextBoxFString *label=new TextBoxFString(x,y,104,tHgt,("Enter text:"));
    AddChild(label);
    edtBox=new TextBoxStrTouch(xOff,y,xW,tHgt,"Some text to edit");
    initEditBox(edtBox);

    y += yOff;
    label=new TextBoxFString(x,y,104,tHgt,("Short Text:"));
    AddChild(label);
    edtBox=new TextBoxStrTouch(xOff,y,xW,tHgt,"Short text");
    initEditBox(edtBox);

    y += yOff;
    label=new TextBoxFString(x,y,104,tHgt,("Long Text:"));
    AddChild(label);
    edtBox=new TextBoxStrTouch(xOff,y,xW,tHgt,"Some long text that is greater than the edit box size will be around");
    initEditBox(edtBox);

    y += yOff;
    label=new TextBoxFString(x,y,104,tHgt,("Long Text:"));
    AddChild(label);
    edtBox=new TextBoxStrTouch(xOff,y,xW,tHgt,"This will exceed the length of the edit area");
    initEditBox(edtBox);
  }
protected:
  void initEditBox(TextBoxStrTouch *edt)
  {
    edt->SetDecorators(GetDecorators());
    edt->SetMargins(4,0);
    AddChild(edt);
  }

public:
	void Create()
	{
	}
	void Notify(Window * wnd)
	{
	}
};

//Sensor Demo
/**************************************************************************//**
 * @brief  Main function
 *****************************************************************************/
int main(void) {
	/* Chip errata */
	CHIP_Init();
	CMU_OscillatorEnable(cmuOsc_HFXO, true, false);
	CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO); //32MHZ
	CMU_ClockDivSet(cmuClock_HF, cmuClkDiv_1);
	CMU_ClockDivSet(cmuClock_HFPER, cmuClkDiv_1);
	CMU_ClockEnable(cmuClock_HFPER, true);
	//----------------------- ILI9320 first working tests -----------------------
	//BSP_TraceProfilerSetup();
	/*BSP_LedsInit();
	 BSP_LedSet(0);
	 BSP_LedSet(1);*/
	// Initialization of USTIMER driver

	USTIMER_Init();
	ILI9320 ili9320;
	ili9320.init();
	ADS7843 ads7843;
	ads7843.performThreePointCalibration(ili9320);

	//Awind Tests
	DefaultDecorators::InitAll();
	//initialize window manager
	//WindowsManager<TextExampleWindow> windowsManager(&ili9320,&ads7843);
	WindowsManager<wnd_info> windowsManager(&ili9320,&ads7843);
	windowsManager.Initialize();
	while(1)
	{
		windowsManager.loop();
	}
	while (1) {
		if (ads7843.dataAvailable())
		{
			ads7843.read();
			TouchPoint p = ads7843.getTouchedPoint();
			ili9320.fillCircle(p.x, p.y,3, ILI9320::Colors::BLUE);
		}
	}
}

