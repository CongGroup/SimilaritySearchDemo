/******************************************************************************
 * 文件名称： cbase_ctl.cpp
 * 类    名： CBaseCtl
 * 描    述:  此类为所有Ctl类的基类，提供最基本的错误处理。
 * 创建日期:  2013-10-3
 * 作    者： rainewang
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
