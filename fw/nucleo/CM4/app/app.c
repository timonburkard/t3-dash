#include "app.h"

#include <stdbool.h>

#include "FreeRTOS.h"
#include "acquisition.h"
#include "conversion.h"
#include "filtering.h"
#include "main.h"
#include "task.h"

QueueHandle_t queue_data_raw;
QueueHandle_t queue_data_converted;
QueueHandle_t queue_data_filtered;

void led_task(void* params);

void app(void)
{
    if ((queue_data_raw = xQueueCreate(20, sizeof(sensor_data_t))) == NULL) {
        while (true) {} // TODO: Error handling
    }

    if ((queue_data_converted = xQueueCreate(20, sizeof(sensor_data_t))) == NULL) {
        while (true) {} // TODO: Error handling
    }

    if ((queue_data_filtered = xQueueCreate(20, sizeof(sensor_data_t))) == NULL) {
        while (true) {} // TODO: Error handling
    }

    if (xTaskCreate(led_task, "LED Task", 512, NULL, 1, NULL) != pdPASS) {
        while (true) {} // TODO: Error handling
    }

    if (xTaskCreate(acquisition_task, "Acquisition Task", 512, NULL, 2, NULL) != pdPASS) {
        while (true) {} // TODO: Error handling
    }

    if (xTaskCreate(conversion_task, "Conversion Task", 512, NULL, 1, NULL) != pdPASS) {
        while (true) {} // TODO: Error handling
    }

    if (xTaskCreate(filtering_task, "Filtering Task", 512, NULL, 1, NULL) != pdPASS) {
        while (true) {} // TODO: Error handling
    }

    vTaskStartScheduler();

    while (true) {
        // Should never reach here
    }
}

void led_task(void* params)
{
    while (true) {
        HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
