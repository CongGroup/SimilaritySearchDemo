/******************************************************************************
 * 文件名称： cimg_ctl.cpp
 * 类    名： CImgCtl
 * 描    述:  用于图像处理的类
 * 创建日期:  2013-10-8
 * 作    者： rainewang
 * 修改历史：
 ******************************************************************************/
#include <stdint.h>
#include <map>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <opencv2/opencv.hpp>
#include "../inc/cimg_ctl.h"
#include "../inc/opensurf.h"

using namespace std;
using namespace cv;


CImgCtl::CImgCtl()
{
    m_matcher = DescriptorMatcher::create("FlannBased");
    m_extractor = DescriptorExtractor::create("SURF");
    m_detector = FeatureDetector::create("SURF");
    m_pBowTrainer = NULL;
    m_pBowDE = NULL; 

    
    
}

CImgCtl::~CImgCtl()
{
    if (NULL != m_pBowTrainer){
        delete m_pBowTrainer;
    }
    if (NULL != m_pBowDE){
        delete m_pBowDE;
    }
}

int CImgCtl::SaveVocabulary(string strPath, uint32_t uiDictSize)
{
    FileStorage fs(strPath, FileStorage::WRITE);
    fs<<"vocabulary"<<m_matVocabulary;
    fs.release();
    m_uiDictSize = uiDictSize;
    printf("Finish Save Vocabulary to Path : %s\n", strPath.c_str());
    return 0;
}

int CImgCtl::LoadVocabulary(string strPath, uint32_t uiDictSize)
{
    FileStorage fs(strPath, FileStorage::READ);
    fs["vocabulary"] >> m_matVocabulary;
    m_uiDictSize = uiDictSize;
    
    //加载分类库
    printf("begin read from img %s", strPath.c_str());

    printf("Finish Load Vocabulary From Path : %s\n", strPath.c_str());
    return 0;
}

int CImgCtl::Train(vector<string> &vecPicPath, uint32_t uiDictSize)
{ 



    //test static
    int cnt = 0;
    //Init BowTrainer
    if (NULL != m_pBowTrainer)
    {
        delete m_pBowTrainer;
    }
    //Define the retry times
    int iRetries = 1;
    //Define the Terminate Criteria
    TermCriteria tc(CV_TERMCRIT_ITER, 4, 0.001);
    m_pBowTrainer = new BOWKMeansTrainer(uiDictSize, tc, iRetries, KMEANS_PP_CENTERS);
    m_uiDictSize = uiDictSize;
    //foreach the pictures in each category, compute the features and add to train
    for(vector<string>::iterator it = vecPicPath.begin(); it != vecPicPath.end(); it++)
    {
        //Load the Img
        Mat matImg = imread(*it);
        if (!matImg.empty()) {
            vector<KeyPoint> vecKeypoints;
            m_detector->detect(matImg, vecKeypoints);
            if (vecKeypoints.empty()) {
                //Could not find key points in image
                printf("No.%u Path %s Keypoints empty !!!\n", cnt++, it->c_str());
                continue;
            } else {
                //Test
                printf("No.%u Path %s KeyPoints %u\n", cnt++, it->c_str(), vecKeypoints.size());
                Mat matFeatures;
                m_extractor->compute(matImg, vecKeypoints, matFeatures);
                matImg.release();
                m_pBowTrainer->add(matFeatures);
            }
            vecKeypoints.clear();
        } else {
            //Could not read Image
            return -1;
        }
    }

    printf("Finish All !!!\n");
    m_matVocabulary = m_pBowTrainer->cluster();
    return 0;
}

