#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/portable.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "hal/wdt_hal.h"
#include "hal/wdt_types.h"

#define WDT_LEVEL_INTR_SOURCE ETS_TG0_WDT_LEVEL_INTR_SOURCE
#define IWDT_PRESCALER MWDT_LL_DEFAULT_CLK_PRESCALER // Tick period of 500us if WDT source clock is 80MHz
#define IWDT_TICKS_PER_US 500
#define IWDT_INSTANCE WDT_MWDT0
#define IWDT_INITIAL_TIMEOUT_S 10 //dispara o WDT_MWDT0 em 10 segundos
#define GPIO_OUTPUT_IO_0 18
#define GPIO_OUTPUT_IO_1 19
#define GPIO_OUTPUT_PIN_SEL ((1ULL << GPIO_OUTPUT_IO_0) | (1ULL << GPIO_OUTPUT_IO_1))

/* Definitions for the event bits in the event group. */
#define mainTaskA_TASK_BIT (1UL << 0UL) /* Event bit 0, which is set by the first task. */   // 0x01
#define mainTaskB_TASK_BIT (1UL << 1UL) /* Event bit 1, which is set by the second task. */ // 0x02
#define mainTaskC_TASK_BIT (1UL << 2UL) /* Event bit 2, which is set by the second task. */ // 0x04
const EventBits_t uxAllSyncBits = (mainTaskA_TASK_BIT | mainTaskB_TASK_BIT | mainTaskC_TASK_BIT); // 0x07

/* Declare the event group used to synchronize the three tasks. */
EventGroupHandle_t xEventGroup;
static const char *TAG = "example";
wdt_hal_context_t iwdt_context;

static TaskHandle_t TaskA_Handler;
static TaskHandle_t TaskB_Handler;
static TaskHandle_t TaskC_Handler;
static TaskHandle_t TaskFeedWDT_Handler;

SemaphoreHandle_t  myMutexRecurso1;
SemaphoreHandle_t  myMutexRecurso2;
SemaphoreHandle_t  myMutexRecurso3;


void vTaskMonitorWDT(void);
void ConfigInitWDT();
void FeedWDT(void);


void app_main(void)
{
    ConfigInitWDT();

    xTaskCreatePinnedToCore(vTaskMonitorWDT, "vTaskFeedWDT", 2048 * 3, NULL, 2, &TaskFeedWDT_Handler, 0);
}

void ConfigInitWDT()
{
    //inicia a strutura de contexto do WDT_MWDT0
    wdt_hal_init(&iwdt_context, IWDT_INSTANCE, IWDT_PRESCALER, true);

    //desabilita a protecão de escrita no WDT
    wdt_hal_write_protect_disable(&iwdt_context);

    //configura o WDT_MWDT0 para reinicializa o sistema em 100ms
    wdt_hal_config_stage(&iwdt_context, WDT_STAGE0, IWDT_INITIAL_TIMEOUT_S * (1000000 / IWDT_TICKS_PER_US), WDT_STAGE_ACTION_RESET_SYSTEM);

    //habilita o WDT_MWDT0
    wdt_hal_enable(&iwdt_context);

    //habilita a proteção de escrita no WDT_MWDT0
    wdt_hal_write_protect_enable(&iwdt_context);
}


void vTaskMonitorWDT(void)
{
    int cnt = 0;

    while (true)
    {
        vTaskDelay(100);

        FeedWDT();

        cnt++;
        if (cnt > 5)
        {
            for (;;);
        }
    }
}


void FeedWDT(void)
{
    //desabilita a proteção de escrita no WDT_MWDT0
    wdt_hal_write_protect_disable(&iwdt_context);

    //alimenta o contador do WDT_MWDT0
    wdt_hal_feed(&iwdt_context);

    //habilita a proteção de escrita no WDT_MWDT0
    wdt_hal_write_protect_enable(&iwdt_context);

    //envia pela serial um log indicando a alimentação do WDT_MWDT0
    ESP_LOGI("FeedWDT", "WDT Resetado!");
}




