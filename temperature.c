/*
 * temperature.c
 *
 *  Created on: 28 mai 2026
 *      Author: luqui
 */

#include "sl_status.h"
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"
#include "app_log.h"
#include "sl_sensor_rht.h"
#include "temperature.h"
#include <stdio.h>


struct info_temp temp;

uint32_t humidity_value = 0;
int32_t temperature_value;
sl_status_t sc;

struct info_temp read_temp(void){

  sc = sl_sensor_rht_get(&humidity_value, &temperature_value);
       if(sc == SL_STATUS_OK){
           temp.tempBLE = (int16_t)temperature_value/10;
           temp.converted_dec_temp = (int)temperature_value%1000;
           temp.converted_ent_temp = (int)temperature_value/1000;
        app_log_info("%s Reading : %d,%d \n\r", __FUNCTION__, temp.converted_ent_temp, temp.converted_dec_temp);
       }
       return temp;
}

