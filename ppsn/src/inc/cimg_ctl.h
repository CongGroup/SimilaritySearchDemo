/******************************************************************************
 * 文件名称： cimg_ctl.h
 * 类    名： CImgCtl
 * 描    述:  用于处理图像相关的流程控制
 * 创建日期:  2013-10-8
 * 作    者： rainewang
 * 修改历史：
 ******************************************************************************/
#ifndef __CIM_GCTL_H__
#define __CIM_GCTL_H__

#include <stdint.h>
#include <map>
#include <string.h>
#include <vector>
#include <opencv2/opencv.hpp>

#include "cbase_ctl.h"

using namespace std;
using namespace cv;

class CImgCtl: public CBaseCtl
{
public:

    CImgCtl();
    ~CImgCtl(void);

    int Train(vector<string> &vecPicPath, uint32_t uiDictSize);

    //Save The k-Vocabulary Into A file
    int SaveVocabulary(string strPath, uint32_t uiDictSize);

    //Load the k-Vocabulary From A file
    int LoadVocabulary(string strPath, uint32_t uiDictSize);

    int ExtractPictureBOWtoPath(vector<string> &vecPicPath, string sPath);
    
    int ExtractBOWDescriptor(vector<string> *arUserPath, double **arUserBow, uint32_t uiUserNum);

    int ExtractBOWDescriptorClean(vector<string> *pUserPath, double *arUserBow);

    int TrainFromFeatures(vector<string> &vecSurfPath, uint32_t uiDictSize);

    void ExtractBow(string strPath, double *parDouble);

private:

    Ptr<DescriptorMatcher> m_matcher;
    Ptr<DescriptorExtractor> m_extractor;
    Ptr<FeatureDetector> m_detector;

    BOWKMeansTrainer *m_pBowTrainer;
    BOWImgDescriptorExtractor *m_pBowDE;

    Mat m_matVocabulary;

    uint32_t m_uiDictSize;

    //for fast
    map<string, Mat> m_mapImgCache;

};



#endif
