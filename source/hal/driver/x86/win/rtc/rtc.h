/*****************************************************************************
 *   Copyright(C)2009-2022 by VSF Team                                       *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 ****************************************************************************/

#ifndef __HAL_X86_WIN_RTC_H__
#define __HAL_X86_WIN_RTC_H__

/*============================ INCLUDES ======================================*/

#include "hal/vsf_hal_cfg.h"

#if VSF_HAL_USE_RTC == ENABLED

/*============================ MACROS ========================================*/
/*============================ INCLUDES ======================================*/

#include "hal/driver/common/template/vsf_template_rtc.h"

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

typedef struct vsf_hw_rtc_t vsf_hw_rtc_t;

/*============================ GLOBAL VARIABLES ==============================*/

extern vsf_hw_rtc_t vsf_hw_rtc0;

/*============================ PROTOTYPES ====================================*/

extern vsf_err_t vsf_hw_rtc_init(vsf_rtc_t *rtc_ptr, rtc_cfg_t *cfg_ptr);
extern fsm_rt_t vsf_hw_rtc_enable(vsf_rtc_t *rtc_ptr);
extern fsm_rt_t vsf_hw_rtc_disable(vsf_rtc_t *rtc_ptr);
extern vsf_err_t vsf_hw_rtc_get(vsf_rtc_t *rtc_ptr, vsf_rtc_tm_t *tm);
extern vsf_err_t vsf_hw_rtc_set(vsf_rtc_t *rtc_ptr, const vsf_rtc_tm_t *tm);

#endif
#endif      // __HAL_X86_WIN_RTC_H__