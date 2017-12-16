/*******************************************************************************
 *
 * File: vga_text.c
 *
 * Author: Alexy Torres Aurora Dugo
 *
 * Date: 03/10/2017
 *
 * Version: 1.0
 *
 * VGA text mode driver.
 * Allows the kernel to display text and general ASCII characters to be 
 * displayed on the screen. 
 * Includes cursor management, screen colors management and other fancy
 * screen driver things.
 *
 ******************************************************************************/

/* Generic int types */
#include "../lib/stdint.h"

/* OS_RETURN_E, NULL */
#include "../lib/stddef.h"

/* memmove */
#include "../lib/string.h"

/* outb */
#include "../cpu/cpu.h"

/* Header file */
#include "vga_text.h"

/* Screen runtime parameters */
static colorscheme_t screen_scheme = BG_BLACK | FG_WHITE;
static cursor_t      screen_cursor;
static cursor_t      last_printed_cursor;

uint16_t *get_memory_addr(uint8_t line, uint8_t column)
{
    /* Avoid overflow on text mode */
    if(line > SCREEN_LINE_SIZE - 1 || column > SCREEN_COL_SIZE -1)
    {
        return (uint16_t *)(SCREEN_ADDR);
    }

    /* Returns the mem adress of the coordinates */
    return (uint16_t *)(SCREEN_ADDR + 2 * 
           (column + line * SCREEN_COL_SIZE));
}

OS_RETURN_E print_char(const uint8_t line, const uint8_t column, 
                       const char character)
{
    if(line > SCREEN_LINE_SIZE - 1 || column > SCREEN_COL_SIZE - 1)
    {
        return OS_ERR_OUT_OF_BOUND;
    }

    /* Get address to inject */
    uint16_t *screen_mem = get_memory_addr(line, column);

    /* Inject the character with the current colorscheme */
    *screen_mem = character | (screen_scheme << 8);

    return OS_NO_ERR;
}

void clear_screen(void)
{
    uint8_t i, j;

    /* Clear all screen cases */
    for(i = 0; i < SCREEN_LINE_SIZE; ++i)
    {
        for(j = 0; j < SCREEN_COL_SIZE; ++j)
        {
            print_char(i, j, ' ');
        }
    }
}

OS_RETURN_E put_cursor_at(uint8_t line, uint8_t column)
{   
    /* Set new cursor position */
    screen_cursor.x = column;
    screen_cursor.y = line;

    /* Display new position on screen */
    int16_t cursor_position = column + line * SCREEN_COL_SIZE;
    /* Send low part to the screen */
    outb(CURSOR_COMM_LOW, SCREEN_COMM_PORT);
    outb((int8_t)(cursor_position & 0x00FF), SCREEN_DATA_PORT);

    /* Send high part to the screen */
    outb(CURSOR_COMM_HIGH, SCREEN_COMM_PORT);
    outb((int8_t)((cursor_position & 0xFF00) >> 8), SCREEN_DATA_PORT); 

    return OS_NO_ERR;
}

