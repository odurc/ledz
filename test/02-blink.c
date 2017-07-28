#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "ledz.h"

#define UNUSED_PARAM(var) do { (void)(var); } while (0)

static void* tick(void *arg)
{
    UNUSED_PARAM(arg);

    while (1)
    {
        usleep(1000);
        ledz_tick();
    }

    return 0;
}

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

    ledz_blink(led1, LEDZ_RED, 500, 500);
    ledz_blink(led2, LEDZ_GREEN, 100, 1000);

    // set thread attributes
    pthread_attr_t attributes;
    pthread_attr_init(&attributes);
    pthread_attr_setinheritsched(&attributes, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setscope(&attributes, PTHREAD_SCOPE_PROCESS);

    // create thread
    pthread_t thread;
    pthread_create(&thread, &attributes, tick, 0);

    sleep(5);

    // kill thread
    pthread_cancel(thread);
    pthread_join(thread, NULL);

    // show cursor and reset color
    fputs("\e[?25h\e[39m\n", stdout);

    return 0;
}
