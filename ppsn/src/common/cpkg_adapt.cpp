/******************************************************************************
 * 文件名称： cpkg_adapt.cpp
 * 类    名： CPkgAdapt
 * 描    述:  此类用于解析和组合通用包
 * 创建日期:  2013-10-3
 * 作    者： rainewang
 * 修改历史：
 ******************************************************************************/

#include <arpa/inet.h>
#include <stdint.h>
#include <string>

#include "../inc/cpkg_adapt.h"


using namespace std;

CPkgAdapt::CPkgAdapt(void)
{
    m_pstCommonPkg = NULL;
    memset(m_szBuf, 0, sizeof(CommonPkg));
}

CPkgAdapt::~CPkgAdapt(void)
{
}

//Private

void CPkgAdapt::Hton(CommonPkg *pMsg)
{
    pMsg->dwCommand = htonl(pMsg->dwCommand);
    pMsg->dwResult = htonl(pMsg->dwResult);
    pMsg->dwSize = htonl(pMsg->dwSize);
    Hton(&pMsg->stISCommHead);
}

void CPkgAdapt::Ntoh(CommonPkg *pMsg)
{
    pMsg->dwCommand = ntohl(pMsg->dwCommand);
    pMsg->dwResult = ntohl(pMsg->dwResult);
    pMsg->dwSize = ntohl(pMsg->dwSize);
    Ntoh(&pMsg->stISCommHead);
}

void CPkgAdapt::Hton(ISCommHead *pMsg)
{
    pMsg->dwLength = htonl(pMsg->dwLength);
    pMsg->dwUin = htonl(pMsg->dwUin);
    pMsg->wSysType = htons(pMsg->wSysType);
    pMsg->wMsgType = htons(pMsg->wMsgType);
    pMsg->dwClientIP = htonl(pMsg->dwClientIP);
    pMsg->wClientPort = htons(pMsg->wClientPort);
    pMsg->dwConnIP = htonl(pMsg->dwConnIP);
    pMsg->wConnPort = htons(pMsg->wConnPort);
    pMsg->dwSeq = htonl(pMsg->dwSeq);
    pMsg->dwTraceID = htonl(pMsg->dwTraceID);
    pMsg->dwSendTime = htonl(pMsg->dwSendTime);
    pMsg->wVersion = htons(pMsg->wVersion);
}

void CPkgAdapt::Ntoh(ISCommHead *pMsg)
{
    pMsg->dwLength = ntohl(pMsg->dwLength);
    pMsg->dwUin = ntohl(pMsg->dwUin);
    pMsg->wSysType = ntohs(pMsg->wSysType);
    pMsg->wMsgType = ntohs(pMsg->wMsgType);
    pMsg->dwClientIP = ntohl(pMsg->dwClientIP);
    pMsg->wClientPort = ntohs(pMsg->wClientPort);
    pMsg->dwConnIP = ntohl(pMsg->dwConnIP);
    pMsg->wConnPort = ntohs(pMsg->wConnPort);
    pMsg->dwSeq = ntohl(pMsg->dwSeq);
    pMsg->dwTraceID = ntohl(pMsg->dwTraceID);
    pMsg->dwSendTime = ntohl(pMsg->dwSendTime);
    pMsg->wVersion = ntohs(pMsg->wVersion);
}

//Public

uint32_t CPkgAdapt::GetRecvSize()
{
    return m_pstCommonPkg->dwSize;
}

uint32_t CPkgAdapt::GetRecvRet()
{
    return m_pstCommonPkg->dwResult;
}

void CPkgAdapt::SetRecvRet(uint32_t dwRet)
{
    m_pstCommonPkg->dwResult = dwRet;
}

uint32_t CPkgAdapt::GetRecvCmd()
{
    return m_pstCommonPkg->dwCommand;
}

ISCommHead *CPkgAdapt::GetRecvHead()
{
    return &m_pstCommonPkg->stISCommHead;
}

ISCommHead *CPkgAdapt::GetSendHead()
{
    return &m_pSendBuf->stISCommHead;
}

char *CPkgAdapt::GetSendPkg()
{
    return m_szBuf;
}

uint32_t CPkgAdapt::GetSendLen()
{
    return m_dwPkgLen;
}

void CPkgAdapt::DoHton()
{
    Hton(m_pSendBuf);
}

int CPkgAdapt::SetSendAttr(uint32_t dwCmd, uint32_t dwRet, uint32_t dwSize)
{
    m_pSendBuf->cStx = SEC_IS_STX;
    m_pSendBuf->dwResult = dwRet;
    m_pSendBuf->dwCommand = dwCmd;
    m_pSendBuf->dwSize = dwSize;
    m_pSendBuf->szBody[dwSize] = SEC_IS_ETX;
    m_dwPkgLen = m_pSendBuf->stISCommHead.dwLength = sizeof(CommonPkg) + dwSize + sizeof(char);
    return m_dwPkgLen;
}

char *CPkgAdapt::MakeSendPkg()
{
    memset(m_szBuf, 0, sizeof(CommonPkg));
    m_pSendBuf = (CommonPkg*)m_szBuf;
    m_dwPkgLen = 0;
    return m_szBuf;
}

int CPkgAdapt::RecvPkg(const char *pPkg, uint32_t dwLen)
{
    if(NULL == pPkg)
    {
        SetError("NULL == pPkg");
        return EM_PKG_NULL_ERROR;
    }
    if(sizeof(CommonPkg) + sizeof(char) > dwLen)
    {
        SetError("sizeof(CommonPkg) + sizeof(char) > dwLen");
        return EM_PKG_MIN_ERROR;
    }
    if(SEC_IS_STX != pPkg[0] || SEC_IS_ETX != pPkg[dwLen - 1])
    {
        SetError("SEC_IS_STX != pPkg[0] || SEC_IS_ETX != pPkg[dwLen - 1]");
        return EM_PKG_HEAD_ERROR;
    }
    m_pstCommonPkg = (CommonPkg *)pPkg;
    Ntoh(m_pstCommonPkg);
    if(dwLen != m_pstCommonPkg->stISCommHead.dwLength)
    {
        SetError("dwLen != m_pstCommonPkg->stCommHead.dwLength");
        return EM_PKG_LEN_ERROR;
    }
    if(dwLen != sizeof(CommonPkg) + sizeof(char) + m_pstCommonPkg->dwSize)
    {
        SetError("dwLen != sizeof(CommonPkg) + sizeof(char) + m_pstCommonPkg->dwSize");
        return EM_PKG_SIZE_ERROR;
    }
    return EM_PKG_RECV_SUCCESS;
}

int CPkgAdapt::RecvPkgNet(const char *pPkg, uint32_t dwLen)
{
    if(NULL == pPkg)
    {
        SetError("NULL == pPkg");
        return EM_PKG_NULL_ERROR;
    }
    if(SEC_IS_STX != pPkg[0] || SEC_IS_ETX != pPkg[dwLen - 1])
    {
        SetError("SEC_IS_STX != pPkg[0] || SEC_IS_ETX != pPkg[dwLen - 1]");
        return EM_PKG_HEAD_ERROR;
    }
    m_pstCommonPkg = (CommonPkg *)pPkg;
    if(sizeof(CommonPkg) + sizeof(char) > dwLen)
    {
        SetError("sizeof(CommonPkg) + sizeof(char) > dwLen");
        return EM_PKG_MIN_ERROR;
    }
    return EM_PKG_RECV_SUCCESS;
}
