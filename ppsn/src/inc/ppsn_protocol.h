/******************************************************************************
 * 文件名称： ppsn_protocol.h
 * 描    述:  项目所有协议的集合。
 * 创建日期:  2013-10-7
 * 作    者： rainewang
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
    uint32_t dwUID;                     //表示用户的ID
    uint32_t dwK;                       //矩阵的宽
    uint32_t dwL;                       //矩阵的高
    double arMatrix[0];                  //矩阵数据
};

typedef struct stUserRsp{
    uint32_t dwUID;                     //表示用户ID
    uint32_t dwUserNum;                 //返回的User个数
    uint32_t arUserID[0];               //返回的用户数组
};

#pragma pack()


#endif