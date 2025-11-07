

#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"

enum state {WAITING=1, WRITE_TO_MEMORY, SEND_MESSAGE, };

enum lower_state = = WAITING

char current_morse;

char morse_message[257]

int morse_index = 0;

static void read_sensor() {
    float ax, ay, az, gx, gy, gz, t;

    if (lower_state != WAITING)
        return;  // Do nothing if not ready to detect motion

    init_ICM42670();

    ICM42670_start_with_default_values();

    if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0)
    {
        if (az > 0.15) {
            printf("UP: %.2fg)\n", az); // delete after testing
            current_morse = ".";
        }
        else if (az < -0.15) {
            printf("DOWN: %.2fg)\n", az); // delete after testing
            current_morse = "-";
        }

        lower_state = RECORDING;
    }
    
    vTaskDelay(pdMS_TO_TICKS(100)); // ~10 Hz update rate
    
}


static void read_button() {

    if (lower_state != WRITE_TO_MEMORY)
        return;

    if (current_morse != '\0' && morse_index < MAX_MORSE_LEN) {
        morse_message[morse_index++] = current_morse;
        morse_message[morse_index] = '\0'; /// keep string terminated
        printf("Stored: %c | Buffer: %s\n", current_morse, morse_message); /// only for testing
    }

    current_morse = '\0';
    lower_state = WAITING;  /// Ready for next motion
}