/**
******************************************************************************
* @file    user_main.c 
* @author  Eshen Wang
* @version V1.0.0
* @date    14-May-2015
* @brief   user main functons in user_main thread.
******************************************************************************
* @attention
*
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
* TIME. AS A RESULT, MXCHIP Inc. SHALL NOT BE HELD LIABLE FOR ANY
* DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
* FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
* CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* <h2><center>&copy; COPYRIGHT 2014 MXCHIP Inc.</center></h2>
******************************************************************************
*/ 

#include "mico.h"
#include "MicoFogCloud.h"
#include "json_c/json.h"
#include "VGM128064\oled.h"
#include "DHT11\DHT11.h"


/* User defined debug log functions
 * Add your own tag like: 'USER_UPSTREAM', the tag will be added at the beginning of a log
 * in MICO debug uart, when you call this function.
 */
#define user_log(M, ...) custom_log("USER_UPSTREAM", M, ##__VA_ARGS__)
#define user_log_trace() custom_log_trace("USER_UPSTREAM")

// DTH11 termperature && humidity data
uint8_t dht11_temperature = 0;
uint8_t dht11_humidity = 0;

extern bool rgbled_switch;  
extern uint8_t msg_id;
//extern volatile bool hasImage;
extern volatile bool pump_switch;
extern volatile bool lamp_switch;
extern mico_semaphore_t switch_change_sem;

/* Message upload thread
 * Get DHT11 temperature/humidity data && upload to cloud
 */
void user_upstream_thread(void* arg)
{
  user_log_trace();
  OSStatus err = kUnknownErr;
  app_context_t *app_context = (app_context_t *)arg;
  json_object *send_json_object = NULL;
  const char *upload_data = NULL;
  uint8_t ret = 0;
    
  require(app_context, exit);
  
  /* init humiture sensor DHT11 */
  do{
    ret = DHT11_Init();
    if(0 != ret){  // init error
      user_log("DHT11 init failed!");
      err = kNoResourcesErr;
      mico_thread_sleep(1);  // sleep 1s then retry
    }
    else{
      err = kNoErr;
      break;
    }
  }while(kNoErr != err);
  
  /* thread loop */
  while(1){
    // do data acquisition
    ret = DHT11_Read_Data(&dht11_temperature, &dht11_humidity);
    if(0 != ret){
      err = kReadErr;
    }
    else{
      err = kNoErr;
      user_log("dht11_temperature=%d, dht11_humidity=%d",dht11_temperature, dht11_humidity);
//       create json object to format upload data
      send_json_object = json_object_new_object();
      if(NULL == send_json_object){
        user_log("create json object error!");
        err = kNoMemoryErr;
      }
      else{
        // add temperature/humidity data into a json oject
      
        json_object_object_add(send_json_object, "dht11_temperature", json_object_new_int(dht11_temperature)); 
        json_object_object_add(send_json_object, "dht11_humidity", json_object_new_int(dht11_humidity)); 
        json_object_object_add(send_json_object, "lamp_switch", json_object_new_boolean(lamp_switch));
        json_object_object_add(send_json_object, "pump_switch", json_object_new_boolean(pump_switch)); 
       
        //        json_object_object_add(send_json_object, "rgbled_switch", json_object_new_boolean(rgbled_switch)); 
        //      json_object_object_add(send_json_object, "hasImage", json_object_new_boolean(hasImage)); 
        
        upload_data = json_object_to_json_string(send_json_object);
        if(NULL == upload_data){
          user_log("create upload data string error!");
          err = kNoMemoryErr;
        }
        else{
          // check fogcloud connect status
          if(app_context->appStatus.fogcloudStatus.isCloudConnected){
            // upload data string to fogcloud, the seconde param(NULL) means send to defalut topic: '<device_id>/out'
            MiCOFogCloudMsgSend(app_context, NULL, (unsigned char*)upload_data, strlen(upload_data));
//            user_log("upload data success! \t topic=%s/out \t dht11_temperature=%d, dht11_humidity=%d", 
//                     app_context->appConfig->fogcloudConfig.deviceId,
//                     dht11_temperature, dht11_humidity);
            err = kNoErr;
          }
        }
        
        // free json object memory
        json_object_put(send_json_object);
        send_json_object = NULL;
      }
    }
    
    mico_thread_sleep(2);  // data acquisition && upload every 2 seconds
  }

exit:
  if(kNoErr != err){
    user_log("ERROR: user_uptream exit with err=%d", err);
  }
  mico_rtos_delete_thread(NULL);  // delete current thread
}



void user2_upstream_thread(void* arg)
{
  user_log_trace();
  OSStatus err = kUnknownErr;
  app_context_t *app_context = (app_context_t *)arg;
  
  json_object *send_json_object = NULL;
  const char *upload_data = NULL;
  
 
//  user_log("ip address is :%s", ip_address);
  mico_rtos_init_semaphore( &switch_change_sem, 1);
  
  require(app_context, exit);
  
  /* thread loop */
  while(1){
    
    mico_rtos_get_semaphore(&switch_change_sem, MICO_WAIT_FOREVER);
    
    // create json object to format upload data
    send_json_object = json_object_new_object();
    if(NULL == send_json_object){
      user_log("create json object error!");
      err = kNoMemoryErr;
    }
    else{
      
      switch (msg_id )
      {
      case 1:
        {
          json_object_object_add(send_json_object, "lamp_switch", json_object_new_boolean(lamp_switch));
          break;
        }
      case 2:
        {
          json_object_object_add(send_json_object, "pump_switch", json_object_new_boolean(pump_switch)); 
          break;
        }
      case 3:
        {
          json_object_object_add(send_json_object, "ip_address", json_object_new_string(app_context->mico_context->micoStatus.localIp)); 
          break;
        }
      case 4:
        {
          json_object_object_add(send_json_object, "hasImage", json_object_new_boolean(true)); 
          break;
        }
      default:
        {
          break;
        }
      }   
      // add temperature/humidity data into a json oject
      upload_data = json_object_to_json_string(send_json_object);
      if(NULL == upload_data){
        user_log("create upload data string error!");
        err = kNoMemoryErr;
      }
      else{
        // check fogcloud connect status
        if(app_context->appStatus.fogcloudStatus.isCloudConnected){
          // upload data string to fogcloud, the seconde param(NULL) means send to defalut topic: '<device_id>/out'
          MiCOFogCloudMsgSend(app_context, NULL, (unsigned char*)upload_data, strlen(upload_data));
          //            user_log("upload data success! \t topic=%s/out \t dht11_temperature=%d, dht11_humidity=%d", 
          //                     app_context->appConfig->fogcloudConfig.deviceId,
          //                     dht11_temperature, dht11_humidity);
          err = kNoErr;
        }
      }
    }
    // free json object memory
    json_object_put(send_json_object);
    send_json_object = NULL;    
    
  }
exit:
  if(kNoErr != err){
    user_log("ERROR: user_uptream exit with err=%d", err);
  }
  mico_rtos_delete_thread(NULL);  // delete current thread
  
}