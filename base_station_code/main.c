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

//	typedef struct {
//	  uint8 addr[6];
//	}bd_addr;
	//Mobile Device bluetooth address
	bd_addr address = {addr: {0x00, 0x0B, 0x57, 0x4F, 0x43, 0xE7}};
	bd_addr address1 = {addr: {0xE7, 0x43, 0x4F, 0x57, 0x0B, 0x00}};
//	printf("bd_addr address is: %02x:%02x:%02x:%02x:%02x:%02x  \r\n",
//			address.addr[0],address.addr[1],address.addr[2],address.addr[3],address.addr[4],address.addr[5]);



//	b7c4b694-bee3-45dd-ba9f-f3b5e994f49a
//	typedef struct {
//	  uint8 len;
//	  uint8 data[];
//	}uint8array;

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

	int i = 0;
	printf("Byte at index %d : %02x \r\n", i, c_uuid.data[i]);
	i = 1;
	printf("Byte at index %d : %02x \r\n", i, c_uuid.data[i]);
	i = 2;
	printf("Byte at index %d : %02x \r\n", i, c_uuid.data[i]);
	i = 3;
	printf("Byte at index %d : %02x \r\n", i, c_uuid.data[i]);
	i = 4;
	printf("Byte at index %d : %02x \r\n", i, c_uuid.data[i]);
	i = 5;
	printf("Byte at index %d : %02x \r\n", i, c_uuid.data[i]);
	i = 6;
	printf("Byte at index %d : %02x \r\n", i, c_uuid.data[i]);
	i = 7;
	printf("Byte at index %d : %02x \r\n", i, c_uuid.data[i]);
	i = 8;
	printf("Byte at index %d : %02x \r\n", i, c_uuid.data[i]);
	i = 9;
	printf("Byte at index %d : %02x \r\n", i, c_uuid.data[i]);
	i = 10;
	printf("Byte at index %d : %02x \r\n", i, c_uuid.data[i]);
	i = 11;
	printf("Byte at index %d : %02x \r\n", i, c_uuid.data[i]);
	i = 12;
	printf("Byte at index %d : %02x \r\n", i, c_uuid.data[i]);
	i = 13;
	printf("Byte at index %d : %02x \r\n", i, c_uuid.data[i]);
	i = 14;
	printf("Byte at index %d : %02x \r\n", i, c_uuid.data[i]);
	i = 15;
	printf("Byte at index %d : %02x \r\n", i, c_uuid.data[i]);

