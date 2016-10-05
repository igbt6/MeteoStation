#pragma once
/*
  AWind.h - Arduino window library support for Color TFT LCD Boards
  Copyright (C)2015 Andrei Degtiarev. All right reserved
  

  You can always find the latest version of the library at 
  https://github.com/AndreiDegtiarev/AWind


  This library is free software; you can redistribute it and/or
  modify it under the terms of the CC BY-NC-SA 3.0 license.
  Please see the included documents for further information.

  Commercial use of this library requires you to buy a license that
  will allow commercial use. This includes using the library,
  modified or not, as a tool to sell products.

  The license applies to all part of the library including the 
  examples and tools supplied with the library.
*/
#include "Color.h"
#include "../../src/ILI9320.h"
#include <cstdio>
#include <math.h>

///Device context. Abstraction layer to the device specific drawing code. Coordinates in drawing function are in window coordinate system that internaly translated into screen coordinate system
class DC
{
public:
	enum HorizontalAlignment
	{
		Left,
		Center,
		Right
	};
private:
	ILI9320 *_lcd;  //!< Pointer to the lcd class class
	int _offset_x; //!< Offset of coordinate system alon x axis
	int _offset_y;//!< Offset of coordinate system along y axis
	int _last_x;  //!< Last x coordinate. It is needed in MoveTo and LineTo functions
	int _last_y;  //!< Last x coordinate. It is needed in MoveTo and LineTo functions
	char _buffer[15]; //!< Internal buffer for numbers convertion into string
public:
	enum ScreenOrientation
	{
		Landscape,
		Portrate,
	};
	///Constructor for global context, that created only once in WindowsManager
	DC(ILI9320 *lcd)
	{
		_lcd=lcd;
		Reset();
	}
	///Returns screen orientation vertical or horisontal
	ScreenOrientation ScreenOrientation()
	{
		return _lcd->getScreenOrientation() == ILI9320::Orientation::LANDSCAPE ? Landscape : Portrate;
	}
	///Returns screen width
	int DeviceWidth()
	{
		return _lcd->getResolution().x-1;
	}
	///Returns screen height
	int DeviceHeight()
	{
		return _lcd->getResolution().y-1;
	}
	///Resets device context into initial condition
	void Reset()
	{
		_offset_x = 0;
		_offset_y = 0;
		_last_x = 0;
		_last_y = 0;
	}
	///Initializes drawing coordinate system offset
	/**
	\param offset_x offset in x direction
	\param offset_y offset in y direction
	*/
	void Offset(int offset_x,int offset_y)
	{
		_offset_x+=offset_x;
		_offset_y+=offset_y;
	}
	///Converts x coordinate from window into screen coordinate system
	int ToDC_X(int x)
	{
		return x+_offset_x;
	}
	///Converts y coordinate from window into screen coordinate system
	int ToDC_Y(int y)
	{
		return y+_offset_y;
	}
	///Draws rectangle. Input coordinates have to be defined in the window coordinate system
	void Rectangle(int left,int top,int right,int bottom)
	{
		_lcd->drawRect(ToDC_X(left),ToDC_Y(top),ToDC_X(right),ToDC_Y(bottom),ILI9320::GREEN); //TODO
	}
	///Draws rectangle with 3D border. Input coordinates have to be defined in the window coordinate system
	void Rectangle3D(int left,int top,int right,int bottom,Color color1,Color color2)
	{
		SetColor(color2);
		MoveTo(left,bottom);
		LineTo(right,bottom);
		LineTo(right,top);
		SetColor(color1);
		LineTo(left,top);
		LineTo(left,bottom);
	}
	///Fills rectangle. Input coordinates have to be defined in the window coordinate system
	void FillRect(int left,int top,int right,int bottom)
	{
		_lcd->fillRect (ToDC_X(left),ToDC_Y(top),ToDC_X(right),ToDC_Y(bottom),ILI9320::WHITE); //TODO
	}
	///Fills rectangle with gradient color. Input coordinates have to be defined in the window coordinate system
	void FillGradientRect(int left,int top,int right,int bottom,Color color1,Color color2)
	{
		int start_x=ToDC_X(left);
		int start_y=ToDC_Y(top);
		int length_x=ToDC_X(right)-start_x;
		int length_y=ToDC_Y(bottom)-start_y;
		uint8_t rgb[3];
		uint8_t rgb1[3];
		uint8_t rgb2[3];
		rgb1[0]=color1.GetR();
		rgb1[1]=color1.GetG();
		rgb1[2]=color1.GetB();
		rgb2[0]=color2.GetR();
		rgb2[1]=color2.GetG();
		rgb2[2]=color2.GetB();
		float fctr;
		for(int i=0;i<=length_y;i++)
		{
			fctr=(float)i/length_y;
			for(int j=0;j<3;j++)
			{
				rgb[j]=rgb1[j]+fctr*(rgb2[j]-rgb1[j]);
				//out<<rgb2[j]<<" ";
			}
			//out<<endln;
			_lcd->setColor(rgb[0],rgb[1],rgb[2]);
			_lcd->drawHLine (start_x,start_y+i,length_x,rgb[0]|rgb[1]|rgb[2]);
		}
	}
	///Fills rounded rectangle. Input coordinates have to be defined in the window coordinate system
	void FillRoundRect(int left,int top,int right,int bottom)
	{
		_lcd->fillRoundRect (ToDC_X(left),ToDC_Y(top),ToDC_X(right),ToDC_Y(bottom),ILI9320::RED); //TODO
	}
	///Draws rounded rectangle. Input coordinates have to be defined in the window coordinate system
	void DrawRoundRect(int left,int top,int right,int bottom)
	{
		_lcd->drawRoundRect (ToDC_X(left),ToDC_Y(top),ToDC_X(right),ToDC_Y(bottom),ILI9320::YELLOW); //TODO
	}
	///Draws circle. Input coordinates have to be defined in the window coordinate system
	void FillCircle(int x0, int y0, int radius)
	{
		_lcd->fillCircle(ToDC_X(x0), ToDC_Y(y0),radius,ILI9320::BLUE); //TODO
	}

