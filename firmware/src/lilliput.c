/* SPDX-License-Identifier: GPL-3.0-only
 * LILLIPUT -- a firmware for the M-VAVE FM-1: a no-input mixer. Six channels, their loops, a rack of effects.
 * One compilation unit (the HAL is header-only); the order matters.
 *
 * After NoInputMixer (Igor Vasiliev), its signal flow and controls, written anew for the FM-1. Built on TONITOR
 * (Nuno Zimas: the screen, the panel's ways, the sessions), itself on Felucca (Leo Kuroshita,
 * Hügelton Instruments) and SLOOP (isod89), GPL-3.0: the HAL, the boot, the USB stack, the update entry and loader,
 * the rescue mode; CPU1's worker from Melodee (Kerem Kilic). */
#include <stdint.h>
#ifndef LILLIPUT_ID
#define LILLIPUT_ID "FM-1_800"   /* the package identity (tools/build.py sets it) */
#endif
#ifndef LILLIPUT_VERSION
#define LILLIPUT_VERSION "1"
#endif
#define FELUCCA_ID LILLIPUT_ID
#ifndef TELEMETRY
#define TELEMETRY 0
#endif
#define FELUCCA_OTA 1            /* the update entry (the installer), and the rescue mode */
#define FELUCCA_OTA_DRYRUN 0
#define FELUCCA_CDC 0
#define FELUCCA_UAC 1            /* USB audio out to the host (usb.c) */
#define UAC_NBLK 8u              /* (USB audio's block: the output L R, then channels 1 .. 6 mono) */
#define UAC_NMAX 6u
#define FELUCCA_UAC_48K 1
#define HALF_FRAMES 256u         /* an I2S half buffer: 5.8 ms */
#define FM1_CORE1 1              /* CPU1 runs the output stage (hal/fm1_multicore.h; after BIGORNA and Melodee) */

#include "fm1_time.h"
#include "fm1_sys.h"
#include "fm1_power.h"
#include "fm1_irq.h"
#include "fm1_core1_bar.h"
#include "fm1_multicore.h"
#include "fm1_guard.h"
#include "fm1_input.h"
#include "fm1_timer.h"
#include "fm1_audio.h"
#include "fm1_adc.h"
#include "fm1_lcd_hw.h"
#include "tonitor_tables.h"

#define NELEM(a) (sizeof(a) / sizeof((a)[0]))
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
static volatile uint32_t fm1_ms;                           /* milliseconds since boot (TIMER5) */
#include "bootguard.h"
bootguard_t bootguard __attribute__((section(".noinit")));

#include "libc.c"
#include "lcd.c"
#include "dsp.h"
#include "gfx.c"
#include "logo.c"
#include "usb.c"
#include "midi_uart.c"
#include "reverb.c"
#define NLAYER 6
#include "midi.c"
#undef NLAYER
#include "mixer.c"
#include "patch.c"
#include "rhythm.c"
#include "lui.c"
#include "lses.c"
#include "setup.c"
#include "backup.c"
#include "main.c"
