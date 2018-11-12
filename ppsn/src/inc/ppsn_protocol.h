/******************************************************************************
 * �ļ����ƣ� ppsn_protocol.h
 * ��    ��:  ��Ŀ����Э��ļ��ϡ�
 * ��������:  2013-10-7
 * ��    �ߣ� rainewang
 ******************************************************************************/

#ifndef __PPSN_PROTOCOL_H__
#define __PPSN_PROTOCOL_H__

#include <stdlib.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <string.h>

enum emCommand
{
    Single_QueryUser,
    Single_AddUser,
    Single_DelUser,
    Batch_QueryUser,
    Batch_AddUser,
    Batch_DelUser
};

enum emCommandState
{
    Success,
    Error
};

#pragma pack(1)

typedef struct stUserReq{
    uint32_t dwUID;                     //��ʾ�û���ID
    uint32_t dwK;                       //����Ŀ�
    uint32_t dwL;                       //����ĸ�
    double arMatrix[0];                  //��������
};

typedef struct stUserRsp{
    uint32_t dwUID;                     //��ʾ�û�ID
    uint32_t dwUserNum;                 //���ص�User����
    uint32_t arUserID[0];               //���ص��û�����
};

#pragma pack()


#endif