	///Draws integer number. Input coordinates have to be defined in the window coordinate system
	void DrawNumber(int number,int x,int y, HorizontalAlignment aligment = HorizontalAlignment::Left, int width = 0)
	{
		sprintf(_buffer,"%d",number);
		DrawText(_buffer,x,y, aligment, width);
	}
	///Draws float number. Input coordinates have to be defined in the window coordinate system
	/**
	\param number float input value
	\param dec number decimal places
	*/
	void DrawNumber(float number,int dec,int x,int y, HorizontalAlignment aligment = HorizontalAlignment::Left, int width = 0)
	{
		sprintf(_buffer, "%3.2f", number);
		DrawText(_buffer,x,y, aligment, width);
	}
	///Draws PROGMEM string. Input coordinates have to be defined in the window coordinate system
	/*
	void DrawText(const char * text,int x,int y, HorizontalAlignment aligment = HorizontalAlignment::Left, int width = 0)
	{
		int stl, i;

		stl = strlen((const char *)text);

		if (aligment == HorizontalAlignment::Center)
			x = x + (width - stl*_lcd->getFontXsize()) / 2;
		else if (aligment == HorizontalAlignment::Right)
			x = x + width - stl*_lcd->getFontXsize();

		x=ToDC_X(x);
		y=ToDC_Y(y);
		if (_lcd->getScreenOrientation()==ILI9320::Orientation::PORTRAIT)
		{
			if (x==RIGHT)
				x=(_lcd->getResolution().x)-(stl*_lcd->getFontXsize());
			if (x==CENTER)
				x=((_lcd->getResolution().x)-(stl*_lcd->getFontXsize()))/2;
		}
		else
		{
			if (x==RIGHT)
				x=(_lcd->getResolution().y)-(stl*_lcd->getFontXsize());
			if (x==CENTER)
				x=((_lcd->getResolution().y)-(stl*_lcd->getFontXsize()))/2;
		}
		for (i=0; i<stl; i++)
		{
			unsigned char c = text[i];
			DrawSymbol(c, x + (i*(_lcd->getFontXsize())), y);
		}

	}
	*/
	///Returns symbol width for the current font 
	int FontWidth()
	{
		return _lcd->getFontXsize();
	}
	///Returns symbol jeight for the current font 
	int FontHeight()
	{
		return _lcd->getFontXsize();
	}
	///Draws symbol. Input coordinates have to be defined in the screen system
	void DrawSymbol(const char c,int dc_x,int dc_y)
	{
		_lcd->printChar(c, dc_x, dc_y,ILI9320::FUCHSIA); //TODO
	}
	///Draws a character. Input coordinates have to be defined in the window coordinate system
	void DrawChar(const char c,int x, int y)
	{
		x=ToDC_X(x);
		y=ToDC_Y(y);
		DrawSymbol(c, x, y);
	}
	///Draw caret. Input coordinates have to be defined in the window coordinate system
	void DrawCaret(int pos, int x, int y)
	{
		x = x+(pos*_lcd->getFontXsize());
		DrawChar('_', x, y+2);
	}
	///Draws string. Input coordinates have to be defined in the window coordinate system
	void DrawText(const char * text, int x, int y, HorizontalAlignment aligment = HorizontalAlignment::Left, int width = 0)
	{
		if (aligment == HorizontalAlignment::Center)
			x = x+(width - strlen(text)*_lcd->getFontXsize()) / 2;
		else if(aligment == HorizontalAlignment::Right)
			x = x + width - strlen(text)*_lcd->getFontXsize();
		x=ToDC_X(x);
		y=ToDC_Y(y);
		for(int i=0;i<strlen(text);i++)
		{
			char c=text[i];
			//if(c==' ')
			//	break;
			DrawSymbol(c, x + (i*(_lcd->getFontXsize())), y);
		}
	}
	///Draws sector. Input coordinates have to be defined in the window coordinate system
	void Sector(int x0, int y0, int radius,float start_angle_rad,float angle_rad)
	{
		x0=ToDC_X(x0);
		y0=ToDC_Y(y0);
		int x = 0;
		int y = radius;
		float cos_factor = cos(-start_angle_rad);
		float sin_factor = sin(-start_angle_rad);
		float tan_stop=tan(angle_rad/2);
		int delta = 2 - 2 * radius;
		int error = 0;
		float ratio;
		float rot_x,rot_y;
		while(y >= 0) {
			ratio=x/(float)y;
			if(ratio>tan_stop)
				break;
			rot_x = x*cos_factor - y*sin_factor;
			rot_y = x*sin_factor + y*cos_factor;
			_lcd->drawPixel(x0 + rot_x, y0 - rot_y,ILI9320::BLUE);
			rot_x =-x*cos_factor - y*sin_factor;
			rot_y = -x*sin_factor + y*cos_factor;
			_lcd->drawPixel(x0 + rot_x, y0 - rot_y,ILI9320::BLUE);
			//_lcd->drawPixel(x0 + x, y0 - y);
			//_lcd->drawPixel(x0 - x, y0 - y);
			error = 2 * (delta + y) - 1;
			if(delta < 0 && error <= 0)
			{
				++x;
				delta += 2 * x + 1;
				continue;
			}
			error = 2 * (delta - x) - 1;
			if(delta > 0 && error > 0)
			{
				--y;
				delta += 1 - 2 * y;
				continue;
			}
			++x;
			delta += 2 * (x - y);
			--y;
		}
	}
	void SetDeviceColor(Color color)
	{
		_lcd->setColor(color.GetR(),color.GetG(),color.GetB());
	}

