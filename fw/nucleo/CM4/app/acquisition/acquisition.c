#include "acquisition.h"

#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"

void acquisition_task(void* params)
{
    TickType_t last_wakeup = xTaskGetTickCount();

    while (true) {

        // Start all sensor acquisitions in parallel, TODO:
        // HAL_ADC_Start_IT(&hadc1); // Water temperature
        // HAL_ADC_Start_IT(&hadc2); // Battery voltage

        vTaskDelayUntil(&last_wakeup, pdMS_TO_TICKS(1000));
    }
}
