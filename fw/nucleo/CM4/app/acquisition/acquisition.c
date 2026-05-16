#include "acquisition.h"

#include <stdbool.h>

#include "FreeRTOS.h"
#include "app.h"
#include "main.h"
#include "queue.h"
#include "task.h"

extern ADC_HandleTypeDef hadc1;

static volatile uint16_t adc1_buffer[2] = {0};

void acquisition_task(void* params)
{
    TickType_t last_wakeup = xTaskGetTickCount();

    while (true) {

        // Start all sensor acquisitions
        HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc1_buffer, 2);

        vTaskDelayUntil(&last_wakeup, pdMS_TO_TICKS(1000));
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    BaseType_t higher_priority_task_woken = pdFALSE;

    if (hadc->Instance == ADC1) {
        sensor_data_t water_temperature = {.id = SENSOR_ID_WATER_TEMPERATURE, .value = 0};
        sensor_data_t battery_voltage   = {.id = SENSOR_ID_BATTERY_VOLTAGE, .value = 0};

        water_temperature.value = adc1_buffer[0];
        battery_voltage.value   = adc1_buffer[1];

        if (xQueueSendFromISR(queue_data_raw, &water_temperature, &higher_priority_task_woken) != pdPASS) {
            while (true) {} // TODO: Error handling
        }

        if (xQueueSendFromISR(queue_data_raw, &battery_voltage, &higher_priority_task_woken) != pdPASS) {
            while (true) {} // TODO: Error handling
        }
    } else {
        while (true) {} // TODO: Error handling
    }

    portYIELD_FROM_ISR(higher_priority_task_woken);
}