	void SetColor(Color color)
	{
		SetDeviceColor(color);
	}
	void SetBackColor(Color color)
	{
		//_lcd->setBackColor(VGA_TRANSPARENT);
		SetDeviceColor(color);
	}
	void SetFont(const uint8_t *font)
	{
		_lcd->setFont(font);
	}
	void MoveTo(int x,int y)
	{ 
		_last_x=ToDC_X(x);
		_last_y=ToDC_Y(y);
	}
	void LineTo(int x,int y)
	{ 
		_lcd->drawLine(_last_x,_last_y,ToDC_X(x),ToDC_Y(y),ILI9320::LIME); //TODO
		_last_x=ToDC_X(x);
		_last_y=ToDC_Y(y);
	}
	void Line(int x1,int y1,int x2,int y2)
	{
		MoveTo(x1,y1);
		LineTo(x2,y2);
	}
};





#if 0
//NEW CHANGED VERSION

#pragma once
/*
  AWind.h - Arduino window library support for Color TFT LCD Boards
  Copyright (C)2015 Andrei Degtiarev. All right reserved


  You can always find the latest version of the library at
  https://github.com/AndreiDegtiarev/AWind


  This library is free software; you can redistribute it and/or
  modify it under the terms of the CC BY-NC-SA 3.0 license.
  Please see the included documents for further information.

  Commercial use of this library requires you to buy a license that
  will allow commercial use. This includes using the library,
  modified or not, as a tool to sell products.

  The license applies to all part of the library including the
  examples and tools supplied with the library.
*/
#include "Color.h"
#include "../../src/ILI9320.h"
#include <cstdio>
#include <math.h>

