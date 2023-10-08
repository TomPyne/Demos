#include "Logging.h"

#include <cassert>
#include <stdio.h>
#include <stdarg.h>
#include <Windows.h>

typedef void (*LogFunc) (const char* str);

#define PlatformFormatLogMessage(fn) \
	constexpr size_t BufferSize = 16 * 1024; \
	char buf[BufferSize]; \
	va_list ap; \
	va_start(ap, fmt); \
	vsprintf_s(buf, fmt, ap); \
	va_end(ap); \
	fn(buf); \

#define PlatformFormatLogMessageLf(fn) \
	constexpr size_t BufferSize = 16 * 1024; \
	char buf[BufferSize]; \
	va_list ap; \
	va_start(ap, fmt); \
	vsprintf_s(buf, fmt, ap); \
	va_end(ap); \
	strcat_s(buf, "\n"); \
	fn(buf); \

void LogFatal(const char* str)
{
	OutputDebugStringA(str);
}

void LogError(const char* str)
{
	OutputDebugStringA(str);
}

void LogWarning(const char* str)
{
	OutputDebugStringA(str);
}

void LogInfo(const char* str)
{
	OutputDebugStringA(str);
}

void LogDebug(const char* str)
{
	OutputDebugStringA(str);
}

void _LogFatalfLF(const char* fmt, ...)
{
	PlatformFormatLogMessageLf(LogFatal, 0);
}

void _LogErrorfLF(const char* fmt, ...)
{
	PlatformFormatLogMessageLf(LogError, 0);
}

void _LogWarningfLF(const char* fmt, ...)
{
	PlatformFormatLogMessageLf(LogWarning, 0);
}

void _LogInfofLF(const char* fmt, ...)
{
	PlatformFormatLogMessageLf(LogInfo, 0);
}

void _LogDebugfLF(const char* fmt, ...)
{
	PlatformFormatLogMessageLf(LogDebug, 0);
}

bool _EnsureMsg(bool condition, const char * fmt, ...)
{
	if (condition == false)
	{		
		PlatformFormatLogMessageLf(LogError, 0);
		__debugbreak();
	}
	return condition;
}

void _AssertMsg(bool condition, const char* fmt, ...)
{
	if (condition == false)
	{
		PlatformFormatLogMessageLf(LogFatal, 0);
		__debugbreak();
		assert(0);
	}
}
