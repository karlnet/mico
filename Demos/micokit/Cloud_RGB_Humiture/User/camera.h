#ifndef  __CAMERA_H__
#define  __CAMERA_H__


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

//#if	USING_CAMERA_FOR_MMS            //条件宏，判断是否需要使用摄像头拍照发彩信

#include <string.h>
#include <stdio.h>
#include "mico.h"

typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;

//用户自定义宏变量
#define N_BYTE  256        //每次读取N_BYTE字节，N_BYTE必须是8的倍数

#define IMAGE_SIZE_160X120     0x22
#define IMAGE_SIZE_320X240     0x11
#define IMAGE_SIZE_640X480     0x00


#define COMPRESS_RATE_36       0x36   //该压缩率是默认压缩率，160x120和320x240可用此压缩率

#define COMPRESS_RATE_60       0x60   //640X480尺寸，默认压缩率36会占用45K左右的空间
                                      //选择60压缩率可将45K压缩到20K左右
extern u32 picLen;//图片长度

//串口驱动接口函数，移植时需要修改接口函数
void cam_write(const u8 *buf,u8 len);
u16 cam_receiver( u8 *buf,u16 send_len);


// 应用实例函数

u8 camera_init(u8 Serialnumber,u8 nSetImageSize);
u8 send_cmd(const u8 *cmd,u8 n0,const u8 *rev,u8 n1);
void SetSerailNumber(u8 *DstCmd, const u8 *SrcCmd, u8 SrcCmdLength,
                     u8 *DstRcv, const u8 *SrcRcv, u8 SrcRcvLength,u8 nID);

//摄像头属性设置：复位/图片尺寸大小/图片压缩率
u8 send_reset(u8 Serialnumber);
u8 current_photo_size(u8 Serialnumber,u8 * nImageSize);
u8 send_photo_size(u8 Serialnumber,u8 nImageSize);
u8 send_compress_rate(u8 Serialnumber,u8 nCompressRate);

//拍照处理函数
u8 send_photoBuf_cls(u8 Serialnumber);
u8 send_start_photo(u8 Serialnumber);
u32 send_read_len(u8 Serialnumber);
u8 send_get_photo(u16 add,u16 read_len,u8 *buf,u8 Serialnumber);



//#endif

#endif	


