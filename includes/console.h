/**
 * console.h - Used to send console data to the client
 */

#ifndef CONSOLE_H_
#define CONSOLE_H_

#include <stdbool.h>

/**
 * Initialize the console
 */
void consoleInit(void);

bool consoleTest(void);

/**
 * Put a character to the console buffer
 *
 * @param ch character that shall be printed
 * @return The character casted to unsigned int or EOF in case of error
 */
int consolePutchar(int ch);

/**
 * Put a character to the console buffer
 *
 * @param ch character that shall be printed
 * @return The character casted to unsigned int or EOF in case of error
 *
 * @note This version can be called by interrup. In such case the internal
 * buffer is going to be used. If a task currently is printing or if the
 * interrupts prints too much the data will be ignored.
 */
int consolePutcharFromISR(int ch);

/**
 * Put a null-terminated string on the console buffer
 *
 * @param str Null terminated string
 * @return a nonnegative number on success, or EOF on error.
 */
int consolePuts(const char *str);

/**
 * Flush the console buffer
 */
void consoleFlush(void);

/**
 * Macro implementing consolePrintf with eprintf
 *
 * @param FMT String format
 * @patam ... Parameters to print
 */
#define consolePrintf(FMT, ...) eprintf(consolePutchar, FMT, ## __VA_ARGS__)

#endif /*CONSOLE_H_*/
