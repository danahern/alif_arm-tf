/* Copyright (C) 2023 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 *
 */
#include <platform_def.h>
#include <mhu.h>
#include <lib/mmio.h>
#include <common/debug.h>
#include <arch_helpers.h>
#include "services_lib_protocol.h"
#include "services_lib_ids.h"

#define AES_ENC_KEY_LEN                 16
/**
 * OSPI write key commands
 */
#define OSPI_WRITE_OTP_KEY_OSPI0        0
#define OSPI_WRITE_OTP_KEY_OSPI1        1
#define OSPI_WRITE_EXTERNAL_KEY_OSPI0   2
#define OSPI_WRITE_EXTERNAL_KEY_OSPI1   3
#define CH_ST                           0
#define CH_CLR                          0x8
#define CH_INT_ST                       0x10
#define CH_INT_ST0                      0xFA0
#define DELAY                           0xFFF00
#define SYNC_DELAY                      100000
#define READ_DELAY                      100
#define MAX_TRIES                       100

extern void delay_in_us(uint32_t delay);
service_header_t *heartbeat_req = (service_header_t *) MHU0_PAYLOAD_ADDR;
ospi_write_key_svc_t *ospi1_write_key = (ospi_write_key_svc_t *)
					MHU0_PAYLOAD_ADDR;
/* ASCII values of key '0123456789ABCDEF' or
 * '0', '1', '2', '3', etc.
 */
uint8_t aes_enc_key[AES_ENC_KEY_LEN] = AES_ENC_KEY;

/* @brief - Function to synchronize with SE by sending heartbeat service
 *          through MHU0.
 *          The function tries to sync with SE for MAX_TRIES iterations
 *          each iteration have timeout/delay in microseconds given by
 *          SYNC_DELAY.
 * returns,
 * 0  - Success. SE synchronization successful.
 * -1 - Failure. Unable to sync with SE even after MAX_TRIES.
 */
static int service_se_sync(void)
{
	uint8_t iteration = 0;

	/* Send heartbeat for synchronization */
	memset(heartbeat_req, 0x0, sizeof(service_header_t));
	heartbeat_req->hdr_service_id = SERVICE_MAINTENANCE_HEARTBEAT_ID;
	/* Initiate data/service transfer through MHU0 */
	mhu_secure_message_start(PLAT_SDK700_MHU0_SEND, CH_ID);
	/* Send the MHU message and wait for SE response at receiver channel*/
	do {
		if (iteration++ >= MAX_TRIES)
			break;
		/* send SE heartbeat service request */
		mhu_secure_message_send(PLAT_SDK700_MHU0_SEND, CH_ID,
					(uint32_t)heartbeat_req);
		dmb();

		/* Wait for SE to send response for the message sent */
		delay_in_us(3 * SYNC_DELAY);
	} while (((mmio_read_32(PLAT_SDK700_MHU0_SEND + CH_INT_ST0) &
		(1 << CH_ID)) == 0x0) ||
		((mmio_read_32(PLAT_SDK700_MHU0_SEND + CH_INT_ST) &
		(1 << CH_ID)) == 0x0) ||
		(mmio_read_32(PLAT_SDK700_MHU0_SEND + CH_ST) != 0));

	/* MAX_TRIES expired return failure */
	if (iteration >= MAX_TRIES) {
		ERROR("Unable to send sync messages to SE\n");
		mhu_secure_message_end(PLAT_SDK700_MHU0_SEND, 0);
		return -1;
	}
	/* Delay so the SE populates the response */
	delay_in_us(READ_DELAY);
	mmio_write_32(PLAT_SDK700_MHU0_RECV + CH_CLR, 0xFFFFFFFF);

	INFO("Sync with SE successful\n");
	return 0;
}

/* @brief Function which sends SE service request through MHU0
 *        to write 16bytes AES keys to AES decoder register in order
 *        decrypt encrypted images from the OSPI1 NOR flash on the fly.
 * returns,
 * 0   - Success. SE service request to write AES keys is successful.
 * -1  - Failure. SE service request to write AES keys is unsuccessful.
 */
