/**
 * @file buffer.h
 * @brief Priority Queue implementation
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

#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <stdint.h>
#include <stdbool.h>

/**
 * @addtogroup Buffers
 * @{
 */

struct buffer
{
   uint16_t capacity;
   uint16_t size;

   uint8_t  data[];
};

struct buffer* buf_alloc(uint16_t capacity);

void buf_free(struct buffer* buf);

/** @} */ // Buffers

/**
 * @addtogroup Buffer Chains
 * @{
 */

struct buffer_chain
{
   struct buffer_chain* next;

   uint16_t capacity;
   uint16_t size;

   uint8_t  data[];
};

struct buffer_chain* buf_allocChain(uint16_t capacity);

struct buf_freeChain(struct buffer_chain* chain);

uint32_t buf_getChainCapacity(const struct buffer_chain* chain);

struct buffer_chain* buf_clone(struct buffer_chain* chain*);

void buf_mem2bufcopy(const uint8_t* source, struct buffer_chain* destination, uint32_t size);

void buf_buf2memcopy(const struct buffer_chain* source, uint8_t* destination, uint32_t size);

/** @} */ // Buffer Chains

/**
 * @addtogroup Buffer Chain Iterators
 * @{
 */

struct buffer_chain_iterator
{
   struct buffer_chain* firstElement;
   struct buffer_chain* currentElement;
   uint16_t             offsetInElement;
   uint16_t             totalOffset;
};

struct buffer_chain_iterator* buf_iterateBegin(struct buffer_chain* chain);

struct buffer_chain_iterator* buf_iterateEnd(struct buffer_chain* chain);

void buf_freeIterator(struct buffer_chain_iterator* iter);

bool buf_isIterationDone(struct buffer_chain_iterator* iter);

bool buf_incrementIterator(struct buffer_chain_iterator* iter);

bool buf_decrementIterator(struct buffer_chain_iterator* iter);

bool buf_advanceIterator(struct buffer_chain_iterator* iter, uint16_t offset);

bool buf_reverseIterator(struct buffer_chain_iterator* iter, uint16_t offset);

uint8_t buf_value(const struct buffer_chain_iterator* iter);

void buf_resetIterator(struct buffer_chain_iterator* iter);

struct buffer_chain_iterator* buf_findFirst(struct buffer_chain* chain, uint8_t value);

bool buf_findNext(struct buffer_chain_iterator* iter, uint8_t value);

/** @} */ // Buffer Chain Iterators

#endif // __BUFFER_H__

