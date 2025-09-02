#include <stdio.h>
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

extern void async_server(void);

// Task handle for the async server
static TaskHandle_t async_server_task_handle = NULL;

// Timer handle for statistics
static TimerHandle_t stats_timer = NULL;

// Async server task function
static void async_server_task(void *pvParameters) {
    ESP_LOGI("SERVER", "Async server task started");
    async_server();
    // This should never return, but if it does, delete the task
    vTaskDelete(NULL);
}

// Timer callback for printing statistics
static void stats_timer_callback(TimerHandle_t xTimer) {
    // Get task statistics
    UBaseType_t uxArraySize = uxTaskGetNumberOfTasks();
    TaskStatus_t *pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));
    
    if (pxTaskStatusArray != NULL) {
        unsigned long ulTotalRunTime;
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);
        
        printf("\n=== FreeRTOS Task Statistics ===\n");
        printf("Total tasks: %d\n", uxArraySize);
        printf("%-20s %-8s %-8s %-8s %-8s\n", "Task Name", "State", "Prio", "Stack", "CPU%%");
        printf("------------------------------------------------------------\n");
        
        // For percentage calculations
        unsigned long ulTotalRunTimeDiv100 = ulTotalRunTime / 100UL;
        
        for (int i = 0; i < uxArraySize; i++) {
            TaskStatus_t *pxTaskStatus = &pxTaskStatusArray[i];
            const char* state_str;
            
            switch (pxTaskStatus->eCurrentState) {
                case eRunning: state_str = "RUN"; break;
                case eReady: state_str = "RDY"; break;
                case eBlocked: state_str = "BLK"; break;
                case eSuspended: state_str = "SUS"; break;
                case eDeleted: state_str = "DEL"; break;
                default: state_str = "UNK"; break;
            }
            
            // Calculate CPU usage percentage following FreeRTOS official pattern
            unsigned long ulStatsAsPercentage = 0;
            if (ulTotalRunTimeDiv100 > 0) {
                ulStatsAsPercentage = pxTaskStatus->ulRunTimeCounter / ulTotalRunTimeDiv100;
            }
            
            // Format CPU percentage - show <1% for very low usage
            char cpu_str[12];
            if (ulStatsAsPercentage > 0) {
                snprintf(cpu_str, sizeof(cpu_str), "%lu%%", ulStatsAsPercentage);
            } else {
                snprintf(cpu_str, sizeof(cpu_str), "<1%%");
            }
            
            printf("%-20s %-8s %-8d %-8d %-8s\n",
                   pxTaskStatus->pcTaskName,
                   state_str,
                   (int)pxTaskStatus->uxCurrentPriority,
                   (int)pxTaskStatus->usStackHighWaterMark,
                   cpu_str);
        }
        printf("============================================================\n");
        
        vPortFree(pxTaskStatusArray);
    }
    
    // Print heap info
    size_t free_heap = esp_get_free_heap_size();
    size_t min_free_heap = esp_get_minimum_free_heap_size();
    printf("Free heap: %zu bytes, Min free heap: %zu bytes\n", free_heap, min_free_heap);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    // Create the async server task
    BaseType_t ret = xTaskCreate(
        async_server_task,           // Task function
        "AsyncServer",               // Task name
        8192,                        // Stack size (8KB)
        NULL,                        // Task parameters
        5,                           // Task priority
        &async_server_task_handle    // Task handle
    );
    
    if (ret != pdPASS) {
        ESP_LOGE("SERVER", "Failed to create async server task");
        return;
    }
    
    ESP_LOGI("SERVER", "Async server task created successfully");
    
    // Create timer for statistics (every 500ms)
    stats_timer = xTimerCreate(
        "StatsTimer",                // Timer name
        pdMS_TO_TICKS(500),         // Period (500ms)
        pdTRUE,                      // Auto-reload
        NULL,                        // Timer ID
        stats_timer_callback         // Callback function
    );
    
    if (stats_timer == NULL) {
        ESP_LOGE("SERVER", "Failed to create stats timer");
        return;
    }
    
    // Start the timer
    if (xTimerStart(stats_timer, 0) != pdPASS) {
        ESP_LOGE("SERVER", "Failed to start stats timer");
        return;
    }
    
    ESP_LOGI("SERVER", "Statistics timer started - printing every 500ms");
    
    // Main loop - just keep the task alive
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