int service_ospi_write_aes_key(void)
{
	uint8_t temp;

	/* Sync with SE */
	if (service_se_sync()) {
		ERROR("service_se_sync failed\n");
		return -1;
	}

	/* Write external key into OSPI1 AES decoder */
	for (int i = 0; i < (AES_ENC_KEY_LEN>>1) ; ++i) {
		temp = aes_enc_key[i];
		aes_enc_key[i] = aes_enc_key[AES_ENC_KEY_LEN - i - 1];
		aes_enc_key[AES_ENC_KEY_LEN - i - 1] = temp;
	}
	memset(ospi1_write_key, 0x0, sizeof(ospi_write_key_svc_t));
	ospi1_write_key->send_command = OSPI_WRITE_EXTERNAL_KEY_OSPI1;
	memcpy((void *)ospi1_write_key->send_key, aes_enc_key, AES_ENC_KEY_LEN);
	ospi1_write_key->header.hdr_service_id =
			SERVICE_APPLICATION_OSPI_WRITE_KEY_ID;
	mhu_secure_message_send(PLAT_SDK700_MHU0_SEND, CH_ID,
				(uint32_t) ospi1_write_key);
	dmb();

	/* Delay to make sure SE service request is sent successfully */
	delay_in_us(READ_DELAY);

	/* Fail to send SE service request */
	if (((mmio_read_32(PLAT_SDK700_MHU0_SEND + CH_INT_ST0) &
	     (1 << CH_ID)) == 0x0) ||
	    ((mmio_read_32(PLAT_SDK700_MHU0_SEND + CH_INT_ST) &
	     (1 << CH_ID)) == 0x0) ||
	    (mmio_read_32(PLAT_SDK700_MHU0_SEND + CH_ST) != 0)) {
		ERROR("Unable to send OSPI1 key ID\n");
		mhu_secure_message_end(PLAT_SDK700_MHU0_SEND, 0);
		return -1;
	}

	/* Delay to allow SE populate response */
	delay_in_us(READ_DELAY);
	mmio_write_32(PLAT_SDK700_MHU0_RECV + CH_CLR, 0xFFFFFFFF);

	/* Channel 0 is cleared */
	mhu_secure_message_end(PLAT_SDK700_MHU0_SEND, 0);

	/* Delay needed for setup to work */
	delay_in_us(SYNC_DELAY / 2);

	INFO("AES encryption key wrote to decryption register\n");
	return 0;
}

/* USB PHY power gating bit in AIPM run profile phy_pwr_gating field */
#define USB_PHY_MASK                    (1 << 1)

/* VBAT power control register — CLEAR bits to enable */
#define VBAT_PWR_CTRL                   0x1A609008
#define VBAT_UPHY_PWR_MASK              (1 << 16)  /* PHY power */
#define VBAT_UPHY_ISO                   (1 << 17)  /* PHY isolation */

/* USB PHY power-on-reset — CLEAR bit to release POR */
#define CLKCTL_USB_CTRL2                0x4903F0AC
#define USB_CTRL2_PHY_POR               (1 << 8)

/* @brief Function which sends SE AIPM service requests through MHU0
 *        to enable USB PHY power gating. Uses two separate MHU cycles:
 *        Cycle 1: GET current run profile from SE
 *        Cycle 2: SET modified profile with USB_PHY_MASK enabled
 *
 *        Each cycle follows the proven sync→request→end pattern used by
 *        service_ospi_write_aes_key(). The GET response is saved to a
 *        local struct since sync overwrites the MHU payload buffer.
 *
 *        Without this call, the SE keeps USB PHY power gated and the
 *        DWC3 USB controller cannot drive the bus.
 * returns,
 * 0   - Success.
 * -1  - Failure.
 */
