/******************************************************************************
 * �ļ����ƣ� cbase_ctl.cpp
 * ��    ���� CBaseCtl
 * ��    ��:  ����Ϊ����Ctl��Ļ��࣬�ṩ������Ĵ�����
 * ��������:  2013-10-3
 * ��    �ߣ� rainewang
 ******************************************************************************/

#include "../inc/cbase_ctl.h"

using namespace std;

CBaseCtl::CBaseCtl(void)
{
    m_strError = "";
}

CBaseCtl::~CBaseCtl(void)
{
}

string CBaseCtl::GetError()
{
    return m_strError;
}

void CBaseCtl::SetError(string strError)
{
    m_strError = strError;
}
