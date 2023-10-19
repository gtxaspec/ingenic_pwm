#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include "version.h"

#define PWM_NUM          8
#define PWM_QUERY_STATUS 0x200
#define PWM_CONFIG       0x001
#define PWM_CONFIG_DUTY  0x002
#define PWM_ENABLE       0x010
#define PWM_DISABLE      0x100

struct pwm_ioctl_t {
    int index;
    int duty;
    int period;
    int polarity;
    int enabled;
};

void print_usage(const char *prog_name) {
    printf("INGENIC PWM Control Version: %s\n", VERSION);
    printf("Usage: %s [options]\n\n", prog_name);
    printf("Options:\n");
    printf("  -c, --channel=<0-%d>            Specify PWM channel number\n", PWM_NUM-1);
    printf("  -q, --query                    Query channel state\n");
    printf("  -e, --enable                   Enable channel\n");
    printf("  -d, --disable                  Disable channel\n");
    printf("  -p, --polarity=<0|1>           Set polarity (0: Inversed, 1: Normal)\n");
    printf("  -D, --duty=<duty_ns>           Set duty cycle in ns\n");
    printf("  -P, --period=<period_ns>       Set period in ns\n");
    printf("  -u, --ramp_up                  Ramp up PWM\n");
    printf("  -w, --ramp_down                Ramp down PWM\n");
    printf("  -i, --increment=<value>        Set increment for ramp up\n");
    printf("  -m, --decrement=<value>        Set decrement for ramp down\n");
    printf("  -x, --max_duty=<max_duty_ns>   Set max duty for ramping\n");
    printf("  -n, --min_duty=<min_duty_ns>   Set min duty for ramping\n");
    printf("  -h, --help                     Display this help message\n");
}

void query_pwm_status(int fd, int channel) {
    struct pwm_ioctl_t pwm_status;
    pwm_status.index = channel;
    if (ioctl(fd, PWM_QUERY_STATUS, &pwm_status) != -1) {
        printf("PWM Channel %d Status:\n", channel);
        printf("Enabled: %s\n", pwm_status.enabled ? "Enabled" : "Disabled");
        printf("Polarity: %s\n", pwm_status.polarity ? "Normal" : "Inversed");
        printf("Duty: %d ns\n", pwm_status.duty);
        printf("Period: %d ns\n", pwm_status.period);
    } else {
        perror("Error querying PWM status");
    }
}

void ramp_pwm(int fd, int channel, int increment, int decrement, int max_duty, int min_duty, int ramp_up) {
    int duty;
    if (ramp_up) {
        for (duty = min_duty; duty <= max_duty; duty += increment) {
            struct pwm_ioctl_t pwm_data;
            pwm_data.index = channel;
            pwm_data.duty = duty;
            ioctl(fd, PWM_CONFIG_DUTY, &pwm_data);
            usleep(50000);  // 50 ms delay, can be adjusted
        }
    } else {
        for (duty = max_duty; duty >= min_duty; duty -= decrement) {
            struct pwm_ioctl_t pwm_data;
            pwm_data.index = channel;
            pwm_data.duty = duty;
            ioctl(fd, PWM_CONFIG_DUTY, &pwm_data);
            usleep(50000);  // 50 ms delay, can be adjusted
        }
    }
}

int main(int argc, char *argv[]) {
    int channel = -1;
    int polarity = -1;
    int duty_ns = -1;
    int period_ns = -1;
    int enable = 0;
    int disable = 0;
    int query = 0;
    int ramp_up = 0;
    int ramp_down = 0;
    int increment = -1;
    int decrement = -1;
    int max_duty = -1;
    int min_duty = -1;

    struct option long_options[] = {
        {"channel", required_argument, NULL, 'c'},
        {"query", no_argument, NULL, 'q'},
        {"enable", no_argument, NULL, 'e'},
        {"disable", no_argument, NULL, 'd'},
        {"polarity", required_argument, NULL, 'p'},
        {"duty", required_argument, NULL, 'D'},
        {"period", required_argument, NULL, 'P'},
        {"ramp_up", no_argument, NULL, 'u'},
        {"ramp_down", no_argument, NULL, 'w'},
        {"increment", required_argument, NULL, 'i'},
        {"decrement", required_argument, NULL, 'm'},
        {"max_duty", required_argument, NULL, 'x'},
        {"min_duty", required_argument, NULL, 'n'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    if (argc == 1) {
        print_usage(argv[0]);
        return 0;
    }

    int opt;
    while ((opt = getopt_long(argc, argv, "c:qedp:D:P:uw:i:m:x:n:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'c':
                channel = atoi(optarg);
                break;
            case 'q':
                query = 1;
                break;
            case 'e':
                enable = 1;
                break;
            case 'd':
                disable = 1;
                break;
            case 'p':
                polarity = atoi(optarg);
                break;
            case 'D':
                duty_ns = atoi(optarg);
                break;
            case 'P':
                period_ns = atoi(optarg);
                break;
            case 'u':
                ramp_up = 1;
                break;
            case 'w':
                ramp_down = 1;
                break;
            case 'i':
                increment = atoi(optarg);
                break;
            case 'm':
                decrement = atoi(optarg);
                break;
            case 'x':
                max_duty = atoi(optarg);
                break;
            case 'n':
                min_duty = atoi(optarg);
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return -1;
        }
    }

    if (channel < 0 || channel >= PWM_NUM) {
        printf("Error: Invalid PWM channel number.\n");
        return -1;
    }

    int fd = open("/dev/pwm", O_RDWR);
    if (fd < 0) {
        perror("Error opening /dev/pwm, make sure pwm drivers are enabled");
        return -1;
    }

    struct pwm_ioctl_t pwm_data;
    pwm_data.index = channel;

    if (polarity != -1) {
        pwm_data.polarity = polarity;
        ioctl(fd, PWM_CONFIG, &pwm_data);
    }

    if (duty_ns != -1) {
        pwm_data.duty = duty_ns;
        ioctl(fd, PWM_CONFIG_DUTY, &pwm_data);
    }

    if (period_ns != -1) {
        pwm_data.period = period_ns;
        ioctl(fd, PWM_CONFIG, &pwm_data);
    }

    if (enable) {
        ioctl(fd, PWM_ENABLE, channel);
    }

    if (disable) {
        ioctl(fd, PWM_DISABLE, channel);
    }

    if (query) {
        query_pwm_status(fd, channel);
    }

    if (ramp_up) {
        ramp_pwm(fd, channel, increment, decrement, max_duty, min_duty, 1);
    }

    if (ramp_down) {
        ramp_pwm(fd, channel, increment, decrement, max_duty, min_duty, 0);
    }

    close(fd);
    return 0;
}
