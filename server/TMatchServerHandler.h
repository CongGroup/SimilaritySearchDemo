#ifndef __TMATCH_SERVER_HANDLER_H__
#define __TMATCH_SERVER_HANDLER_H__

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include "TMatchServer.h"


#include <boost/filesystem.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/nonfree/nonfree.hpp>

#include <openssl/aes.h>
#include<openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>


#include "cimg_ctl.h"
#include "ce2lsh.h"
#include "cnest_hash.h"

#include <sys/time.h>
#include <time.h>
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <map>
#include <iostream>

#define __DEBUG__

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using namespace ::apache::thrift::concurrency;


//using boost::shared_ptr;
using namespace std;
using namespace  ::ImgMatch;

const uint32_t DEF_KMEANS_K = 1000;
const string DEF_IMG_PATH = "/data/InformationDayDemoData/data/";
const uint32_t DEF_MAX_IMG = 30607;
//const uint32_t DEF_MAX_IMG = 3000;
const uint32_t LSH_L = 150;
const uint32_t LSH_K = 9;
const double LSH_W = 4;
const double LSH_R = 0.03;
const double NEST_LOAD = 0.5;
const uint32_t NEST_L = LSH_L;
const uint32_t NEST_W = DEF_MAX_IMG / NEST_LOAD / NEST_L + 1;
const uint32_t NEST_DELTA = 5000 / LSH_L;
const uint32_t NEST_MAXKICKOUT = 10000;

const string DEF_STR_KEY = "abcdefghijklmn";

uint32_t DiffTime(){
    static timeval t_start, t_end;
    gettimeofday(&t_end, NULL);
    uint32_t uiTimeInterval = 1000000 * (t_end.tv_sec - t_start.tv_sec) + t_end.tv_usec - t_start.tv_usec;
    gettimeofday(&t_start, NULL);
    return uiTimeInterval;
}

uint32_t DiffLongTime(){
    static time_t t_cur;
    uint32_t ui_Time = time(NULL) - t_cur;
    t_cur = time(NULL);
    return ui_Time;
}

void SHA256(char *pKey, uint32_t uiKeyLen, char *pData, uint32_t uiDataLen, char *pOut, uint32_t uiOutLen)
{
    memset(pOut, 0, uiOutLen);
    HMAC(EVP_sha256(), pKey, uiKeyLen, (unsigned char*)pData, uiDataLen, (unsigned char*)pOut, &uiOutLen);
}

void printLsh(uint32_t *arLsh)
{

    cout << "-----------------------------------------" << endl;
    for (int i = 0; i < LSH_L; i++)
    {
        cout << arLsh[i] << " ";
    }
    cout << endl<< "-----------------------------------------" << endl;

}

uint32_t HashU32(char *pBuf, uint32_t uiLen)
{
    uint32_t uiRet = *(uint32_t*)pBuf;
    for (uint32_t uiCur = 4; uiCur + 4 <= uiLen; uiCur += 4)
    {
        uiRet ^= *(uint32_t*)(pBuf + uiCur);
    }
    return uiRet;
}

uint32_t SHA256_LSH(uint32_t uiIn)
{
    char szSHA[32];
    uint32_t uiLsh = uiIn;
    memset(szSHA, 0, sizeof(szSHA));
    SHA256(DEF_STR_KEY.c_str(), DEF_STR_KEY.length(), (char*)&uiLsh, sizeof(uiLsh), szSHA, sizeof(szSHA));
    return HashU32(szSHA, sizeof(szSHA));
}

class TMatchServerHandler : virtual public TMatchServerIf {
public:

    CImgCtl oImgCtl;
    CE2lsh g_oE2lsh;
    double arImgBow[DEF_MAX_IMG][DEF_KMEANS_K];
    uint32_t arImgLSH[DEF_MAX_IMG][LSH_L];
    CNestHash<uint32_t> g_oNestHash;
    char *g_memory;

    uint32_t g_downName;

    void tiqutezheng(int i)
    {
        

        //加载kmeans vocabulary
        oImgCtl.LoadVocabulary("/data/InformationDayDemoData/kmeans.vob", DEF_KMEANS_K);


        char buf[500];
        for(uint32_t uiCur = 0; uiCur < DEF_MAX_IMG; uiCur++)
        {

            if(uiCur % 20 != i)
            {
                continue;
            }

            memset(arImgBow[uiCur], 0, DEF_KMEANS_K);

            snprintf(buf, sizeof(buf),"%s%u", DEF_IMG_PATH.c_str(), uiCur);
            string sPath(buf);

            oImgCtl.ExtractBow(sPath, arImgBow[uiCur]);

            ofstream fout; 
            char szTmp[500];


            snprintf(szTmp, sizeof(szTmp), "/data/InformationDayDemoData/bow/%u", uiCur);

            sPath.assign(szTmp);
            fout.open(sPath);
            char szBuf[1000];
            for (uint32_t uiIdx = 0; uiIdx < DEF_KMEANS_K; uiIdx++)
            {
                snprintf(szBuf, sizeof(szBuf), "%.8lf ", arImgBow[uiCur][uiIdx]);
                fout << szBuf;
            }
            fout.close();

        }

    }

