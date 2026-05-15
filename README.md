# 🛻 T3 Dash -- Digital dashboard for the VW T3

(C) 2026 Matthias Schär, Rinaldo Leone, Timon Burkard

This is a project work part of the CAS Embedded Systems at FHNW Brugg-Windisch.

## FW

### Nucleo

#### M4

```mermaid
flowchart TB
    ACQ[Acquisition Task\n1 Hz trigger]
    ISR[HAL_ADC_ConvCpltCallback]
    CONV[Conversion Task]
    FILT[Filtering Task]
    PUB[Publication Task]
    HB[Heartbeat Task]

    QRAW[(queue_data_raw)]
    QCONV[(queue_data_converted)]
    QFILT[(queue_data_filtered)]

    ADC[(ADC1 + DMA1 Stream0)]
    UART[(USART3)]
    LED[(LD1)]

    ACQ -->|HAL_ADC_Start_DMA| ADC
    ADC -->|DMA transfer complete IRQ| ISR

    ISR -->|xQueueSendFromISR\nSENSOR_ID_WATER_TEMPERATURE| QRAW
    ISR -->|xQueueSendFromISR\nSENSOR_ID_BATTERY_VOLTAGE| QRAW

    QRAW -->|xQueueReceive| CONV
    CONV -->|xQueueSend| QCONV

    QCONV -->|xQueueReceive| FILT
    FILT -->|xQueueSend| QFILT

    QFILT -->|xQueueReceive| PUB
    PUB -->|HAL_UART_Transmit| UART

    HB -->|toggle every 1 s| LED
```

#### M7

```mermaid
flowchart TB
    HB[Heartbeat Task]
    LED[(LD2)]

    HB -->|toggle every 1 s| LED
```
