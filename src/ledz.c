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

// calculate (rounded) how many ticks are need to reach a period of 1ms
#define TICKS_TO_1ms        ((10000 / LEDZ_TICK_PERIOD + 5) / 10)

// invert led value if user has defined LEDZ_TURN_ON_VALUE as zero
#define LED_VALUE(val)      (!(LEDZ_TURN_ON_VALUE ^ (val)))

// macro to set led GPIO
#define LED_SET(led, val)   LEDZ_GPIO_SET(led->pins[0], led->pins[1], LED_VALUE(val)); \
                            led->state = (val);

// macro to set PWM
#ifdef LEDZ_GPIO_PWM
#define LED_PWM(led,duty)   LEDZ_GPIO_PWM(led->pins[0], led->pins[1], duty)
#else
#define LED_PWM(led,duty)
#endif


/*
****************************************************************************************************
*       INTERNAL CONSTANTS
****************************************************************************************************
*/

// table from: http://jared.geek.nz/2013/feb/linear-led-pwm
const unsigned char cie1931[101] = {
      0,   0,   0,   0,   0,   1,   1,   1,   1,   1,
      1,   1,   1,   2,   2,   2,   2,   2,   3,   3,
      3,   3,   4,   4,   4,   4,   5,   5,   5,   6,
      6,   7,   7,   8,   8,   8,   9,  10,  10,  11,
     11,  12,  12,  13,  14,  15,  15,  16,  17,  18,
     18,  19,  20,  21,  22,  23,  24,  25,  26,  27,
     28,  29,  30,  32,  33,  34,  35,  37,  38,  39,
     41,  42,  44,  45,  47,  48,  50,  52,  53,  55,
     57,  58,  60,  62,  64,  66,  68,  70,  72,  74,
     76,  78,  81,  83,  85,  88,  90,  92,  95,  97,
    100,
};


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
        unsigned int blink_state : 1;
        unsigned int brightness : 1;
    };

    uint16_t time_on, time_off, time;
    unsigned int pwm, brightness_value;

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
    // disable blinking and brightness control
    led->blink = 0;
    led->brightness = 0;

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
            {
                led->blink_state = 1;
                led->time = time_on;
            }
            else
            {
                led->blink_state = 0;
                led->time = time_off;
            }

            // start blinking
            led->blink = 1;
        }
    }
}

void ledz_brightness(ledz_t* led, ledz_color_t color, unsigned int value)
{
    if (value >= 100)
        value = 100;

    for (int i = 0; led; led = led->next, i++)
    {
        if (led->color & color)
        {
            // convert brightness value to duty cycle according cie 1931
            int duty_cycle = cie1931[value];

            // enable hardware PWM
            if (duty_cycle > 0 && duty_cycle < 100)
                LED_PWM(led, duty_cycle);

            // does not use PWM if value is min or max
            else
                LED_SET(led, (duty_cycle >> 2) & 1);

            // enable brightness control
            led->pwm = 0;
            led->brightness_value = value;
            led->brightness = 1;
        }
    }
}

void ledz_tick(void)
{
    static uint16_t counter_1ms;
    int flag_1ms = 0;

    // check if 1ms has been passed
    if (++counter_1ms >= TICKS_TO_1ms)
    {
        counter_1ms = 0;
        flag_1ms = 1;
    }

    for (int i = 0; i < LEDZ_MAX_INSTANCES; i++)
    {
        ledz_t *led = &g_leds[i];

        // skip if led is not in use
        if (led->pins == 0)
            continue;

        // execute blink control if 1ms has been passed
        if (led->blink && flag_1ms)
        {
            if (led->time > 0)
                led->time--;

            if (led->time == 0)
            {
                if (led->blink_state)
                {
                    // disable hardware PWM
                    LED_PWM(led, 0);

                    // turn off led
                    LED_SET(led, 0);

                    // load counter with time off value
                    led->time = led->time_off;
                }
                else
                {
                    // turn on led
                    LED_SET(led, 1);

                    // enable hardware PWM
                    LED_PWM(led, cie1931[led->brightness_value]);

                    // load counter with time on value
                    led->time = led->time_on;
                }

                // toggle blink state
                led->blink_state = 1 - led->blink_state;

                // go to next led if blink control has updated led state
                continue;
            }
        }

        // brightness control
#ifndef LEDZ_GPIO_PWM
        if (led->brightness && (!led->blink || (led->blink && led->blink_state)))
        {
            if (led->pwm > 0)
                led->pwm--;

            if (led->pwm == 0)
            {
                // load counter with duty cycle according led state
                if (led->state)
                    led->pwm = 100 - cie1931[led->brightness_value];
                else
                    led->pwm = cie1931[led->brightness_value];

                // change led state only if value is between min and max
                if (led->pwm > 0 && led->pwm < 100)
                    LED_SET(led, !led->state);
            }
        }
#endif
    }
}
