#
# Copyright (c) 2019, ARM Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

$(eval $(call add_define,AES_EN))
$(eval $(call add_define,FLASH_EN))
$(eval $(call add_define,HYPRAM_EN))
$(eval $(call add_define,MODEM_SRAM))
$(eval $(call add_define,OSPI_NO_XIP_SER))
$(eval $(call add_define,UART))
$(eval $(call add_define,USB_INIT_HALT))

ifeq "1" "${AES_EN}"
ifneq "1" "${FLASH_EN}"
$(error AES_EN is set to 1 but not FLASH_EN. Set FLASH_EN to 1)
endif
ifeq "" "${AES_ENC_KEY}"
$(error AES_EN is set to 1 but AES_ENC_KEY is empty.Set AES key in AES_ENC_KEY)
endif
endif
CPPFLAGS 		+= -DAES_ENC_KEY=\"${AES_ENC_KEY}\"
DEVKIT_E7_CPU_SOURCES	+=	lib/cpus/aarch32/cortex_a32.S

BL32_SOURCES		+=      plat/arm/board/corstone700/drivers/mhu/mhu.c \
				plat/arm/board/$(PLAT)/drivers/ospi/ospi_drv.c \
				plat/arm/board/$(PLAT)/ospi_flash/norflash_ospi_setup.c \
				plat/arm/board/$(PLAT)/drivers/ospi/ospi.c \
				plat/arm/board/$(PLAT)/drivers/ospi/ospi_hyperram_xip.c \
				plat/arm/board/$(PLAT)/drivers/ospi/ospi_hram_reg_access.c \
				plat/arm/board/$(PLAT)/ospi_hyperram/ospi_hyperram_xip_setup.c \
				plat/arm/board/$(PLAT)/devkit_e7_plat.c \
				plat/arm/board/$(PLAT)/se_service/services.c \
				drivers/ti/uart/aarch32/16550_console.S

PLAT_INCLUDES		:=      -Iplat/arm/board/$(PLAT)/include \
				-Iinclude/plat/arm/common/ \
				-Iplat/arm/board/corstone700/drivers/mhu/ \
				-Iplat/arm/board/$(PLAT)/drivers/include \
				-Iplat/arm/board/$(PLAT)/ospi_flash/ \
				-Iplat/arm/board/$(PLAT)/ospi_hyperram

NEED_BL32		:=	yes

DEVKIT_E7_GIC_SOURCES	:=	drivers/arm/gic/common/gic_common.c     \
				drivers/arm/gic/v2/gicv2_main.c         \
				drivers/arm/gic/v2/gicv2_helpers.c      \
				plat/common/plat_gicv2.c                \
				plat/arm/common/arm_gicv2.c

# BL1/BL2 Image not a part of the capsule Image for corstone700
override NEED_BL1	:=	no
override NEED_BL2	:=	no
override NEED_BL2U	:=	no

#TFA for corstone700 starts from BL32
override RESET_TO_SP_MIN	:=	1

include plat/arm/board/common/board_common.mk
include plat/arm/common/arm_common.mk
