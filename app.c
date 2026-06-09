/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"
#include "app_log.h"
#include "sl_sensor_rht.h"
#include "sl_sensor_light.h"
#include "temperature.h"
#include "sl_bt_api.h"
#include "gatt_db.h"
#include "sl_simple_led_instances.h"
#include <stdio.h>


#define TEMPERATURE_TIMER_SIGNAL (1<<0)

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
static uint32_t compterTimer = 0;
static sl_sleeptimer_timer_handle_t handleTimer;

struct info_temp tempe;


uint8_t connection_id;
uint8_t att_appcode;
uint8_t Notif_ConnectionID;
uint16_t getCarac;
uint16_t config_flags;
uint8_t StatusFlags;
uint8_t* Data;
uint8array mesData;
float lux = 0.0;
float longD = 0.0;
float uvi = 0.0;
uint16_t convertedLong = 0;

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////

  app_log_info("%s\n\r", __FUNCTION__);
}

//void sl_bt_external_signal(TEMPERATURE_TIMER_SIGNAL){
//
//}



void CallBackTimer(sl_sleeptimer_timer_handle_t * handle, void *data){
  (void)handle;
  (void)data;
  compterTimer++;

      app_log_info("------------Boucle de Timer------------\n\r");
      app_log_info("timer step : %ld\n\r",compterTimer);
  sl_bt_external_signal(TEMPERATURE_TIMER_SIGNAL);
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      sl_simple_led_init_instances();
      sl_sensor_light_init();

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);
      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:

      sl_sensor_rht_init();
      app_log_info("%s Connection Opened\n\r", __FUNCTION__);
      break;

    case sl_bt_evt_gatt_server_user_read_request_id :
      getCarac = evt->data.evt_gatt_server_user_read_request.characteristic;
      if(getCarac == gattdb_temperature){
        app_log_info("%s carac = %d\n\r", __FUNCTION__,getCarac);
        tempe = read_temp();
        connection_id = evt->data.evt_gatt_server_user_read_request.connection;
        sc = sl_bt_gatt_server_send_user_read_response(connection_id, gattdb_temperature, 0, sizeof(tempe.tempBLE), (uint8_t *)&tempe.tempBLE, NULL);
        app_log_info("temp : %d",tempe.tempBLE);

      }
      if(getCarac == gattdb_irradiance_0){
          app_log_info("-------Demande lecture luminosite-------\n\r");
          sl_sensor_light_get(&lux, &uvi);
          longD = lux/(683);
          convertedLong = (uint16_t)(longD*10);
          app_log_info("luminosite = %d.%d \n\r",(int)longD,((int)(longD*10))%10);
          connection_id = evt->data.evt_gatt_server_user_read_request.connection;
          sc = sl_bt_gatt_server_send_user_read_response(connection_id,gattdb_irradiance_0,0,sizeof(convertedLong),(uint8_t *)&convertedLong,NULL);
          app_log_info("-------Demande lecture luminosite-------\n\r");

      }
      break;
    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      // Generate data for advertising
      sl_sensor_rht_deinit();
      app_log_info("%s Connection Closed\n\r", __FUNCTION__);
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      break;

    case sl_bt_evt_gatt_server_characteristic_status_id:

      getCarac = evt->data.evt_gatt_server_characteristic_status.characteristic;
      app_log_info("---------Nous avons reçu une notif----------\n\r");
      if(getCarac==gattdb_temperature){
          StatusFlags = evt->data.evt_gatt_server_characteristic_status.status_flags;
          if(StatusFlags == sl_bt_gatt_server_client_config){
              config_flags = evt->data.evt_gatt_server_characteristic_status.client_config_flags;
              app_log_info("Q18 : Action sur le notify de temperature : %d\n\r", StatusFlags);
              app_log_info("Q19 : Action : %d\n\r", config_flags);
              if(config_flags == 1){

                  compterTimer = 0;
                  Notif_ConnectionID = evt->data.evt_gatt_server_characteristic_status.connection;
                  sc = sl_sleeptimer_start_periodic_timer_ms(&handleTimer, 1000, CallBackTimer, NULL, 0, 0);
                  app_log_info("Q19 : notification activé\n\r");
              }
              else if(config_flags == 0){

                  sc = sl_sleeptimer_stop_timer(&handleTimer);
                  app_log_info("Q19 : notification désactivé\n\r");
              }
          }
          app_log_info("--------------------------------------------\n\r");
      }


      break;

    case sl_bt_evt_system_external_signal_id:

         if (evt->data.evt_system_external_signal.extsignals & TEMPERATURE_TIMER_SIGNAL) {


           tempe = read_temp();

           sc = sl_bt_gatt_server_send_notification(Notif_ConnectionID,gattdb_temperature,sizeof(tempe.tempBLE),(uint8_t *)&tempe.tempBLE);

           app_log_info("Notification envoyée : %d,%d\n\r", tempe.converted_ent_temp, tempe.converted_dec_temp);
           app_log_info("---------------------------------------\n\r");
         }
         break;

    case sl_bt_evt_gatt_server_user_write_request_id:
      app_log_info("------------User write request-------------\n\r");
      mesData = evt->data.evt_gatt_server_user_write_request.value;
      Data = evt->data.evt_gatt_server_user_write_request.value.data;
      att_appcode = evt->data.evt_gatt_server_user_write_request.att_opcode;

      app_log_info("appCode : %x | len : %d | message : %x \n\r",att_appcode,mesData.len,Data[0]);
      if(Data[0] == 1){
            sl_led_turn_on(&sl_led_led0);
      }else{
          sl_led_turn_off(&sl_led_led0);
      }
      if(att_appcode == 12){
      connection_id = evt->data.evt_gatt_server_user_write_request.connection;
      getCarac = evt->data.evt_gatt_server_user_write_request.characteristic;

      sc =  sl_bt_gatt_server_send_user_write_response(connection_id,
                                                             getCarac,
                                                             SL_STATUS_OK);
      }else if(att_appcode == 52){

      }

      app_log_info("-------------------------------------------\n\r");
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}
