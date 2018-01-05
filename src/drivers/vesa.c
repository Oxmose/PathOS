/*******************************************************************************
 *
 * File: vesa.c
 *
 * Author: Alexy Torres Aurora Dugo
 *
 * Date: 03/01/2018
 *
 * Version: 1.0
 *
 * VESA VBE 2 graphic drivers
 ******************************************************************************/

#include "../lib/stdint.h"     /* Generic int types */
#include "../lib/stddef.h"     /* OS_RETURN_E */
#include "../lib/malloc.h"     /* malloc */
#include "../cpu/bios_call.h"  /* regs_t, bios_call */
#include "../font/uni_vga.c"   /* __fonct_bitmap__ */
#include "serial.h"            /* serial_write */
#include "vga_text.h"          /* vga_get_framebuffer */
#include "graphic.h"            /* structures */

/* Header file */
#include "vesa.h"

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/* VESA data structures */
extern vbe_info_structure_t      vbe_info_base;
extern vbe_mode_info_structure_t vbe_mode_info_base;

/* VESA modes */
static vesa_mode_t* saved_modes;
static uint16_t     mode_count     = 0;
static uint8_t      vesa_supported = 0;
static vesa_mode_t* current_mode;

/* VESA console settigns, used in tty mode */
static cursor_t      screen_cursor;
static cursor_t      last_printed_cursor;
static colorscheme_t screen_scheme;

