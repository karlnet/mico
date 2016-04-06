/******************************************************************************
����ǰ[��ѡָ��]
����ͼƬ�ߴ�(Ĭ��ֵ��320X240���޸ĳߴ��踴λ)
����ͼƬѹ����(Ĭ��ֵ��36)

����Ƭ��ָ��˳��
1.���ͼƬ����
2.����ָ��
3.��ͼƬ����ָ��
4.��ͼƬ����ָ��


******************************************************************************/
#include "camera.h"

//#if USING_CAMERA_FOR_MMS

#define CLEAR_FRAME            1     //ȥ������ͼƬ����Я����Э��ͷ��β76 00 32 00
#define ECHO_CMD_DEBUG_INFO    0     //1������ָ����ԣ�0���ر�

#define camera_log(M, ...) custom_log("USER_DOWNSTREAM", M, ##__VA_ARGS__)


//��λָ���븴λ�ظ�
const u8 reset_rsp[] = {0x76,0x00,0x26,0x00};
const u8 reset_cmd[] = {0x56,0x00,0x26,0x00};

//���ͼƬ����ָ����ظ�
const u8 photoBufCls_cmd [] = {0x56,0x00,0x36,0x01,0x02};
const u8 photoBufCls_rsp[] = {0x76,0x00,0x36,0x00,0x00};  	

//����ָ����ظ�
const u8 start_photo_cmd[] = {0x56,0x00,0x36,0x01,0x00};    
const u8 start_photo_rsp[] = {0x76,0x00,0x36,0x00,0x00};   

//��ͼƬ����ָ����ظ�
//ͼƬ����ָ��ظ���ǰ7���ֽ��ǹ̶��ģ����2���ֽڱ�ʾͼƬ�ĳ���
//��0xA0,0x00,10���Ʊ�ʾ��40960,��ͼƬ����(��С)Ϊ40K
const u8 read_len_cmd[] = {0x56,0x00,0x34,0x01,0x00};
const u8 read_len_rsp[] = {0x76,0x00,0x34,0x00,0x04,0x00,0x00};

//��ͼƬ����ָ����ظ�
//get_photo_cmdǰ6���ֽ��ǹ̶��ģ�
//��9,10�ֽ���ͼƬ����ʼ��ַ
//��13,14�ֽ���ͼƬ��ĩβ��ַ�������ζ�ȡ�ĳ���

//�����һ���Զ�ȡ����9,10�ֽڵ���ʼ��ַ��0x00,0x00;
//��13,14�ֽ���ͼƬ���ȵĸ��ֽڣ�ͼƬ���ȵĵ��ֽ�(��0xA0,0x00)

//����Ƿִζ�ȡ��ÿ�ζ�N�ֽ�(N������8�ı���)���ȣ�
//����ʼ��ַ���ȴ�0x00,0x00��ȡN����(��N & 0xff00, N & 0x00ff)��
//�󼸴ζ�����ʼ��ַ������һ�ζ�ȡ���ݵ�ĩβ��ַ
const u8 get_photo_cmd [] = {0x56,0x00,0x32,0x0C,0x00,0x0A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF }; 
const u8 get_photo_rsp []  = {0x76,0x00,0x32,0x00,0x00};

//����ѹ����ָ����ظ������1���ֽ�Ϊѹ����ѡ��
//��Χ�ǣ�00 - FF
//Ĭ��ѹ������36
const u8 set_compress_cmd [] = {0x56,0x00,0x31,0x05,0x01,0x01,0x12,0x04,0x36};
const u8 compress_rate_rsp [] = {0x76,0x00,0x31,0x00,0x00};

//����ͼƬ�ߴ�ָ����ظ�
//set_photo_size_cmd���1���ֽڵ�����
//0x22 - 160X120
//0x11 - 320X240
//0x00 - 640X480
const u8 set_photo_size_cmd [] = {0x56,0x00,0x31,0x05,0x04,0x01,0x00,0x19,0x11};
const u8 set_photo_size_rsp [] = {0x76,0x00,0x31,0x00,0x00 };

