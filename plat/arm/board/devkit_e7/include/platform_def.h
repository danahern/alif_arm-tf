/*
 * Copyright (c) 2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <plat/arm/common/arm_def.h>
#include <plat/arm/common/arm_spm_def.h>
#include <plat/common/common_def.h>
#include <lib/utils_def.h>
#include <plat/arm/board/common/v2m_def.h>

#define PLAT_ARM_CORSTONE700
#define PLAT_DEVKIT_E7

#define CORSTONE700_MAX_CPUS_PER_CLUSTER	2
#define PLAT_ARM_CLUSTER_COUNT		1
#define CORSTONE700_MAX_PE_PER_CPU	1
#define PLATFORM_CORE_COUNT		(PLAT_ARM_CLUSTER_COUNT *       \
					CORSTONE700_MAX_CPUS_PER_CLUSTER *   \
					CORSTONE700_MAX_PE_PER_CPU)

#define KILOBYTES 1024
#define PRELOADED_DTB_SIZE (48*KILOBYTES)

/* DevKit Generic Timer Frequency */
#define DEVKIT_E7_TIMER_BASE_FREQUENCY  100000000 /* 100Mhz */

#define PLAT_MAX_PWR_LVL		2

#define PLAT_ARM_TRUSTED_MAILBOX_BASE	ARM_TRUSTED_SRAM_BASE
#define PLAT_ARM_NSTIMER_FRAME_ID	U(1)

#define PLAT_ARM_BOOT_UART_BASE		0x1a510000
#define PLAT_ARM_BOOT_UART_CLK_IN_HZ	V2M_IOFPGA_UART0_CLK_IN_HZ
#define PLAT_ARM_RUN_UART_BASE		0x1a520000
#define PLAT_ARM_RUN_UART_CLK_IN_HZ	V2M_IOFPGA_UART1_CLK_IN_HZ

#define PLAT_ARM_NS_IMAGE_OFFSET	(ARM_DRAM1_BASE + UL(0x8000000))

/* CORSTONE700 Power controller base address*/
#define PWRC_BASE		UL(0x1A020000)
#define PSYSR_WK_SHIFT		24
#define PSYSR_WK_WIDTH		0x2
#define PSYSR_WK_MASK		((1U << PSYSR_WK_WIDTH) - 1U)
#define PSYSR_WK(x)		((x) >> PSYSR_WK_SHIFT) & PSYSR_WK_MASK
#define WKUP_COLD		U(0x0)
#define WKUP_RESET		U(0x1)
#define WKUP_PPONR		U(0x2)
#define WKUP_GICREQ		U(0x3)
#define PPOFFR_OFF		U(0x0)
#define PPONR_OFF		U(0x4)
#define PCOFFR_OFF		U(0x8)
#define PWKUPR_OFF		U(0xc)
#define PSYSR_OFF		U(0x10)
#define PWKUPR_WEN		BIT_32(31)
#define PSYSR_AFF_L2		BIT_32(31)
#define PSYSR_AFF_L1		BIT_32(30)
#define PSYSR_AFF_L0		BIT_32(29)
#define PSYSR_WEN		BIT_32(28)
#define PSYSR_PC		BIT_32(27)
#define PSYSR_PP		BIT_32(26)

/* MODEM SRAM Power Control */
#define PWR_CTRL		UL(0x1A60A004)
#define MDM_PWR_CTRL		UL(0x1A60B004)
#define MDM_CLK_SEL		UL(0x1A605040)
#define MDM_CPU_CTRL		UL(0x1A605048)
#define MDM_PD_CLK_PLL		UL(0x1A60504C)

# define PLAT_ARM_MMAP_ENTRIES          18
/* Set MAX_XLAT_TABLES to 12 in order to solve assertion failure at
 * ASSERT: lib/xlat_tables_v2/xlat_tables_core.c:97 */
# define MAX_XLAT_TABLES                13
#if MODEM_SRAM
#define PLAT_ARM_TRUSTED_SRAM_SIZE	UL(0x00029000)  /* 164 KB */
#else
#define PLAT_ARM_TRUSTED_SRAM_SIZE	UL(0x00028000)  /* 160 KB */
#endif

/* The remaining Trusted SRAM is used to load the BL images */
#define ARM_BL_RAM_BASE			(ARM_SHARED_RAM_BASE +  \
					ARM_SHARED_RAM_SIZE)
