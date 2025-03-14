#include "log.hpp"
#include <ntddk.h>
#include <stdarg.h>
#include <stdio.h>

#define DRIVER_PREFIX "[frame] "
void LogGeneric(const char* level, const char* format, va_list args)
{
    char buffer[512];
    _vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
    KdPrint((DRIVER_PREFIX "%s: %s\n", level, buffer));
}

void LOG_INFO(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    LogGeneric("INFO", format, args);
    va_end(args);
}

void LOG_DEBUG(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    LogGeneric("DEBUG", format, args);
    va_end(args);
}

void LOG_ERROR(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    LogGeneric("ERROR", format, args);
    va_end(args);
}