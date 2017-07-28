#include <stdio.h>
#include <unistd.h>
#include "ledz.h"

void gpio_set(int port, int pin, int value)
{
    if (value != LEDZ_TURN_ON_VALUE)
    {
        // change to color gray (led off)
        pin = 90;
    }

    // select color and draw full circle
    printf("\e[%iG\e[0;%im\xE2\x97\x8F ",(port * 2) - 1, pin);

    fflush(stdout);
}

int main(void)
{
    // hide cursor
    fputs("\e[?25l", stdout);

    ledz_t* led1 =
        ledz_create(LEDZ_1COLOR, (const ledz_color_t []){LEDZ_RED},   (const int []){1, 31});

    ledz_t* led2 =
        ledz_create(LEDZ_1COLOR, (const ledz_color_t []){LEDZ_GREEN}, (const int []){2, 32});

    ledz_on(led1, LEDZ_RED);
    ledz_on(led2, LEDZ_GREEN);
    sleep(1);

    ledz_off(led1, LEDZ_RED);
    ledz_off(led2, LEDZ_GREEN);
    usleep(500000);

    ledz_toggle(led2, LEDZ_GREEN);

    ledz_destroy(led1);
    ledz_destroy(led2);

    printf("\n");

    ledz_t* led_rgb =
        ledz_create(LEDZ_3COLOR, (const ledz_color_t []){LEDZ_RED, LEDZ_GREEN, LEDZ_BLUE},
                    (const int []){1, 31, 2, 32, 3, 34});

    ledz_on(led_rgb, LEDZ_RED | LEDZ_GREEN | LEDZ_BLUE);
    sleep(1);
    ledz_off(led_rgb, LEDZ_GREEN);
    usleep(500000);

    // show cursor and reset color
    fputs("\e[?25h\e[39m\n", stdout);

    return 0;
}
