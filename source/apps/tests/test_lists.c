/**
 * @file test_lists.c
 * @brief Test Cortex-M implementation of list operations
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
#include <stdbool.h>
#include <string.h>

#include <board.h>
#include <task.h>

#include <list_utils.h>

struct list_of_integers
{
   union
   {
      struct list_of_integers* next;
      struct list_element element;
   };

   int value;
};

static struct list_element* theList = NULL;

static struct list_of_integers one, three, five;

static struct list_element* otherList = NULL;

static void testLists(const void* arg __attribute__((unused)))
{
   one.value   = 1;
   three.value = 3;
   five.value  = 5;

   lst_pushElement(&theList, &one.element);
   lst_pushElement(&theList, &three.element);
   lst_pushElement(&theList, &five.element);

   assert(3 == lst_computeLength(theList));
   
   otherList = lst_fetchAll(&theList);
   
   assert(NULL == theList);
   
   assert(3 == lst_computeLength(otherList));

   __BKPT(42);
}

static uint8_t testListsStack[256] __attribute__ ((aligned (16)));


static const struct task_config testListsTaskConfig =
{
   .name            = "Test Lists",
   .handler         = testLists,
   .argument        = NULL,
   .priority        = 4,
   .stackBase       = testListsStack,
   .stackSize       = sizeof(testListsStack),
   .timeSlice_ticks = 0,
};

static struct task_control_block testListsTCB;

int main(void)
{
   bsp_initialize();
   fx3_initialize();

   fx3_createTask(&testListsTCB,  &testListsTaskConfig);

   fx3_startMultitasking();

   // never reached
   assert(false);

   return 0;
}

