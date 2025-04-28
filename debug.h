#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <string.h>

// COLOUR definitions
#define COLOUR_RED     "\x1b[31m"
#define COLOUR_GREEN   "\x1b[32m"
#define COLOUR_YELLOW  "\x1b[33m"
#define COLOUR_BLUE    "\x1b[34m"
#define COLOUR_MAGENTA "\x1b[35m"
#define COLOUR_CYAN    "\x1b[36m"
#define COLOUR_RESET   "\x1b[0m"

// Define DEBUG level (0=off, 1=basic, 2=verbose, 3=very verbose)
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0  // Default to off
#endif

// Strip file path to just the filename
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// Debug print macros
#define LOG_ERROR(fmt, ...) fprintf(stderr, COLOUR_RED "[ERROR] %s %s(): %d: " fmt COLOUR_RESET "\n", __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)

#if DEBUG_LEVEL >= 1
#define LOG_INFO(fmt, ...)  fprintf(stderr, COLOUR_GREEN "[INFO]  %s %s(): %d: " fmt COLOUR_RESET "\n", __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_INFO(fmt, ...)
#endif

#if DEBUG_LEVEL >= 2
#define LOG_DEBUG(fmt, ...) fprintf(stderr, COLOUR_CYAN "[DEBUG] %s %s(): %d: " fmt COLOUR_RESET "\n", __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...)
#endif

#if DEBUG_LEVEL >= 3
#define LOG_TRACE(fmt, ...) fprintf(stderr, COLOUR_MAGENTA "[TRACE] %s %s(): %d: " fmt COLOUR_RESET "\n", __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_TRACE(fmt, ...)
#endif

#endif // DEBUG_H