static const uint32_t vga_color_table[16] = {
    0x00000000,
    0x000000AA,
    0x0000aa00,
    0x0000aaaa,
    0x00aa0000,
    0x00aa00aa,
    0x00aa5500,
    0x00aaaaaa,
    0x00555555,
    0x005555ff,
    0x0055ff55,
    0x0055ffff,
    0x00ff5555,
    0x00ff55ff,
    0x00ffff55,
    0x00ffffff
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/* Process the character in parameters.
 *
 * @param character The character to process.
 */
static void vesa_process_char(const char character)
{
    uint32_t i;
    uint32_t j;

	int32_t diff;
	int8_t tab_width = 4;

    /* Write on serial */
    serial_write(COM1, character);

    /* If character is a normal ASCII character */
    if(character > 31 && character < 127)
    {
        /* Manage end of line cursor position */
        if(screen_cursor.x + font_width >= current_mode->width)
        {
            /* remove cursor */
            for(i = screen_cursor.x; i < current_mode->width; ++i)
            {
                for(j = screen_cursor.y; j < screen_cursor.y + font_height; ++j)
                {
                    vesa_draw_pixel(i, j,
                                   (screen_scheme.background & 0x00ff0000) >> 16,
                                   (screen_scheme.background & 0x0000ff00) >> 8,
                                   (screen_scheme.background & 0x000000ff));
                }
            }
            vesa_put_cursor_at(screen_cursor.y + font_height, 0);
        }

        /* Manage end of screen cursor position */
        if(screen_cursor.y >= current_mode->height)
        {
            vesa_scroll(SCROLL_DOWN, 1);
        }

        /* Display character */
        vesa_drawchar(character, screen_cursor.x, screen_cursor.y,
        	          screen_scheme.foreground, screen_scheme.background);

        /* Update cursor position */
        vesa_put_cursor_at(screen_cursor.y, screen_cursor.x + font_width);

        /* Manage end of line cursor position */
        if(screen_cursor.x + font_width >= current_mode->width)
        {
            /* remove cursor */
            for(i = screen_cursor.x; i < current_mode->width; ++i)
            {
                for(j = screen_cursor.y; j < screen_cursor.y + font_height; ++j)
                {
                    vesa_draw_pixel(i, j,
                                   (screen_scheme.background & 0x00ff0000) >> 16,
                                   (screen_scheme.background & 0x0000ff00) >> 8,
                                   (screen_scheme.background & 0x000000ff));
                }
            }
            vesa_put_cursor_at(screen_cursor.y + font_height, 0);
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
                        vesa_drawchar(' ', screen_cursor.x, screen_cursor.y,
                                      screen_scheme.foreground,
                                      screen_scheme.background);
                        vesa_drawchar(' ', screen_cursor.x - font_width, screen_cursor.y,
                                      screen_scheme.foreground,
                                      screen_scheme.background);
                        vesa_put_cursor_at(screen_cursor.y,
                                           screen_cursor.x - font_width);
                    }
                }
                else if(last_printed_cursor.y < screen_cursor.y)
                {
                    if(screen_cursor.x > 0)
                    {
                        vesa_drawchar(' ', screen_cursor.x, screen_cursor.y,
                                      screen_scheme.foreground,
                                      screen_scheme.background);
                        vesa_drawchar(' ', screen_cursor.x - font_width, screen_cursor.y,
                                      screen_scheme.foreground,
                                      screen_scheme.background);
                        vesa_put_cursor_at(screen_cursor.y,
                        	               screen_cursor.x - font_width);

                    }
                    else
                    {
                        int q = current_mode->width / font_height;
                        int m = current_mode->width % (font_height * q);
                        vesa_drawchar(' ', screen_cursor.x, screen_cursor.y,
                                      screen_scheme.foreground,
                                      screen_scheme.background);
                        vesa_drawchar(' ', current_mode->width - m - font_width, screen_cursor.y - font_height,
                                      screen_scheme.foreground,
                                      screen_scheme.background);
                        vesa_put_cursor_at(screen_cursor.y - font_height,
                                           current_mode->width - m - font_width);
                    }
                }
                break;
            /* Tab */
            case '\t':
            	diff = current_mode->width -
            	       (screen_cursor.x + tab_width * font_width);

            	if(diff < 0)
            	{
            		tab_width = tab_width + (diff / font_width);
            	}
        		while(tab_width-- > 0)
        		{
            		vesa_process_char(' ');
            	}

                break;
            /* Line feed */
            case '\n':
                if(screen_cursor.y < current_mode->height - font_height)
                {
                    /* remove cursor */
                    for(i = screen_cursor.x; i < current_mode->width &&  i < screen_cursor.x + font_width; ++i)
                    {
                        for(j = screen_cursor.y; j < screen_cursor.y + font_height; ++j)
                        {
                            vesa_draw_pixel(i, j,
                                           (screen_scheme.background & 0x00ff0000) >> 16,
                                           (screen_scheme.background & 0x0000ff00) >> 8,
                                           (screen_scheme.background & 0x000000ff));
                        }
                    }
                    vesa_put_cursor_at(screen_cursor.y + font_height, 0);
                }
                else
                {
                    vesa_scroll(SCROLL_DOWN, 1);
                }
                break;
            /* Clear screen */
            case '\f':
                vesa_clear_screen();
                break;
            /* Line return */
            case '\r':
                vesa_put_cursor_at(screen_cursor.y, 0);
                break;
            /* Undefined */
            default:
                break;
        }
    }
}

