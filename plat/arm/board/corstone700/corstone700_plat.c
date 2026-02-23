/*
 * Copyright (c) 2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <plat/arm/common/arm_def.h>
#include <common/bl_common.h>
#include <plat/arm/common/plat_arm.h>
#include <plat/common/platform.h>
#include <platform_def.h>
#include <mhu.h>

/*
 * Table of regions to map using the MMU.
 * Replace or extend the below regions as required
 */

const mmap_region_t plat_arm_mmap[] = {
	ARM_MAP_SHARED_RAM,
	ARM_MAP_NS_DRAM1,
	UART_MAP_DEVICE,
	CORSTONE700_MAP_DEVICE,
	OSPI0_MAP_DEVICE,
	AES0_MAP_DEVICE,
	OSPI1_MAP_DEVICE,
	AES1_MAP_DEVICE,
	SE_MHU0_SEND_DEVICE,
	SE_MHU0_RECV_DEVICE,
	MHU0_PAYLOAD_MAP,
	LPGPIO_MAP_DEVICE,
#if FLASH_EN
	OSPI1_XIP_MAP,
#endif
	{0}
};

/* Nothing to do*/
void __init plat_arm_pwrc_setup(void)
{
	mhu_secure_init();
}

#ifndef PLAT_DEVKIT_E7
unsigned int plat_get_syscnt_freq2(void)
{
	return FPGA_TIMER_BASE_FREQUENCY;
}
#else
unsigned int plat_get_syscnt_freq2(void)
{
	/* Returns 100Mhz as base frequency of system timer */
	return DEVKIT_E7_TIMER_BASE_FREQUENCY;
}
#endif