int CImgCtl::TrainFromFeatures(vector<string> &vecSurfPath, uint32_t uiDictSize)
{
    int cnt = 0;
    //Init BowTrainer
    if (NULL != m_pBowTrainer)
    {
        delete m_pBowTrainer;
    }
    //Define the retry times
    int iRetries = 1;
    //Define the Terminate Criteria
    TermCriteria tc(CV_TERMCRIT_ITER, 4, 0);
    m_pBowTrainer = new BOWKMeansTrainer(uiDictSize, tc, iRetries, KMEANS_PP_CENTERS);
    m_uiDictSize = uiDictSize;
    //foreach the pictures in each category, compute the features and add to train
    for(vector<string>::iterator it = vecSurfPath.begin(); it != vecSurfPath.end(); it++)
    {
        int ipoints;
        OpenSurfInterestPoint *points;
        printf("axxx : %u\n", cnt);
        if(!OpenSurf_LoadDescriptor(it->c_str(), points, ipoints))
        {
            printf("Do with error! files: %s \n", it->c_str());
        }
        Mat matFeatures;
        printf("printf %d pintes\n", ipoints);
        if (0 == ipoints)
        {
            continue;
        }
        for (uint32_t uiCur = 0; uiCur < ipoints; uiCur++)
        {
            //SURF is 64 D features
            Mat mapTmp(64, 1, CV_32F, points[uiCur].descriptor);
            if (mapTmp.empty())
            {
                printf("EMPTY Mat !!\n");
            }
            matFeatures.push_back(mapTmp);
        }
        if (matFeatures.empty())
        {
            printf("EMPTY mapFeatures!!\n");
        }
        m_pBowTrainer->add(matFeatures);
        delete[] points;
        printf("No.%u Path %s \n", cnt++, it->c_str());
    }
    m_matVocabulary = m_pBowTrainer->cluster();
    return 0;
}

int CImgCtl::ExtractPictureBOWtoPath(vector<string> &vecPicPath, string sPath)
{
    //FileStorage fs(sPath, FileStorage::WRITE);
    //uint32_t uiCnt = 0;
    //if (NULL != m_pBowDE)
    //{
    //    delete m_pBowDE;
    //}
    //m_pBowDE = new BOWImgDescriptorExtractor(m_extractor, m_matcher);
    //m_pBowDE->setVocabulary(m_matVocabulary);
    //for(vector<string>::iterator itPath = vecPicPath.begin(); itPath != vecPicPath.end(); itPath++)
    //{

    //        Mat matImg = imread(*itPath);
    //        if (!matImg.empty()) {
    //            vector<KeyPoint> vecKeypoints;
    //            m_detector->detect(matImg, vecKeypoints);
    //            if (vecKeypoints.empty()) {
    //                //Could not find key points in image
    //                continue;
    //            } else {
    //                Mat matImgBow(0, m_uiDictSize, CV_32FC1);
    //                m_pBowDE->compute(matImg, vecKeypoints, matImgBow);
    //                
    //                fs<<"vocabulary"<<m_matVocabulary;
    //                fs.release();
    //                m_uiDictSize = uiDictSize;
    //                printf("Finish Save Vocabulary to Path : %s\n", strPath.c_str());
    //                m_mapImgCache[*itPath] = matImgBow;
    //            }
    //        } else {
    //            //Could not read Image
    //            return -1;
    //        }
    //    matUserBow = matUserBow + m_mapImgCache[*itPath];
    //}
    //}
    return 0;
}



void CImgCtl::ExtractBow(string strPath, double *parDouble)
{
    //printf("Begin %s ", strPath.c_str());
    
    if (NULL == m_pBowDE)
    {
        m_pBowDE = new BOWImgDescriptorExtractor(m_extractor, m_matcher);
        m_pBowDE->setVocabulary(m_matVocabulary);
    }
  
    //从文件读到Mat
    //printf("begin read from img %s", strPath.c_str());
    Mat matImg = imread(strPath);
    if(!matImg.empty())
    {
        //printf("finish read from img %s", strPath.c_str());
        vector<KeyPoint> vecKeypoints;
        //提取特征点
        m_detector->detect(matImg, vecKeypoints);
        if(vecKeypoints.size() == 0)
        {
            for(uint32_t uiIdx = 0; uiIdx < m_uiDictSize; uiIdx++)
            {
                parDouble[uiIdx] = 0;
            }
            return;
        }
        Mat matImgBow(0, m_uiDictSize, CV_32FC1);
        //计算BOW
        m_pBowDE->compute(matImg, vecKeypoints, matImgBow);
        //printf("FINISH COMPT KEYPOINT :%u ", vecKeypoints.size());
        float fUserBow = 0;
        for(uint32_t uiCur = 0; uiCur < m_uiDictSize; uiCur++)
        {
            fUserBow += matImgBow.at<float>(0, uiCur);
        }
        //printf("  work matImgBow ||");
        if(fUserBow != 0)
        {
            matImgBow = matImgBow / fUserBow;
        }
        //printf("  Begin to write \n");
        for(uint32_t uiIdx = 0; uiIdx < m_uiDictSize; uiIdx++)
        {
            parDouble[uiIdx] = (double)matImgBow.at<float>(0, uiIdx);
        }
    }
    else
    {
        //printf("bad imgasdfasdf\n");
    }

}

