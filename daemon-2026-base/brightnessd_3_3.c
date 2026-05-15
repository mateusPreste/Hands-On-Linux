#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_LDR_PATH "/sys/kernel/smartlamp/ldr"
#define DEFAULT_BACKLIGHT_PATH "/sys/class/backlight/intel_backlight"
#define DEFAULT_INTERVAL_MS 1000
#define MIN_PERCENT 10

static volatile sig_atomic_t running = 1;

struct config {
    const char *ldr_path;
    const char *backlight_path;
    int interval_ms;
};

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
    FILE *file;

    file = fopen(path, "r");
    if (!file)
        return -errno;

    if (fscanf(file, "%d", value) != 1) {
        fclose(file);
        return -EIO;
    }

    fclose(file);
    return 0;
}

static int write_int_file(const char *path, int value)
{
    FILE *file;

    file = fopen(path, "w");
    if (!file)
        return -errno;

    if (fprintf(file, "%d\n", value) < 0) {
        fclose(file);
        return -EIO;
    }

    fclose(file);
    return 0;
}

static int ldr_to_brightness(int ldr, int max_brightness)
{
    int percent;
    int brightness;

    // Limita o LDR para 0-100
    percent = clamp(ldr, 0, 100);

    // Aplica brilho minimo se o valor for muito baixo
    if (percent < MIN_PERCENT)
        percent = MIN_PERCENT;

    // Converte o percentual para a escala real de brilho
    brightness = (percent * max_brightness) / 100;

    return brightness;
}

static void sleep_ms(int milliseconds)
{
    struct timespec request;

    request.tv_sec = milliseconds / 1000;
    request.tv_nsec = (long)(milliseconds % 1000) * 1000000L;

    while (running && nanosleep(&request, &request) == -1 && errno == EINTR) {
    }
}

static int parse_args(int argc, char **argv, struct config *config)
{
    int opt;

    while ((opt = getopt(argc, argv, "l:b:i:h")) != -1) {
        switch (opt) {
        case 'l':
            config->ldr_path = optarg;
            break;
        case 'b':
            config->backlight_path = optarg;
            break;
        case 'i':
            config->interval_ms = atoi(optarg);
            if (config->interval_ms <= 0)
                return -EINVAL;
            break;
        case 'h':
            printf("Usage: %s [-l ldr_path] [-b backlight_path] [-i interval_ms]\n", argv[0]);
            exit(0);
        default:
            return -EINVAL;
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    char max_path[512];
    char brightness_path[512];
    struct config config = {
        .ldr_path = DEFAULT_LDR_PATH,
        .backlight_path = DEFAULT_BACKLIGHT_PATH,
        .interval_ms = DEFAULT_INTERVAL_MS,
    };
    int max_brightness;

    if (parse_args(argc, argv, &config) < 0)
        return EXIT_FAILURE;

    snprintf(max_path, sizeof(max_path), "%s/max_brightness", config.backlight_path);
    snprintf(brightness_path, sizeof(brightness_path), "%s/brightness", config.backlight_path);

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    if (read_int_file(max_path, &max_brightness) < 0 || max_brightness <= 0) {
        fprintf(stderr, "failed to read %s\n", max_path);
        return EXIT_FAILURE;
    }

    while (running) {
        int ldr;
        int brightness;

        if (read_int_file(config.ldr_path, &ldr) == 0) {
            brightness = ldr_to_brightness(ldr, max_brightness);

            if (write_int_file(brightness_path, brightness) < 0) {
                fprintf(stderr, "failed to write %s\n", brightness_path);
            }

            printf("ldr=%d brightness=%d max_brightness=%d\n", ldr, brightness, max_brightness);
            fflush(stdout);
        } else {
            fprintf(stderr, "failed to read %s\n", config.ldr_path);
        }

        sleep_ms(config.interval_ms);
    }

    return EXIT_SUCCESS;
}
