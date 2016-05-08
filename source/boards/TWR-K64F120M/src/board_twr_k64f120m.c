/**
 * @file board_twr_k64f120m.c
 * @brief Board Support Package implementation for TWR-K64F120M
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

#include <board.h>

#include <kinetis_chp.h>

#include <fsl_common.h>
#include <fsl_smc.h>
#include <fsl_port.h>
#include <fsl_gpio.h>

static void initializeLEDs(void)
{
   // Enable GPIO port clock
   CLOCK_EnableClock(kCLOCK_PortE);

   PORT_SetPinMux(PORTE, 6 + LED_ID_GREEN, kPORT_MuxAsGpio);
   PORT_SetPinMux(PORTE, 6 + LED_ID_ORANGE, kPORT_MuxAsGpio);
   PORT_SetPinMux(PORTE, 6 + LED_ID_RED, kPORT_MuxAsGpio);
   PORT_SetPinMux(PORTE, 6 + LED_ID_BLUE, kPORT_MuxAsGpio);

   /* Init output LED GPIO. */
   GPIOE->PCOR |= (1 << (6 + LED_ID_GREEN))
      | (1 << (6 + LED_ID_ORANGE))
      | (1 << (6 + LED_ID_RED))
      | (1 << (6 + LED_ID_BLUE));

   GPIOE->PDDR |= (1 << (6 + LED_ID_GREEN))
      | (1 << (6 + LED_ID_ORANGE))
      | (1 << (6 + LED_ID_RED))
      | (1 << (6 + LED_ID_BLUE));
}

#define BOARD_XTAL0_CLK_HZ 50000000U
#define BOARD_XTAL32K_CLK_HZ 32768U

typedef struct _clock_config
{
    mcg_config_t mcgConfig;       /*!< MCG configuration.      */
    sim_clock_config_t simConfig; /*!< SIM configuration.      */
    osc_config_t oscConfig;       /*!< OSC configuration.      */
    uint32_t coreClock;           /*!< core clock frequency.   */
} clock_config_t;

/* Configuration for enter RUN mode. Core clock = 120MHz. */
const clock_config_t g_defaultClockConfigRun =
{
   .mcgConfig =
   {
      .mcgMode = kMCG_ModePEE,             /* Work in PEE mode */
      .irclkEnableMode = kMCG_IrclkEnable, /* MCGIRCLK enable */
      .ircs = kMCG_IrcSlow,                /* Select IRC32k */
      .fcrdiv = 0U,                        /* FCRDIV is 0 */
      .frdiv = 7U,
      .drs = kMCG_DrsLow,         /* Low frequency range */
      .dmx32 = kMCG_Dmx32Default, /* DCO has a default range of 25% */
      .oscsel = kMCG_OscselOsc,   /* Select OSC */
      .pll0Config =
      {
         .enableMode = 0U, .prdiv = 0x13U, .vdiv = 0x18U,
      },
   },
   .simConfig =
   {
      .pllFllSel = 1U,        /* PLLFLLSEL select PLL */
      .er32kSrc = 2U,         /* ERCLK32K selection, use RTC */
      .clkdiv1 = 0x01140000U, /* SIM_CLKDIV1 */
   },
   .oscConfig = {.freq = BOARD_XTAL0_CLK_HZ,
      .capLoad = 0,
      .workMode = kOSC_ModeExt,
      .oscerConfig =
      {
         .enableMode = kOSC_ErClkEnable,
#if (defined(FSL_FEATURE_OSC_HAS_EXT_REF_CLOCK_DIVIDER) && FSL_FEATURE_OSC_HAS_EXT_REF_CLOCK_DIVIDER)
         .erclkDiv = 0U,
#endif
      }},
   .coreClock = 120000000U, /* Core clock frequency */
};

extern uint32_t SystemCoreClock;

static void configureClocks(void)
{
   CLOCK_SetSimSafeDivs();

   CLOCK_InitOsc0(&g_defaultClockConfigRun.oscConfig);
   CLOCK_SetXtal0Freq(BOARD_XTAL0_CLK_HZ);

   CLOCK_BootToPeeMode(g_defaultClockConfigRun.mcgConfig.oscsel, kMCG_PllClkSelPll0,
         &g_defaultClockConfigRun.mcgConfig.pll0Config);

   CLOCK_SetInternalRefClkConfig(g_defaultClockConfigRun.mcgConfig.irclkEnableMode,
         g_defaultClockConfigRun.mcgConfig.ircs, g_defaultClockConfigRun.mcgConfig.fcrdiv);

   CLOCK_SetSimConfig(&g_defaultClockConfigRun.simConfig);

   SystemCoreClock = g_defaultClockConfigRun.coreClock;
}

void bsp_initialize(void)
{
   configureClocks();

   chp_initialize();

   NVIC_SetPriority(PendSV_IRQn, 0xFF); // Set PendSV to lowest possible priority

   initializeLEDs();
}

void bsp_turnOnLED(uint32_t ledId)
{
   if (LED_COUNT > ledId)
   {
      GPIOE->PCOR |= (1 << (6 + ledId));
   }
}

void bsp_turnOffLED(uint32_t ledId)
{
   if (LED_COUNT > ledId)
   {
      GPIOE->PSOR |= (1 << (6 + ledId));
   }
}

