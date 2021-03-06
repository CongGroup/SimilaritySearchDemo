/******************************************************************************
 * 文件名称： cpkg_adapt.h
 * 类    名： CPkgAdapt
 * 描    述:  此类用于解析和组合通用包
 * 创建日期:  2013-10-3
 * 作    者： rainewang
 * 修改历史：
 ******************************************************************************/
#ifndef __CPKG_ADAPT_H__
#define __CPKG_ADAPT_H__

#include <stdint.h>
#include <string>
#include <string.h>
#include "cbase_ctl.h"

using namespace std;

enum emRecvPkgReturn
{
    EM_PKG_HEAD_ERROR = -1,                     //包头尾字符错误
    EM_PKG_NULL_ERROR = -2,                     //包指针为空
    EM_PKG_MIN_ERROR = -3,                      //接受到的包长度太小无法分析
    EM_PKG_LEN_ERROR = -4,                      //包接收长度与描述长度不等
    EM_PKG_SIZE_ERROR = -5,                     //包体长度与包头描述不等
    EM_PKG_RECV_SUCCESS = 0                     //成功接受包
};

/* 公共报文的开始, 结束标志 */
const char SEC_IS_STX = 0x02;
const char SEC_IS_ETX = 0x03;

/* 公共报文头签名长度 */
const uint32_t SEC_IS_ECHO_LEN = 64;
const uint32_t SIZEOF_STX_ETX = sizeof(char) *2;
const uint32_t DW_PKG_LEN = 500000;

#pragma pack(1)

//内部协议的公共头部
typedef struct _ISCommHead
{
    uint32_t dwLength;                          //消息的总长度
    uint32_t dwUin;                             //Q号
    uint16_t wSysType;                          //系统类型，指发送该消息的内部系统
    uint16_t wMsgType;                          //消息类型
    uint32_t dwClientIP;                        //用户访问或者上传图片的IP
    uint16_t wClientPort;                       //用户port
    uint32_t dwConnIP;                          //业务svr的IP
    uint16_t wConnPort;                         //业务svr的Port
    uint32_t dwSeq;                             //sequence id，回带字段，用以滤包
    uint32_t dwTraceID;                         //trace id，透传使用，用以在各个不同系统间追踪数据流
    uint32_t dwSendTime;                        //发送消息的时间，由发送方填本地时间
    uint16_t wVersion;                          //协议版本
    char szReserved[16];                        //预留字段
    char szEcho[SEC_IS_ECHO_LEN];               //回送字段, 应用不能更改,必须保证一字不改地在响应中回带给调用者，以支持回传方式的异步svr
}ISCommHead;

//公共通用包
typedef struct _CommonPkg
{
    char cStx;
    ISCommHead stISCommHead;
    uint32_t dwCommand;                         //命令字
    uint32_t dwResult;                          //命令执行结果
    uint32_t dwSize;                            //返回信息大小，指szData的大小
    char szBody[0];                             //包括返回信息和cEtx
    //char cEtx;
}CommonPkg;

#pragma pack()

class CPkgAdapt : public CBaseCtl
{
public:
    CPkgAdapt(void);
    ~CPkgAdapt(void);

/******************************************************************************
 * 函数名称：RecvPkg
 * 函数描述: 接收包，验证包的正确性，转字节序。
 * 输入参数：pPkg：包的指针   dwLen：包的长度
 * 返回值  : int 验证包的正确性结果，如 < 0，可通过GetError来获取错误原因。
 ******************************************************************************/
    int RecvPkg(const char *pPkg, uint32_t dwLen);

/******************************************************************************
 * 函数名称：RecvPkgNet
 * 函数描述: 接收包，验证包的基本正确性，不转字节序。
 * 输入参数：pPkg：包的指针   dwLen：包的长度
 * 返回值  : int 验证包的正确性结果，如 < 0，可通过GetError来获取错误原因。
 ******************************************************************************/
    int RecvPkgNet(const char *pPkg, uint32_t dwLen);

/******************************************************************************
 * 函数名称：GetRecvHead
 * 函数描述: 获取包头结构指针
 * 返回值  : 返回包头的指针。
 ******************************************************************************/
    ISCommHead *GetRecvHead();

/******************************************************************************
 * 函数名称：GetRecvBody
 * 函数描述: 返回包体结构指针
 * 返回值  : T* 包体结构的模板指针
 ******************************************************************************/
    template<class T>
    T *GetRecvBody()
    {
        return (T*)m_pstCommonPkg->szBody;
    }

/******************************************************************************
 * 函数名称：GetSendHead
 * 函数描述: 获取包头结构指针
 * 返回值  : 返回包头的指针。
 ******************************************************************************/
    ISCommHead *GetSendHead();

/******************************************************************************
 * 函数名称：GetSendBody
 * 函数描述: 返回发送缓冲区的包体结构
 * 返回值  : T* 包体结构的模板指针
 ******************************************************************************/
    template<class T>
    T *GetSendBody()
    {
        return (T*)(m_pSendBuf->szBody);
    }

/******************************************************************************
 * 函数名称：SetSendAttr
 * 函数描述: 设置发送包属性
 * 输入参数：dwCmd：命令字   dwRet：包返回结果   dwSize：包体的大小
 * 返回值  : int 如 < 0，表示组包错误，返回>0，则表示组包的长度。
 ******************************************************************************/
    int SetSendAttr(uint32_t dwCmd, uint32_t dwRet, uint32_t dwSize);

/******************************************************************************
 * 函数名称：DoHton
 * 函数描述: 转字节序等操作，转字节序后，请不要再修改包内容，此函数在发包前调用！
 ******************************************************************************/
    void DoHton();

/******************************************************************************
 * 函数名称：GetSendPkg
 * 函数描述: 返回发送缓冲区指针
 * 返回值  : char* 发送缓冲区指针
 ******************************************************************************/
    char *GetSendPkg();

/******************************************************************************
 * 函数名称：GetSendLen
 * 函数描述: 返回发送包的大小
 * 返回值  : uint32_t 发送包的大小
 ******************************************************************************/
    uint32_t GetSendLen();

/******************************************************************************
 * 函数名称：MakeSendPkg
 * 函数描述: 在缓冲区创建一个通用包
 * 返回值  : char* 返回创建包的指针，用作发送用。
 ******************************************************************************/
    char *MakeSendPkg();

/******************************************************************************
 * 函数名称：GetRecvCmd
 * 函数描述: 获取命令字
 * 返回值  : uint32_t 命令字
 ******************************************************************************/
    uint32_t GetRecvCmd();

/******************************************************************************
 * 函数名称：GetRecvRet
 * 函数描述: 获取包返回值
 * 返回值  : uint32_t 包返回值
 ******************************************************************************/
    uint32_t GetRecvRet();

/******************************************************************************
 * 函数名称：SetRecvRet
 * 函数描述: 设置包返回值，不处理字节序！
 * 返回值  : uint32_t 包返回值
 ******************************************************************************/
    void SetRecvRet(uint32_t dwRet);

/******************************************************************************
 * 函数名称：GetRecvSize
 * 函数描述: 获取包体大小
 * 返回值  : uint32_t 包体包小
 ******************************************************************************/
    uint32_t GetRecvSize();

private:
    CommonPkg *m_pstCommonPkg;
    CommonPkg *m_pSendBuf;
    uint32_t m_dwPkgLen;
    char m_szBuf[DW_PKG_LEN];

    void Hton(CommonPkg *pMsg);
    void Ntoh(CommonPkg *pMsg);

    void Hton(ISCommHead *pMsg);
    void Ntoh(ISCommHead *pMsg);

};



#endif
