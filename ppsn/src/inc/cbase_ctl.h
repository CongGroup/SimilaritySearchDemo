/******************************************************************************
 * 文件名称： cbase_ctl.h
 * 类    名： CBaseCtl
 * 描    述:  此类为所有Ctl类的基类，提供最基本的错误处理。
 * 创建日期:  2013-10-3
 * 作    者： rainewang
 ******************************************************************************/

#ifndef __CBASE_CTL_H__
#define __CBASE_CTL_H__

#include <string>

using namespace std;

class CBaseCtl
{
public:
    CBaseCtl(void);
    ~CBaseCtl(void);

/******************************************************************************
 * 函数名称：GetError
 * 函数描述: 获取上一次出错的内容
 * 返回值  : string 上次出错的内容
 ******************************************************************************/
    string GetError();

protected:
    string m_strError;

    void SetError(string strError);
};

#endif
