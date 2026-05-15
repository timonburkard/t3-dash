#include "publication.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "app.h"
#include "main.h"
#include "queue.h"

extern UART_HandleTypeDef huart3;

void publication_task(void* params)
{
    sensor_data_t data;
    char          string[50] = "";

    while (true) {
        if (xQueueReceive(queue_data_filtered, &data, portMAX_DELAY) == pdTRUE) {
            switch (data.id) {
                case SENSOR_ID_WATER_TEMPERATURE:
                    snprintf(string, sizeof(string), "Water Temperature: %d\n", data.value);
                    break;

                case SENSOR_ID_BATTERY_VOLTAGE:
                    snprintf(string, sizeof(string), "Battery Voltage:   %d\n", data.value);
                    break;

                default:
                    while (true) {} // TODO: Error handling
                    break;
            }

            HAL_UART_Transmit(&huart3, (uint8_t*)string, strlen(string), HAL_MAX_DELAY);
        }
    }
}
