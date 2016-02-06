#ifndef  __CAMERA_H__
#define  __CAMERA_H__


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

//#if	USING_CAMERA_FOR_MMS            //�����꣬�ж��Ƿ���Ҫʹ������ͷ���շ�����

#include <string.h>
#include <stdio.h>
#include "mico.h"

typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;

//�û��Զ�������
#define N_BYTE  256        //ÿ�ζ�ȡN_BYTE�ֽڣ�N_BYTE������8�ı���

#define IMAGE_SIZE_160X120     0x22
#define IMAGE_SIZE_320X240     0x11
#define IMAGE_SIZE_640X480     0x00


#define COMPRESS_RATE_36       0x36   //��ѹ������Ĭ��ѹ���ʣ�160x120��320x240���ô�ѹ����

#define COMPRESS_RATE_60       0x60   //640X480�ߴ磬Ĭ��ѹ����36��ռ��45K���ҵĿռ�
                                      //ѡ��60ѹ���ʿɽ�45Kѹ����20K����
extern u32 picLen;//ͼƬ����

//���������ӿں�������ֲʱ��Ҫ�޸Ľӿں���
void cam_write(const u8 *buf,u8 len);
u16 cam_receiver( u8 *buf,u16 send_len);


// Ӧ��ʵ������

u8 camera_init(u8 Serialnumber,u8 nSetImageSize);
u8 send_cmd(const u8 *cmd,u8 n0,const u8 *rev,u8 n1);
void SetSerailNumber(u8 *DstCmd, const u8 *SrcCmd, u8 SrcCmdLength,
                     u8 *DstRcv, const u8 *SrcRcv, u8 SrcRcvLength,u8 nID);

//����ͷ�������ã���λ/ͼƬ�ߴ��С/ͼƬѹ����
u8 send_reset(u8 Serialnumber);
u8 current_photo_size(u8 Serialnumber,u8 * nImageSize);
u8 send_photo_size(u8 Serialnumber,u8 nImageSize);
u8 send_compress_rate(u8 Serialnumber,u8 nCompressRate);

//���մ�����
u8 send_photoBuf_cls(u8 Serialnumber);
u8 send_start_photo(u8 Serialnumber);
u32 send_read_len(u8 Serialnumber);
u8 send_get_photo(u16 add,u16 read_len,u8 *buf,u8 Serialnumber);



//#endif

#endif	


