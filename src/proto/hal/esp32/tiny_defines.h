/*
    Copyright 2019 (C) Alexey Dynda

    This file is part of Tiny Protocol Library.

    Protocol Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Protocol Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Protocol Library.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "tiny_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

/* For fastest version of protocol assign all defines to zero.
 * In this case protocol supports no CRC field, and
 * all api functions become not thread-safe.
 */

#define PLATFORM_MUTEX SemaphoreHandle_t

#define MUTEX_INIT(x) x = xSemaphoreCreateMutex()

#define MUTEX_LOCK(x) xSemaphoreTake( x, portMAX_DELAY )

#define MUTEX_TRY_LOCK(x) (xSemaphoreTake( x, 0 ) == pdTRUE)

#define MUTEX_UNLOCK(x) xSemaphoreGive( x )

#define MUTEX_DESTROY(x) vSemaphoreDelete( x )

#define PLATFORM_COND EventGroupHandle_t

#define COND_INIT(x) x = xEventGroupCreate()

#define COND_DESTROY(x) vEventGroupDelete(x)

#define COND_WAIT(cond, mutex) xEventGroupWaitBits(cond, 0x01, pdTRUE, pdFALSE, portMAX_DELAY)

#define COND_SIGNAL(x) xEventGroupSetBits(x, 0x01)

#define TASK_YIELD() vTaskDelay(0)

#define PLATFORM_TICKS()    millis()

#if defined(__XTENSA__)
#include <stdint.h>
static inline uint32_t millis() {   return (uint32_t)(esp_timer_get_time()/1000); }
#endif