//��ȡͼƬ�ߴ�ָ����ظ�
//read_photo_size_rsp���1���ֽڵ�����
//0x22 - 160X120
//0x11 - 320X240
//0x00 - 640X480
const u8 read_photo_size_cmd [] = {0x56,0x00,0x30,0x04,0x04,0x01,0x00,0x19};
const u8 read_photo_size_rsp [] = {0x76,0x00,0x30,0x00,0x01,0x00};

u32 picLen = 0;   //���ݳ���

//CommandPacket��ResponsePacket���ڿ���ֻ������ָ�Ӧ���ڴ�
u8 CommandPacket[16];
u8 ResponsePacket[7];

#define ID_SERIAL_NUM       1        //��������������λ��

/****************************************************************
��������SetSerailNumber
��������: �޸�Э���е����
���������Ŀ��ָ����׵�ַ��Դָ���׵�ַ��Դָ��ȣ�
          Ŀ��Ӧ�𻺴��׵�ַ��ԴӦ���׵�ַ��ԴӦ�𳤶ȣ���Ҫ�޸ĵ�
          ���ֵ
����:��
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
��������cam_write
��������: �ӿں�����д���������ͷ�Ĵ���
������������ݵ��׵�ַ������
����:��
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
��������cam_receiver
�����������ӿں�������ȡ��������ͷ�Ĵ���
����������������ݵĵ�ַ������
����:���յ����ݸ���
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
��������camera_init
��������������ͷ��ʼ��
������������кţ���Ҫ���õ�ͼƬ�ߴ�
����:��ʼ���ɹ�����1����ʼ��ʧ�ܷ���0
******************************************************************/		

u8 camera_init(u8 Serialnumber,u8 nSetImageSize)
{    
    u8 CurrentImageSize = 0xFF;
    u8 CurrentCompressRate = COMPRESS_RATE_36;
    
    //��ȡ��ǰ��ͼƬ�ߴ絽currentImageSize
    if ( !current_photo_size(Serialnumber,&CurrentImageSize))
    {
        camera_log("\r\nread_photo_size error\r\n");
        return 0;
    }
    
    //�ж��Ƿ���Ҫ�޸�ͼƬ�ߴ�
    if(nSetImageSize != CurrentImageSize)
    {
        //����ͼƬ�ߴ磬���ú�λ��Ч���������ú�����ñ���
        if ( !send_photo_size(Serialnumber,nSetImageSize))
        {
            camera_log("\r\nset_photo_size error\r\n");
            return 0;
        }
        else
        {
            //��λ��Ч
            if ( !send_reset(Serialnumber))
            {
                camera_log("\r\reset error\r\n");
                return 0;
            }
            mico_thread_msleep(1000);
            CurrentImageSize = nSetImageSize;
        }
        
    }
    
    //����ͬͼƬ�ߴ������ʵ���ͼƬѹ����
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
    //����ͼƬѹ���ʣ�������棬ÿ���ϵ������������
    if ( !send_compress_rate(Serialnumber,CurrentCompressRate))
    {
        camera_log("\r\nsend_compress_rate error%02X\r\n",CurrentCompressRate);
        return 0;
    }

    //����Ҫע��,����ѹ���ʺ�Ҫ��ʱ
    mico_thread_msleep(100);

    return 1;
    
}