    bool insertIntoNest(uint32_t uiInsertId, uint32_t *pLsh, int iKickL, int iMaxKickout, uint32_t uiBeg = 0)
    {
        uint32_t oUser;
        if (iMaxKickout < 0)
        {
            return false;
        }
        uint32_t *pNode = g_oNestHash.NewPlace(iKickL, pLsh, uiBeg);
        if (iKickL < 0)
        {
            *pNode = uiInsertId;
            return true;
        }
        else
        {
            oUser = *pNode;
            //set the new
            *pNode = uiInsertId;
            //continue insert old
            return insertIntoNest(oUser, arImgLSH[oUser], iKickL, iMaxKickout - 1, (uint32_t)iKickL + 1);
        }
        return false;
    }


    TMatchServerHandler() {

        printf("%u %u %u %u \n", SHA256_LSH(123123123), SHA256_LSH(123123123), SHA256_LSH(1231231234), SHA256_LSH(1231231234));

        //加载kmeans vocabulary
        oImgCtl.LoadVocabulary("/data/InformationDayDemoData/kmeans.vob", DEF_KMEANS_K);

        //从文件中读BOW
        fstream fp;
        char szBuf[1000];

        for (uint32_t uiCur = 0; uiCur < DEF_MAX_IMG; uiCur++)
        {
            snprintf(szBuf, sizeof(szBuf), "/data/InformationDayDemoData/bow/%u", uiCur);
            fp.open(szBuf);
            memset(arImgBow[uiCur], 0, DEF_KMEANS_K);

            for (uint32_t uiD = 0; uiD < DEF_KMEANS_K; uiD++)
            {
                fp>>arImgBow[uiCur][uiD];
            }
            fp.close();

            if(uiCur % 100 == 0)
            {
                printf("Finish Load Bow %.4lf \n", (double)uiCur / ((double)DEF_MAX_IMG / 100));
            }
        }

        //计算LSH
        g_oE2lsh.InitLSH(LSH_R, DEF_KMEANS_K, LSH_L, LSH_K, LSH_W);
        for(uint32_t uiCur = 0; uiCur < DEF_MAX_IMG; uiCur++)
        {
            g_oE2lsh.ComputeLSH(arImgBow[uiCur], arImgLSH[uiCur]);

            if(uiCur % 100 == 0)
            {
                printf("Finish Load LSH %.4lf \n", (double)uiCur / ((double)DEF_MAX_IMG / 100));

            }

            for(uint32_t uiEnc = 0; uiEnc < LSH_L; uiEnc++)
            {
                arImgLSH[uiCur][uiEnc] = SHA256_LSH(arImgLSH[uiCur][uiEnc]);
            }

        }

        cout << "Begin to Build Index" << endl;

        //建立索引
        g_memory = new char[DEF_MAX_IMG * 2 * 10];
        memset(g_memory, 0, sizeof(g_memory));
        g_oNestHash.InitNestHash(g_memory, NEST_L, NEST_DELTA, NEST_W);


        for (uint32_t uiCur = 0; uiCur < DEF_MAX_IMG; uiCur++)
        {

            //printLsh(arImgLSH[uiCur]);

            if (!insertIntoNest(uiCur, arImgLSH[uiCur], -1, NEST_MAXKICKOUT))
            {
                g_oNestHash.CheckNestLoad();
                printf("With No.%u, Can not Kick after %u Times.\n", uiCur, NEST_MAXKICKOUT);
                return;
            }
            if(uiCur % 100 == 0)
            {
                printf("Finish Insert %.4lf \n", (double)uiCur / ((double)DEF_MAX_IMG / 100));
            }
        }

        g_oNestHash.CheckNestLoad();

        //初始化全局临时文件符
        g_downName = 0;

    }

    void GetImage(std::string& _return, const std::string& fid) {


        char * buffer = NULL;

        string sfile = DEF_IMG_PATH + fid;

        ifstream in;
        in.open(sfile.c_str(), ios::binary);
        in.seekg (0, ios::end);
        size_t size = in.tellg();
        
        buffer = new char[size];
        in.seekg (0, ios::beg);
        in.read(buffer, size);
        in.close();

        _return.assign(buffer, size);
        delete[] buffer;
        
        in.close();
    }