#define ARM_BL_RAM_SIZE			(PLAT_ARM_TRUSTED_SRAM_SIZE -   \
					ARM_SHARED_RAM_SIZE)
#define PLATFORM_STACK_SIZE		UL(0x440)

#define PLAT_ARM_CRASH_UART_BASE	PLAT_ARM_RUN_UART_BASE

#define PLAT_ARM_CRASH_UART_CLK_IN_HZ	PLAT_ARM_RUN_UART_CLK_IN_HZ

#define CORSTONE700_DEVICE_BASE		(0x1A000000)
#define CORSTONE700_DEVICE_SIZE		(0x26000000)
#define CORSTONE700_MAP_DEVICE                       MAP_REGION_FLAT(        \
						CORSTONE700_DEVICE_BASE,      \
						CORSTONE700_DEVICE_SIZE,      \
						MT_DEVICE | MT_RW | MT_SECURE)

/* EXPMST0 Control Register */
#define EXPMST0_CTRL_REG		(0x4902F000)
/* UART Control Register */
#define UART_CTRL_REG			(0x4902F008)
#if UART == 4
/* PINMUX address for UART4_B RX and TX */
#define PINMUX_UART_RX_ADDR		(0x1A603184)
#define PINMUX_UART_TX_ADDR		(0x1A603188)
/* UART4_B RX and TX selection values */
#define PINMUX_UART_RX_VAL		(0x00230002)
#define PINMUX_UART_TX_VAL		(0x00230002)
#define UART_BASE_ADDR			(0x4901C000)
#elif UART == 2
/* PINMUX address for UART2_A RX and TX */
#define PINMUX_UART_RX_ADDR		(0x1A603020)
#define PINMUX_UART_TX_ADDR		(0x1A603024)
/* UART2_A RX and TX selection values */
#define PINMUX_UART_RX_VAL		(0x00230001)
#define PINMUX_UART_TX_VAL		(0x00230001)
#define UART_BASE_ADDR			(0x4901A000)
#else
#error "Set UART with an appropriate value. Example: Either 2 or 4."
#endif

#if MODEM_SRAM
/* Map SRAM-6B 1MB */
#define SRAM6B_BASE_ADDR		(0x62400000)
#define SRAM6B_SIZE			(0x100000)
#define MAP_SRAM6B			MAP_REGION_FLAT(		\
					SRAM6B_BASE_ADDR,		\
					SRAM6B_SIZE,			\
					MT_MEMORY | MT_RW | MT_SECURE)
#endif

/* Map 4MB */
#define SRAM0_BASE_ADDR			(0x02000000)
#define SRAM0_SIZE			(0x400000)
#define MAP_SRAM0			MAP_REGION_FLAT(		\
					SRAM0_BASE_ADDR,		\
					SRAM0_SIZE,			\
					MT_MEMORY | MT_RW | MT_SECURE)

#define UART_SIZE			(0x1000)
#define UART_CLOCK_FREQ			100000000
#define UART_BAUDRATE			115200

#define UART_MAP_DEVICE			MAP_REGION_FLAT(		\
						UART_BASE_ADDR,		\
						UART_SIZE,              \
						MT_DEVICE | MT_RW | MT_SECURE)

#define OSPI0_BASE_ADDR			(0x83000000)
#define OSPI0_SIZE			(0x1000)
#define OSPI0_MAP_DEVICE		MAP_REGION_FLAT(		\
						OSPI0_BASE_ADDR, 	\
						OSPI0_SIZE,             \
						MT_DEVICE | MT_RW | MT_SECURE)

#define LPGPIO_MAP_DEVICE		MAP_REGION_FLAT(		\
						0x42002000,		\
						0x1000,			\
						MT_DEVICE | MT_RW | MT_SECURE)

/* CLKCTL_PER_MST: USB_CTRL2 at 0x4903F0AC, PERIPH_CLK_ENA at 0x4903F00C */
#define CLKCTL_PER_MST_BASE		(0x4903F000)
#define CLKCTL_PER_MST_MAP		MAP_REGION_FLAT(		\
						CLKCTL_PER_MST_BASE,	\
						0x1000,			\
						MT_DEVICE | MT_RW | MT_SECURE)