///Device context. Abstraction layer to the device specific drawing code. Coordinates in drawing function are in window coordinate system that internaly translated into screen coordinate system
class DC
{
public:
	enum HorizontalAlignment
	{
		Left,
		Center,
		Right
	};
private:
	ILI9320 *_lcd;  //!< Pointer to the lcd class class
	int _offset_x; //!< Offset of coordinate system alon x axis
	int _offset_y;//!< Offset of coordinate system along y axis
	int _last_x;  //!< Last x coordinate. It is needed in MoveTo and LineTo functions
	int _last_y;  //!< Last x coordinate. It is needed in MoveTo and LineTo functions
	char _buffer[15]; //!< Internal buffer for numbers convertion into string
public:
	enum ScreenOrientation
	{
		Landscape,
		Portrate,
	};
	///Constructor for global context, that created only once in WindowsManager
	DC(ILI9320 *lcd)
	{
		_lcd=lcd;
		Reset();
	}
	///Returns screen orientation vertical or horisontal
	ScreenOrientation ScreenOrientation()
	{
		return _lcd->getScreenOrientation() == ILI9320::Orientation::LANDSCAPE ? Landscape : Portrate;
	}
	///Returns screen width
	int DeviceWidth()
	{
		return _lcd->getResolution().x-1;
	}
	///Returns screen height
	int DeviceHeight()
	{
		return _lcd->getResolution().y-1;
	}
	///Resets device context into initial condition
	void Reset()
	{
		_offset_x = 0;
		_offset_y = 0;
		_last_x = 0;
		_last_y = 0;
	}
	///Initializes drawing coordinate system offset
	/**
	\param offset_x offset in x direction
	\param offset_y offset in y direction
	*/
	void Offset(int offset_x,int offset_y)
	{
		_offset_x+=offset_x;
		_offset_y+=offset_y;
	}
	///Converts x coordinate from window into screen coordinate system
	int ToDC_X(int x)
	{
		return x+_offset_x;
	}
	///Converts y coordinate from window into screen coordinate system
	int ToDC_Y(int y)
	{
		return y+_offset_y;
	}
	///Draws rectangle. Input coordinates have to be defined in the window coordinate system
	void Rectangle(int left,int top,int right,int bottom, Color color)
	{
		_lcd->drawRect(ToDC_X(left),ToDC_Y(top),ToDC_X(right),ToDC_Y(bottom),color.GetValue());
	}
	///Draws rectangle with 3D border. Input coordinates have to be defined in the window coordinate system
	void Rectangle3D(int left,int top,int right,int bottom,Color color1,Color color2)
	{
		//SetColor(color2);
		MoveTo(left,bottom);
		LineTo(right,bottom,color2.GetValue());
		LineTo(right,top,color2.GetValue());
		//SetColor(color1);
		LineTo(left,top,color1.GetValue());
		LineTo(left,bottom,color1.GetValue());
	}
	///Fills rectangle. Input coordinates have to be defined in the window coordinate system
	void FillRect(int left,int top,int right,int bottom, Color color)
	{
		_lcd->fillRect(ToDC_X(left),ToDC_Y(top),ToDC_X(right),ToDC_Y(bottom),color.GetValue());
	}
	///Fills rectangle with gradient color. Input coordinates have to be defined in the window coordinate system
	void FillGradientRect(int left,int top,int right,int bottom,Color color1,Color color2)
	{
		int start_x=ToDC_X(left);
		int start_y=ToDC_Y(top);
		int length_x=ToDC_X(right)-start_x;
		int length_y=ToDC_Y(bottom)-start_y;
		uint8_t rgb[3];
		uint8_t rgb1[3];
		uint8_t rgb2[3];
		rgb1[0]=color1.GetR();
		rgb1[1]=color1.GetG();
		rgb1[2]=color1.GetB();
		rgb2[0]=color2.GetR();
		rgb2[1]=color2.GetG();
		rgb2[2]=color2.GetB();
		float fctr;
		for(int i=0;i<=length_y;i++)
		{
			fctr=(float)i/length_y;
			for(int j=0;j<3;j++)
			{
				rgb[j]=rgb1[j]+fctr*(rgb2[j]-rgb1[j]);
			}
			_lcd->setColor(rgb[0],rgb[1],rgb[2]);
			_lcd->drawHLine (start_x,start_y+i,length_x,rgb[0]|rgb[1]|rgb[2]);
		}
	}
	///Fills rounded rectangle. Input coordinates have to be defined in the window coordinate system
	void FillRoundRect(int left,int top,int right,int bottom, Color color)
	{
		_lcd->fillRoundRect (ToDC_X(left),ToDC_Y(top),ToDC_X(right),ToDC_Y(bottom),color.GetValue());
	}
	///Draws rounded rectangle. Input coordinates have to be defined in the window coordinate system
	void DrawRoundRect(int left,int top,int right,int bottom, Color color)
	{
		_lcd->drawRoundRect (ToDC_X(left),ToDC_Y(top),ToDC_X(right),ToDC_Y(bottom),color.GetValue());
	}
	///Draws circle. Input coordinates have to be defined in the window coordinate system
	void FillCircle(int x0, int y0, int radius, Color color)
	{
		_lcd->fillCircle(ToDC_X(x0), ToDC_Y(y0),radius,color.GetValue());
	}