    void SearchSimilar(std::vector<std::string> & _return, const std::string& img, const bool add) {

        string sBow = "";
        string sLsh = "";
        string sEncLsh = "";
        string sQueryBlock = "";
        string sRandomMask = "";
        string sDistance = "";

        //写文件
        char temp[300];
        snprintf(temp, sizeof(temp), "%s999999", DEF_IMG_PATH.c_str());
        ofstream out;
        out.open(temp, ios::out);
        out.seekp(0,ios::beg);
        out.write(img.c_str(), img.length());
        char szTmp[500];

        //提特征
        
        printf("Get a Pictures , the size is %u .\n", img.size());
        
        DiffTime();

        string sPath(temp);
        double arBow[DEF_KMEANS_K];
        memset(arBow, 0, sizeof(arBow));

        oImgCtl.ExtractBow(sPath, arBow);

        uint32_t uiExtractBow = DiffTime();

        for(uint32_t uiCur = 0; uiCur < 100; uiCur++)
        {
            snprintf(temp, sizeof(temp), "%.7f", arBow[uiCur]);
            sBow += ",";
            sBow += temp;
        }
        
        DiffTime();

        //计算LSH
        uint32_t arLsh[LSH_L];
        memset(arLsh, 0, sizeof(arLsh));
        g_oE2lsh.ComputeLSH(arBow, arLsh);

        uint32_t uiComputeLsh = DiffTime();

        for(uint32_t uiCur = 0; uiCur < 100; uiCur++)
        {
            snprintf(temp, sizeof(temp), "%u", arLsh[uiCur]);
            sLsh += ",";
            sLsh += temp;
        }

        printf("Encrypted Search Query\n");

        DiffTime();

        for(uint32_t uiCur = 0; uiCur < LSH_L; uiCur++)
        {
            arLsh[uiCur] = SHA256_LSH(arLsh[uiCur]);
        }
        
        uint32_t uiEncryptLsh = DiffTime();
        
        for(uint32_t uiCur = 0; uiCur < 100; uiCur++)
        {
            snprintf(temp, sizeof(temp), "%u", arLsh[uiCur]);
            sEncLsh += ",";
            sEncLsh += temp;
        }

        printf("Begin to Search!\n");

        //查询
        DiffTime();

        const uint32_t uiQnum = LSH_L * ( NEST_DELTA + 1 );
        uint32_t arQuery[uiQnum];
        memset(arQuery, 0, sizeof(arQuery));
        g_oNestHash.QueryToMemory(arLsh, (char*)arQuery);
        
        uint32_t uiQueryImg = DiffTime();

        //Query Block
        srand(arLsh[0]);
        for(uint32_t uiCur = 0; uiCur < 100; uiCur++)
        {
            uint32_t uiID = arQuery[uiCur];
            uint32_t uiRand = rand();
            uiID = uiID ^ uiRand;
            snprintf(szTmp, sizeof(szTmp), "%u", uiID);
            sQueryBlock += ",";
            sQueryBlock += szTmp;
            snprintf(szTmp, sizeof(szTmp), "%u", uiRand);
            sRandomMask += ",";
            sRandomMask += szTmp;
        }

        printf("Return Top 9 Images.");

        DiffTime();

        //排序
        uint32_t uiMaxRet = 9;
        uint32_t uiCnt = 0;
        map<double, vector<uint32_t> > mapDisID;
        
        sDistance = "";

        for(uint32_t uiCur = 0; uiCur < uiQnum; uiCur++)
        {
            double dDistance = g_oE2lsh.ComputeL2(arImgBow[arQuery[uiCur]], arBow, DEF_KMEANS_K);
            snprintf(szTmp, sizeof(szTmp), "%f", dDistance);
            sDistance += ",";
            sDistance += szTmp;
            mapDisID[dDistance].push_back(arQuery[uiCur]);
        }

        for(map<double, vector<uint32_t> >::iterator it = mapDisID.begin(); it != mapDisID.end(); it++)
        {
            for(vector<uint32_t>::iterator vit = it->second.begin(); vit != it->second.end(); vit++)
            {
                if(uiCnt < uiMaxRet)
                {
                    snprintf(temp, sizeof(temp), "%u", *vit);
                    string sRet(temp);
                    _return.push_back(sRet);
                    uiCnt++;
                }
            }
        }
        
        uint32_t uiSortTime = DiffTime();

        //提取时间
        
        snprintf(szTmp, sizeof(szTmp), "%u", uiExtractBow);
        string sTmp(szTmp);
        _return.push_back(sTmp);
        cout<<"Extract Bow Time : "<<szTmp<<endl;

        //LSH计算时间
        snprintf(szTmp, sizeof(szTmp), "%u", uiComputeLsh);
        sTmp.assign(szTmp);
        _return.push_back(sTmp);
        cout<<"Compute LSH Time : "<<szTmp<<endl;
        
        //LSH的加密时间
        snprintf(szTmp, sizeof(szTmp), "%u", uiEncryptLsh);
        sTmp.assign(szTmp);
        _return.push_back(sTmp);
        cout<<"Encrypt LSH Time : "<<szTmp<<endl;
        
        //索引时间
        snprintf(szTmp, sizeof(szTmp), "%u", uiQueryImg);
        sTmp.assign(szTmp);
        _return.push_back(sTmp);
        cout<<"Query Time : "<<szTmp<<endl;

        //排序时间
        snprintf(szTmp, sizeof(szTmp), "%u", uiSortTime);
        sTmp.assign(szTmp);
        _return.push_back(sTmp);
        cout<<"Sort Time : "<<szTmp<<endl;
        
        //Bow
        _return.push_back(sBow);

        //Lsh
        _return.push_back(sLsh);

        //EncLsh
        _return.push_back(sEncLsh);

        //Query Block
        _return.push_back(sQueryBlock);

        //Random Mask
        _return.push_back(sRandomMask);

        //Distance
        _return.push_back(sDistance);

    }

};

#endif