OS_RETURN_E save_cursor(cursor_t *buffer)
{
    if(buffer == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    /* Save cursor attributes */
    buffer->x = screen_cursor.x;
    buffer->y = screen_cursor.y;

    return OS_NO_ERR;
}

OS_RETURN_E restore_cursor(const cursor_t buffer)
{
    if(buffer.x >= SCREEN_COL_SIZE || buffer.y >= SCREEN_LINE_SIZE)
    {
        return OS_ERR_OUT_OF_BOUND;
    }
    /* Restore cursor attributes */
    put_cursor_at(buffer.y, buffer.x);

    return OS_NO_ERR;
}

void process_char(const char character)
{
    /* If character is a normal ASCII character */
    if(character > 31 && character < 127)
    {
        /* Display character and move cursor */
        print_char(screen_cursor.y, screen_cursor.x++, 
                character);

        /* Manage end of line cursor position */
        if(screen_cursor.x > SCREEN_COL_SIZE - 1)
        {
            put_cursor_at(screen_cursor.y + 1, 0);
        }

        /* Manage end of screen cursor position */
        if(screen_cursor.y >= SCREEN_LINE_SIZE)
        {
            scroll(SCROLL_DOWN, 1);
            
        }
        else
        {
            /* Move cursor */
            put_cursor_at(screen_cursor.y, screen_cursor.x);
        }
    }
    else
    {
        /* Manage special ACSII characters*/
        switch(character)
        {
            /* Backspace */
            case '\b':
                if(last_printed_cursor.y == screen_cursor.y)
                {
                    if(screen_cursor.x > last_printed_cursor.x)
                    {
                        put_cursor_at(screen_cursor.y, screen_cursor.x - 1);
                        print_char(screen_cursor.y, screen_cursor.x, ' '); 
                    }
                }
                else if(last_printed_cursor.y < screen_cursor.y)
                {
                    if(screen_cursor.x > 0)
                    {
                        put_cursor_at(screen_cursor.y, screen_cursor.x - 1);
                        print_char(screen_cursor.y, screen_cursor.x, ' '); 
                    }
                    else
                    {       
                        put_cursor_at(screen_cursor.y - 1, 
                                      SCREEN_COL_SIZE - 1);
                        print_char(screen_cursor.y, screen_cursor.x, ' ');
                    }
                }
                break;
            /* Tab */
            case '\t':
                if(screen_cursor.x + 8 < SCREEN_COL_SIZE - 1)
                {
                    put_cursor_at(screen_cursor.y,
                            screen_cursor.x  +
                            (8 - screen_cursor.x % 8));
                }
                else
                {
                    put_cursor_at(screen_cursor.y,
                            SCREEN_COL_SIZE - 1);
                }

                break;
            /* Line feed */
            case '\n':
                if(screen_cursor.y < SCREEN_LINE_SIZE - 1)
                {
                    put_cursor_at(screen_cursor.y + 1, 0);
                }
                else
                {
                    scroll(SCROLL_DOWN, 1);
                }
                break;
            /* Clear screen */
            case '\f':
                clear_screen();
                break;
            /* Line return */
            case '\r':
                put_cursor_at(screen_cursor.y, 0);
                break;
            /* Undefined */
            default:
                break;
        }
    }
}

void scroll(const SCROLL_DIRECTION_E direction, const uint8_t lines_count)
{
    uint8_t to_scroll;

    if(SCREEN_LINE_SIZE < lines_count)
    {
        to_scroll = SCREEN_LINE_SIZE;
    }
    else
    {
        to_scroll = lines_count;
    }

    /* Select scroll direction */
    if(direction == SCROLL_DOWN)
    {
        /* For each line scroll we want */
        for(uint8_t j = 0; j < to_scroll; ++j)
        {
            /* Copy all the lines to the above one */
            uint8_t i;
            for(i = 0; i < SCREEN_LINE_SIZE - 1; ++i)
            {
                memmove(get_memory_addr(i, 0), get_memory_addr(i + 1, 0),  
                        sizeof(uint16_t) * SCREEN_COL_SIZE);
            }

            /* Clear last line */
            for(i = 0; i < SCREEN_COL_SIZE; ++i)
            {
                print_char(SCREEN_LINE_SIZE - 1, i, ' ');
            }
        }
    }

    /* Replace cursor */
    put_cursor_at(SCREEN_LINE_SIZE - to_scroll, 0);

    if(to_scroll <= last_printed_cursor.y)
    {
        last_printed_cursor.y -= to_scroll;
    }
    else
    {
        last_printed_cursor.x = 0;
        last_printed_cursor.y = 0;
    }
}

void console_putbytes(const char *string, const uint32_t size)
{
    /* Output each character of the string */
    uint32_t i;
    for(i = 0; i < size; ++i)
    {
        process_char(string[i]);
    }
    last_printed_cursor = screen_cursor;
}

void console_write_keyboard(const char *string, const uint32_t size)
{
    /* Output each character of the string */
    uint32_t i;
    for(i = 0; i < size; ++i)
    {
        process_char(string[i]);
    }
}

void set_color_scheme(const colorscheme_t color_scheme)
{
    screen_scheme = color_scheme;
}

OS_RETURN_E save_color_scheme(colorscheme_t *buffer)
{
    if(buffer == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    /* Save color scheme into buffer */
    *buffer = screen_scheme;

    return OS_NO_ERR;
}

void restore_color_scheme(const colorscheme_t buffer)
{
    /* Retore color scheme */
    screen_scheme = buffer;
}