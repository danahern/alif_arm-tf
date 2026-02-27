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

/* Stub: test if any binary change breaks boot */
int service_enable_usb_phy(void)
{
	return 0;
}