int CImgCtl::ExtractBOWDescriptorClean(vector<string> *pUserPath, double *arUserBow)
{
    if (NULL != m_pBowDE)
    {
        delete m_pBowDE;
    }
    m_pBowDE = new BOWImgDescriptorExtractor(m_extractor, m_matcher);
    m_pBowDE->setVocabulary(m_matVocabulary);

    Mat matUserBow = Mat::zeros(1, m_uiDictSize, CV_32F);
    //Foreach Image file to get the BOW
    for(vector<string>::iterator itPath = pUserPath->begin(); itPath != pUserPath->end(); itPath++)
    {
        
        //matUserBow = matUserBow + matImgBow;
    }
        
    //Computing the User 's all Bow
    float fUserBow = 0;
    for(uint32_t uiCur = 0; uiCur < m_uiDictSize; uiCur++)
    {
        fUserBow += matUserBow.at<float>(0, uiCur);
    }
    //Computing the frequency and set to float[]
    matUserBow = matUserBow / fUserBow;
    for(uint32_t uiIdx = 0; uiIdx < m_uiDictSize; uiIdx++)
    {
        arUserBow[uiIdx] = (double)matUserBow.at<float>(0, uiIdx);
    }
    return 0;
}


int CImgCtl::ExtractBOWDescriptor(vector<string> *arUserPath, double **arUserBow, uint32_t uiUserNum)
{
    uint32_t uiUseCache = 0;
    if (NULL != m_pBowDE)
    {
        delete m_pBowDE;
    }
    m_pBowDE = new BOWImgDescriptorExtractor(m_extractor, m_matcher);
    m_pBowDE->setVocabulary(m_matVocabulary);
    for(uint32_t uiCur = 0; uiCur < uiUserNum; uiCur++)
    {
        if (uiCur % (uiUserNum/100) == 0)
        {
            printf("Finish Percent %u %%\n", uiCur / (uiUserNum/100));
        }
        uint32_t uiUseCachePerUser = 0;
        arUserBow[uiCur] = new double[m_uiDictSize];
        vector<string> *pVecPath = arUserPath + uiCur;
        //Init A Matrix For compute
        Mat matUserBow = Mat::zeros(1, m_uiDictSize, CV_32F);
        //Foreach Image file to get the BOW
        for(vector<string>::iterator itPath = pVecPath->begin(); itPath != pVecPath->end(); itPath++)
        {
            //use the cache
            if (m_mapImgCache.find(*itPath) == m_mapImgCache.end())
            {
                Mat matImg = imread(*itPath);
                if (!matImg.empty()) {
                    vector<KeyPoint> vecKeypoints;
                    m_detector->detect(matImg, vecKeypoints);
                    if (vecKeypoints.empty()) {
                        //Could not find key points in image
                        printf("\nNo Keypoint\n");
                        continue;
                    } else {
                        Mat matImgBow(0, m_uiDictSize, CV_32FC1);
                        m_pBowDE->compute(matImg, vecKeypoints, matImgBow);
                        m_mapImgCache[*itPath] = matImgBow;
                    }
                } else {
                    //Could not read Image
                    printf("\n no read img\n");
                    continue;
                }
            }
            else
            {
                uiUseCache++;
                uiUseCachePerUser++;
            }
            matUserBow = matUserBow + m_mapImgCache[*itPath];
        }
        
        //Computing the User 's all Bow
        float fUserBow = 0;
        for(uint32_t uiCur = 0; uiCur < m_uiDictSize; uiCur++)
        {
            fUserBow += matUserBow.at<float>(0, uiCur);
        }
        //Computing the frequency and set to float[]
        matUserBow = matUserBow / fUserBow;
        for(uint32_t uiIdx = 0; uiIdx < m_uiDictSize; uiIdx++)
        {
            arUserBow[uiCur][uiIdx] = (double)matUserBow.at<float>(0, uiIdx);
        }
    }
    return 0;
}


