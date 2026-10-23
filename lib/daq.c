#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "daq.h"

#define DAQ_PATH_MAX 512

static volatile sig_atomic_t stop_requested = 0;

static void handle_stop(int sig)
{
    (void)sig;
    stop_requested = 1;
}

static void print_usage(const char *prog, const char *default_name,
                        const char *count_label, long default_count)
{
    fprintf(stderr, "Uso: %s [file_output] [%s]\n", prog, count_label);
    fprintf(stderr, "  file_output  nome del file in %s (default: %s)\n", DATA_DIR, default_name);
    fprintf(stderr, "               se inizia con '/' è usato come path completo\n");
    fprintf(stderr, "  %-12s intero > 0 (default: %ld)\n", count_label, default_count);
}

int daq_parse_args(int argc, char **argv,
                   const char *default_name,
                   const char *count_label, long default_count,
                   const char **filename, long *count)
{
    *filename = default_name;
    *count = default_count;

    if (argc > 3 ||
        (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))) {
        print_usage(argv[0], default_name, count_label, default_count);
        return -1;
    }

    if (argc > 1) {
        *filename = argv[1];
    }

    if (argc > 2) {
        char *end = NULL;
        errno = 0;
        long value = strtol(argv[2], &end, 10);

        if (errno != 0 || end == argv[2] || *end != '\0' || value <= 0) {
            fprintf(stderr, "%s non valido: '%s'\n", count_label, argv[2]);
            print_usage(argv[0], default_name, count_label, default_count);
            return -1;
        }
        *count = value;
    }

    return 0;
}

FILE *daq_open_output(const char *filename)
{
    char path[DAQ_PATH_MAX];
    int len;

    if (filename[0] == '/') {
        len = snprintf(path, sizeof(path), "%s", filename);
    } else {
        len = snprintf(path, sizeof(path), "%s/%s", DATA_DIR, filename);
    }

    if (len < 0 || (size_t)len >= sizeof(path)) {
        fprintf(stderr, "Path troppo lungo: %s\n", filename);
        return NULL;
    }

    FILE *out = fopen(path, "w");
    if (!out) {
        fprintf(stderr, "Errore apertura file %s: %s\n", path, strerror(errno));
        return NULL;
    }

    printf("Output file: %s\n", path);
    return out;
}

void daq_install_signal_handlers(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_stop;
    sigemptyset(&sa.sa_mask);
    /* senza SA_RESTART: sleep() e usleep() vengono interrotti subito;
       SA_RESETHAND: il secondo Ctrl-C usa il comportamento di default */
    sa.sa_flags = SA_RESETHAND;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

int daq_stop_requested(void)
{
    return stop_requested;
}

int daq_sleep(unsigned int seconds)
{
    unsigned int left = seconds;

    while (left > 0 && !stop_requested) {
        left = sleep(left);
    }

    return !stop_requested;
}

double daq_now(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

void daq_write_line(FILE *out, const char *prefix, const long *values, int n)
{
    if (prefix) {
        printf("%s", prefix);
        fprintf(out, "%s", prefix);
    }

    for (int i = 0; i < n; i++) {
        const char *sep = (i == 0 && !prefix) ? "" : " ";
        printf("%s%ld", sep, values[i]);
        fprintf(out, "%s%ld", sep, values[i]);
    }

    printf("\n");
    fprintf(out, "\n");
    fflush(out);
}

void daq_print_start(long count, const char *what)
{
    printf("=== Start acquisition: %ld %s ===\n", count, what);
    printf("Premi Ctrl-C per interrompere pulitamente.\n\n");
}

void daq_print_summary(long count, const char *what)
{
    if (stop_requested) {
        printf("\nCtrl-C ricevuto: acquisizione interrotta.\n");
    }
    printf("Acquisizione terminata. %s: %ld\n", what, count);
}
