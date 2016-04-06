/**
******************************************************************************
* @file    AM2301.c
* @author  Eshen Wang
* @version V1.0.0
* @date    1-May-2015
* @brief   AM2301 temperature and humidity sensor driver. 
******************************************************************************
* @attention
*
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDATAG CUSTOMERS
* WITH CODATAG INFORMATION REGARDATAG THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
* TIME. AS A RESULT, MXCHIP Inc. SHALL NOT BE HELD LIABLE FOR ANY
* DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
* FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
* CODATAG INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* <h2><center>&copy; COPYRIGHT 2014 MXCHIP Inc.</center></h2>
******************************************************************************
*/ 

#include "mico.h"
#include "am2301.h"
#define user_log(M, ...) custom_log("USER", M, ##__VA_ARGS__)
#define user_log_trace() custom_log_trace("USER")
/*------------------------------ delay function ------------------------------*/

//void Delay_us(uint32_t nus)
//{ 
//  MicoNanosendDelay( 1000*nus );
//}
//
//void Delay_ms(uint16_t nms)
//{	  	  
//  mico_thread_msleep(nms);	  	    
//}

/*--------------------------------- AM2301 Operations -------------------------*/

//Reset AM2301
void AM2301_Rst(void)	   
{                 
  AM2301_IO_OUT(); 						//SET OUTPUT
  AM2301_DATA_Clr(); 						//GPIOA.0=0
  Delay_ms(1);    					        //Pull down Least 1ms
  AM2301_DATA_Set(); 						//GPIOA.0=1 
  Delay_us(20);     						//Pull up 20~40us
}

uint8_t AM2301_Check(void) 	   
{   
  uint8_t retry=0;
  AM2301_IO_IN();						//SET INPUT	 
  while (AM2301_DATA&&retry<100)				//AM2301 Pull down 40~80us
  {
    retry++;
    Delay_us(1);
  }	 
  
  if(retry>=100)
    return 1;
  else 
    retry=0;
  
  while (!AM2301_DATA&&retry<100)				//AM2301 Pull up 40~80us
  {
    retry++;
    Delay_us(1);
  }
  
  if(retry>=100)
    return 1;							//chack error	    
   else 
    retry=0;
   
  while (AM2301_DATA&&retry<100)				//AM2301 Pull down 40~80us
  {
    retry++;
    Delay_us(1);
  }
  
  if(retry>=100)
    return 1;	
  
  return 0;
}

uint8_t AM2301_Read_Bit(void) 			 
{
  uint8_t retry=0;
  uint8_t data=0;
  
  while(!AM2301_DATA&&retry<100)			//wait 50us , Low level
  {
    retry++;
    Delay_us(1);
  }
  
  Delay_us(30);                 //wait 40us
  
  if(AM2301_DATA)
    data= 1;
   
  retry=0;
  while(AM2301_DATA&&retry<100)				// become High level 
  {
    retry++;
    Delay_us(1);
  }
  
 return data;	   
}

uint8_t AM2301_Read_Byte(void)    
{        
  uint8_t i,dat;
  dat=0;
  for (i=0;i<8;i++) 
  {
    dat<<=1; 
    dat|=AM2301_Read_Bit();
  }						    
  
  return dat;
}

uint8_t AM2301_Read_Data(float *temperature,float *humidity)    
{        
  uint8_t buf[5];
  uint8_t i,tmp;
  AM2301_Rst();
  if(AM2301_Check()==0)
  {
    for(i=0;i<5;i++)
    {
      buf[i]=AM2301_Read_Byte();      
    }
    // user_log("%d.%d.%d.%d.%d",buf[0],buf[1],buf[2],buf[3],buf[4]);
    
    tmp=buf[0]+buf[1]+buf[2]+buf[3];
    if((tmp==buf[4])||(tmp==256+buf[4]))
    {
      *humidity=(buf[0]*256+buf[1])/10.0;     
      *temperature=(buf[2]*256+buf[3])/10.0;
    }
    else{
      return 1;
    }
  }
  else {
    return 1;
  }
  
  return 0;	    
}

uint8_t AM2301_Init(void)
{	 
  AM2301_IO_OUT();
  
  AM2301_Rst();  
  return AM2301_Check();
} 
