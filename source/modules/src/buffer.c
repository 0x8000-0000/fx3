/**
 * @file buffer.h
 * @brief Buffer allocator implementation
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

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <buffer.h>
#include <bitops.h>

#include <buf_config.h>

#define DEFINE_BUFFER(xx, XX) \
   static volatile uint32_t xx ## BufferBitmap; \
   static volatile uint32_t xx ## BufferHistogram[BUF_ ## XX ## _BUF_COUNT]; \
   static uint8_t xx ## BufferPool[(sizeof(struct buffer) + BUF_ ## XX ## _BUF_SIZE) * BUF_ ## XX ## _BUF_COUNT];

DEFINE_BUFFER(small, SMALL)

DEFINE_BUFFER(medium, MEDIUM)

DEFINE_BUFFER(large, LARGE)

static uint32_t exhaustedPool[3];

void buf_initialize(void)
{
   bit_initialize(&smallBufferBitmap , BUF_SMALL_BUF_COUNT);
   bit_initialize(&mediumBufferBitmap, BUF_MEDIUM_BUF_COUNT);
   bit_initialize(&largeBufferBitmap , BUF_LARGE_BUF_COUNT);

   memset((void*) smallBufferHistogram,  0, sizeof(smallBufferHistogram));
   memset((void*) mediumBufferHistogram, 0, sizeof(mediumBufferHistogram));
   memset((void*) largeBufferHistogram,  0, sizeof(largeBufferHistogram));

   memset(exhaustedPool, 0, sizeof(exhaustedPool));
}

struct buffer* buf_alloc(uint16_t capacity)
{
   struct buffer* buf = NULL;

   if (BUF_SMALL_BUF_SIZE >= capacity)
   {
      uint32_t smallBufIndex = bit_alloc(&smallBufferBitmap);
      if (32 > smallBufIndex)
      {
         buf = (struct buffer*) &smallBufferPool[smallBufIndex * BUF_SMALL_BUF_SIZE];

         buf->next     = NULL;
         buf->capacity = BUF_SMALL_BUF_SIZE;
         buf->size     = 0;

         int usage = BUF_SMALL_BUF_COUNT - __builtin_popcount(smallBufferBitmap);
         smallBufferHistogram[usage] ++;
      }
      else
      {
         buf_on_poolExhausted(BUF_SMALL_BUF_SIZE);
      }
   }

   if ((NULL == buf) && (BUF_MEDIUM_BUF_SIZE >= capacity))
   {
      uint32_t mediumBufIndex = bit_alloc(&mediumBufferBitmap);
      if (32 > mediumBufIndex)
      {
         buf = (struct buffer*) &mediumBufferPool[mediumBufIndex * BUF_MEDIUM_BUF_SIZE];

         buf->next     = NULL;
         buf->capacity = BUF_MEDIUM_BUF_SIZE;
         buf->size     = 0;

         int usage = BUF_MEDIUM_BUF_COUNT - __builtin_popcount(mediumBufferBitmap);
         mediumBufferHistogram[usage] ++;
      }
      else
      {
         buf_on_poolExhausted(BUF_MEDIUM_BUF_SIZE);
      }
   }

   if ((NULL == buf) && (BUF_LARGE_BUF_SIZE >= capacity))
   {
      uint32_t largeBufIndex = bit_alloc(&largeBufferBitmap);
      if (32 > largeBufIndex)
      {
         buf = (struct buffer*) &largeBufferPool[largeBufIndex * BUF_LARGE_BUF_SIZE];

         buf->next     = NULL;
         buf->capacity = BUF_LARGE_BUF_SIZE;
         buf->size     = 0;

         int usage = BUF_LARGE_BUF_COUNT - __builtin_popcount(largeBufferBitmap);
         largeBufferHistogram[usage] ++;
      }
      else
      {
         buf_on_poolExhausted(BUF_LARGE_BUF_SIZE);
      }
   }

   return buf;
}

void buf_free(struct buffer* buf)
{
   volatile uint32_t* bitmap = NULL;
   ptrdiff_t bufferIndex = 0;

   switch (buf->capacity)
   {
   case BUF_SMALL_BUF_SIZE:
      bitmap      = &smallBufferBitmap;
      bufferIndex = (((uint8_t*) buf) - smallBufferPool) / BUF_SMALL_BUF_SIZE;
      assert(BUF_SMALL_BUF_COUNT > bufferIndex);
      break;

   case BUF_MEDIUM_BUF_SIZE:
      bitmap      = &mediumBufferBitmap;
      bufferIndex = (((uint8_t*) buf) - mediumBufferPool) / BUF_MEDIUM_BUF_SIZE;
      assert(BUF_MEDIUM_BUF_COUNT > bufferIndex);
      break;

   case BUF_LARGE_BUF_SIZE:
      bitmap      = &largeBufferBitmap;
      bufferIndex = (((uint8_t*) buf) - largeBufferPool) / BUF_LARGE_BUF_SIZE;
      assert(BUF_LARGE_BUF_COUNT > bufferIndex);
      break;

   default:
      assert(false);
   }

   if (bitmap)
   {
      bit_free(bitmap, (uint32_t) bufferIndex);
   }
}

__attribute__((weak)) void buf_on_poolExhausted(uint16_t capacityClass)
{
   switch (capacityClass)
   {
   case BUF_SMALL_BUF_SIZE:
      exhaustedPool[0] ++;
      break;

   case BUF_MEDIUM_BUF_SIZE:
      exhaustedPool[1] ++;
      break;

   case BUF_LARGE_BUF_SIZE:
      exhaustedPool[2] ++;

      /* intentional fall-through */

   default:
      assert(false);
   }
}

