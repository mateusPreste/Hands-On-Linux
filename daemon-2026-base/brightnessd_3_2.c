#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_LDR_PATH "/sys/kernel/smartlamp/ldr"
#define DEFAULT_INTERVAL_MS 1000
#define MIN_PERCENT 10

static volatile sig_atomic_t running = 1;

static void handle_signal(int signal)
{
    (void)signal;
    running = 0;
}

static int __attribute__((unused)) clamp(int value, int min, int max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

static int read_int_file(const char *path, int *value)
{
    FILE *file = fopen(path, "r");
    if (!file)        return -errno;
    if (fscanf(file, "%d", value) != 1) {
        fclose(file);
        return -EINVAL;
    }
    fclose(file);
    return 0;
}

static int ldr_to_percent(int ldr)
{
    int percent = ldr;

    if (percent < MIN_PERCENT)
    {
        percent = MIN_PERCENT;
    }

    if (percent > 100)
    {
        percent = 100;
    }

    return percent;
}

static void sleep_ms(int milliseconds)
{
    struct timespec request;

    request.tv_sec = milliseconds / 1000;
    request.tv_nsec = (long)(milliseconds % 1000) * 1000000L;

    while (running && nanosleep(&request, &request) == -1 && errno == EINTR) {
    }
}

int main(void)
{
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    while (running) {
        int ldr;
        int percent;

        if (read_int_file(DEFAULT_LDR_PATH, &ldr) == 0) {
            percent = ldr_to_percent(ldr);
            printf("ldr=%d brightness_percent=%d\n", ldr, percent);
            fflush(stdout);
        } else {
            fprintf(stderr, "failed to read %s\n", DEFAULT_LDR_PATH);
        }

        sleep_ms(DEFAULT_INTERVAL_MS);
    }

    return EXIT_SUCCESS;
}
