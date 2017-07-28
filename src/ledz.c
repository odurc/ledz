/*
 * LEDZ - The LED Zeppelin
 * https://github.com/ricardocrudo/ledz
 *
 * Copyright (c) 2016 Ricardo Crudo <ricardo.crudo@gmail.com>
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

/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include "ledz.h"


/*
****************************************************************************************************
*       INTERNAL MACROS
****************************************************************************************************
*/

#define LEDZ_TURN_OFF_VALUE     (1 - LEDZ_TURN_ON_VALUE)

// calculate (rounded) how many ticks are need to reach a period of 1ms
#define TICKS_TO_1ms            ((10000 / LEDZ_TICK_PERIOD + 5) / 10)

// macro to set led GPIO
#if LEDZ_TURN_ON_VALUE == 1
#define LED_SET(led, val)       LEDZ_GPIO_SET(led->pins[0], led->pins[1], (val));
#else
#define LED_SET(led, val)       LEDZ_GPIO_SET(led->pins[0], led->pins[1], 1-(val));
#endif


/*
****************************************************************************************************
*       INTERNAL CONSTANTS
****************************************************************************************************
*/


/*
****************************************************************************************************
*       INTERNAL DATA TYPES
****************************************************************************************************
*/

struct LEDZ_T {
    ledz_color_t color;
    const int *pins;

    struct {
        unsigned int state : 1;
        unsigned int blink : 1;
    };

    uint16_t time_on, time_off, counter;
    ledz_t *next;
};


/*
****************************************************************************************************
*       INTERNAL GLOBAL VARIABLES
****************************************************************************************************
*/

static ledz_t g_leds[LEDZ_MAX_INSTANCES];
static unsigned int g_leds_available = LEDZ_MAX_INSTANCES;


/*
****************************************************************************************************
*       INTERNAL FUNCTIONS
****************************************************************************************************
*/

static inline ledz_t* ledz_take(void)
{
    static unsigned int ledz_counter;

    // first request round
    if (ledz_counter < LEDZ_MAX_INSTANCES)
    {
        g_leds_available--;
        ledz_t *led = &g_leds[ledz_counter++];
        return led;
    }

    // iterate all array searching for a free spot
    // a led is considered free when pins is null
    for (int i = 0; i < LEDZ_MAX_INSTANCES; i++)
    {
        ledz_t *led  = &g_leds[i];

        if (led->pins == 0)
        {
            g_leds_available--;
            return led;
        }
    }

    return 0;
}

static inline void ledz_give(ledz_t *led)
{
    if (led)
    {
        led->pins = 0;
        g_leds_available++;
    }
}


/*
****************************************************************************************************
*       GLOBAL FUNCTIONS
****************************************************************************************************
*/

ledz_t* ledz_create(ledz_type_t type, const ledz_color_t *colors, const int *pins)
{
    if (g_leds_available < type)
        return 0;

    ledz_t *next = 0;

    for (int i = type - 1; i >= 0; i--)
    {
        ledz_t *led = ledz_take();
        led->color = colors[i];
        led->pins = &pins[i * 2];
        led->state = 0;
        led->blink = 0;
        led->next = next;
        next = led;
    }

    return next;
}

void ledz_destroy(ledz_t* led)
{
    if (led->next)
        ledz_destroy(led->next);

    ledz_give(led);
}

void ledz_on(ledz_t* led, ledz_color_t color)
{
    ledz_set(led, color, 1);
}

void ledz_off(ledz_t* led, ledz_color_t color)
{
    ledz_set(led, color, 0);
}

void ledz_toggle(ledz_t* led, ledz_color_t color)
{
    ledz_set(led, color, -1);
}

void ledz_set(ledz_t* led, ledz_color_t color, int value)
{
    // stop blinking
    led->blink = 0;

    // adjust value
    if (value >= 1)
        value = 1;

    for (int i = 0; led; led = led->next, i++)
    {
        if (led->color & color)
        {
            // skip update if value match current state
            if (led->state == value)
                continue;

            // toggle led if value is negative
            if (value < 0)
                value = 1 - led->state;

            // update led state
            led->state = value;

            // update led GPIO
            LED_SET(led, value);
        }
    }
}

void ledz_blink(ledz_t* led, ledz_color_t color, uint16_t time_on, uint16_t time_off)
{
    if (time_on == 0 || time_off == 0)
    {
        led->blink = 0;
        return;
    }

    for (int i = 0; led; led = led->next, i++)
    {
        if (led->color & color)
        {
            led->time_on = time_on;
            led->time_off = time_off;

            // load counter according current state
            if (led->state)
                led->counter = time_on;
            else
                led->counter = time_off;

            // start blinking
            led->blink = 1;
        }
    }
}

void ledz_tick(void)
{
    static uint16_t counter;

    if (++counter < TICKS_TO_1ms)
        return;

    // reset counter
    counter = 0;

    for (int i = 0; i < LEDZ_MAX_INSTANCES; i++)
    {
        ledz_t *led = &g_leds[i];

        if (led->pins == 0 || led->blink == 0)
            continue;

        if (led->counter > 0)
            led->counter--;

        if (led->counter == 0)
        {
            if (led->state)
            {
                // turn off led
                LED_SET(led, 0);
                led->state = 0;

                // load counter with time off value
                led->counter = led->time_off;
            }
            else
            {
                // turn on led
                LED_SET(led, 1);
                led->state = 1;

                // load counter with time on value
                led->counter = led->time_on;
            }
        }
    }
}
