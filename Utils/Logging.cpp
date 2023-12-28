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
	PlatformFormatLogMessageLf(LogFatal);
}

void _LogErrorfLF(const char* fmt, ...)
{
	PlatformFormatLogMessageLf(LogError);
}

void _LogWarningfLF(const char* fmt, ...)
{
	PlatformFormatLogMessageLf(LogWarning);
}

void _LogInfofLF(const char* fmt, ...)
{
	PlatformFormatLogMessageLf(LogInfo);
}

void _LogDebugfLF(const char* fmt, ...)
{
	PlatformFormatLogMessageLf(LogDebug);
}

bool _EnsureMsg(bool condition, const char * fmt, ...)
{
	if (condition == false)
	{		
		PlatformFormatLogMessageLf(LogError);
		__debugbreak();
	}
	return condition;
}

void _AssertMsg(bool condition, const char* fmt, ...)
{
	if (condition == false)
	{
		PlatformFormatLogMessageLf(LogFatal);
		__debugbreak();
		assert(0);
	}
}
