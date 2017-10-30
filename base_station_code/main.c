/***********************************************************************************************//**
 * \file   main.c
 * \brief  Silicon Labs Empty Example Project
 *
 * This example demonstrates the bare minimum needed for a Blue Gecko C application
 * that allows Over-the-Air Device Firmware Upgrading (OTA DFU). The application
 * starts advertising after boot and restarts advertising after a connection is closed.
 ***************************************************************************************************
 * <b> (C) Copyright 2016 Silicon Labs, http://www.silabs.com</b>
 ***************************************************************************************************
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 **************************************************************************************************/

/* Board headers */
#include "boards.h"
#include "ble-configuration.h"
#include "board_features.h"

/* Bluetooth stack headers */
#include "bg_types.h"
#include "native_gecko.h"
#include "gatt_db.h"
#include "aat.h"

/* Libraries containing default Gecko configuration values */
#include "em_emu.h"
#include "em_cmu.h"
#ifdef FEATURE_BOARD_DETECTED
#include "bspconfig.h"
#include "pti.h"
#endif

/* Device initialization header */
#include "InitDevice.h"

#ifdef FEATURE_SPI_FLASH
#include "em_usart.h"
#include "mx25flash_spi.h"
#endif /* FEATURE_SPI_FLASH */

/***********************************************************************************************//**
 * @addtogroup Application
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup app
 * @{
 **************************************************************************************************/

#ifndef MAX_CONNECTIONS
#define MAX_CONNECTIONS 4
#endif
uint8_t bluetooth_stack_heap[DEFAULT_BLUETOOTH_HEAP(MAX_CONNECTIONS)];

#ifdef FEATURE_PTI_SUPPORT
static const RADIO_PTIInit_t ptiInit = RADIO_PTI_INIT;
#endif

/* Gecko configuration parameters (see gecko_configuration.h) */
static const gecko_configuration_t config = { .config_flags = 0, .sleep.flags =
		SLEEP_FLAGS_DEEP_SLEEP_ENABLE, .bluetooth.max_connections =
		MAX_CONNECTIONS, .bluetooth.heap = bluetooth_stack_heap,
		.bluetooth.heap_size = sizeof(bluetooth_stack_heap),
		.bluetooth.sleep_clock_accuracy = 100, // ppm
		.gattdb = &bg_gattdb_data, .ota.flags = 0, .ota.device_name_len = 3,
		.ota.device_name_ptr = "OTA",
#ifdef FEATURE_PTI_SUPPORT
		.pti = &ptiInit,
#endif
	};

/* Flag for indicating DFU Reset must be performed */
uint8_t boot_to_dfu = 0;

int _write_r(struct _reent *r, int fd, const void *data, unsigned int count) {
	char *c = (char *) data;
	for (unsigned int i = 0; i < count; i++) {
		USART_Tx(USART0, c[i]);
	}
	return count;
}

static uint8 *i_data;
static uint8 *val_data;
/**
 * @brief  Main function
 */
