/*******************************************************************************
 *
 * File: pointer.h
 *
 * Author: Alexy Torres Aurora Dugo
 *
 * Date: 20/01/2018
 *
 * Version: 1.0
 *
 * GUI pointer (mouse) manager.
 ******************************************************************************/

#ifndef __POINTER_H__
#define __POINTER_H__

#include "../lib/stdint.h" /* Generic int types */
#include "../lib/stddef.h" /* OS_RETURN_E */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/*******************************************************************************
* STRUCTURES
******************************************************************************/

/*******************************************************************************
* FUNCTIONS
******************************************************************************/

OS_RETURN_E init_pointer(void);
void update_mouse(const uint32_t x, const uint32_t y);
void draw_mouse(void);

#endif /* __POINTER_H__ */
