/**
 * @file test_list_utils.cpp
 * @brief Tests for list utilities
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

#include <list_utils.h>

extern "C"
{

struct list_of_integers
{
   union
   {
      struct list_of_integers* next;
      struct list_element element;
   };

   int value;
};

static inline int __attribute__((pure)) compareIntegerElements(const struct list_element* left, const struct list_element* right) 
{
   const struct list_of_integers* leftInteger = (const struct list_of_integers*) left;
   const struct list_of_integers* rightInteger = (const struct list_of_integers*) right;

   return leftInteger->value - rightInteger->value;
}


}

#include <CppUTest/TestHarness.h>

TEST_GROUP(ListUtils)
{

   struct list_element* testList;

   struct list_of_integers one, three, five;

   struct list_of_integers tau, phi, rho;

   void setup()
   {
      one.value   = 1;
      three.value = 3;
      five.value  = 5;

      one.next   = &three;
      three.next = &five;
      five.next  = NULL;

      testList   = &one.element;

      tau.next = &phi;
      phi.next = &rho;
      rho.next = NULL;
   }

   void tearDown()
   {
   }
};

TEST(ListUtils, NullOnNull)
{
   struct list_element* alpha = NULL;
   struct list_element* beta = NULL;

   lst_insertIntoSortedList(&alpha, beta, compareIntegerElements);

   CHECK(! alpha);
}

TEST(ListUtils, AddElementToEmptyList)
{
   struct list_element* alpha = NULL;
   struct list_element beta;

   lst_insertIntoSortedList(&alpha, &beta, compareIntegerElements);

   POINTERS_EQUAL(alpha, &beta);
}

TEST(ListUtils, AddNothingToList)
{
   struct list_element element;
   struct list_element* alpha = &element;
   struct list_element* beta = NULL;

   element.next = NULL;

   lst_insertIntoSortedList(&alpha, beta, compareIntegerElements);

   POINTERS_EQUAL(alpha, &element);
   CHECK(! element.next);
}

TEST(ListUtils, Add0To135)
{
   struct list_of_integers newValue;

   newValue.value = 0;
   newValue.next  = NULL;

   CHECK_EQUAL(3, lst_computeLength(testList));
   CHECK(lst_isSortedAscending(testList, compareIntegerElements));

   lst_insertIntoSortedList(&testList, (struct list_element*) &newValue, compareIntegerElements);

   CHECK_EQUAL(4, lst_computeLength(testList));
   CHECK(lst_isSortedAscending(testList, compareIntegerElements));

   POINTERS_EQUAL(testList, (struct list_element*) &newValue);
}

TEST(ListUtils, Add2To135)
{
   struct list_of_integers newValue;

   newValue.value = 2;
   newValue.next  = NULL;

   CHECK_EQUAL(3, lst_computeLength(testList));
   CHECK(lst_isSortedAscending(testList, compareIntegerElements));

   const struct list_element* oldHead = testList;

   lst_insertIntoSortedList(&testList, (struct list_element*) &newValue, compareIntegerElements);

   POINTERS_EQUAL(oldHead, testList);

   CHECK_EQUAL(4, lst_computeLength(testList));
   CHECK(lst_isSortedAscending(testList, compareIntegerElements));
}

TEST(ListUtils, Add4To135)
{
   struct list_of_integers newValue;

   newValue.value = 4;
   newValue.next  = NULL;

   CHECK_EQUAL(3, lst_computeLength(testList));
   CHECK(lst_isSortedAscending(testList, compareIntegerElements));

   const struct list_element* oldHead = testList;

   lst_insertIntoSortedList(&testList, (struct list_element*) &newValue, compareIntegerElements);

   POINTERS_EQUAL(oldHead, testList);

   CHECK_EQUAL(4, lst_computeLength(testList));
   CHECK(lst_isSortedAscending(testList, compareIntegerElements));
}

TEST(ListUtils, Add6To135)
{
   struct list_of_integers newValue;

   newValue.value = 6;
   newValue.next  = NULL;

   CHECK_EQUAL(3, lst_computeLength(testList));
   CHECK(lst_isSortedAscending(testList, compareIntegerElements));

   const struct list_element* oldHead = testList;

   lst_insertIntoSortedList(&testList, (struct list_element*) &newValue, compareIntegerElements);

   POINTERS_EQUAL(oldHead, testList);

   CHECK_EQUAL(4, lst_computeLength(testList));
   CHECK(lst_isSortedAscending(testList, compareIntegerElements));
}

TEST(ListUtils, Add135To103050)
{
   CHECK_EQUAL(3, lst_computeLength(testList));
   CHECK(lst_isSortedAscending(testList, compareIntegerElements));

   one.value   = 10;
   three.value = 30;
   five.value  = 50;

   tau.value = 1;
   phi.value = 3;
   rho.value = 5;

   lst_mergeListIntoSortedList(&testList, (struct list_element*) &tau, compareIntegerElements);

   CHECK_EQUAL(6, lst_computeLength(testList));
   CHECK(lst_isSortedAscending(testList, compareIntegerElements));
}

TEST(ListUtils, Add1205To103050)
{
   CHECK_EQUAL(3, lst_computeLength(testList));
   CHECK(lst_isSortedAscending(testList, compareIntegerElements));

   one.value   = 10;
   three.value = 30;
   five.value  = 50;

   tau.value = 1;
   phi.value = 20;
   rho.value = 5;

   lst_mergeListIntoSortedList(&testList, (struct list_element*) &tau, compareIntegerElements);

   CHECK_EQUAL(6, lst_computeLength(testList));
   CHECK(lst_isSortedAscending(testList, compareIntegerElements));
}

TEST(ListUtils, Add1905To103050)
{
   CHECK_EQUAL(3, lst_computeLength(testList));
   CHECK(lst_isSortedAscending(testList, compareIntegerElements));

   one.value   = 10;
   three.value = 30;
   five.value  = 50;

   tau.value = 1;
   phi.value = 90;
   rho.value = 5;

   lst_mergeListIntoSortedList(&testList, (struct list_element*) &tau, compareIntegerElements);

   CHECK_EQUAL(6, lst_computeLength(testList));
   CHECK(lst_isSortedAscending(testList, compareIntegerElements));
}