void main(void) {
#ifdef FEATURE_SPI_FLASH
	/* Put the SPI flash into Deep Power Down mode for those radio boards where it is available */
	MX25_init();
	MX25_DP();
	/* We must disable SPI communication */
	USART_Reset(USART1);

#endif /* FEATURE_SPI_FLASH */

	/* Initialize peripherals */
	enter_DefaultMode_from_RESET();

	/* Initialize stack */
	gecko_init(&config);
	uint8 _conn_handle;
	uint16 _handle_characteristic;
	uint32 _handle_service;
	uint8array _uuid;
	uint16 _result;
	int _timerflag =0;
	int _finishflag =0;

	//Mobile Device bluetooth address
	bd_addr address = {addr: {0x00, 0x0B, 0x57, 0x4F, 0x43, 0xE7}};
	bd_addr address1 = {addr: {0xE7, 0x43, 0x4F, 0x57, 0x0B, 0x00}};

	uint16 characteristic_handle;

	bool characteristicsDiscovered = false;
	bool previousCommandFinished = false;
	bool valueRead = false;

	uint8array c_uuid;
	c_uuid.len = 16;
	c_uuid.data[15]=0xb7; c_uuid.data[14]=0xc4; c_uuid.data[13]=0xb6; c_uuid.data[12]=0x94; c_uuid.data[11]=0xbe;
	c_uuid.data[10]=0xe3; c_uuid.data[9]=0x45; c_uuid.data[8]=0xdd; c_uuid.data[7]=0xba; c_uuid.data[6]=0x9f;
	c_uuid.data[5]=0xf3; c_uuid.data[4]=0xb5;
	c_uuid.data[3]=0xe9;
	c_uuid.data[2]=0x94;
	c_uuid.data[1]=0xf4;
	c_uuid.data[0]=0x9a;


	while (1) {
		/* Event pointer for handling events */
		struct gecko_cmd_packet* evt;

		/* Check for stack event. */
		evt = gecko_wait_event();

		// Execute the API commands one by one (wait for each to finish before moving on to next one)
		if(!characteristicsDiscovered)
		{
			if (previousCommandFinished)
			{
				struct gecko_msg_gatt_discover_characteristics_rsp_t * response1;
				response1= gecko_cmd_gatt_discover_characteristics(_conn_handle, _handle_service);
//				printf("\n\r gecko_cmd_gatt_discover_characteristics: 0x%04x \n\r", response1->result);
				characteristicsDiscovered = true;
				previousCommandFinished = false;
			}
		}
		else
		{
			if (previousCommandFinished)
			{
				struct gecko_msg_gatt_read_characteristic_value_rsp_t* response;
				response = gecko_cmd_gatt_read_characteristic_value(_conn_handle, characteristic_handle);
//				printf("\n\r gecko_cmd_gatt_read_characteristic_value: 0x%04x \n\r", response->result);
				previousCommandFinished = false;
				valueRead = true;
			}
		}




		/* Handle events */
		switch (BGLIB_MSG_ID(evt->header)) {
		/* This boot event is generated when the system boots up after reset.
		 * Here the system is set to start advertising immediately after boot procedure. */
		case gecko_evt_system_boot_id:

			gecko_cmd_hardware_set_soft_timer(164000, 1, 0);

    	    gecko_cmd_le_gap_set_scan_parameters(0x03E8,0x0064,0);

			gecko_cmd_le_gap_discover(2);

			gecko_cmd_le_gap_set_mode(le_gap_general_discoverable,
								le_gap_undirected_connectable);

			struct gecko_msg_le_gap_open_rsp_t * response4;
			response4 = gecko_cmd_le_gap_open(address1, 0);

		break;



		// Can comment these prints back in to see service discovery
		case gecko_evt_gatt_service_id:
			_handle_service = evt->data.evt_gatt_service.service;
//			printf("===================Get _handle_service==================\r\n");
//			printf("\r\n");
//			printf("service: %d", _handle_service);
//			printf("\r\n");

			_uuid = evt->data.evt_gatt_service.uuid;
//			printf("===================Get _uuid==================\r\n");
//			printf("\r\n");
//			printf("uuid len: %d", evt->data.evt_gatt_service.uuid.len);
//			printf("\r\n");

			uint8* uuid_value = evt->data.evt_gatt_service.uuid.data;
//			printf("data: %d ---- value: ");
//			for(int i=evt->data.evt_gatt_service.uuid.len-1; i > -1; i--){
//				printf("%02x ", evt->data.evt_gatt_service.uuid.data[i]);
//			}
//			printf("\r\n");

		break;


		// Event triggered when the previous command has completed
		case gecko_evt_gatt_procedure_completed_id:
			_result = *&evt->data.evt_gatt_procedure_completed.result;;
			previousCommandFinished = true; // A new command can now execute
		break;


		// Event triggered when characteristic discovered
		case gecko_evt_gatt_characteristic_id:
			_uuid = *&evt->data.evt_gatt_characteristic.uuid;
			if(*evt->data.evt_gatt_characteristic.uuid.data == *c_uuid.data){
				characteristic_handle = evt->data.evt_gatt_characteristic.characteristic;
			}
		break;


		// Get characteristic value (location and orientation) from the mobile device
		case gecko_evt_gatt_characteristic_value_id:
			_handle_characteristic = *&evt->data.evt_gatt_characteristic_value.characteristic;

			// Convert two uint8s to one number
			uint16_t ovecZero = ((uint16_t)evt->data.evt_gatt_characteristic_value.value.data[0] << 8) | evt->data.evt_gatt_characteristic_value.value.data[1];
			uint16_t ovecOne = ((uint16_t)evt->data.evt_gatt_characteristic_value.value.data[2] << 8) | evt->data.evt_gatt_characteristic_value.value.data[3];
			uint16_t ovecTwo = ((uint16_t)evt->data.evt_gatt_characteristic_value.value.data[4] << 8) | evt->data.evt_gatt_characteristic_value.value.data[5];
			uint16_t xCoordinate = ((uint16_t)evt->data.evt_gatt_characteristic_value.value.data[6] << 8) | evt->data.evt_gatt_characteristic_value.value.data[7];
			uint16_t yCoordinate = ((uint16_t)evt->data.evt_gatt_characteristic_value.value.data[8] << 8) | evt->data.evt_gatt_characteristic_value.value.data[9];

			int16_t ovec0 = (int16_t)ovecZero;
			int16_t ovec1 = (int16_t)ovecOne;
			int16_t ovec2 = (int16_t)ovecTwo;

			if (ovec0 < 0) {ovec0 = -1*ovec0;}
			if (ovec1 < 0) {ovec1 = -1*ovec1;}
			if (ovec2 < 0) {ovec2 = -1*ovec2;}

			if (evt->data.evt_gatt_characteristic_value.value.data[10] == 1) {ovec0 = -1*ovec0;}
			if (evt->data.evt_gatt_characteristic_value.value.data[11] == 1) {ovec1 = -1*ovec1;}
			if (evt->data.evt_gatt_characteristic_value.value.data[12] == 1) {ovec2 = -1*ovec2;}

			printf("{\"IMU\":\"%d,%d,%d\", \"Location\":\"%d,%d\"}\r\n", ovec0, ovec1, ovec2, xCoordinate, yCoordinate);

		break;


		// Event triggered when base station connects to mobile device
		case gecko_evt_le_connection_opened_id:

			_conn_handle = evt->data.evt_le_connection_opened.connection;

			gecko_cmd_le_gap_set_mode(le_gap_user_data,le_gap_undirected_connectable);

			struct gecko_msg_gatt_discover_primary_services_rsp_t * response3;
			response3 = gecko_cmd_gatt_discover_primary_services(_conn_handle);

			struct gecko_msg_le_connection_opened_evt_t *pStatus;
			pStatus = &(evt->data.evt_le_connection_opened);
			uint8_t addressType = pStatus->address_type;
			bd_addr addr = pStatus->address;

		break;


		// Connection lost
		case gecko_evt_le_connection_closed_id:
			/* Check if need to boot to dfu mode */
			if (boot_to_dfu) {
				/* Enter to DFU OTA mode */
				gecko_cmd_system_reset(2);
			} else {
				/* Restart advertising after client has disconnected */
				gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);
			}
       break;



        /* Events related to OTA upgrading
			 ----------------------------------------------------------------------------- */
			/* Check if the user-type OTA Control Characteristic was written.
			 * If ota_control was written, boot the device into Device Firmware Upgrade (DFU) mode. */
      case gecko_evt_gatt_server_user_write_request_id:

			if (evt->data.evt_gatt_server_user_write_request.characteristic
					== gattdb_ota_control) {
				/* Set flag to enter to OTA mode */
				boot_to_dfu = 1;
				/* Send response to Write Request */
				gecko_cmd_gatt_server_send_user_write_response(
						evt->data.evt_gatt_server_user_write_request.connection,
						gattdb_ota_control, bg_err_success);
				/* Close connection to enter to DFU OTA
				 *  mode */
				gecko_cmd_endpoint_close(
						evt->data.evt_gatt_server_user_write_request.connection);
			}
			break;

		default:
			break;
		}
	}
}

/** @} (end addtogroup app) */
/** @} (end addtogroup Application) */