int service_enable_usb_phy(void)
{
	aipm_get_run_profile_svc_t *get_req =
		(aipm_get_run_profile_svc_t *) MHU0_PAYLOAD_ADDR;
	aipm_set_run_profile_svc_t *set_req =
		(aipm_set_run_profile_svc_t *) MHU0_PAYLOAD_ADDR;
	aipm_get_run_profile_svc_t saved;

	/* === Cycle 1: GET current run profile === */
	if (service_se_sync()) {
		ERROR("service_se_sync failed for AIPM GET\n");
		return -1;
	}

	memset(get_req, 0x0, sizeof(aipm_get_run_profile_svc_t));
	get_req->header.hdr_service_id = SERVICE_POWER_GET_RUN_REQ_ID;
	mhu_secure_message_send(PLAT_SDK700_MHU0_SEND, CH_ID,
				(uint32_t) get_req);
	dmb();

	delay_in_us(SYNC_DELAY);

	if (((mmio_read_32(PLAT_SDK700_MHU0_SEND + CH_INT_ST0) &
	     (1 << CH_ID)) == 0x0) ||
	    ((mmio_read_32(PLAT_SDK700_MHU0_SEND + CH_INT_ST) &
	     (1 << CH_ID)) == 0x0) ||
	    (mmio_read_32(PLAT_SDK700_MHU0_SEND + CH_ST) != 0)) {
		ERROR("AIPM GET_RUN failed\n");
		mhu_secure_message_end(PLAT_SDK700_MHU0_SEND, 0);
		return -1;
	}

	delay_in_us(READ_DELAY);
	mmio_write_32(PLAT_SDK700_MHU0_RECV + CH_CLR, 0xFFFFFFFF);

	/* Save response before ending channel (next sync overwrites buffer) */
	memcpy(&saved, get_req, sizeof(saved));

	INFO("AIPM GET_RUN: phy_pwr_gating=0x%x\n",
	     saved.resp_phy_pwr_gating);

	mhu_secure_message_end(PLAT_SDK700_MHU0_SEND, 0);
	delay_in_us(SYNC_DELAY / 2);

	/* === Cycle 2: SET run profile with USB PHY enabled === */
	if (service_se_sync()) {
		ERROR("service_se_sync failed for AIPM SET\n");
		return -1;
	}

	/* Copy saved GET response as SET request (identical field layout) */
	memcpy(set_req, &saved, sizeof(aipm_set_run_profile_svc_t));
	set_req->send_phy_pwr_gating |= USB_PHY_MASK;
	set_req->header.hdr_service_id = SERVICE_POWER_SET_RUN_REQ_ID;
	set_req->header.hdr_flags = 0;
	set_req->header.hdr_error_code = 0;
	set_req->header.hdr_padding = 0;
	mhu_secure_message_send(PLAT_SDK700_MHU0_SEND, CH_ID,
				(uint32_t) set_req);
	dmb();

	delay_in_us(SYNC_DELAY);

	if (((mmio_read_32(PLAT_SDK700_MHU0_SEND + CH_INT_ST0) &
	     (1 << CH_ID)) == 0x0) ||
	    ((mmio_read_32(PLAT_SDK700_MHU0_SEND + CH_INT_ST) &
	     (1 << CH_ID)) == 0x0) ||
	    (mmio_read_32(PLAT_SDK700_MHU0_SEND + CH_ST) != 0)) {
		ERROR("AIPM SET_RUN failed\n");
		mhu_secure_message_end(PLAT_SDK700_MHU0_SEND, 0);
		return -1;
	}

	delay_in_us(READ_DELAY);
	mmio_write_32(PLAT_SDK700_MHU0_RECV + CH_CLR, 0xFFFFFFFF);

	mhu_secure_message_end(PLAT_SDK700_MHU0_SEND, 0);
	delay_in_us(SYNC_DELAY / 2);

	INFO("AIPM: USB PHY power enabled (phy_pwr_gating |= 0x%x)\n",
	     USB_PHY_MASK);

	/* === VBAT power control: enable PHY power, disable isolation === */
	mmio_clrbits_32(VBAT_PWR_CTRL, VBAT_UPHY_PWR_MASK | VBAT_UPHY_ISO);

	/* === Release PHY power-on-reset === */
	mmio_clrbits_32(CLKCTL_USB_CTRL2, USB_CTRL2_PHY_POR);

	INFO("USB PHY: VBAT power enabled, PHY POR released\n");
	return 0;
}

