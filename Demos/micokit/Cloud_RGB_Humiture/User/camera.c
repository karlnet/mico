/******************************************************************************
拍照前[可选指令]
设置图片尺寸(默认值：320X240，修改尺寸需复位)
设置图片压缩率(默认值：36)

拍照片的指令顺序：
1.清空图片缓存
2.拍照指令
3.读图片长度指令
4.读图片数据指令


******************************************************************************/
#include "camera.h"

//#if USING_CAMERA_FOR_MMS

#define CLEAR_FRAME            1     //去掉返回图片数据携带的协议头和尾76 00 32 00
#define ECHO_CMD_DEBUG_INFO    0     //1，开启指令调试；0，关闭

#define camera_log(M, ...) custom_log("USER_DOWNSTREAM", M, ##__VA_ARGS__)


//复位指令与复位回复
const u8 reset_rsp[] = {0x76,0x00,0x26,0x00};
const u8 reset_cmd[] = {0x56,0x00,0x26,0x00};

//清除图片缓存指令与回复
const u8 photoBufCls_cmd [] = {0x56,0x00,0x36,0x01,0x02};
const u8 photoBufCls_rsp[] = {0x76,0x00,0x36,0x00,0x00};  	

//拍照指令与回复
const u8 start_photo_cmd[] = {0x56,0x00,0x36,0x01,0x00};    
const u8 start_photo_rsp[] = {0x76,0x00,0x36,0x00,0x00};   

//读图片长度指令与回复
//图片长度指令回复的前7个字节是固定的，最后2个字节表示图片的长度
//如0xA0,0x00,10进制表示是40960,即图片长度(大小)为40K
const u8 read_len_cmd[] = {0x56,0x00,0x34,0x01,0x00};
const u8 read_len_rsp[] = {0x76,0x00,0x34,0x00,0x04,0x00,0x00};

//读图片数据指令与回复
//get_photo_cmd前6个字节是固定的，
//第9,10字节是图片的起始地址
//第13,14字节是图片的末尾地址，即本次读取的长度

//如果是一次性读取，第9,10字节的起始地址是0x00,0x00;
//第13,14字节是图片长度的高字节，图片长度的低字节(如0xA0,0x00)

//如果是分次读取，每次读N字节(N必须是8的倍数)长度，
//则起始地址最先从0x00,0x00读取N长度(即N & 0xff00, N & 0x00ff)，
//后几次读的起始地址就是上一次读取数据的末尾地址
const u8 get_photo_cmd [] = {0x56,0x00,0x32,0x0C,0x00,0x0A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF }; 
const u8 get_photo_rsp []  = {0x76,0x00,0x32,0x00,0x00};

//设置压缩率指令与回复，最后1个字节为压缩率选项
//范围是：00 - FF
//默认压缩率是36
const u8 set_compress_cmd [] = {0x56,0x00,0x31,0x05,0x01,0x01,0x12,0x04,0x36};
const u8 compress_rate_rsp [] = {0x76,0x00,0x31,0x00,0x00};

//设置图片尺寸指令与回复
//set_photo_size_cmd最后1个字节的意义
//0x22 - 160X120
//0x11 - 320X240
//0x00 - 640X480
const u8 set_photo_size_cmd [] = {0x56,0x00,0x31,0x05,0x04,0x01,0x00,0x19,0x11};
const u8 set_photo_size_rsp [] = {0x76,0x00,0x31,0x00,0x00 };

//读取图片尺寸指令与回复
//read_photo_size_rsp最后1个字节的意义
//0x22 - 160X120
//0x11 - 320X240
//0x00 - 640X480
const u8 read_photo_size_cmd [] = {0x56,0x00,0x30,0x04,0x04,0x01,0x00,0x19};
const u8 read_photo_size_rsp [] = {0x76,0x00,0x30,0x00,0x01,0x00};

u32 picLen = 0;   //数据长度

//CommandPacket和ResponsePacket用于拷贝只读区的指令及应答到内存
u8 CommandPacket[16];
u8 ResponsePacket[7];

#define ID_SERIAL_NUM       1        //序号在数组的所在位置

