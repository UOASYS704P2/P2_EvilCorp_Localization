#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_int.h"

#include "radio.h"
#include "token.h"

#include "retargetserial.h"

#include "main.h"
#include "appl_ver.h"

#include "native_gecko.h"
#include "aat.h"

#include "thunderboard/util.h"
#include "thunderboard/board.h"
#include "thunderboard/bmp.h"
#include "thunderboard/si7021.h"
#include "thunderboard/si7210.h"
#include "thunderboard/si1133.h"
#include "thunderboard/ccs811.h"
#include "thunderboard/mic.h"

#include "app.h"
#include "radio.h"
#include "radio_ble.h"

#define MIC_SAMPLE_RATE            1000
#define MIC_SAMPLE_BUFFER_SIZE     512

static uint16_t micSampleBuffer[MIC_SAMPLE_BUFFER_SIZE];

static void     init                (bool radio);
static void     readTokens          (void);

uint16_t RADIO_xoTune = 344;

static uint8_t *i_data;


int main(void)
{

  int _conn_handle;

  /**************************************************************************/
  /* Device errata init                                                     */
  /**************************************************************************/
  CHIP_Init();

  /**************************************************************************/
  /* Read tokens and store settings                                         */
  /**************************************************************************/
  readTokens();

  /**************************************************************************/
  /* Board init                                                             */
  /**************************************************************************/
  init(true);

  printf("\r\n\r\n#### Thunderboard Sense BLE application - %d.%d.%d build %d ####\r\n",
         APP_VERSION_MAJOR,
         APP_VERSION_MINOR,
         APP_VERSION_PATCH,
         APP_VERSION_BUILD
         );



  // Enable IMU
  BOARD_imuEnable(true);
  BOARD_imuEnableIRQ(true);
  IMU_init();
  float sampleRate = 100;
  IMU_config(sampleRate);


  while (1) {



	  /**************************************************************************/
	  /* Get IMU and RSSI data                                                          */
	  /**************************************************************************/

	  // Detect bluetooth events
      struct gecko_cmd_packet* evt;
      evt = gecko_wait_event();

      // React to bluetooth events
      switch (BGLIB_MSG_ID(evt->header)) {

        case gecko_evt_system_boot_id:
        	gecko_cmd_hardware_set_soft_timer(20000, 1, 0);	// 328000
        	gecko_cmd_le_connection_get_rssi(_conn_handle);
        	gecko_cmd_le_gap_set_scan_parameters(0x03E8,0x0064,0);
        	gecko_cmd_le_gap_discover(2);
        	printf("\n\r");
        	printf("++++boot++++");
        	break;


        case gecko_evt_le_connection_opened_id:
        	_conn_handle = evt->data.evt_le_connection_opened.connection;
        	printf("++connected!!++");
        	break;

        case gecko_evt_le_connection_rssi_id:
            printf("++++connection_rssi_evt++++");
            printf("\n\r");
            struct gecko_msg_le_connection_rssi_evt_t *pStatus;
            pStatus = &(evt->data.evt_le_connection_rssi);
        	uint8_t rssi_id;
            rssi_id = pStatus->rssi;
            printf("%d",rssi_id);
            printf("\n\r");
            break;

        // Get RSSI data
        case gecko_evt_le_gap_scan_response_id:
            {
          	  i_data = &evt->data.evt_le_gap_scan_response.data.data;
          	  if(i_data[25] == 0x03 && i_data[26] == 0x87){
          		  printf("=============================================\r\n");
          		          	  printf("ibeacon: %d %d %d \r\n", i_data[0], i_data[1], i_data[2]);
          		          	  printf("major: %02x %02x \r\n", i_data[25], i_data[26]);
          		          	  printf("minor: %02x %02x \r\n", i_data[27], i_data[28]);
          		          	  printf("signal power: %d \r\n", i_data[29]);
          		          	  printf("rssi: %d dB \r\n", i_data[29]-256);
          		          	  printf("=============================================\r\n");
          	  }
            }
            break;

	    // Software timer event
        case gecko_evt_hardware_soft_timer_id:
        	switch (evt->data.evt_hardware_soft_timer.handle)
        	{
        		case 1:
        			  // Get IMU orientation data
        			  IMU_update();
        			  int16_t ovec[3];
        			  IMU_orientationGet(ovec);
        			  //printf("\n\rOrientation Data: %d %d %d\n\r", ovec[0], ovec[1], ovec[2]);
        			break;

        		default:
        			break;
        	}
        	break;

        default:
        	break;
        }
  }
}





/**************************************************************************/
/* Thunderboard Sense function definitions                                */
/**************************************************************************/