OS_RETURN_E init_vesa(void)
{
	bios_int_regs_t regs;
	vesa_mode_t*    new_mode;
	uint32_t        i;
	uint16_t*       modes;

	/* Init data */
	saved_modes    = NULL;
	mode_count     = 0;
	vesa_supported = 0;
	current_mode   = NULL;

	/* Console mode init */
	screen_cursor.x = 0;
	screen_cursor.y = 0;
	screen_scheme.foreground = 0x00FFFFFF;
	screen_scheme.background = 0x00000000;
	screen_scheme.vga_color  = 0;

	/* Init structure */
	strncpy(vbe_info_base.signature, "VBE2", 4);

 	/* Init the registers for the bios call */
 	regs.ax = BIOS_CALL_GET_VESA_INFO;
 	regs.es = 0;
 	regs.di = (uint16_t)(uint32_t)(&vbe_info_base);

 	/* Issue call */
 	bios_int(BIOS_INTERRUPT_VESA, &regs);

 	/* Check call result */
 	if(regs.ax != 0x004F || strncmp(vbe_info_base.signature, "VESA", 4) != 0)
 	{
 		return OS_ERR_VESA_NOT_SUPPORTED;
 	}

 	/* Get all the supported modes */
  	modes = (uint16_t*)vbe_info_base.video_modes;
  	for (i = 0 ; modes[i] != 0xFFFF ; ++i)
  	{
  		/* Prepare registers for mode query call */
  	 	regs.ax = BIOS_CALL_GET_VESA_MODE;
  	  	regs.cx = modes[i];
  	  	regs.es = 0;
  	  	regs.di = (uint16_t)(uint32_t)(&vbe_mode_info_base);

      	bios_int(BIOS_INTERRUPT_VESA, &regs);

      	/* Check call result */
	 	if(regs.ax != 0x004F)
	 	{
	 		continue;
	 	}

      	/* The driver only support linear frame buffer management */
      	if ((vbe_mode_info_base.attributes & VESA_FLAG_LINEAR_FB) !=
      		VESA_FLAG_LINEAR_FB)
  		{
  			continue;
  		}

      	/* Check if this is a packed pixel or direct color mode */
      	if (vbe_mode_info_base.memory_model != 4 &&
      		vbe_mode_info_base.memory_model != 6 )
  		{
  			continue;
  		}

  		/* The mode is compatible, save it */
		new_mode = malloc(sizeof(vesa_mode_t));
		if(new_mode == NULL)
		{
			mode_count = 0;
			return OS_ERR_MALLOC;
		}

		new_mode->width       = vbe_mode_info_base.width;
		new_mode->height      = vbe_mode_info_base.height;
		new_mode->bpp         = vbe_mode_info_base.bpp;
		new_mode->mode_id     = modes[i];
		new_mode->framebuffer = vbe_mode_info_base.framebuffer;
		new_mode->next = NULL;

  		/* Store the mode */
  		if(saved_modes == NULL)
  		{
  			saved_modes = new_mode;
  		}
  		else
  		{
  			new_mode->next = saved_modes;
  			saved_modes = new_mode;
	  	}
	  	++mode_count;
    }
    vesa_supported = 1;
    return OS_NO_ERR;
}

#include "../core/kernel_output.h"
OS_RETURN_E text_vga_to_vesa(void)
{
	OS_RETURN_E err;
	uint32_t i;
	uint32_t j;
	uint16_t vesa_mode_count;
	vesa_mode_info_t* modes;
	uint16_t* vga_fb;
	cursor_t vga_cursor;
	uint16_t temp_buffer[SCREEN_LINE_SIZE * SCREEN_COL_SIZE];
	colorscheme_t new_colorscheme;
	colorscheme_t old_colorscheme = screen_scheme;

	/* Save VGA content */
	vga_save_cursor(&vga_cursor);
	vga_fb = vga_get_framebuffer(0, 0);
	memcpy(temp_buffer, vga_fb,
		   sizeof(uint16_t) * SCREEN_LINE_SIZE * SCREEN_COL_SIZE);

	/* Set VESA mode */
	vesa_mode_count = get_vesa_mode_count();

	if(vesa_supported == 0)
	{
		return OS_ERR_VESA_NOT_SUPPORTED;
	}

	if(mode_count == 0)
	{
		return OS_NO_ERR;
	}

	modes = malloc(sizeof(vesa_mode_info_t) * vesa_mode_count);

	err = get_vesa_modes(modes, vesa_mode_count);
	if(err != OS_NO_ERR)
	{
		return err;
	}

	for(uint32_t i = 0; i < vesa_mode_count; ++i)
	{
		if(modes[i].width == 1280 &&
	      	 modes[i].height == 800 &&
	      	 modes[i].bpp == 32)
	    {
	      	err = set_vesa_mode(modes[i]);
	      	if(err != OS_NO_ERR)
			{
				return err;
		 	}
		}
	}

	vesa_clear_screen();

	vga_fb = temp_buffer;

	/* Browse the framebuffer to get the cahracters and the colorscheme */
	for(i = 0; i < SCREEN_LINE_SIZE; ++i)
	{
		for(j = 0; j < SCREEN_COL_SIZE; ++j)
		{
			if(vga_cursor.y < i)
			{
				break;
			}
			else if(vga_cursor.y == i && vga_cursor.x == j)
			{
				break;
			}

			/* Get data character */
			char character = (*vga_fb) & 0x00FF;
			new_colorscheme.foreground =
				vga_color_table[((*vga_fb) & 0x0F00) >> 8];
			new_colorscheme.background =
				vga_color_table[((*vga_fb) & 0xF000) >> 12];

			/* Print char */
			vesa_set_color_scheme(new_colorscheme);
			vesa_process_char(character);

			++vga_fb;
		}
		if(vga_cursor.y == i)
		{
			break;
		}
		vesa_process_char('\n');
	}

	/* Restore previous screen scheme */
	screen_scheme = old_colorscheme;
	return OS_NO_ERR;
}