/****************************************************************
函数名：SetSerailNumber
函数描述: 修改协议中的序号
输入参数：目标指令缓存首地址，源指令首地址，源指令长度，
          目标应答缓存首地址，源应答首地址，源应答长度，需要修改的
          序号值
返回:无
******************************************************************/		
void SetSerailNumber(u8 *DstCmd, const u8 *SrcCmd, u8 SrcCmdLength,
                     u8 *DstRsp, const u8 *SrcRsp, u8 SrcRspLength,u8 nID)
{
    memset(&CommandPacket,'\0',sizeof(CommandPacket));
    memset(&ResponsePacket,'\0',sizeof(ResponsePacket));
    
    memcpy(DstCmd,SrcCmd,SrcCmdLength);
    memcpy(DstRsp,SrcRsp,SrcRspLength);
    
    DstCmd[ID_SERIAL_NUM] = nID & 0xFF;
    DstRsp[ID_SERIAL_NUM] = nID & 0xFF;

}

/****************************************************************
函数名：cam_write
函数描述: 接口函数，写入控制摄像头的串口
输入参数：数据的首地址，长度
返回:无
******************************************************************/		
void cam_write(const u8 *buf,u8 len)
{
  
#if ECHO_CMD_DEBUG_INFO
  camera_log("uart send command length=%d",len);
  MicoUartSend(STDIO_UART, buf, len);
  camera_log("uart send command length end ");
#endif
  
  MicoGpioOutputHigh( (mico_gpio_t)MICO_GPIO_2 );
  
  
  user_uartSend( buf,len);
  
 
  MicoGpioOutputLow( (mico_gpio_t)MICO_GPIO_2 );
 
}

/****************************************************************
函数名：cam_receiver
函数描述：接口函数，读取控制摄像头的串口
输出参数：接收数据的地址，长度
返回:接收到数据个数
******************************************************************/		
u16 cam_receiver(u8 *inBuf,u16 inBufLen)
{    
  int len=user_uartRecv(inBuf, inBufLen);  
  
#if ECHO_CMD_DEBUG_INFO
  camera_log("uart receive command length=%d",len);
    MicoUartSend(STDIO_UART, inBuf, inBufLen);
#endif
  
  return len;
  
}

/****************************************************************
函数名：camera_init
函数描述：摄像头初始化
输入参数：序列号，需要设置的图片尺寸
返回:初始化成功返回1，初始化失败返回0
******************************************************************/		

u8 camera_init(u8 Serialnumber,u8 nSetImageSize)
{    
    u8 CurrentImageSize = 0xFF;
    u8 CurrentCompressRate = COMPRESS_RATE_36;
    
    //读取当前的图片尺寸到currentImageSize
    if ( !current_photo_size(Serialnumber,&CurrentImageSize))
    {
        camera_log("\r\nread_photo_size error\r\n");
        return 0;
    }
    
    //判断是否需要修改图片尺寸
    if(nSetImageSize != CurrentImageSize)
    {
        //设置图片尺寸，设置后复位生效，该项设置后会永久保存
        if ( !send_photo_size(Serialnumber,nSetImageSize))
        {
            camera_log("\r\nset_photo_size error\r\n");
            return 0;
        }
        else
        {
            //复位生效
            if ( !send_reset(Serialnumber))
            {
                camera_log("\r\reset error\r\n");
                return 0;
            }
            mico_thread_msleep(1000);
            CurrentImageSize = nSetImageSize;
        }
        
    }
    
    //给不同图片尺寸设置适当的图片压缩率
    if(nSetImageSize == CurrentImageSize)
    {
        switch(CurrentImageSize)
        {
            case IMAGE_SIZE_160X120:
            case IMAGE_SIZE_320X240:
                 CurrentCompressRate = COMPRESS_RATE_36;
                 break;
            case IMAGE_SIZE_640X480:
                 CurrentCompressRate = COMPRESS_RATE_60;
                 break;
            default:
                break;
        }
    }
    //设置图片压缩率，该项不保存，每次上电后需重新设置
    if ( !send_compress_rate(Serialnumber,CurrentCompressRate))
    {
        camera_log("\r\nsend_compress_rate error%02X\r\n",CurrentCompressRate);
        return 0;
    }

    //这里要注意,设置压缩率后要延时
    mico_thread_msleep(100);

    return 1;
    
}

/****************************************************************
 函数名：send_cmd
 函数描述：发送指令并识别指令返回
 输入参数：指令的首地址，指令的长度，匹配指令的首地址，需验证的个数
 返回：成功返回1,失败返回0
******************************************************************/	
u8 send_cmd( const u8 *cmd,u8 n0,const u8 *rev,u8 n1)
{
    u8  i;
    u8  tmp[5] = {0x00,0x00,0x00,0x00,0x00};

  
    cam_write(cmd, n0);

    if ( !cam_receiver(tmp,5) ) 
    {
       
        return 0;
    }
  
    //检验数据
    for (i = 0; i < n1; i++)
    {  
        if (tmp[i] != rev[i]) 
        {
         
            return 0;
        }
    }
    
    return 1;

}


