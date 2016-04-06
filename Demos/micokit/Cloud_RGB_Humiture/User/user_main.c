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
#include "MICOAppDefine.h"
#include "MicoFogCloud.h"
#include "user_uart.h"
#include "json_c/json.h"
#include "VGM128064/oled.h"
#include "P9813/hsb2rgb_led.h"
#include "camera.h"
#include "am2301.h"
/* User defined debug log functions
 * Add your own tag like: 'USER', the tag will be added at the beginning of a log
 * in MICO debug uart, when you call this function.
 */
#define user_log(M, ...) custom_log("USER", M, ##__VA_ARGS__)
#define user_log_trace() custom_log_trace("USER")

// DTH11 termperature && humidity data to display on OLED
extern uint8_t dht11_temperature;
extern uint8_t dht11_humidity;

/* rgb led control variants, use hsb color.
* h -- hues
* s -- saturation
* b -- brightness
*/
bool rgbled_switch = false;
volatile bool rgbled_changed = false;  // rgb led state changed flag



int rgbled_hues = 0;
int rgbled_saturation = 0;
int rgbled_brightness = 128;

// device on/off status
volatile bool device_switch = true;  // device state true: on , false: off
volatile bool device_switch_changed = false;  // device state changed flag



static mico_thread_t user_downstrem_thread_handle = NULL;
static mico_thread_t user_upstream_thread_handle = NULL;
static mico_thread_t user2_upstream_thread_handle = NULL;
static mico_thread_t take_photo_thread_handle = NULL;

extern void user_downstream_thread(void* arg);
extern void user_upstream_thread(void* arg);
extern void user2_upstream_thread(void* arg);
extern void take_photo_thread(void* arg);

/* user main function, called by AppFramework after system init done && wifi
 * station on in user_main thread.
 */
OSStatus user_main( app_context_t * const app_context )
{
  user_log_trace();
  OSStatus err = kUnknownErr;
  // one line on OLED, 16 chars max, we just update line2~4 for the first line is not need to change
  char oled_show_line[OLED_DISPLAY_MAX_CHAR_PER_ROW+1] = {'\0'};
  
  err = user_uartInit();
  require_noerr_action( err, exit, user_log("ERROR: user_uartInit err = %d.", err) );
  
  
  MicoGpioInitialize( (mico_gpio_t)MICO_GPIO_2, OUTPUT_PUSH_PULL );     // 485 direction
  
   MicoGpioInitialize( (mico_gpio_t)MICO_GPIO_17, OUTPUT_PUSH_PULL );   // i2c scl , am2301
  
  MicoGpioInitialize( (mico_gpio_t)MICO_GPIO_19, OUTPUT_PUSH_PULL );
  MicoGpioInitialize( (mico_gpio_t)MICO_GPIO_35, OUTPUT_PUSH_PULL );  
  MicoGpioInitialize( (mico_gpio_t)MICO_GPIO_34, OUTPUT_PUSH_PULL );
  MicoGpioInitialize( (mico_gpio_t)MICO_GPIO_36, OUTPUT_PUSH_PULL );
  MicoGpioInitialize( (mico_gpio_t)MICO_GPIO_37, OUTPUT_PUSH_PULL );
  MicoGpioInitialize( (mico_gpio_t)MICO_GPIO_38, OUTPUT_PUSH_PULL );
  MicoGpioInitialize( (mico_gpio_t)MICO_GPIO_27, OUTPUT_PUSH_PULL );
  MicoGpioInitialize( (mico_gpio_t)MICO_GPIO_16, OUTPUT_PUSH_PULL );
  
  
  MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_19 );
  MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_16 );
  MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_27 );
  MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_34 );
  MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_35 );
  MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_36);
  MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_37 );
  MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_38 );
  
  MicoGpioOutputHigh( (mico_gpio_t)MICO_GPIO_2 );  
  