//	printf("Orientation UUID: \r\n");
//	for(i=0; i < c_uuid.len; i++){
//		printf("Byte at index %d : %02x \r\n", i, c_uuid.data[i]);
//	}
//	printf("\r\n");

	while (1) {
		/* Event pointer for handling events */
		struct gecko_cmd_packet* evt;

		/* Check for stack event. */
		evt = gecko_wait_event();

//		struct gecko_msg_le_gap_open_rsp_t* response;
//		response = gecko_cmd_le_gap_open(address, 0);
//		gecko_cmd_le_gap_open(address, 0);
//		printf("\n\rConnection Attempt: %d\n\r", response->result);

		if(!characteristicsDiscovered)
		{
			if (previousCommandFinished)
			{
				struct gecko_msg_gatt_discover_characteristics_rsp_t * response1;
				response1= gecko_cmd_gatt_discover_characteristics(_conn_handle, _handle_service);
				printf("\n\r gecko_cmd_gatt_discover_characteristics: 0x%04x \n\r", response1->result);
				characteristicsDiscovered = true;
				previousCommandFinished = false;
			}
		}
		else if(!valueRead)
		{
			if (previousCommandFinished)
			{
				struct gecko_msg_gatt_read_characteristic_value_rsp_t* response;
				response = gecko_cmd_gatt_read_characteristic_value(_conn_handle, characteristic_handle);
				printf("\n\r gecko_cmd_gatt_read_characteristic_value: 0x%04x \n\r", response->result);
				previousCommandFinished = false;
				valueRead = true;
			}
		}

		/* Handle events */
		switch (BGLIB_MSG_ID(evt->header)) {
		/* This boot event is generated when the system boots up after reset.
		 * Here the system is set to start advertising immediately after boot procedure. */
		case gecko_evt_system_boot_id:

			printf("\r\n");
			printf("===================Base Station Booting===================\r\n");

			//The smallest interval value supported is 328 which is around 10
			//milliseconds, any parameters between 0 and 328 will be rounded
			//up to 328. The maximum value is 2147483647, which corresponds
			//to about 18.2 hours.
			gecko_cmd_hardware_set_soft_timer(164000, 1, 0);

    	    gecko_cmd_le_gap_set_scan_parameters(0x03E8,0x0064,0);

			gecko_cmd_le_gap_discover(2);

			gecko_cmd_le_gap_set_mode(le_gap_general_discoverable,
								le_gap_undirected_connectable);

			struct gecko_msg_le_gap_open_rsp_t * response4;
			response4 = gecko_cmd_le_gap_open(address1, 0);
			printf("\n\r gecko_cmd_le_gap_open: 0x%04x \r\n", response4->result);

//			gecko_cmd_le_gap_set_mode(le_gap_user_data,
//					le_gap_undirected_connectable);

//			gecko_cmd_le_gap_bt5_set_mode(_conn_handle, le_gap_general_discoverable,
//					le_gap_undirected_connectable,0,0);
		break;

		case gecko_evt_gatt_service_id:
			_handle_service = evt->data.evt_gatt_service.service;
			printf("===================Get _handle_service==================\r\n");
			printf("\r\n");
			printf("service: %d", _handle_service);
			printf("\r\n");

			_uuid = evt->data.evt_gatt_service.uuid;
			printf("===================Get _uuid==================\r\n");
			printf("\r\n");
			printf("uuid len: %d", evt->data.evt_gatt_service.uuid.len);
			printf("\r\n");
			uint8* uuid_value = evt->data.evt_gatt_service.uuid.data;
			printf("data: %d ---- value: ");
			for(int i=evt->data.evt_gatt_service.uuid.len-1; i > -1; i--){
				printf("%02x ", evt->data.evt_gatt_service.uuid.data[i]);
			}
			printf("\r\n");

			/* Event structure */
//			struct gecko_msg_gatt_service_evt_t
//			{
//			uint8 connection;,
//			uint32 service;,
//			uint8array uuid;
//			};
		break;

		case gecko_evt_gatt_procedure_completed_id:
			_result = *&evt->data.evt_gatt_procedure_completed.result;;
			printf("\n\r COMMAND COMPLETED!!: 0x%04x \n\r", _result);
			printf("\r\n");
			previousCommandFinished = true;
		break;

		case gecko_evt_gatt_characteristic_id:
			_uuid = *&evt->data.evt_gatt_characteristic.uuid;
			printf("===================gecko_evt_gatt_characteristic_id==================\r\n");
			printf("\r\n");
//			printf("data: %d ---- value: ");
//			for(int i=evt->data.evt_gatt_characteristic.uuid.len-1; i > -1; i--){
//				printf("%02x ", evt->data.evt_gatt_characteristic.uuid.data[i]);
//			}

			if(*evt->data.evt_gatt_characteristic.uuid.data == *c_uuid.data){ // note to self: add rest of elements
				printf("\n\r\nOrientation Characteristic Found!!!\n\r\n");
				characteristic_handle = evt->data.evt_gatt_characteristic.characteristic;
			}

			printf("\r\n");

		break;

		case gecko_evt_gatt_characteristic_value_id:
			_handle_characteristic = *&evt->data.evt_gatt_characteristic_value.characteristic;
			printf("===================Get _handle_characteristic==================\r\n");
			printf("\r\n");
			for(int i=evt->data.evt_gatt_characteristic_value.value.len - 1; i > -1; i--){
				printf("%02x ", evt->data.evt_gatt_characteristic_value.value.data[i]);
			}
//			uint8array c_value = evt->data.evt_gatt_characteristic_value.value;
//			val_data = c_value.data;
//			printf("characteristic_value len: %d \r\n", c_value.len);

//			struct gecko_msg_gatt_characteristic_value_evt_t
//			{
//			uint8 connection;,
//			uint16 characteristic;,
//			uint8 att_opcode;,
//			uint16 offset;,
//			uint8array value;
//			};
//			printf("characteristic_value ---------------------------- value:", val_data[0]);
		break;

//		case gecko_evt_gatt_server_characteristic_status_id:
//		//		evt_gatt_server_characteristic_status;
//			printf("++++evt_gatt_server_characteristic_status++++");
//			printf("\n\r");
//		break;

//		case gecko_evt_gatt_server_user_read_request_id:
//			printf("Read request event received \r\n");
//			uint8 connection = _conn_handle;
//			uint16 characteristic = &evt->data.evt_gatt_server_user_read_request.characteristic;
//			uint8 att_errorcode = 0;
//			uint8 value_len = 1;
//			uint8 value_data[1];
//			value_data[0] = 0x44;
//			gecko_cmd_gatt_server_send_user_read_response(connection, characteristic, att_errorcode, value_len, &value_data);
//		break;

		case gecko_evt_le_connection_opened_id:
			_conn_handle = evt->data.evt_le_connection_opened.connection;
//			gecko_cmd_le_gap_set_mode(le_gap_user_data,le_gap_undirected_connectable);
//			gecko_cmd_gatt_discover_characteristics(_conn_handle);

//			struct gecko_msg_gatt_read_characteristic_value_rsp_t* results;
//			results = cmd_gatt_read_characteristic_value(_conn_handle, );
//			gecko_cmd_gatt_read_characteristic_value(_conn_handle, _handle_characteristic);

			struct gecko_msg_gatt_discover_primary_services_rsp_t * response3;
			response3 = gecko_cmd_gatt_discover_primary_services(_conn_handle);
			printf("\n\r gecko_cmd_gatt_discover_primary_services: 0x%04x \n\r", response3->result);



//			c_uuid.data[0]=0xf7; c_uuid.data[1]=0xbf; c_uuid.data[2]=0x35; c_uuid.data[3]=0x64; c_uuid.data[4]=0xfb;
//			c_uuid.data[5]=0x6d; c_uuid.data[6]=0x4e; c_uuid.data[7]=0x53; c_uuid.data[8]=0x88; c_uuid.data[9]=0xa4;
//			c_uuid.data[10]=0x5e; c_uuid.data[11]=0x37; c_uuid.data[12]=0xe0; c_uuid.data[13]=0x32; c_uuid.data[14]=0x60;
//			c_uuid.data[15]=0x63;

			struct gecko_msg_le_connection_opened_evt_t *pStatus;
			pStatus = &(evt->data.evt_le_connection_opened);
			uint8_t addressType = pStatus->address_type;
			bd_addr addr = pStatus->address;
			printf("\n\r");
			printf("===================Awesome Device connected===================\r\n");
			printf("BLE address is: %02x:%02x:%02x:%02x:%02x:%02x  \r\n",
					addr.addr[5],addr.addr[4],addr.addr[3],
					addr.addr[2],addr.addr[1],addr.addr[0]);
			printf("BLE address type %d \r\n", addressType);
			printf("++ HHH ++");

		break;

		case gecko_evt_le_connection_closed_id:
			/* Check if need to boot to dfu mode */
			if (boot_to_dfu) {
				/* Enter to DFU OTA mode */
				gecko_cmd_system_reset(2);
			} else {
				/* Restart advertising after client has disconnected */
				gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);
			}
			printf("\n\r");
			printf("DISCONNECTED!");
			printf("\n\r");
       break;

//		case gecko_evt_le_connection_rssi_id: {
//			printf("++++connection_rssi_evt++++");
//			printf("\n\r");
//			struct gecko_msg_le_connection_rssi_evt_t *pStatus;
//			pStatus = &(evt->data.evt_le_connection_rssi);
//			uint8_t rssi_id;
//			rssi_id = pStatus->rssi;
//			printf("%d", rssi_id);
//			printf("\n\r");
//		}
//			break;

			/* Event id */
//       case gecko_evt_le_gap_scan_response_id:
//          {
//        	  i_data = *&evt->data.evt_le_gap_scan_response.data.data;
//        	  uint8 data_len = *&evt->data.evt_le_gap_scan_response.data.len;
//
//        	  struct gecko_msg_le_gap_scan_response_evt_t *pStatus;
//        	  pStatus = &(evt->data.evt_le_gap_scan_response);
////        	  uint8_t _rssi = pStatus->rssi;
//        	  bd_addr bd_address;
//        	  bd_address = pStatus->address;
//        	  uint8   addresstype;
//        	  addresstype = pStatus->address_type;
//
////        	  printf("=============================================\r\n");
////        	  printf("BLE address is: %02x:%02x:%02x:%02x:%02x:%02x  \r\n",
////        			  bd_address.addr[5],bd_address.addr[4],bd_address.addr[3],
////					  bd_address.addr[2],bd_address.addr[1],bd_address.addr[0]);
////        	  printf("BLE address type %d \r\n", addresstype);
//
//        	  //Filter out the Mobile Device:
//        	  if(
//        		bd_address.addr[0] == address.addr[5] &&
//        		bd_address.addr[1] == address.addr[4] &&
//				bd_address.addr[2] == address.addr[3] &&
//				bd_address.addr[3] == address.addr[2] &&
//			    bd_address.addr[4] == address.addr[1] &&
//				bd_address.addr[5] == address.addr[0]
//        	  ){
//
//
//        	// Testing conversion of two uint8s to one number
//        	 uint16_t ovecZero = ((uint16_t)i_data[0] << 8) | i_data[1];
//        	 uint16_t ovecOne = ((uint16_t)i_data[2] << 8) | i_data[3];
//        	 uint16_t ovecTwo = ((uint16_t)i_data[4] << 8) | i_data[5];
//
//        	 int16_t ovec0 = (int16_t)ovecZero;
//        	 int16_t ovec1 = (int16_t)ovecOne;
//        	 int16_t ovec2 = (int16_t)ovecTwo;
//
//        	 if (ovec0 < 0) {ovec0 = -1*ovec0;}
//        	 if (ovec1 < 0) {ovec1 = -1*ovec1;}
//        	 if (ovec2 < 0) {ovec2 = -1*ovec2;}
//
//        	 if (i_data[9] == 1) {ovec0 = -1*ovec0;}
//        	 if (i_data[10] == 1) {ovec1 = -1*ovec1;}
//        	 if (i_data[11] == 1) {ovec2 = -1*ovec2;}
//
//        	 printf("===================Scanning======================\r\n");
//        	 printf("Awesome Device detected.\r\n");
//        	 printf("Awesome Device address is: %02x:%02x:%02x:%02x:%02x:%02x \r\n",
//        		        bd_address.addr[5],bd_address.addr[4],bd_address.addr[3],
//        		 		bd_address.addr[2],bd_address.addr[1],bd_address.addr[0]);
////        	 printf("BLE address type %d \r\n", addresstype);
//        	 printf("Awesome Device data reading......\r\n");
//             printf("|\n\r");
//        	 printf("|---->Received data length is: %d Bytes.\r\n",data_len);
//        	 printf("|\n\r");
//        	 printf("|---->Orientation data: %d %d %d \n\r", ovec0, ovec1, ovec2);
//        	 printf("|\n\r");
////        	 printf("|---->Negatives: %d %d %d \n\r", i_data[9], i_data[10], i_data[11]);
////        	 printf("|\n\r");
//        	 printf("|---->RSSI data from Beacon(Minor: %02d %02d)is: %d.\r\n", i_data[7],i_data[8],i_data[6]);
//        	 printf("=================================================\r\n");
//        	  }
//        	  /* Software Timer event */
////        	       case gecko_evt_hardware_soft_timer_id:
////        	       {
////        	         switch (evt->data.evt_hardware_soft_timer.handle) {
////        	         case 1:{
////                     	}
////                     	break;
////        	         	default:break;
////        	        }
////        	      }
////        	      break;
//         }
//      break;
       /* Software Timer event */
//      case gecko_evt_hardware_soft_timer_id:
//		  switch (evt->data.evt_hardware_soft_timer.handle) {
//		  	  case 1:{
////		  		struct gecko_msg_gatt_discover_primary_services_rsp_t * response3;
////				response3 = gecko_cmd_gatt_discover_primary_services(_conn_handle);
////				printf("\n\r gecko_cmd_gatt_discover_primary_services: 0x%04x \n\r", response3->result);
////

////
////				if(_finishflag == 1){
////					struct gecko_msg_gatt_discover_characteristics_rsp_t * response1;
////					response1= gecko_cmd_gatt_discover_characteristics(_conn_handle, _handle_service);
////					printf("\n\r gecko_cmd_gatt_discover_characteristics: 0x%04x \n\r", response1->result);
////				}
//////				_finishflag = 0;
////
////				if(_finishflag == 1){
////					struct gecko_msg_gatt_read_characteristic_value_by_uuid_rsp_t* response;
////					response = gecko_cmd_gatt_read_characteristic_value_by_uuid(_conn_handle, _handle_service, 16, *c_uuid.data);
////		//			response = gecko_cmd_gatt_read_characteristic_value_by_uuid(_conn_handle, _handle_service, 6, "b7c4b694-bee3-45dd-ba9f-f3b5e994f49a");
////					printf("\n\r gecko_cmd_gatt_read_characteristic_value_by_uuid: 0x%04x \n\r", response->result);
////				}
////				_finishflag = 0;
//		  	  }
//		  	  break;
//		  	  default:
//		  	break;
//		  }
//	  break;

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