/****************************************************************
函数名：current_photo_size
函数描述:读取当前设置的图片尺寸
输入参数：Serialnumber序列号，nImageSize传递图片尺寸的引用变量
返回:成功返回1,失败返回0
******************************************************************/	
u8 current_photo_size(u8 Serialnumber,u8 * nImageSize)
{  
    u8  i;
    u8  tmp[6] = {0x00,0x00,0x00,0x00,0x00,0x00};

    SetSerailNumber( CommandPacket,
                     read_photo_size_cmd,
                     sizeof(read_photo_size_cmd),
                     ResponsePacket,
                     read_photo_size_rsp,
                     sizeof(read_photo_size_rsp),
                     Serialnumber );
      
    cam_write(CommandPacket, sizeof(read_photo_size_cmd));

    if ( !cam_receiver(tmp,6) ) 
    {
        return 0;
    }
   
    //检验数据,对比前5个字节
    for (i = 0; i < 5; i++)
    {  
        if (tmp[i] != ResponsePacket[i]) 
        {
            return 0;
        }
    }
    
    //最后一个字节表示当前的图片大小
    *nImageSize = tmp[5];
    return 1;
}


/****************************************************************
函数名：send_photo_size
函数描述：设置拍照的图片尺寸（可选择：160X120,320X240,640X480）
输入参数：序列号，需要设置的图片尺寸
返回:成功返回1,失败返回0
******************************************************************/	
u8 send_photo_size(u8 Serialnumber,u8 nImageSize)
{  
    u8  i;
    
    SetSerailNumber( CommandPacket,
                     set_photo_size_cmd,
                     sizeof(set_photo_size_cmd),
                     ResponsePacket,
                     set_photo_size_rsp,
                     sizeof(set_photo_size_rsp),
                     Serialnumber );
    
    CommandPacket [sizeof(set_photo_size_cmd) - 1] = nImageSize;
    
    i = send_cmd( CommandPacket,
                  sizeof(set_photo_size_cmd),
                  ResponsePacket,
                  sizeof(set_photo_size_rsp) );
    return i;
}


/****************************************************************
函数名：send_reset
函数描述：发送复位指令复位后要延时1-2秒
输入参数：序列号
返回:成功返回1 失败返回0
******************************************************************/		
u8 send_reset(u8 Serialnumber)
{  
    u8 i;
    //复制命令与应答，修改序号
    SetSerailNumber( CommandPacket,
                     reset_cmd,
                     sizeof(reset_cmd),
                     ResponsePacket,
                     reset_rsp,
                     sizeof(reset_rsp),
                     Serialnumber );
    
    i = send_cmd( CommandPacket,
                  sizeof(reset_cmd),
                  ResponsePacket,
                  sizeof(reset_rsp) );
    
    return i;

}

/****************************************************************
函数名：send_stop_photo
函数描述：清空图片缓存
输入参数：序列号
返回:成功返回1,失败返回0
******************************************************************/		 
u8 send_photoBuf_cls(u8 Serialnumber)
{ 
    u8 i;
   
    SetSerailNumber( CommandPacket,
                     photoBufCls_cmd,
                     sizeof(photoBufCls_cmd),
                     ResponsePacket,
                     photoBufCls_rsp,
                     sizeof(photoBufCls_rsp),
                     Serialnumber );
   
    i = send_cmd( CommandPacket,
                  sizeof(photoBufCls_cmd),
                  ResponsePacket,
                  sizeof(photoBufCls_rsp) );
    return i;
}  


/****************************************************************
函数名：send_compress_rate
函数描述：发送设置图片压缩率
输入参数：序列号
返回:成功返回1,失败返回0
******************************************************************/		 
u8 send_compress_rate(u8 Serialnumber,u8 nCompressRate)
{
    u8 i;
    
    SetSerailNumber( CommandPacket,
                     set_compress_cmd,
                     sizeof(set_compress_cmd),
                     ResponsePacket,
                     compress_rate_rsp,
                     sizeof(compress_rate_rsp),
                     Serialnumber );
    
    if(nCompressRate > 0x36)
    {
        //最后一个字节表示压缩率
        CommandPacket [sizeof(set_compress_cmd) - 1] = nCompressRate;
    }
    
    i = send_cmd( CommandPacket,
                  sizeof(set_compress_cmd),
                  ResponsePacket,
                  sizeof(compress_rate_rsp) );
    return i;
}


