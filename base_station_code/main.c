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
	int _conn_handle;

	//Mobile Device bluetooth address
	bd_addr address = {addr: {0x00, 0x0B, 0x57, 0x4F, 0x43, 0xE7}};
	bd_addr address1 = {addr: {0xE7, 0x43, 0x4F, 0x57, 0x0B, 0x00}};
//	printf("bd_addr address is: %02x:%02x:%02x:%02x:%02x:%02x  \r\n",
//			address.addr[0],address.addr[1],address.addr[2],address.addr[3],address.addr[4],address.addr[5]);

	while (1) {
		/* Event pointer for handling events */
		struct gecko_cmd_packet* evt;

		/* Check for stack event. */
		evt = gecko_wait_event();

//		struct gecko_msg_le_gap_open_rsp_t* response;

//		response = gecko_cmd_le_gap_open(address, 0);

//		gecko_cmd_le_gap_open(address, 0);

//		printf("\n\rConnection Attempt: %d\n\r", response->result);

		/* Handle events */
		switch (BGLIB_MSG_ID(evt->header)) {
		/* This boot event is generated when the system boots up after reset.
		 * Here the system is set to start advertising immediately after boot procedure. */
		case gecko_evt_system_boot_id: {

			//The smallest interval value supported is 328 which is around 10
			//milliseconds, any parameters between 0 and 328 will be rounded
			//up to 328. The maximum value is 2147483647, which corresponds
			//to about 18.2 hours.
			gecko_cmd_hardware_set_soft_timer(164000, 1, 0);

    	    gecko_cmd_le_gap_set_scan_parameters(0x03E8,0x0064,0);

			gecko_cmd_le_gap_discover(2);

			gecko_cmd_le_gap_open(address1, 0);

			gecko_cmd_le_gap_set_mode(le_gap_general_discoverable,
								le_gap_undirected_connectable);

//			gecko_cmd_le_gap_set_mode(le_gap_user_data,
//					le_gap_undirected_connectable);

//			gecko_cmd_le_gap_bt5_set_mode(1, le_gap_general_discoverable,
//					le_gap_undirected_connectable,0,0);

			printf("=========Base Station Booting===================\r\n");
			printf("\r\n");
		}
			break;

//     case gecko_rsp_le_gap_set_scan_parameters_id:
//	  {
//		  printf("++set_scan_parameters++");
//		  struct gecko_msg_le_gap_set_scan_parameters_rsp_t *pStatus;
//   	  pStatus = &(evt->data.rsp_le_gap_set_scan_parameters);
//   	            		uint8_t rssi_id;
//   	                  	rssi_id = pStatus->result;
//   	                  	printf("%d",rssi_id);
//   	                  	printf("\n\r");
//	  }
//   	     break;

//		case gecko_evt_le_connection_opened_id: {
//
//			_conn_handle = evt->data.evt_le_connection_opened.connection;
//
//			struct gecko_msg_le_connection_opened_evt_t *pStatus;
//			pStatus = &(evt->data.evt_le_connection_opened);
//			uint8_t addressType = pStatus->address_type;
//			bd_addr addr = pStatus->address;
//
//			printf("=========Awesome Device connected!! ============\r\n");
//			printf("BLE address is: %02x:%02x:%02x:%02x:%02x:%02x  \r\n",
//					addr.addr[5],addr.addr[4],addr.addr[3],
//					addr.addr[2],addr.addr[1],addr.addr[0]);
//			printf("BLE address type %d \r\n", addressType);
//
////			printf("++ HHH ++");
//
//		}
//			break;

//      case gecko_evt_le_connection_closed_id:
//        /* Check if need to boot to dfu mode */
//        if (boot_to_dfu) {
//          /* Enter to DFU OTA mode */
//          gecko_cmd_system_reset(2);
//        } else {
//          /* Restart advertising after client has disconnected */
//          gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);
//        }
//        printf("\n\r");
//        printf("DISCONNECTED!");
//        break;

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
       case gecko_evt_le_gap_scan_response_id:
          {
        	  i_data = *&evt->data.evt_le_gap_scan_response.data.data;
        	  uint8 data_len = *&evt->data.evt_le_gap_scan_response.data.len;

        	  struct gecko_msg_le_gap_scan_response_evt_t *pStatus;
        	  pStatus = &(evt->data.evt_le_gap_scan_response);
//        	  uint8_t _rssi = pStatus->rssi;
        	  bd_addr bd_address;
        	  bd_address = pStatus->address;
        	  uint8   addresstype;
        	  addresstype = pStatus->address_type;

//        	  printf("=============================================\r\n");
//        	  printf("BLE address is: %02x:%02x:%02x:%02x:%02x:%02x  \r\n",
//        			  bd_address.addr[5],bd_address.addr[4],bd_address.addr[3],
//					  bd_address.addr[2],bd_address.addr[1],bd_address.addr[0]);
//        	  printf("BLE address type %d \r\n", addresstype);

        	  //Filter out the Mobile Device:
        	  if(
        		bd_address.addr[0] == address.addr[5] &&
        		bd_address.addr[1] == address.addr[4] &&
				bd_address.addr[2] == address.addr[3] &&
				bd_address.addr[3] == address.addr[2] &&
			    bd_address.addr[4] == address.addr[1] &&
				bd_address.addr[5] == address.addr[0]
        	  ){


        	// Testing conversion of two uint8s to one number
        	 uint16_t ovecZero = ((uint16_t)i_data[0] << 8) | i_data[1];
        	 uint16_t ovecOne = ((uint16_t)i_data[2] << 8) | i_data[3];
        	 uint16_t ovecTwo = ((uint16_t)i_data[4] << 8) | i_data[5];

        	 int16_t ovec0 = (int16_t)ovecZero;
        	 int16_t ovec1 = (int16_t)ovecOne;
        	 int16_t ovec2 = (int16_t)ovecTwo;

        	 if (ovec0 < 0) {ovec0 = -1*ovec0;}
        	 if (ovec1 < 0) {ovec1 = -1*ovec1;}
        	 if (ovec2 < 0) {ovec2 = -1*ovec2;}

        	 if (i_data[9] == 1) {ovec0 = -1*ovec0;}
        	 if (i_data[10] == 1) {ovec1 = -1*ovec1;}
        	 if (i_data[11] == 1) {ovec2 = -1*ovec2;}

        	 printf("===================Scanning======================\r\n");
        	 printf("Awesome Device detected.\r\n");
        	 printf("Awesome Device address is: %02x:%02x:%02x:%02x:%02x:%02x \r\n",
        		        bd_address.addr[5],bd_address.addr[4],bd_address.addr[3],
        		 		bd_address.addr[2],bd_address.addr[1],bd_address.addr[0]);
//        	 printf("BLE address type %d \r\n", addresstype);
        	 printf("Awesome Device data reading......\r\n");
             printf("|\n\r");
        	 printf("|---->Received data length is: %d Bytes.\r\n",data_len);
        	 printf("|\n\r");
        	 printf("|---->Orientation data: %d %d %d \n\r", ovec0, ovec1, ovec2);
        	 printf("|\n\r");
//        	 printf("|---->Negatives: %d %d %d \n\r", i_data[9], i_data[10], i_data[11]);
//        	 printf("|\n\r");
        	 printf("|---->RSSI data from Beacon(Minor: %02d %02d)is: %d.\r\n", i_data[7],i_data[8],i_data[6]);
        	 printf("=================================================\r\n");
        	  }
        	  /* Software Timer event */
//        	       case gecko_evt_hardware_soft_timer_id:
//        	       {
//        	         switch (evt->data.evt_hardware_soft_timer.handle) {
//        	         case 1:{
//                     	}
//                     	break;
//        	         	default:break;
//        	        }
//        	      }
//        	      break;
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
