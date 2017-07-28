/*
 * LEDZ - The LED Zeppelin
 * https://github.com/ricardocrudo/ledz
 *
 * Copyright (c) 2017 Ricardo Crudo <ricardo.crudo@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef LEDZ_H
#define LEDZ_H

#ifdef __cplusplus
extern "C"
{
#endif

/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include <stdint.h>

// adjust the header according your library
#include "gpio.h"


/*
****************************************************************************************************
*       MACROS
****************************************************************************************************
*/

#define LEDZ_VERSION     "0.0.0"


/*
****************************************************************************************************
*       CONFIGURATION
****************************************************************************************************
*/

// configure the function to set a GPIO
#define LEDZ_GPIO_SET(port,pin,value)   gpio_set(port,pin,value)

// maximum of LEDs to control (note: RGB count as 3 LEDs)
#define LEDZ_MAX_INSTANCES      3

// configure the logic value which the led turn on (must be 0 or 1)
#define LEDZ_TURN_ON_VALUE      1

// tick period in us
#define LEDZ_TICK_PERIOD        1000


/*
****************************************************************************************************
*       DATA TYPES
****************************************************************************************************
*/

typedef struct LEDZ_T ledz_t;

typedef enum ledz_type_t {LEDZ_1COLOR = 1, LEDZ_2COLOR, LEDZ_3COLOR} ledz_type_t;

typedef enum ledz_color_t {
    LEDZ_RED    = 0x01,
    LEDZ_GREEN  = 0x02,
    LEDZ_BLUE   = 0x04,
    LEDZ_YELLOW = 0x08,
    LEDZ_CYAN   = 0x10,
    LEDZ_WHITE  = 0x20,
    LEDZ_AMBER  = 0x40,
    LEDZ_ORANGE = 0x80,
} ledz_color_t;


/*
****************************************************************************************************
*       FUNCTION PROTOTYPES
****************************************************************************************************
*/

ledz_t* ledz_create(ledz_type_t type, const ledz_color_t *colors, const int *pins);
void ledz_destroy(ledz_t* led);
void ledz_on(ledz_t* led, ledz_color_t color);
void ledz_off(ledz_t* led, ledz_color_t color);
void ledz_toggle(ledz_t* led, ledz_color_t color);
void ledz_set(ledz_t* led, ledz_color_t color, int value);
void ledz_blink(ledz_t* led, ledz_color_t color, uint16_t time_on, uint16_t time_off);
void ledz_tick(void);


/*
****************************************************************************************************
*       CONFIGURATION ERRORS
****************************************************************************************************
*/

#ifndef LEDZ_GPIO_SET
#error "LEDZ_GPIO_SET macro must defined"
#endif

#if LEDZ_TURN_ON_VALUE < 0 || LEDZ_TURN_ON_VALUE > 1
#error "LEDZ_TURN_ON_VALUE must be set to 0 or 1"
#endif

#if LEDZ_TICK_PERIOD <= 0 || LEDZ_TICK_PERIOD > 1000
#error "LEDZ_TICK_PERIOD macro value must be set between 1 and 1000"
#endif

#ifdef __cplusplus
}
#endif

// LEDZ_H
#endif