/****************************************************************
函数名：send_start_photo
函数描述：发送开始拍照的指令
输入参数：序列号
返回:识别成功返回1 失败返回0
******************************************************************/		
u8 send_start_photo(u8 Serialnumber)
{
    u8 i;
    
    SetSerailNumber( CommandPacket,
                     start_photo_cmd,
                     sizeof(start_photo_cmd),
                     ResponsePacket,
                     start_photo_rsp,
                     sizeof(start_photo_rsp),
                     Serialnumber );
    
    i = send_cmd( CommandPacket,
                  sizeof(start_photo_cmd),
                  ResponsePacket,
                  sizeof(start_photo_rsp) );
    return i;
}	  


/****************************************************************
函数名：send_read_len
函数描述：读取拍照后的图片长度，即图片占用空间大小
输入参数：序列号
返回:图片的长度
******************************************************************/	
u32 send_read_len(u8 Serialnumber)
{
    u8 i;
    u32 len;
    u8 tmp[9];	
    
    SetSerailNumber( CommandPacket,
                     read_len_cmd,
                     sizeof(read_len_cmd),
                     ResponsePacket,
                     read_len_rsp,
                     sizeof(read_len_rsp),
                     Serialnumber );
    
    //发送读图片长度指令
    cam_write(CommandPacket, 5);

    if ( !cam_receiver(tmp,9)) 
    {
        return 0;
    }

    //检验数据
    for (i = 0; i < 7; i++)
    {
        if ( tmp[i] != ResponsePacket[i]) 
        {
            return 0;
        }
    }
    
    len = (u32)tmp[7] << 8;//高字节
    len |= tmp[8];//低字节
    
    return len;
}


/****************************************************************
函数名：send_get_photo
函数描述：读取图片数据
输入参数：读图片起始地址StaAdd, 
          读取的长度readLen ，
          接收数据的缓冲区buf
          序列号
返回:成功返回1，失败返回0
FF D8 ... FF D9 是JPG的图片格式

1.一次性读取的回复格式：76 00 32 00 00 FF D8 ... FF D9 76 00 32 00 00

2.分次读取，每次读N字节,循环使用读取图片数据指令读取M次或者(M + 1)次读取完毕：
如第一次执行后回复格式
76 00 32 00 <FF D8 ... N> 76 00 32 00
下次执行读取指令时，起始地址需要偏移N字节，即上一次的末尾地址，回复格式
76 00 32 00 <... N> 76 00 32 00
......
76 00 32 00 <... FF D9> 76 00 32 00 //lastBytes <= N

Length = N * M 或 Length = N * M + lastBytes

******************************************************************/	
u8 send_get_photo(u16 staAdd,u16 readLen,u8 *buf,u8 Serialnumber)
{
    u8 i = 0;
    u8 *ptr = NULL;
    
    
    SetSerailNumber( CommandPacket,
                     get_photo_cmd,
                     sizeof(get_photo_cmd),
                     ResponsePacket,
                     get_photo_rsp,
                     sizeof(get_photo_rsp),
                     Serialnumber );
    
    //装入起始地址高低字节
    CommandPacket[8] = (staAdd >> 8) & 0xff;
    CommandPacket[9] = staAdd & 0xff;
    //装入末尾地址高低字节
    CommandPacket[12] = (readLen >> 8) & 0xff;
    CommandPacket[13] = readLen & 0xff;
    
    //执行指令
    cam_write(CommandPacket,16);
    
    //等待图片数据存储到buf，超时或无数据回复则返回0
    if ( !cam_receiver(buf,readLen + 10))
    {
        return 0;
    }
    
    //检验帧头76 00 32 00 00
    for (i = 0; i < 5; i++)
    {
        if ( buf[i] != ResponsePacket[i] )
        {
            return 0;
        }
    }

    //检验帧尾76 00 32 00 00
    for (i = 0; i < 5; i++)
    {
        if ( buf[i + 5 + readLen] != ResponsePacket[i] )
        {
            return 0;
        }
    }
    
    for (i = 0; i < 5; i++)
    {
        if ( buf[i + 5 + readLen] != ResponsePacket[i] )
        {
            return 0;
        }
    }
    if( ( buf[5]!=0xFF)&& ( buf[6]!=0xD8)&&  ( buf[readLen+5-2]!=0xFF)&&( buf[readLen+5-1]!=0xD9))             {
      return 0;
    }
    //宏开关选择丢弃/保留 帧头帧尾76 00 32 00 00
    #if CLEAR_FRAME
//    memcpy(buf,buf + 5,read_len);
    ptr = buf;
    
    for (; readLen > 0; ++ptr)
    {
        *(ptr) = *(ptr + 5);
        readLen--;
    }
    #endif
    
    return 1;
}


//#endif




