#include "app.h"

#include <stdbool.h>

#include "FreeRTOS.h"
#include "main.h"
#include "task.h"

void task_led(void* params)
{
    while (true) {
        HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app(void)
{
    xTaskCreate(task_led, "LED Task", 512, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true) {
    }
}