/****************************************************************
 ��������send_cmd
 ��������������ָ�ʶ��ָ���
 ���������ָ����׵�ַ��ָ��ĳ��ȣ�ƥ��ָ����׵�ַ������֤�ĸ���
 ���أ��ɹ�����1,ʧ�ܷ���0
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
  
    //��������
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
��������current_photo_size
��������:��ȡ��ǰ���õ�ͼƬ�ߴ�
���������Serialnumber���кţ�nImageSize����ͼƬ�ߴ�����ñ���
����:�ɹ�����1,ʧ�ܷ���0
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
   
    //��������,�Ա�ǰ5���ֽ�
    for (i = 0; i < 5; i++)
    {  
        if (tmp[i] != ResponsePacket[i]) 
        {
            return 0;
        }
    }
    
    //���һ���ֽڱ�ʾ��ǰ��ͼƬ��С
    *nImageSize = tmp[5];
    return 1;
}


/****************************************************************
��������send_photo_size
�����������������յ�ͼƬ�ߴ磨��ѡ��160X120,320X240,640X480��
������������кţ���Ҫ���õ�ͼƬ�ߴ�
����:�ɹ�����1,ʧ�ܷ���0
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
��������send_reset
�������������͸�λָ�λ��Ҫ��ʱ1-2��
������������к�
����:�ɹ�����1 ʧ�ܷ���0
******************************************************************/		
u8 send_reset(u8 Serialnumber)
{  
    u8 i;
    //����������Ӧ���޸����
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
��������send_stop_photo
�������������ͼƬ����
������������к�
����:�ɹ�����1,ʧ�ܷ���0
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
��������send_compress_rate
������������������ͼƬѹ����
������������к�
����:�ɹ�����1,ʧ�ܷ���0
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
        //���һ���ֽڱ�ʾѹ����
        CommandPacket [sizeof(set_compress_cmd) - 1] = nCompressRate;
    }
    
    i = send_cmd( CommandPacket,
                  sizeof(set_compress_cmd),
                  ResponsePacket,
                  sizeof(compress_rate_rsp) );
    return i;
}


/****************************************************************
��������send_start_photo
�������������Ϳ�ʼ���յ�ָ��
������������к�
����:ʶ��ɹ�����1 ʧ�ܷ���0
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
��������send_read_len
������������ȡ���պ��ͼƬ���ȣ���ͼƬռ�ÿռ��С
������������к�
����:ͼƬ�ĳ���
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
    
    //���Ͷ�ͼƬ����ָ��
    cam_write(CommandPacket, 5);

    if ( !cam_receiver(tmp,9)) 
    {
        return 0;
    }

    //��������
    for (i = 0; i < 7; i++)
    {
        if ( tmp[i] != ResponsePacket[i]) 
        {
            return 0;
        }
    }
    
    len = (u32)tmp[7] << 8;//���ֽ�
    len |= tmp[8];//���ֽ�
    
    return len;
}


/****************************************************************
��������send_get_photo
������������ȡͼƬ����
�����������ͼƬ��ʼ��ַStaAdd, 
          ��ȡ�ĳ���readLen ��
          �������ݵĻ�����buf
          ���к�
����:�ɹ�����1��ʧ�ܷ���0
FF D8 ... FF D9 ��JPG��ͼƬ��ʽ

1.һ���Զ�ȡ�Ļظ���ʽ��76 00 32 00 00 FF D8 ... FF D9 76 00 32 00 00

2.�ִζ�ȡ��ÿ�ζ�N�ֽ�,ѭ��ʹ�ö�ȡͼƬ����ָ���ȡM�λ���(M + 1)�ζ�ȡ��ϣ�
���һ��ִ�к�ظ���ʽ
76 00 32 00 <FF D8 ... N> 76 00 32 00
�´�ִ�ж�ȡָ��ʱ����ʼ��ַ��Ҫƫ��N�ֽڣ�����һ�ε�ĩβ��ַ���ظ���ʽ
76 00 32 00 <... N> 76 00 32 00
......
76 00 32 00 <... FF D9> 76 00 32 00 //lastBytes <= N

Length = N * M �� Length = N * M + lastBytes

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
    
    //װ����ʼ��ַ�ߵ��ֽ�
    CommandPacket[8] = (staAdd >> 8) & 0xff;
    CommandPacket[9] = staAdd & 0xff;
    //װ��ĩβ��ַ�ߵ��ֽ�
    CommandPacket[12] = (readLen >> 8) & 0xff;
    CommandPacket[13] = readLen & 0xff;
    
    //ִ��ָ��
    cam_write(CommandPacket,16);
    
    //�ȴ�ͼƬ���ݴ洢��buf����ʱ�������ݻظ��򷵻�0
    if ( !cam_receiver(buf,readLen + 10))
    {
        return 0;
    }
    
    //����֡ͷ76 00 32 00 00
    for (i = 0; i < 5; i++)
    {
        if ( buf[i] != ResponsePacket[i] )
        {
            return 0;
        }
    }

    //����֡β76 00 32 00 00
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
    //�꿪��ѡ����/���� ֡ͷ֡β76 00 32 00 00
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




