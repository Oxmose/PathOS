/*******************************************************************************
 *
 * File: kernel_output.h
 *
 * Author: Alexy Torres Aurora Dugo
 *
 * Date: 15/12/2017
 *
 * Version: 1.0
 *
 * Simple output functions to print messages to screen. These are really basic
 * output too allow early kernel boot output and debug.
 ******************************************************************************/

#ifndef __KERNEL_OUTPUT_H_
#define __KERNEL_OUTPUT_H_

/* Print the desired string to the screen. Add a [SYS] tag at the 
 * beggining of the string before printing it.
 *
 * @param __format The format string to output.
 */
void kernel_printf(const char *__format, ...) __attribute__((format (printf, 1, 2)));

/* Print the desired string to the screen. Add a red [ERROR] tag at the 
 * beggining of the string before printing it.
 *
 * @param __format The format string to output.
 */
void kernel_error(const char *__format, ...) __attribute__((format (printf, 1, 2)));

/* Print the desired string to the screen. Add a green [OK] tag at the 
 * beggining of the string before printing it.
 *
 * @param __format The format string to output.
 */
void kernel_success(const char *__format, ...) __attribute__((format (printf, 1, 2)));

/* Print the desired string to the screen. Add a cyan [INFO] tag at the 
 * beggining of the string before printing it.
 *
 * @param __format The format string to output.
 */
void kernel_info(const char *__format, ...) __attribute__((format (printf, 1, 2)));

#endif /* __KERNEL_OUTPUT_H_ */
