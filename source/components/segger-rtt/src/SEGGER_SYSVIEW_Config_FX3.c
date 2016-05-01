/*********************************************************************
*               SEGGER MICROCONTROLLER GmbH & Co. KG                 *
*       Solutions for real time microcontroller applications         *
**********************************************************************
*                                                                    *
*       (c) 2015 - 2016  SEGGER Microcontroller GmbH & Co. KG        *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
*       SEGGER SystemView * Real-time application analysis           *
*                                                                    *
**********************************************************************
*                                                                    *
* All rights reserved.                                               *
*                                                                    *
* * This software may in its unmodified form be freely redistributed *
*   in source form.                                                  *
* * The source code may be modified, provided the source code        *
*   retains the above copyright notice, this list of conditions and  *
*   the following disclaimer.                                        *
* * Modified versions of this software in source or linkable form    *
*   may not be distributed without prior consent of SEGGER.          *
*                                                                    *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND     *
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,  *
* THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A        *
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL               *
* SEGGER Microcontroller BE LIABLE FOR ANY DIRECT, INDIRECT,         *
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES           *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS    *
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS            *
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,       *
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING          *
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS *
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.       *
*                                                                    *
**********************************************************************
*                                                                    *
*       SystemView version: V2.34                                    *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File        : SEGGER_SYSVIEW_Config_FX3.c
Purpose     : Setup configuration of SystemView for FX3 Apps.
*/
#include <stdint.h>
#include <string.h>

#include <board.h>
#include <task.h>

#include "SEGGER_SYSVIEW.h"

// SystemcoreClock can be used in most CMSIS compatible projects.
// In non-CMSIS projects define SYSVIEW_CPU_FREQ.
//extern unsigned int SystemCoreClock;

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
// The application name to be displayed in SystemViewer
#define SYSVIEW_APP_NAME        "FX3 Application"

// The target device name
#define SYSVIEW_DEVICE_NAME     "Cortex-M4"

// Frequency of the timestamp. Must match SEGGER_SYSVIEW_Conf.h
#define SYSVIEW_TIMESTAMP_FREQ  (SystemCoreClock >> 4)

// System Frequency. SystemcoreClock is used in most CMSIS compatible projects.
#define SYSVIEW_CPU_FREQ        (SystemCoreClock)

// The lowest RAM address used for IDs (pointers)
#define SYSVIEW_RAM_BASE        (0x20000000)

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/
#define DWT_CTRL                  (*(volatile uint32_t*) (0xE0001000uL))  // DWT Control Register
#define NOCYCCNT_BIT              (1uL << 25)                           // Cycle counter support bit
#define CYCCNTENA_BIT             (1uL << 0)                            // Cycle counter enable bit

/********************************************************************* 
*
*       _cbSendSystemDesc()
*
*  Function description
*    Sends SystemView description strings.
*/
static void _cbSendSystemDesc(void)
{
   SEGGER_SYSVIEW_SendSysDesc("N="SYSVIEW_APP_NAME",D="SYSVIEW_DEVICE_NAME);
   SEGGER_SYSVIEW_SendSysDesc("I#15=SysTick");
   SEGGER_SYSVIEW_SendSysDesc("I#44=TIM2");
}

static U64 _cbGetTime(void)
{
   return bsp_getTimestamp64_ticks();
}

extern struct task_control_block idleTask;

static void _cbSendTaskList(void)
{
   struct task_control_block* tcb = idleTask.nextTaskInTheGreatLink;

   while (&idleTask != tcb)
   {
      SEGGER_SYSVIEW_TASKINFO taskInfo;
      memset(&taskInfo, 0, sizeof(taskInfo));

      taskInfo.TaskID    = (uint32_t) tcb,
      taskInfo.sName     = tcb->config->name,
      taskInfo.Prio      = tcb->config->priority,
      taskInfo.StackBase = (uint32_t) tcb->stackPointer,
      taskInfo.StackSize = tcb->config->stackSize,

      SEGGER_SYSVIEW_SendTaskInfo(&taskInfo);

      tcb = tcb->nextTaskInTheGreatLink;
   }
}

static const SEGGER_SYSVIEW_OS_API SYSVIEW_FX3_OS_TraceAPI =
{
  _cbGetTime,
  _cbSendTaskList,
};

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/
void SEGGER_SYSVIEW_Conf(void)
{
#if USE_CYCCNT_TIMESTAMP
   //
   //  The cycle counter must be activated in order
   //  to use time related functions.
   //
   if ((DWT_CTRL & NOCYCCNT_BIT) == 0)          // Cycle counter supported?
   {
      if ((DWT_CTRL & CYCCNTENA_BIT) == 0)      // Cycle counter not enabled?
      {
         DWT_CTRL |= CYCCNTENA_BIT;             // Enable Cycle counter
      }
   }
#endif

   SEGGER_SYSVIEW_Init(SYSVIEW_TIMESTAMP_FREQ, SYSVIEW_CPU_FREQ,
         &SYSVIEW_FX3_OS_TraceAPI, _cbSendSystemDesc);
   SEGGER_SYSVIEW_SetRAMBase(SYSVIEW_RAM_BASE);
}

/*************************** End of file ****************************/
