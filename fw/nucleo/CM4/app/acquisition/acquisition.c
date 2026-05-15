#include "acquisition.h"

#include <stdbool.h>

#include "FreeRTOS.h"
#include "app.h"
#include "queue.h"
#include "task.h"

void acquisition_task(void* params)
{
    TickType_t last_wakeup = xTaskGetTickCount();

    while (true) {

        // Start all sensor acquisitions in parallel, TODO:
        // HAL_ADC_Start_IT(&hadc1); // Water temperature
        // HAL_ADC_Start_IT(&hadc2); // Battery voltage

        sensor_data_t water_temperature = {.id = SENSOR_ID_WATER_TEMPERATURE, .value = 123};
        sensor_data_t battery_voltage   = {.id = SENSOR_ID_BATTERY_VOLTAGE, .value = 567};

        if (xQueueSend(queue_data_raw, &water_temperature, portMAX_DELAY) != pdPASS) {
            while (true) {} // TODO: Error handling
        }
        if (xQueueSend(queue_data_raw, &battery_voltage, portMAX_DELAY) != pdPASS) {
            while (true) {} // TODO: Error handling
        }

        vTaskDelayUntil(&last_wakeup, pdMS_TO_TICKS(1000));
    }
}