#if HYPRAM_EN
/* Map 4MB for copying the DTB */
#define HYPERRAM_BASE_ADDR		(0xA0000000)
#define HYPERRAM_SIZE			(0x400000)
#define MAP_HYPERRAM			MAP_REGION_FLAT(		\
					HYPERRAM_BASE_ADDR,		\
					HYPERRAM_SIZE,			\
					MT_MEMORY | MT_RW | MT_SECURE)
#endif

#if FLASH_EN
#define OSPI1_XIP_ADDR			(0xC0000000)
#define OSPI1_XIP_SIZE			(0x04000000)
#define OSPI1_XIP_MAP			MAP_REGION_FLAT(		\
						OSPI1_XIP_ADDR,		\
						OSPI1_XIP_SIZE,		\
						MT_MEMORY | MT_RO | MT_NS)
#endif

#define AES0_BASE_ADDR			(0x83001000)
#define AES0_SIZE			(0x1000)
#define AES0_MAP_DEVICE			MAP_REGION_FLAT(		\
						AES0_BASE_ADDR, 	\
						AES0_SIZE,              \
						MT_DEVICE | MT_RW | MT_SECURE)

#define OSPI1_BASE_ADDR			(0x83002000)
#define OSPI1_SIZE			(0x1000)
#define OSPI1_MAP_DEVICE		MAP_REGION_FLAT(		\
						OSPI1_BASE_ADDR,	\
						OSPI1_SIZE,		\
						MT_DEVICE | MT_RW | MT_SECURE)

#define AES1_BASE_ADDR			(0x83003000)
#define AES1_SIZE			(0x1000)
#define AES1_MAP_DEVICE			MAP_REGION_FLAT(		\
						AES1_BASE_ADDR,		\
						AES1_SIZE,		\
						MT_DEVICE | MT_RW | MT_SECURE)

#define SE_MHU0_SEND_ADDR               (0x1B800000)
#define MHU0_SIZE                       (0x1000)
#define SE_MHU0_SEND_DEVICE             MAP_REGION_FLAT(		\
						SE_MHU0_SEND_ADDR,	\
						MHU0_SIZE,		\
						MT_DEVICE | MT_RW | MT_SECURE)

#define SE_MHU0_RECV_ADDR               (0x1B810000)
#define SE_MHU0_RECV_DEVICE             MAP_REGION_FLAT(		\
						SE_MHU0_RECV_ADDR,	\
						MHU0_SIZE,              \
						MT_DEVICE | MT_RW | MT_SECURE)

/* SRAM0 memory 0x02380000 - 0x02380FFF is used for MHU0 */
/* communication with SE.*/
#if MODEM_SRAM
#define MHU0_PAYLOAD_ADDR                       0x08020000
#else
#define MHU0_PAYLOAD_ADDR                       0x02380000
#endif

#define MHU0_PAYLOAD_MAP                MAP_REGION_FLAT(		\
						MHU0_PAYLOAD_ADDR,	\
						0x1000,			\
						MT_DEVICE | MT_RW | MT_SECURE)

/* GIC related constants */
#define PLAT_ARM_GICD_BASE              0x1C010000
#define PLAT_ARM_GICC_BASE              0x1C02F000


/* MHUv2 Secure Channel receiver and sender */
#define PLAT_SDK700_MHU0_SEND            0x1B800000
#define PLAT_SDK700_MHU0_RECV            0x1B810000
#define CH_ID                            0

#define CORSTONE700_IRQ_TZ_WDOG		64
#define CORSTONE700_IRQ_SEC_SYS_TIMER	65
/*
 * Define a list of Group 1 Secure and Group 0 interrupts as per GICv3
 * terminology. On a GICv2 system or mode, the lists will be merged and treated
 * as Group 0 interrupts.
 */
#define PLAT_ARM_G1S_IRQ_PROPS(grp) \
	ARM_G1S_IRQ_PROPS(grp), \
	INTR_PROP_DESC(CORSTONE700_IRQ_TZ_WDOG, GIC_HIGHEST_SEC_PRIORITY, \
			(grp), GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(CORSTONE700_IRQ_SEC_SYS_TIMER, \
			GIC_HIGHEST_SEC_PRIORITY, (grp), GIC_INTR_CFG_LEVEL)

#define PLAT_ARM_G0_IRQ_PROPS(grp)      ARM_G0_IRQ_PROPS(grp)