uint16_t get_vesa_mode_count(void)
{
	return mode_count;
}

OS_RETURN_E get_vesa_modes(vesa_mode_info_t* buffer, const uint32_t size)
{
	uint32_t i;
	vesa_mode_t* cursor;

	if(vesa_supported == 0)
	{
		return OS_ERR_VESA_NOT_SUPPORTED;
	}

	if(mode_count == 0)
	{
		return OS_NO_ERR;
	}

	if(buffer == NULL)
	{
		return OS_ERR_NULL_POINTER;
	}

	cursor = saved_modes;
	for(i = 0; cursor != NULL && i < mode_count && i < size; ++i)
	{
		buffer[i].width       = cursor->width;
	    buffer[i].height      = cursor->height;
	    buffer[i].bpp         = cursor->bpp;
	    buffer[i].mode_id     = cursor->mode_id;
	    cursor = cursor->next;
	}

	return OS_NO_ERR;
}

OS_RETURN_E set_vesa_mode(const vesa_mode_info_t mode)
{
	vesa_mode_t* cursor;
	bios_int_regs_t regs;

	if(vesa_supported == 0)
	{
		return OS_ERR_VESA_NOT_SUPPORTED;
	}

	/* Search for the mode in the saved modes */
	cursor = saved_modes;
	while(cursor != NULL)
	{
		if(cursor->mode_id == mode.mode_id &&
		   cursor->width == mode.width     &&
		   cursor->height == mode.height   &&
		   cursor->bpp == mode.bpp)
		{
			break;
		}
		cursor = cursor->next;
	}

	/* The mode was not found */
	if(cursor == NULL)
	{
		return OS_ERR_VESA_MODE_NOT_SUPPORTED;
	}

	/* Set the VESA mode */
	regs.ax = BIOS_CALL_SET_VESA_MODE;
	regs.bx = cursor->mode_id | VESA_FLAG_LFB_ENABLE;
	bios_int(BIOS_INTERRUPT_VESA, &regs);

	/* Check call result */
 	if(regs.ax != 0x004F)
 	{
 		return OS_ERR_VESA_MODE_NOT_SUPPORTED;
 	}

 	current_mode = cursor;

 	/* Tell generic driver we loaded a VESA mode */
 	set_selected_driver(VESA_DRIVER);

 	return OS_NO_ERR;
}

OS_RETURN_E vesa_draw_pixel(const uint16_t x, const uint16_t y,
                            const uint8_t red, const uint8_t green,
                            const uint8_t blue)
{
	if(vesa_supported == 0)
	{
		return OS_ERR_VESA_NOT_SUPPORTED;
	}

	if(current_mode == NULL)
	{
		return OS_ERR_VESA_NOT_INIT;
	}

	if(x > current_mode->width || y > current_mode->height)
	{
		return OS_ERR_OUT_OF_BOUND;
	}

	/* Get framebuffer address */
	uint32_t* addr = ((uint32_t*)current_mode->framebuffer) +
	                 (current_mode->width * y) + x;

	uint8_t pixel[4] = {blue, green, red, 0x00};

	*addr = *((uint32_t*)pixel);

	return OS_NO_ERR;
}