//  user_log("start GPIO test  ...");
//  user_log("start 1 test...");
//  mico_thread_msleep(1*1000);
//  MicoGpioOutputHigh( (mico_gpio_t)MICO_GPIO_16 );
//  mico_thread_msleep(1*1000);
//  MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_16 );
//  user_log("start 2 test...");
//  mico_thread_msleep(1*1000);
//  MicoGpioOutputHigh( (mico_gpio_t)MICO_GPIO_19 );
//  mico_thread_msleep(1*1000);
//  MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_19 );
//  user_log("start 3 test...");
//  mico_thread_msleep(1*1000);
//  MicoGpioOutputHigh( (mico_gpio_t)MICO_GPIO_27 );
//  mico_thread_msleep(1*1000);
//  MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_27 );
//  user_log("start 4 test...");
//  mico_thread_msleep(1*1000);
//  MicoGpioOutputHigh( (mico_gpio_t)MICO_GPIO_34 );
//  mico_thread_msleep(1*1000);
//  MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_34 );  
//  user_log("start 5 test...");
//  mico_thread_msleep(1*1000);
//  MicoGpioOutputHigh( (mico_gpio_t)MICO_GPIO_35 );
//  mico_thread_msleep(1*1000);
//  MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_35 );
//  user_log("start 6 test...");
//  mico_thread_msleep(1*1000);
//  MicoGpioOutputHigh( (mico_gpio_t)MICO_GPIO_36 );
//  mico_thread_msleep(1*1000);
//  MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_36 );
//  user_log("start 7 test...");
//  mico_thread_msleep(1*1000);
//  MicoGpioOutputHigh( (mico_gpio_t)MICO_GPIO_37 );
//  mico_thread_msleep(1*1000);
//  MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_37 );
//  user_log("start 8 test...");
//  mico_thread_msleep(1*1000);
//  MicoGpioOutputHigh( (mico_gpio_t)MICO_GPIO_38 );
//  mico_thread_msleep(1*1000);
//  MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_38 );
  
    uint8_t ret = 0;
    /* init humiture sensor DHT11 */
  do{
    ret = AM2301_Init();
    if(0 != ret){  // init error
      user_log("AM2301 init failed!");
      err = kNoResourcesErr;
      mico_thread_sleep(1);  // sleep 1s then retry
    }
    else{
      err = kNoErr;
      break;
    }
  }while(kNoErr != err);
  
  /* thread loop */
  float AM2301_temperature = 0.0;
  float AM2301_humidity = 0.0;
  
//  MicoGpioInitialize( (mico_gpio_t)MICO_GPIO_17, OUTPUT_PUSH_PULL );
  while(1){
    
//  MicoGpioOutputHigh( (mico_gpio_t)MICO_GPIO_17 );
//  Delay_us(500); 
//  MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_17 );
//  Delay_us(500); 
    
  Delay_ms(2*1000);
  // do data acquisition
  ret = AM2301_Read_Data(&AM2301_temperature, &AM2301_humidity);
  if(0 != ret){
    err = kReadErr;
    user_log("AM2301 read failed!");
  }
  else{
    err = kNoErr;
    user_log("AM2301_temperature=%.2f, AM2301_humidity=%.2f",AM2301_temperature, AM2301_humidity);
  }
  }
  
  
//  MicoGpioOutputHigh( (mico_gpio_t)MICO_GPIO_2 );  
//  user_log("start uart test...");
//  unsigned char* image="hello world";
// 
//    user_uartSend( image,strlen(image));
////     mico_thread_msleep(100);
// 
//  char aa[200];  
//  memset(aa, '\0', 200);
//  MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_2 );
//  mico_thread_sleep(5);
//  int len=0;
//  len=user_uartRecv((unsigned char *)aa, 200);
//  user_log("uart_data_recv: [%d][%.*s]", len,  len,(unsigned char*)aa);
 
  require(app_context, exit);
  
