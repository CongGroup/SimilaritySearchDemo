/******************************************************************************
 * �ļ����ƣ� cbase_ctl.h
 * ��    ���� CBaseCtl
 * ��    ��:  ����Ϊ����Ctl��Ļ��࣬�ṩ������Ĵ�����
 * ��������:  2013-10-3
 * ��    �ߣ� rainewang
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
 * �������ƣ�GetError
 * ��������: ��ȡ��һ�γ��������
 * ����ֵ  : string �ϴγ��������
 ******************************************************************************/
    string GetError();

protected:
    string m_strError;

    void SetError(string strError);
};

#endif