	///Draws integer number. Input coordinates have to be defined in the window coordinate system
	void DrawNumber(int number,int x,int y, HorizontalAlignment aligment = HorizontalAlignment::Left, int width = 0)
	{
		sprintf(_buffer,"%d",number);
		DrawText(_buffer,x,y, aligment, width);
	}
	///Draws float number. Input coordinates have to be defined in the window coordinate system
	/**
	\param number float input value
	\param dec number decimal places
	*/
	void DrawNumber(float number,int dec,int x,int y, HorizontalAlignment aligment = HorizontalAlignment::Left, int width = 0)
	{
		sprintf(_buffer, "%3.2f", number);
		DrawText(_buffer,x,y, aligment, width);
	}
	///Draws PROGMEM string. Input coordinates have to be defined in the window coordinate system
	/*
	void DrawText(const char * text,int x,int y, HorizontalAlignment aligment = HorizontalAlignment::Left, int width = 0)
	{
		int stl, i;

		stl = strlen((const char *)text);

		if (aligment == HorizontalAlignment::Center)
			x = x + (width - stl*_lcd->getFontXsize()) / 2;
		else if (aligment == HorizontalAlignment::Right)
			x = x + width - stl*_lcd->getFontXsize();

		x=ToDC_X(x);
		y=ToDC_Y(y);
		if (_lcd->getScreenOrientation()==ILI9320::Orientation::PORTRAIT)
		{
			if (x==RIGHT)
				x=(_lcd->getResolution().x)-(stl*_lcd->getFontXsize());
			if (x==CENTER)
				x=((_lcd->getResolution().x)-(stl*_lcd->getFontXsize()))/2;
		}
		else
		{
			if (x==RIGHT)
				x=(_lcd->getResolution().y)-(stl*_lcd->getFontXsize());
			if (x==CENTER)
				x=((_lcd->getResolution().y)-(stl*_lcd->getFontXsize()))/2;
		}
		for (i=0; i<stl; i++)
		{
			unsigned char c = text[i];
			DrawSymbol(c, x + (i*(_lcd->getFontXsize())), y);
		}

	}
	*/
	///Returns symbol width for the current font
	int FontWidth()
	{
		return _lcd->getFontXsize();
	}
	///Returns symbol jeight for the current font
	int FontHeight()
	{
		return _lcd->getFontXsize();
	}
	///Draws symbol. Input coordinates have to be defined in the screen system
	void DrawSymbol(const char c,int dc_x,int dc_y, Color color)
	{
		_lcd->printChar(c, dc_x, dc_y, color.GetValue());
	}
	///Draws a character. Input coordinates have to be defined in the window coordinate system
	void DrawChar(const char c,int x, int y, Color color)
	{
		x=ToDC_X(x);
		y=ToDC_Y(y);
		DrawSymbol(c, x, y, color);
	}
	///Draw caret. Input coordinates have to be defined in the window coordinate system
	void DrawCaret(int pos, int x, int y, Color color)
	{
		x = x+(pos*_lcd->getFontXsize());
		DrawChar('_', x, y+2, color);
	}
	///Draws string. Input coordinates have to be defined in the window coordinate system
	void DrawText(const char * text, int x, int y, Color color, HorizontalAlignment aligment = HorizontalAlignment::Left, int width = 0)
	{
		if (aligment == HorizontalAlignment::Center)
			x = x+(width - strlen(text)*_lcd->getFontXsize()) / 2;
		else if(aligment == HorizontalAlignment::Right)
			x = x + width - strlen(text)*_lcd->getFontXsize();
		x=ToDC_X(x);
		y=ToDC_Y(y);
		for(int i=0;i<strlen(text);i++)
		{
			char c=text[i];
			//if(c==' ')
			//	break;
			DrawSymbol(c, x + (i*(_lcd->getFontXsize())), y, color);
		}
	}
	///Draws sector. Input coordinates have to be defined in the window coordinate system
	void Sector(int x0, int y0, int radius,float start_angle_rad,float angle_rad, Color color)
	{
		x0=ToDC_X(x0);
		y0=ToDC_Y(y0);
		int x = 0;
		int y = radius;
		float cos_factor = cos(-start_angle_rad);
		float sin_factor = sin(-start_angle_rad);
		float tan_stop=tan(angle_rad/2);
		int delta = 2 - 2 * radius;
		int error = 0;
		float ratio;
		float rot_x,rot_y;
		while(y >= 0) {
			ratio=x/(float)y;
			if(ratio>tan_stop)
				break;
			rot_x = x*cos_factor - y*sin_factor;
			rot_y = x*sin_factor + y*cos_factor;
			_lcd->drawPixel(x0 + rot_x, y0 - rot_y,color.GetValue());
			rot_x =-x*cos_factor - y*sin_factor;
			rot_y = -x*sin_factor + y*cos_factor;
			_lcd->drawPixel(x0 + rot_x, y0 - rot_y,color.GetValue());
			//_lcd->drawPixel(x0 + x, y0 - y);
			//_lcd->drawPixel(x0 - x, y0 - y);
			error = 2 * (delta + y) - 1;
			if(delta < 0 && error <= 0)
			{
				++x;
				delta += 2 * x + 1;
				continue;
			}
			error = 2 * (delta - x) - 1;
			if(delta > 0 && error > 0)
			{
				--y;
				delta += 1 - 2 * y;
				continue;
			}
			++x;
			delta += 2 * (x - y);
			--y;
		}
	}
	void SetDeviceColor(Color color)
	{
		_lcd->setColor(color.GetR(),color.GetG(),color.GetB());
	}

	void SetColor(Color color)
	{
		SetDeviceColor(color);
	}

	void SetBackColor(Color color)
	{
		//_lcd->setBackColor(VGA_TRANSPARENT);
		SetDeviceColor(color);
	}

	void SetFont(const uint8_t *font)
	{
		_lcd->setFont(font);
	}

	void MoveTo(int x,int y)
	{
		_last_x=ToDC_X(x);
		_last_y=ToDC_Y(y);
	}

	void LineTo(int x,int y, Color color)
	{
		_lcd->drawLine(_last_x,_last_y,ToDC_X(x),ToDC_Y(y),color.GetValue());
		_last_y=ToDC_Y(y);
	}
	void Line(int x1,int y1,int x2,int y2,Color color)
	{
		MoveTo(x1,y1);
		LineTo(x2,y2,color.GetValue());
	}
};

#endif