void MAIN_initSensors()
{
  uint8_t bmpDeviceId;
  uint32_t status;

  SI7021_init();
  SI1133_init();
  BMP_init(&bmpDeviceId);
  printf("Pressure sensor: %s detected\r\n",
         bmpDeviceId == BMP_DEVICE_ID_BMP280 ? "BMP280" : "BMP180");

  status = SI7210_init();
  printf("SI7210 init status: %x\r\n", (unsigned int)status);
  if ( status == SI7210_OK ) {
    SI7210_suspend();
  }

  if ( UTIL_isLowPower() == false ) {
    CCS811_init();
    status = CCS811_startApplication();
    if ( status == CCS811_OK ) {
      status = CCS811_setMeasureMode(CCS811_MEASURE_MODE_DRIVE_MODE_10SEC);
    }
    printf("CCS811 init status: %x\r\n", (unsigned int)status);
  }

  MIC_init(MIC_SAMPLE_RATE, micSampleBuffer, MIC_SAMPLE_BUFFER_SIZE);

  BOARD_rgbledSetRawColor(0, 0, 0);

  return;
}

void MAIN_deInitSensors()
{
  SI7021_deInit();
  SI7210_deInit();
  SI1133_deInit();
  BMP_deInit();
  BOARD_envSensEnable(false);

  if ( UTIL_isLowPower() == false ) {
    CCS811_deInit();
  }

  MIC_deInit();

  BOARD_ledSet(0);
  BOARD_rgbledSetRawColor(0, 0, 0);
  BOARD_rgbledEnable(false, 0xFF);

  return;
}

#define RADIO_XO_TUNE_VALUE 344
void init(bool radio)
{
  uint8_t  supplyType;
  float    supplyVoltage;
  float    supplyIR;
  uint8_t  major, minor, patch, hwRev;
  uint32_t id;

  /**************************************************************************/
  /* Module init                                                            */
  /**************************************************************************/
  UTIL_init();
  BOARD_init();

  id = BOARD_picGetDeviceId();
  BOARD_picGetFwRevision(&major, &minor, &patch);
  hwRev = BOARD_picGetHwRevision();

  printf("\r\n");
  printf("PIC device id    : %08Xh '%c%c%c%c'\r\n", (unsigned int)id,
         (int)id, (int)(id >> 8), (int)(id >> 16), (int)(id >> 24));
  printf("PIC firmware rev : %dv%dp%d\r\n", major, minor, patch);
  printf("PIC hardware rev : %c%.2d\r\n", 'A' + (hwRev >> 4), (hwRev & 0xf) );

  UTIL_supplyProbe();
  UTIL_supplyGetCharacteristics(&supplyType, &supplyVoltage, &supplyIR);

  printf("\r\n");
  printf("Supply voltage : %.3f\r\n", supplyVoltage);
  printf("Supply IR      : %.3f\r\n", supplyIR);
  printf("Supply type    : ");
  if ( supplyType == UTIL_SUPPLY_TYPE_USB ) {
    printf("USB\r\n");
  } else if ( supplyType == UTIL_SUPPLY_TYPE_AA ) {
    printf("Dual AA batteries\r\n");
  } else if ( supplyType == UTIL_SUPPLY_TYPE_AAA ) {
    printf("Dual AAA batteries\r\n");
  } else if ( supplyType == UTIL_SUPPLY_TYPE_CR2032 ) {
    printf("CR2032\r\n");
  } else {
    printf("Unknown\r\n");
  }

  /**************************************************************************/
  /* System clock and timer init                                            */
  /**************************************************************************/
  if ( radio ) {
    RADIO_bleStackInit();
  } else {
    CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
  }

  /* Re-initialize serial port and UTIL which depend on the HF clock frequency */
  RETARGET_SerialInit();
  UTIL_init();
  BOARD_init();

  /* In low power mode, sensors are enabled and disabled when entering/leaving connected mode */
  if ( !UTIL_isLowPower() ) {
    MAIN_initSensors();
  }

  GPIO_PinModeSet(gpioPortD, 14, gpioModeInput, 0);
  GPIO_PinModeSet(gpioPortD, 15, gpioModeInput, 0);

  return;
}

void readTokens(void)
{
  /*uint8_t t8;*/
  uint16_t t16;
  /*uint32_t t32;*/

  /* Dump tokens */
  t16 = TOKEN_getU16(SB_RADIO_CTUNE);
  if ( t16 != 0xFFFF ) {
    RADIO_xoTune = t16;
    printf("\r\nSB_RADIO_CTUNE = %d\r\n", t16);
  }
  t16 = TOKEN_getU16(SB_RADIO_CHANNEL);
  if ( t16 != 0xFFFF ) {
    printf("SB_RADIO_CHANNEL = %d\r\n", t16);
  }
  t16 = TOKEN_getU16(SB_RADIO_OUTPUT_POWER);
  if ( t16 != 0xFFFF ) {
    printf("SB_RADIO_OUTPUT_POWER = %d\r\n", t16);
  }
  printf("\r\n");

  return;
}
