#include <stdlib.h>
#include <stdint.h>

#include "../inc/ppsn_protocol.h"

//About stUserReq

void Hton(stUserReq *pMsg)
{
    pMsg->dwK = htonl(pMsg->dwK);
    pMsg->dwL = htonl(pMsg->dwL);
    pMsg->dwUID = htonl(pMsg->dwUID);
}


void Ntoh(stUserReq *pMsg)
{
    pMsg->dwK = ntohl(pMsg->dwK);
    pMsg->dwL = ntohl(pMsg->dwL);
    pMsg->dwUID = ntohl(pMsg->dwUID);
}

//About stUserRsp

void Hton(stUserRsp *pMsg)
{
    pMsg->dwUID = htonl(pMsg->dwUID);
    for(uint32_t dwIdx = 0; dwIdx < pMsg->dwUserNum; dwIdx++)
    {
        pMsg->arUserID[dwIdx] = htonl(pMsg->arUserID[dwIdx]);
    }
    pMsg->dwUserNum = htonl(pMsg->dwUserNum);
}


void Ntoh(stUserRsp *pMsg)
{
    pMsg->dwUID = ntohl(pMsg->dwUID);
    pMsg->dwUserNum = ntohl(pMsg->dwUserNum);
    for(uint32_t dwIdx = 0; dwIdx < pMsg->dwUserNum; dwIdx++)
    {
        pMsg->arUserID[dwIdx] = ntohl(pMsg->arUserID[dwIdx]);
    }
}