//  hsb2rgb_led_init();   // init rgb led
//  hsb2rgb_led_close();  // close rgb led
// 
  
  //start camera thread 
  err = mico_rtos_create_thread(&take_photo_thread_handle, MICO_APPLICATION_PRIORITY, "take photo", 
                                take_photo_thread, STACK_SIZE_TAKE_PHOTO_THREAD, 
                                NULL );
  require_noerr_action( err, exit, user_log("ERROR: create take photo thread failed!") ); 
  
  // start the downstream thread to handle user command
  err = mico_rtos_create_thread(&user_downstrem_thread_handle, MICO_APPLICATION_PRIORITY, "user_downstream", 
                                user_downstream_thread, STACK_SIZE_USER_DOWNSTREAM_THREAD, 
                                app_context );
  require_noerr_action( err, exit, user_log("ERROR: create user_downstream thread failed!") );
    
  // start the upstream thread to upload temperature && humidity to user
  err = mico_rtos_create_thread(&user_upstream_thread_handle, MICO_APPLICATION_PRIORITY, "user_upstream", 
                                  user_upstream_thread, STACK_SIZE_USER_UPSTREAM_THREAD, 
                                  app_context );
  require_noerr_action( err, exit, user_log("ERROR: create user_uptream thread failed!") );
   // start the 2 upstream thread to upload temperature && humidity to user
  err = mico_rtos_create_thread(&user2_upstream_thread_handle, MICO_APPLICATION_PRIORITY, "user2_upstream", 
                                  user2_upstream_thread, STACK_SIZE_USER2_UPSTREAM_THREAD, 
                                  app_context );
  require_noerr_action( err, exit, user_log("ERROR: create user_uptream thread failed!") );
  
  
   user_log("test  camera...");

   
  // user_main loop, update oled display every 1s
  while(1){
    mico_thread_msleep(500);
    
    // device on/off change
    if(device_switch_changed){
      device_switch_changed = false;
      
      if(!device_switch){  // suspend upstream thread to stop working
        mico_rtos_suspend_thread(&user_upstream_thread_handle);
        hsb2rgb_led_close();
        OLED_Display_Off();
      }
      else{
        mico_rtos_resume_thread(&user_upstream_thread_handle);  // resume upstream thread to work again
        OLED_Display_On();  // resume OLED 
      }
    }
    
    // device on, update OLED && rgbled
    if(device_switch){
      // update OLED line 2~4
      OLED_ShowString(OLED_DISPLAY_COLUMN_START, OLED_DISPLAY_ROW_2, "Demo H/T && LED  ");  // line 2
      
      memset(oled_show_line, '\0', OLED_DISPLAY_MAX_CHAR_PER_ROW+1);
      snprintf(oled_show_line, OLED_DISPLAY_MAX_CHAR_PER_ROW+1, "T: %2dC         ", dht11_temperature);
      OLED_ShowString(OLED_DISPLAY_COLUMN_START, OLED_DISPLAY_ROW_3, (uint8_t*)oled_show_line);
      
      memset(oled_show_line, '\0', OLED_DISPLAY_MAX_CHAR_PER_ROW+1);
      snprintf(oled_show_line, OLED_DISPLAY_MAX_CHAR_PER_ROW+1, "H: %2d%%        ", dht11_humidity);
      OLED_ShowString(OLED_DISPLAY_COLUMN_START, OLED_DISPLAY_ROW_4, (uint8_t*)oled_show_line);
      
      // rgb led operation
      if(rgbled_changed){
        rgbled_changed = false;
        if(rgbled_switch){
          hsb2rgb_led_open(rgbled_hues, rgbled_saturation, rgbled_brightness);  // open rgb led
        }else{
          hsb2rgb_led_close();  // close rgb led
        }
      }
    }
  }

exit:
  if(kNoErr != err){
    user_log("ERROR: user_main thread exit with err=%d", err);
  }
  mico_rtos_delete_thread(NULL);  // delete current thread
  return err;
}