void vesa_drawchar(const unsigned char charracter,
                   const uint32_t x, const uint32_t y,
                   const uint32_t fgcolor, const uint32_t bgcolor)
{
	uint32_t cx;
	uint32_t cy;

	uint32_t mask[8] = {1, 2, 4, 8, 16, 32, 64, 128};

	unsigned char *glyph = __font_bitmap__ + (charracter - 31) * 16;

	uint8_t pixel[4] = {0};

	for(cy = 0; cy < 16; ++cy)
	{
		for(cx = 0; cx < 8; ++cx)
		{
			*((uint32_t*)pixel) = glyph[cy] & mask[cx] ? fgcolor : bgcolor;

			vesa_draw_pixel(x + (7 - cx ), y + cy,
				            pixel[2], pixel[1], pixel[0]);
		}
	}
}

void vesa_clear_screen(void)
{
	memset((void*)current_mode->framebuffer, 0,
		   current_mode->width *
		   current_mode->height *
		   (current_mode->bpp / 8));
}

OS_RETURN_E vesa_put_cursor_at(const uint32_t line, const uint32_t column)
{
    uint8_t i;

	screen_cursor.x = column;
	screen_cursor.y = line;

    if(column + 2 < current_mode->width)
    {
        for(i = 0; i < 16; ++i)
        {
            vesa_draw_pixel(column, line + i, 0xFF, 0xFF, 0xFF);
            vesa_draw_pixel(column + 1, line + i, 0xFF, 0xFF, 0xFF);
        }
    }

	return OS_NO_ERR;
}

OS_RETURN_E vesa_save_cursor(cursor_t* buffer)
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

OS_RETURN_E vesa_restore_cursor(const cursor_t buffer)
{
	if(buffer.x >= current_mode->width || buffer.y >= current_mode->height)
    {
        return OS_ERR_OUT_OF_BOUND;
    }

    /* Restore cursor attributes */
    vesa_put_cursor_at(buffer.y, buffer.x);

    return OS_NO_ERR;
}

/* Scroll in the desired direction of lines_count lines.
 *
 * @param direction The direction to whoch the console should be scrolled.
 * @param lines_count The number of lines to scroll.
 */
void vesa_scroll(const SCROLL_DIRECTION_E direction,
                 const uint32_t lines_count)
{
#if 0
	uint8_t to_scroll;
    uint8_t i;
    uint8_t j;

    if(current_mode < lines_count)
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
        for(j = 0; j < to_scroll; ++j)
        {
            /* Copy all the lines to the above one */
            for(i = 0; i < SCREEN_LINE_SIZE - 1; ++i)
            {
                memmove(get_framebuffer(i, 0), get_framebuffer(i + 1, 0),
                        sizeof(uint16_t) * SCREEN_COL_SIZE);
                last_columns[i] = last_columns[i+1];
            }
            last_columns[SCREEN_LINE_SIZE - 1] = 0;
        }
        /* Clear last line */
        for(i = 0; i < SCREEN_COL_SIZE; ++i)
        {
            print_char(SCREEN_LINE_SIZE - 1, i, ' ');
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
#endif

	(void)direction;
	(void)lines_count;
}

void vesa_set_color_scheme(const colorscheme_t color_scheme)
{
	screen_scheme.foreground = color_scheme.foreground;
    screen_scheme.background = color_scheme.background;
}

OS_RETURN_E vesa_save_color_scheme(colorscheme_t* buffer)
{
	if(buffer == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    /* Save color scheme into buffer */
    buffer->foreground = screen_scheme.foreground;
    buffer->background = screen_scheme.background;

    return OS_NO_ERR;
}

void vesa_console_putbytes(const char* string, const uint32_t size)
{
    /* Output each character of the string */
    uint32_t i;
    for(i = 0; i < size; ++i)
    {
        vesa_process_char(string[i]);
    }
    last_printed_cursor = screen_cursor;
}

void vesa_console_write_keyboard(const char* string, const uint32_t size)
{
    /* Output each character of the string */
    uint32_t i;
    for(i = 0; i < size; ++i)
    {
        vesa_process_char(string[i]);
    }
}