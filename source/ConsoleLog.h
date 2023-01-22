#pragma once
#include <windows.h>
#include <iostream>
#include <tchar.h>

namespace dae
{
	namespace Log
	{
		//constants

		//colors
		constexpr WORD COLOR_BLACK			= 0;
		constexpr WORD COLOR_DARK_BLUE		= 1;
		constexpr WORD COLOR_DARK_GREEN		= 2;
		constexpr WORD COLOR_LIGHT_BLUE		= 3;
		constexpr WORD COLOR_DARK_RED		= 4;
		constexpr WORD COLOR_MAGENTA		= 5;
		constexpr WORD COLOR_ORANGE			= 6;
		constexpr WORD COLOR_LIGHT_GRAY		= 7;
		constexpr WORD COLOR_GRAY			= 8;
		constexpr WORD COLOR_BLUE			= 9;
		constexpr WORD COLOR_GREEN			= 10;
		constexpr WORD COLOR_CYAN			= 11;
		constexpr WORD COLOR_RED			= 12;
		constexpr WORD COLOR_PINK			= 13;
		constexpr WORD COLOR_YELLOW			= 14;
		constexpr WORD COLOR_WHITE			= 15;

		//helper defines
#define MSG_COLOR_MAIN					COLOR_CYAN
#define MSG_COLOR_HARDWARERASTERIZER	COLOR_ORANGE
#define MSG_COLOR_SOFTWARERASTERIZER	COLOR_PINK
#define MSG_COLOR_RENDERER				COLOR_LIGHT_BLUE
#define MSG_COLOR_SCENE					COLOR_MAGENTA
#define MSG_COLOR_WARNING				COLOR_RED
#define MSG_COLOR_SUCCESS				COLOR_GREEN

#ifdef _UNICODE
	#define TSTRING std::wstring
	#define TCOUT std::wcout

#else
	#define TSTRING std::string
	#define TCOUT std::cout

#endif

#define MSG_LOGGER_DEFAULT				_T("")
#define MSG_LOGGER_HARDWARERASTERIZER	_T("[HARDWARE] ")
#define MSG_LOGGER_SOFTWARERASTERIZER	_T("[SOFTWARE] ")
#define MSG_LOGGER_SHARED				_T("[SHARED] ")

		//functions
		static TSTRING BoolToString(bool value)
		{
			return (value) ? _T("True") : _T("False");
		}

		static inline void PrintMessage(const TSTRING& message, const TSTRING& logger = MSG_LOGGER_DEFAULT, WORD color = COLOR_WHITE, WORD textColor = COLOR_WHITE)
		{
			HANDLE hConsole{ GetStdHandle(STD_OUTPUT_HANDLE) };

			SetConsoleTextAttribute(hConsole, color);
			TCOUT << _T("======================================\n");

			TCOUT << logger << _T("	");

			SetConsoleTextAttribute(hConsole, textColor);
			TCOUT << message << _T('\n');

			SetConsoleTextAttribute(hConsole, color);
			TCOUT << _T("======================================\n");

			//set back to default
			SetConsoleTextAttribute(hConsole, COLOR_WHITE);
		}

		static inline void PrintTstring(const TSTRING& message, const TSTRING& logger = MSG_LOGGER_DEFAULT, WORD color = COLOR_WHITE, WORD textColor = COLOR_GRAY)
		{
			HANDLE hConsole{ GetStdHandle(STD_OUTPUT_HANDLE) };

			SetConsoleTextAttribute(hConsole, color);

			TCOUT << logger << _T("	");

			SetConsoleTextAttribute(hConsole, textColor);
			TCOUT << message << _T('\n');

			//set back to default
			SetConsoleTextAttribute(hConsole, COLOR_WHITE);
		}
	}
}