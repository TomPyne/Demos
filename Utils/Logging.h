#pragma once

void _LogFatalfLF(const char* fmt, ...);
void _LogErrorfLF(const char* fmt, ...);
void _LogWarningfLF(const char* fmt, ...);
void _LogInfofLF(const char* fmt, ...);
void _LogDebugfLF(const char* fmt, ...);
bool _EnsureMsg(bool condition, const char* fmt, ...);
void _AssertMsg(bool condition, const char* fmt, ...);

//#define FAST_ENSURES

#ifndef FAST_ENSURES
#define ENSUREMSG(x, ...) _EnsureMsg(x, __VA_ARGS__)
#else
#define ENSUREMSG(x, ...) !!(x)
#endif

#define ASSERTMSG(x, ...) _AssertMsg(x, __VA_ARGS__)

#define LOGERROR(...) 		_LogErrorfLF(__VA_ARGS__)
#define LOGWARNING(...) 	_LogWarningfLF(__VA_ARGS__)
#define LOGINFO(...) 		_LogInfofLF(__VA_ARGS__)
#define LOGDEBUG(...) 		_LogDebugfLF(__VA_ARGS__)

