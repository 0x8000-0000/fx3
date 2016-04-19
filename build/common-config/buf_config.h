/**
 * @file buf_config.h
 * @brief Buffer allocator configuration for test apps
 * @author Florin Iucha <florin@signbit.net>
 * @copyright Apache License, Version 2.0
 */

/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file is part of FX3 RTOS for ARM Cortex-M4
 */

#ifndef __BUF_CONFIG_H__
#define __BUF_CONFIG_H__

#define BUF_SMALL_BUF_COUNT 16
#define BUF_SMALL_BUF_SIZE  32

#define BUF_MEDIUM_BUF_COUNT 16
#define BUF_MEDIUM_BUF_SIZE  128

#define BUF_LARGE_BUF_COUNT 8
#define BUF_LARGE_BUF_SIZE  512

#endif // __BUF_CONFIG_H__

