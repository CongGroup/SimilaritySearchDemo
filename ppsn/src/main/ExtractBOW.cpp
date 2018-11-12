/******************************************************************************
 * 文件名称： user_client.cpp
 * 描    述:  用于模拟user操作的客户端
 * 创建日期:  2013-10-3
 * 作    者： rainewang
 ******************************************************************************/
#include <stdint.h>
#include <string>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <openssl/aes.h>
#include<openssl/md5.h>
//For Img Process
#include "../inc/cimg_ctl.h"
#include "../inc/ce2lsh.h"
#include "../inc/cnest_hash.h"
#include <vector>
#include <boost/filesystem.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/nonfree/nonfree.hpp>



#ifdef __cplusplus
extern "C"
{
#endif 
#include "oi_shm.h"
#ifdef __cplusplus
}
#endif 


using namespace std;
using namespace boost::filesystem;
using namespace cv;

using namespace std;

//Configur All_User_Num for test
const uint32_t ALL_USER_NUM = 50000;
//Configur All Train USER
const uint32_t ALL_TRAIN_NUM = 100;
//Configur Need to do USER_NUMBER
const uint32_t QUERY_BACK_NUM = 400;

//Configur KMEANS Number
#define DEF_KMEANS_K 500
//Configur E2Lsh Information
const uint32_t LSH_L = 180;
const uint32_t LSH_K = 9;
const double LSH_W = 4;
const double LSH_R = 0.01;
//Configur NestHashTable
const double NEST_LOAD = 0.8;
const uint32_t NEST_L = LSH_L;
const uint32_t NEST_W = ALL_USER_NUM / NEST_LOAD / NEST_L + 1;
const uint32_t NEST_DELTA = QUERY_BACK_NUM / LSH_L;
const uint32_t NEST_MAXKICKOUT = 10000;
//Configur Share Memory
const long lKey = 10086;
const long lEncKey = 12306;
//Configur Query Plaintext NEST Test Times
const uint32_t TEST_PLAINTEXT_OPERATOR = 100;
//Configur For Test distance Top K
const uint32_t TOP_K = 6;
const uint32_t AR_TOP_K[TOP_K] = {6, 10 ,50, 100, 200, 300};

//Work out All the Kickout Times;
uint32_t uiKickout;

//Kickout seed
uint32_t g_uiKickoutSeed = 0;

//global query random seed
const uint32_t g_uiQueryRandomSeed = 10086;



#ifndef AES_BLOCK_SIZE
#define AES_BLOCK_SIZE 16
#endif
//Configur for AES key
const string AES_LSH_KEY = "abdujuehdusijdue";
const string AES_UID_KEY = "ajdufisoeicusjdi";

#define DEF_BIG_LEN 500000
#define SP_IP 192.168.0.15
#define SP_PORT 40001

enum emArgCommand{
    //Input Category Path
    InitTrain,

    //Input 
    RandomUser
};

typedef struct
{
    uint32_t uiUid;
    uint32_t uiLabel;
}UserElement;

typedef struct
{
    uint32_t uiUid;
    uint32_t uiLabel;
    char arAES[AES_BLOCK_SIZE];
}UserCipherElement;

typedef struct
{
    uint32_t arLsh[LSH_L];
    char arAES[AES_BLOCK_SIZE];
}CipherLshValue;


//Easy for global
CImgCtl oImgCtl;
//Print CMD FLAG
int iPrintCmdListFlag = 0;
//Global for category pictures path
vector<string> g_vecImgPath;
//Global for open surf features path
vector<string> g_vecSurfPath;
double *g_arUserBow[ALL_USER_NUM];
//Global for E2lsh
CE2lsh g_oE2lsh;
uint32_t *g_arUserLSH[ALL_USER_NUM];
//Global for CNestHash
CNestHash<UserElement> g_oNestHash;
//Global for Share Memory
char *g_memory = NULL;
char *g_encemory = NULL;
//For Encryption
AES_KEY key_enc_uid, key_enc_lsh, key_dec_uid, key_dec_lsh;
//Global for Encryption CNestHash
CNestHash<UserCipherElement> g_oCipherNestHash;
//Global for Encryption LSH
CipherLshValue g_arCipherUserLSH[ALL_USER_NUM];



//Collect All the JPG in the path
int CollectFile(string strPath, vector<string> *pVecFiles, string sExtension = ".jpg")
{
    for (directory_iterator iter = directory_iterator(strPath); iter != directory_iterator(); iter++)
    {
        directory_entry entry = *iter;
        path entrypath = entry.path();
        //if it is a file then add to vector; if it is a directory, deep in.
        if (!is_directory(entrypath)) {
            if (entrypath.extension() == sExtension) {
                pVecFiles->push_back(entrypath.string());
            }
        }
        else
        {
            //is a directory
            CollectFile(entrypath.string(), pVecFiles, sExtension);
        }
    }
    return 0;
}

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

void PrintCmdList(){
    if (0 != iPrintCmdListFlag){
        cout<<"Press [Enter] to continue...\n"<<endl;
        int tmp_i = getchar();
    }
    system("clear");
    cout<<"========================================================================================================"<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=                                          User Client Similar Tool                                    ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=     Input Command Below :                                                                            ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=         1 Init the train vocabulary from a path                                                      ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=         2 Save KMeans Vocabulary To a path                                                           ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=         3 Load KMeans Vocabulary From a path                                                         ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=         4 Make 100 users with random number picture AND send to SP                                   ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=         5 Train the SSH functions params output 100 user's Frequency                                 ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=         6 Compute All the User 's BOW Frequency And Save to file                                     ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=         7 Load User's BOW Frequency                                                                  ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=         8 Computing E2Lsh Value And Save to file                                                     ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=         9 Build Nest Hash Table And Insert Test Data                                                 ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=         A Load User's LSH from file                                                                  ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=         B Test Plaintext Nest Operator                                                               ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=         C Build Encryption Nest Hash Table And Insert Test Data                                      ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=         D Test Ciphertext Nest Operator                                                              ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=         E Make a Nest Hash And then Encrypt It.                                                      ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=         F Try test Map LSH Accurary And Score Method 's Accurary                                     ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=         G Load the Flicker Picutres And train the vocabulary of KMeans ( K = 500 ) Save to a Path    ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=         H Work out Bow and Save to File                                                              ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"=                                                                                                      ="<<endl;
    cout<<"========================================================================================================"<<endl;
    iPrintCmdListFlag++;
    return;
}

void TrainFromPath(){
    //Init Directory
    g_vecImgPath.clear();
    CollectFile("/root/101_ObjectCategories", &g_vecImgPath);
    printf("Prepare the Categories Image : %u\n", g_vecImgPath.size());
    printf("Begin To Train...\n");
    DiffTime();
    int iRet = oImgCtl.Train(g_vecImgPath, DEF_KMEANS_K);
    printf("Finish The Train. Takes %u us.\n", DiffTime());
    return;
}

void TrainSshFunctionParams(){
    printf("This command will output two files:\nFirst:100 user's BOW Requency.\nSecond:10 user's BOW Requency!\n");
    printf("Now Begin...\n");
    double* arUserBow[ALL_TRAIN_NUM];
    vector<string> arUserPath[ALL_TRAIN_NUM];
    DiffTime();
    for (uint32_t uiCur = 0; uiCur < ALL_TRAIN_NUM; uiCur++){
        //Random smaller than 50 pictures for each user
        uint32_t uiRandomNum = rand() % 50 + 10;
        for (uint32_t uiImg = 0; uiImg < uiRandomNum; uiImg++)
        {
            arUserPath[uiCur].push_back(g_vecImgPath[rand() % g_vecImgPath.size()]);
        }
    }
    //Not only extract BOW Description but also work out the requency.
    oImgCtl.ExtractBOWDescriptor(arUserPath, arUserBow, ALL_TRAIN_NUM);
    printf("Finish Work Out the User 's BOW Requency. Use %u us.\n", DiffTime());
    printf("Now Put it into files...\n");
    ofstream fout, fq;
    fout.open("mnist1k.dts");
    fq.open("mnist1k.q");
    char szBuf[100];
    for (uint32_t uiCur = 0; uiCur < ALL_TRAIN_NUM; uiCur++)
    {
        for (uint32_t uiIdx = 0; uiIdx < DEF_KMEANS_K; uiIdx++)
        {
            snprintf(szBuf, sizeof(szBuf), "%.6lf ", arUserBow[uiCur][uiIdx]);
            fout << szBuf;
            if (uiCur < 10)
            {
                fq << szBuf;
            }
        }
        if (uiCur < 10)
        {
            fq << endl;
            fq << flush;
        }
        fout << endl;
        fout << flush;
        //free
        delete[] arUserBow[uiCur];
    }

    fout.close();
    fq.close();
}

void LoadAllUserBOWFromFile()
{
    fstream fp;
    fp.open("/data/ppsn/UserBOWRequency.dat");
    
    for (uint32_t uiCur = 0; uiCur < ALL_USER_NUM; uiCur++)
    {
        if (NULL != g_arUserBow[uiCur])
        {
            delete[] g_arUserBow[uiCur];
            g_arUserBow[uiCur] = NULL;
        }
    }
    
    for (uint32_t uiCur = 0; uiCur < ALL_USER_NUM; uiCur++)
    {
        g_arUserBow[uiCur] = new double[DEF_KMEANS_K];
        for (uint32_t uiD = 0; uiD < DEF_KMEANS_K; uiD++)
        {
            fp>>g_arUserBow[uiCur][uiD];
        }
    }
    printf("Finish Loading User BOW Requency\n");
    fp.close();
}

void LoadAllUserLSHFromFile()
{
    char szTmp[500];
    snprintf(szTmp, sizeof(szTmp), "/data/ppsn/UserLSHVector_%u.dat", LSH_L);
    fstream fp;
    fp.open(szTmp);
    for (uint32_t uiCur = 0; uiCur < ALL_USER_NUM; uiCur++)
    {
        if (NULL != g_arUserLSH[uiCur])
        {
            delete[] g_arUserLSH[uiCur];
            g_arUserLSH[uiCur] = NULL;
        }
    }

    for (uint32_t uiCur = 0; uiCur < ALL_USER_NUM; uiCur++)
    {
        g_arUserLSH[uiCur] = new uint32_t[LSH_L];
        for (uint32_t uiL = 0; uiL < LSH_L; uiL++)
        {
            fp>>g_arUserLSH[uiCur][uiL];
        }
    }
    printf("Finish Loading User Lsh Vector\n");
    fp.close();
}

bool insertIntoEncNestInCommon(uint32_t uiInsertId, uint32_t uiInsertLabel, uint32_t *pLsh, int iKickL, int iMaxKickout, uint32_t uiBeg = 0)
{
    static UserCipherElement oUser;
    if (iMaxKickout < 0)
    {
        return false;
    }
    UserCipherElement *pNode = g_oCipherNestHash.NewPlace(iKickL, pLsh, uiBeg);
    if (iKickL < 0)
    {
        pNode->uiUid = uiInsertId;
        pNode->uiLabel = uiInsertLabel;
        return true;
    }
    else
    {
        uiKickout++;
        //need to kick out
        //save the old to mem
        oUser.uiLabel = pNode->uiLabel;
        oUser.uiUid = pNode->uiUid;
        //set the new
        pNode->uiUid = uiInsertId;
        pNode->uiLabel = uiInsertLabel;
        //continue insert old
        return insertIntoEncNestInCommon(oUser.uiUid, oUser.uiLabel, g_arUserLSH[oUser.uiLabel], iKickL, iMaxKickout - 1, (uint32_t)iKickL + 1);
    }
    return false;
}

bool insertIntoNest(uint32_t uiInsertId, uint32_t uiInsertLabel, uint32_t *pLsh, int iKickL, int iMaxKickout, uint32_t uiBeg = 0)
{
    static UserElement oUser;
    if (iMaxKickout < 0)
    {
        return false;
    }
    UserElement *pNode = g_oNestHash.NewPlace(iKickL, pLsh, uiBeg);
    if (iKickL < 0)
    {
        pNode->uiUid = uiInsertId;
        pNode->uiLabel = uiInsertLabel;
        return true;
    }
    else
    {
        uiKickout++;
        //need to kick out
        //save the old to mem
        oUser = *pNode;
        //set the new
        pNode->uiUid = uiInsertId;
        pNode->uiLabel = uiInsertLabel;
        //continue insert old
        return insertIntoNest(oUser.uiUid, oUser.uiLabel, g_arUserLSH[oUser.uiLabel], iKickL, iMaxKickout - 1, (uint32_t)iKickL + 1);
    }
    return false;
}

void GetUserEncLsh(uint32_t uiLabel, uint32_t* pData)
{
    char arTmpAES[AES_BLOCK_SIZE];
    CipherLshValue *pEnc = g_arCipherUserLSH + uiLabel;
    AES_decrypt(pEnc->arAES, arTmpAES, &key_dec_lsh);
    srand(*(uint32_t*)arTmpAES);
    uint32_t *pTmp = pEnc->arLsh;
    for (uint32_t uiIdx = 0; uiIdx < LSH_L; uiIdx++)
    {
        *pData++ = *pTmp++ ^ rand();
    }
}



bool insertEncIntoNest(uint32_t uiInsertId, uint32_t uiLabel, uint32_t *arLSH, int iMaxKickout, uint32_t uiBeg = 0)
{
    static uint32_t aruiTmp[LSH_L];
    const uint32_t uiQueryNum = NEST_L * (NEST_DELTA + 1);
    static UserCipherElement arUserCipherElement[uiQueryNum];
    UserCipherElement *pUCE;
    uint32_t uiSeed;
    uint32_t uiCurIdx;
    //flag to check if there is a empty to place the element.
    bool bFindEmpty = false;
    if (iMaxKickout < 0)
    {
        return false;
    }
    //if it is first insert or kickout.
    if (NULL == arLSH)
    {
        //Kickout have no LSH save in sp. so need to get from cloud.
        GetUserEncLsh(uiLabel, aruiTmp);
        arLSH = aruiTmp;
    }
    
    g_oCipherNestHash.QueryToMemory(arLSH, (char*)arUserCipherElement);
    //decryption all the data
    pUCE = arUserCipherElement;
    for (uint32_t uiCur = 0; uiCur < uiQueryNum; uiCur++, pUCE++)
    {
        AES_decrypt(pUCE->arAES, pUCE->arAES, &key_dec_uid);
        srand(*(uint32_t*)pUCE->arAES);
        pUCE->uiUid ^= rand();
        pUCE->uiLabel ^= rand();
        *(uint32_t*)pUCE->arAES = rand();
    }
    //check if there is a empty node.
    //check the first node.

    for (uint32_t uiCur = 0; uiCur < LSH_L; uiCur++)
    {
        pUCE = arUserCipherElement + ((uiBeg + uiCur) % NEST_L) * (NEST_DELTA + 1);
        if (0 == pUCE->uiUid && 0 == pUCE->uiLabel)
        {
            pUCE->uiUid = uiInsertId;
            pUCE->uiLabel = uiLabel;
            bFindEmpty = true;
            break;
        }
    }

    //check the delta node.
    if (!bFindEmpty){
        for (uint32_t uiCur = 0; uiCur < LSH_L; uiCur++)
        {
            pUCE = arUserCipherElement + ((uiBeg + uiCur) % NEST_L) * (NEST_DELTA + 1) + 1;
            for (uint32_t uiIdx = 0; uiIdx < NEST_DELTA; uiIdx++, pUCE++)
            {
                if (0 == pUCE->uiUid && 0 == pUCE->uiLabel)
                {
                    pUCE->uiUid = uiInsertId;
                    pUCE->uiLabel = uiLabel;
                    bFindEmpty = true;
                    //sorry for write in this way.
                    //just force to quit the double circle.
                    uiCur = LSH_L;
                    break;
                }
            }
        }
    }
    //if find empty element, need to encrypt all data
    if (bFindEmpty)
    {
        pUCE = arUserCipherElement;
        for (uint32_t uiCur = 0; uiCur < uiQueryNum; uiCur++, pUCE++)
        {
            srand(*(uint32_t*)pUCE->arAES);
            pUCE->uiUid ^= rand();
            pUCE->uiLabel ^= rand();
            AES_encrypt(pUCE->arAES, pUCE->arAES, &key_enc_uid);
        }
        g_oCipherNestHash.SetGroup(arLSH, arUserCipherElement);
        return true;
    }
    //we have to kickout some node.
    uiKickout++;
    static UserCipherElement oTmp;
    srand(g_uiKickoutSeed++);
    uint32_t uiKickL = rand() % NEST_L;
    pUCE = arUserCipherElement + uiKickL * (NEST_DELTA + 1);
    oTmp.uiUid = pUCE->uiUid;
    oTmp.uiLabel = pUCE->uiLabel;
    pUCE->uiUid = uiInsertId;
    pUCE->uiLabel = uiLabel;
    //encrypt the L node
    pUCE = arUserCipherElement;
    for (uint32_t uiCur = 0; uiCur < NEST_L; uiCur++)
    {
        srand(*(uint32_t*)pUCE->arAES);
        pUCE->uiUid ^= rand();
        pUCE->uiLabel ^= rand();
        AES_encrypt(pUCE->arAES, pUCE->arAES, &key_enc_uid);
        pUCE += NEST_DELTA + 1;
    }
    g_oCipherNestHash.SetCore(arLSH, arUserCipherElement);
    return insertEncIntoNest(oTmp.uiUid, oTmp.uiLabel, NULL, iMaxKickout - 1, uiKickL + 1);
}

//this is function is that:
//first build a common Nest hash table.
//second encrypt it!
void BuildNestHashTableAndEncryptIt()
{
    //Build Lookup Table
    srand(1);
    uiKickout = 0;
    uint32_t *puiTmp, *puiLSH;
    CipherLshValue oCipherLshValue;
    CipherLshValue *pEnc = g_arCipherUserLSH;
    printf("Begin To Build Lookup table.\n");
    DiffTime();
    for (uint32_t uiCur = 0; uiCur < ALL_USER_NUM; uiCur++, pEnc++)
    {
        puiTmp = pEnc->arLsh;
        puiLSH = g_arUserLSH[uiCur];
        uint32_t uiSeed = *(uint32_t*)pEnc->arAES = rand();
        srand(uiSeed);
        for (uint32_t uiIdx = 0; uiIdx < LSH_L; uiIdx++)
        {
            *puiTmp++ = *puiLSH++ ^ rand();
        }
        AES_encrypt(pEnc->arAES, pEnc->arAES, &key_enc_lsh);
    }
    uint32_t uiTimeDiff = DiffTime();
    printf("Finish Build The encryption Lookup table for lsh , Using %u us.\n", uiTimeDiff);
    //Build The Encryption Nest
    printf("Begin to build the encryption Nest\n");
    size_t sSize = NEST_W * NEST_L * sizeof(UserCipherElement);
    if (0 > GetShm2(&g_encemory, lEncKey, sSize, 0666 | IPC_CREAT))
    {
        printf("ERROR : Get Shm False![%lu]\n", lKey);
        return;
    }
    printf("Finish Create Encryption Share Memory On Size : %u\n", sSize);
    printf("Begin to Init the Empty Random Hash table\n");
    g_oCipherNestHash.InitNestHash(g_encemory, NEST_L, NEST_DELTA, NEST_W);
    DiffTime();
    for (uint32_t uiCur = 0; uiCur < ALL_USER_NUM; uiCur++)
    {
        //the label is test as same as uid
        if (!insertIntoEncNestInCommon(uiCur, uiCur, g_arUserLSH[uiCur], -1, NEST_MAXKICKOUT))
        {
            printf("With No.%u, Can not Kick after %u Times.\n", uiCur, NEST_MAXKICKOUT);
            printf("All Kickout %u times\n", uiKickout);
            return;
        }
    }
    uiTimeDiff = DiffTime();
    printf("Finish Insert Into Encryption NestHashTable , Using %u us.\n", uiTimeDiff);
    printf("All Kickout %u times\n", uiKickout);
    printf("Begin to Turn it into encryption . \n");
    DiffTime();
    UserCipherElement *pEle = g_oCipherNestHash.Get(0, 0);
    for (uint32_t uiCur = 0; uiCur < NEST_L * NEST_W; uiCur++, pEle++)
    {
        uint32_t uiSeed = *(uint32_t*)pEle->arAES = rand();
        srand(uiSeed);
        pEle->uiUid ^= rand();
        pEle->uiLabel ^= rand();
        AES_encrypt(pEle->arAES, pEle->arAES, &key_enc_uid);
    }
    uiTimeDiff = DiffTime();
    printf("Finish the Nest 's Encryption in %u us.\n", uiTimeDiff);
}

void BuildEncryptionHashTable()
{
    //Build Lookup Table
    srand(1);
    uiKickout = 0;
    uint32_t *puiTmp, *puiLSH;
    CipherLshValue oCipherLshValue;
    CipherLshValue *pEnc = g_arCipherUserLSH;
    printf("Begin To Build Lookup table.\n");
    DiffTime();
    for (uint32_t uiCur = 0; uiCur < ALL_USER_NUM; uiCur++, pEnc++)
    {
        puiTmp = pEnc->arLsh;
        puiLSH = g_arUserLSH[uiCur];
        uint32_t uiSeed = *(uint32_t*)pEnc->arAES = rand();
        srand(uiSeed);
        for (uint32_t uiIdx = 0; uiIdx < LSH_L; uiIdx++)
        {
            *puiTmp++ = *puiLSH++ ^ rand();
        }
        AES_encrypt(pEnc->arAES, pEnc->arAES, &key_enc_lsh);
    }
    uint32_t uiTimeDiff = DiffTime();
    printf("Finish Build The encryption Lookup table for lsh , Using %u us.\n", uiTimeDiff);
    //Build The Encryption Nest
    printf("Begin to build the encryption Nest\n");
    size_t sSize = NEST_W * NEST_L * sizeof(UserCipherElement);
    if (0 > GetShm2(&g_encemory, lEncKey, sSize, 0666 | IPC_CREAT))
    {
        printf("ERROR : Get Shm False![%lu]\n", lKey);
        return;
    }
    printf("Finish Create Encryption Share Memory On Size : %u\n", sSize);
    printf("Begin to Init the Empty Random Hash table\n");
    g_oCipherNestHash.InitNestHash(g_encemory, NEST_L, NEST_DELTA, NEST_W);
    for (uint32_t uiL = 0; uiL < NEST_L; uiL++)
    {
        for (uint32_t uiW = 0; uiW < NEST_W; uiW++)
        {
            UserCipherElement *pEle = g_oCipherNestHash.Get(uiL, uiW);
            uint32_t uiSeed = *(uint32_t*)pEle->arAES = rand();
            srand(uiSeed);
            pEle->uiUid = 0 ^ rand();
            pEle->uiLabel = 0 ^ rand();
            AES_encrypt(pEle->arAES, pEle->arAES, &key_enc_uid);
        }
    }
    printf("Finish Init Encryption Nest Hash , Begin to Insert!\n");
    DiffTime();
    for (uint32_t uiCur = 0; uiCur < ALL_USER_NUM; uiCur++)
    {
        //the label is test as same as uid
        if (!insertEncIntoNest(uiCur, uiCur, g_arUserLSH[uiCur], NEST_MAXKICKOUT))
        {
            printf("With No.%u, Can not Kick after %u Times.\n", uiCur, NEST_MAXKICKOUT);
            printf("All Kickout %u times\n", uiKickout);
            return;
        }
    }
    uiTimeDiff = DiffTime();
    printf("Finish Insert Into Encryption NestHashTable , Using %u us.\n", uiTimeDiff);
}

void BuildNestHashTable()
{
    srand(1);
    //Init Kickout times.
    uiKickout = 0;
    printf("We Build Nest By Delta : %u, L : %u \n", NEST_DELTA, NEST_L);
    map<uint32_t, uint32_t> mapLshCount;
    uint32_t uiMaxCount = 0;
    size_t sSize = NEST_W * NEST_L * sizeof(UserElement);
    if (0 > GetShm2(&g_memory, lKey, sSize, 0666 | IPC_CREAT))
    {
        printf("ERROR : Get Shm False![%lu]\n", lKey);
        return;
    }
    printf("Finish Create Share Memory On Size : %u\n", sSize);
    g_oNestHash.InitNestHash(g_memory, NEST_L, NEST_DELTA, NEST_W);
    //Begin to insert element
    printf("Begin To Insert User Into NestHashTable < L = %u, Delta = %u >\n", NEST_L, NEST_DELTA);
    //collect the information
    for (uint32_t uiCur = 0; uiCur < ALL_USER_NUM; uiCur++)
    {
        mapLshCount[g_arUserLSH[uiCur][0]]++;
        if (mapLshCount[g_arUserLSH[uiCur][0]] > uiMaxCount)
        {
            uiMaxCount = mapLshCount[g_arUserLSH[uiCur][0]];
        }
    }
    DiffTime();
    for (uint32_t uiCur = 0; uiCur < ALL_USER_NUM; uiCur++)
    {
        //use UID as the label for example.
        //g_arUserLSH is index by the label.
        if (!insertIntoNest(uiCur, uiCur, g_arUserLSH[uiCur], -1, NEST_MAXKICKOUT))
        {
            g_oNestHash.CheckNestLoad();
            printf("With No.%u, Can not Kick after %u Times.\n", uiCur, NEST_MAXKICKOUT);
            printf("All Kickout %u times\n", uiKickout);
            return;
        }
    }
    uint32_t uiTimeDiff = DiffTime();
    printf("Finish Insert Into NestHashTable , Using %u us.\n", uiTimeDiff);
    g_oNestHash.CheckNestLoad();
    printf("We Got Max Hit Count is %u\n", uiMaxCount);
    printf("Now Our Nest Plaintext Struct use memory as %u\n", sSize);
    printf("If We use the encryption Key-Array struct , we need to use memory as %u\n", uiMaxCount * mapLshCount.size() * sizeof(uint32_t));
    printf("All operator need %u times kickout!\n", uiKickout);
}

void TestCiphertextOperator(){
    vector<uint32_t*> vecQueryLshVal;
    vector<uint32_t> vecQueryUID;
    UserCipherElement *arQuery = new UserCipherElement[NEST_L * (NEST_DELTA + 1)];
    srand(g_uiQueryRandomSeed);
    for (uint32_t uiCur = 0; uiCur < TEST_PLAINTEXT_OPERATOR; uiCur++)
    {
        uint32_t uiIdx;
        do
        {
            uiIdx = rand() % ALL_USER_NUM;
        }while(0 == uiIdx);
        vecQueryLshVal.push_back(g_arUserLSH[uiIdx]);
        vecQueryUID.push_back(uiIdx);
    }
    printf("Begin to Test Query %u times !\n", TEST_PLAINTEXT_OPERATOR);
    DiffTime();
    for (uint32_t uiCur = 0; uiCur < TEST_PLAINTEXT_OPERATOR; uiCur++)
    {
        g_oCipherNestHash.QueryToMemory(vecQueryLshVal[uiCur], (char*)arQuery);
        for (uint32_t uiIdx = 0; uiIdx < NEST_L * (NEST_DELTA + 1); uiIdx++)
        {
            AES_decrypt(arQuery[uiIdx].arAES, arQuery[uiIdx].arAES, &key_dec_uid);
            srand(*(uint32_t*)arQuery[uiIdx].arAES);
            arQuery[uiIdx].uiUid ^= rand();
            arQuery[uiIdx].uiLabel ^= rand();
        }
    }
    uint32_t uiEndTime = DiffTime();
    printf("Finish %u times Query in %u us.\n", TEST_PLAINTEXT_OPERATOR, uiEndTime);
    //begin to query and test
    uint32_t uiRight = 0;
    uint32_t uiAll = 0;
    vector<double> vecQDistance;
    vector<double> vecAllDistance;
    vector<double> dAllX;
    dAllX.assign(TOP_K, 0);
    for (uint32_t uiCur = 0; uiCur < TEST_PLAINTEXT_OPERATOR; uiCur++)
    {
        //Init Clear the vector<distance>
        vecAllDistance.clear();
        vecQDistance.clear();
        //begin to query

        g_oCipherNestHash.QueryToMemory(vecQueryLshVal[uiCur], (char*)arQuery);
        //compute the distance between l * d
        for (uint32_t uiIdx = 0; uiIdx < NEST_L * (NEST_DELTA + 1); uiIdx++)
        {
            AES_decrypt(arQuery[uiIdx].arAES, arQuery[uiIdx].arAES, &key_dec_uid);
            srand(*(uint32_t*)arQuery[uiIdx].arAES);
            arQuery[uiIdx].uiUid ^= rand();
            arQuery[uiIdx].uiLabel ^= rand();
            if (0 == arQuery[uiIdx].uiUid){
                continue;
            }
            uiAll++;
            double dDistance = g_oE2lsh.ComputeL2(g_arUserBow[vecQueryUID[uiCur]], g_arUserBow[arQuery[uiIdx].uiUid], DEF_KMEANS_K);
            vecQDistance.push_back(dDistance);
            if (dDistance <= LSH_R){
                uiRight++;
            }
        }
        sort(vecQDistance.begin(), vecQDistance.end(), less<double>());
        //Work Out distance between all user
        for (uint32_t uiIdx = 0; uiIdx < ALL_USER_NUM; uiIdx++)
        {
            double dDistance = g_oE2lsh.ComputeL2(g_arUserBow[vecQueryUID[uiCur]], g_arUserBow[uiIdx], DEF_KMEANS_K);
            vecAllDistance.push_back(dDistance);
        }
        sort(vecAllDistance.begin(), vecAllDistance.end(), less<double>());
        //Work Out TOP K 's X
        for (uint32_t uiK = 0; uiK < TOP_K; uiK++)
        {
            double dX = 0;
            uint32_t uiZeroNum = 0;
            for (uint32_t uiIdx = 0; uiIdx < AR_TOP_K[uiK]; uiIdx++)
            {
                if (0 == vecAllDistance[uiIdx] && 0 == vecQDistance[uiIdx]){
                    dX += 1;
                }
                else
                {
                    if (0 == vecAllDistance[uiIdx] || 0 == vecQDistance[uiIdx])
                    {
                        uiZeroNum++;
                    }
                    else
                    {
                        dX += vecAllDistance[uiIdx] / vecQDistance[uiIdx];
                    }
                }
            }
            dX = dX / (AR_TOP_K[uiK] - uiZeroNum);
            //printf("uiCur = %u, AVG<%u> = %f \n", uiCur, AR_TOP_K[uiK], dX);
            dAllX[uiK] += dX;
        }
    }
    for (uint32_t uiK = 0; uiK < TOP_K; uiK++)
    {
        dAllX[uiK] = dAllX[uiK] / TEST_PLAINTEXT_OPERATOR;
        printf(" All K = %u AVG<dAllX> = %f \n", AR_TOP_K[uiK], dAllX[uiK]);
    }
    printf("Right Percent: %u / %u %f %% \n", uiRight, uiAll, (double)uiRight * 100 / (double)uiAll);
    //prepare for delete
    printf("Begin to test Insert %u times !\n", TEST_PLAINTEXT_OPERATOR);
    map<uint32_t, UserCipherElement> mapInsert;
    while (mapInsert.size() != TEST_PLAINTEXT_OPERATOR)
    {
        uint32_t uiTmp;
        do
        {
            uiTmp = rand() % ALL_USER_NUM;
        }while(0 == uiTmp);
        mapInsert[uiTmp].uiUid = uiTmp;
        mapInsert[uiTmp].uiLabel = uiTmp;
    }
    //for more quickly use vector
    vector<UserCipherElement> vecInsertElement;
    vector<uint32_t*> vecInsertLsh;
    for (map<uint32_t, UserCipherElement>::iterator it = mapInsert.begin(); it != mapInsert.end(); it++)
    {
        vecInsertElement.push_back(it->second);
        vecInsertLsh.push_back(g_arUserLSH[it->first]);
    }
    //insert the delete item for test.
    uiKickout = 0;
    DiffTime();
    for (uint32_t uiCur = 0; uiCur < TEST_PLAINTEXT_OPERATOR; uiCur++)
    {
        if (!insertEncIntoNest(vecInsertElement[uiCur].uiUid, vecInsertElement[uiCur].uiLabel, vecInsertLsh[uiCur], NEST_MAXKICKOUT))
        {
            printf("With No.%u, Can not Kick after %u Times.\n", uiCur, NEST_MAXKICKOUT);
            return;
        }
    }
    uiEndTime = DiffTime();
    printf("Finish %u times Insert in %u us. Kickout %u times.\n", TEST_PLAINTEXT_OPERATOR, uiEndTime, uiKickout);
    printf("Begin to test Delete %u times !\n", TEST_PLAINTEXT_OPERATOR);
    map<uint32_t, UserCipherElement> mapDelete;
    while (mapDelete.size() != TEST_PLAINTEXT_OPERATOR)
    {
        uint32_t uiTmp;
        do
        {
            do
            {
                uiTmp = rand() % ALL_USER_NUM;
            }while(mapInsert.find(uiTmp) != mapInsert.end());
        }while(0 == uiTmp);
        mapDelete[uiTmp].uiUid = uiTmp;
        mapDelete[uiTmp].uiLabel = uiTmp;
    }
    //for more quickly use vector
    vector<UserCipherElement> vecDeleteElement;
    vector<uint32_t*> vecDeleteLsh;
    vecDeleteElement.clear();
    vecDeleteLsh.clear();
    for (map<uint32_t, UserCipherElement>::iterator it = mapDelete.begin(); it != mapDelete.end(); it++)
    {
        vecDeleteElement.push_back(it->second);
        vecDeleteLsh.push_back(g_arUserLSH[it->first]);
    }
    uint32_t uiDelete = 0;
    DiffTime();
    for (uint32_t uiCur = 0; uiCur < TEST_PLAINTEXT_OPERATOR; uiCur++)
    {
        g_oCipherNestHash.QueryToMemory(vecDeleteLsh[uiCur], (char*)arQuery);
        for(uint32_t uiIdx = 0; uiIdx < NEST_L * (NEST_DELTA + 1); uiIdx++)
        {
            AES_decrypt(arQuery[uiIdx].arAES, arQuery[uiIdx].arAES, &key_dec_uid);
            srand(*(uint32_t*)arQuery[uiIdx].arAES);
            arQuery[uiIdx].uiUid ^= rand();
            arQuery[uiIdx].uiLabel ^= rand();
            if (arQuery[uiIdx].uiUid == vecDeleteElement[uiCur].uiUid)
            {
                arQuery[uiIdx].uiUid = 0;
                arQuery[uiIdx].uiLabel = 0;
                uiDelete++;
            }
            *(uint32_t*)arQuery[uiIdx].arAES = rand();
            srand(*(uint32_t*)arQuery[uiIdx].arAES);
            arQuery[uiIdx].uiUid ^= rand();
            arQuery[uiIdx].uiLabel ^= rand();
            AES_encrypt(arQuery[uiIdx].arAES, arQuery[uiIdx].arAES, &key_enc_uid);
        }
        g_oCipherNestHash.SetGroup(vecDeleteLsh[uiCur], arQuery);
    }
    uiEndTime = DiffTime();
    printf("Finish %u/%u times Delete in %u us.\n", uiDelete, TEST_PLAINTEXT_OPERATOR, uiEndTime);

}

void TestPlaintextOperator(){
    vector<uint32_t*> vecQueryLshVal;
    vector<uint32_t> vecQueryUID;
    UserElement *arQuery = new UserElement[NEST_L * (NEST_DELTA + 1)];
    srand(g_uiQueryRandomSeed);
    for (uint32_t uiCur = 0; uiCur < TEST_PLAINTEXT_OPERATOR; uiCur++)
    {
        uint32_t uiIdx;
        do
        {
            uiIdx = rand() % ALL_USER_NUM;
        }while(0 == uiIdx);
        vecQueryLshVal.push_back(g_arUserLSH[uiIdx]);
        vecQueryUID.push_back(uiIdx);
    }
    printf("Begin to Test Query %u times !\n", TEST_PLAINTEXT_OPERATOR);
    DiffTime();
    for (uint32_t uiCur = 0; uiCur < TEST_PLAINTEXT_OPERATOR; uiCur++)
    {
        g_oNestHash.QueryToMemory(vecQueryLshVal[uiCur], (char*)arQuery);
    }
    uint32_t uiEndTime = DiffTime();
    printf("Finish %u times Query in %u us.\n", TEST_PLAINTEXT_OPERATOR, uiEndTime);
    //begin to query and test
    uint32_t uiRight = 0;
    uint32_t uiAll = 0;
    vector<double> vecQDistance;
    vector<double> vecAllDistance;
    vector<double> dAllX;
    dAllX.assign(TOP_K, 0);
    for (uint32_t uiCur = 0; uiCur < TEST_PLAINTEXT_OPERATOR; uiCur++)
    {
        //Init Clear the vector<distance>
        vecAllDistance.clear();
        vecQDistance.clear();
        //begin to query
        g_oNestHash.QueryToMemory(vecQueryLshVal[uiCur], (char*)arQuery);
        //compute the distance between l * d
        for (uint32_t uiIdx = 0; uiIdx < NEST_L * (NEST_DELTA + 1); uiIdx++)
        {
            if (0 == arQuery[uiIdx].uiUid){
                continue;
            }
            uiAll++;
            double dDistance = g_oE2lsh.ComputeL2(g_arUserBow[vecQueryUID[uiCur]], g_arUserBow[arQuery[uiIdx].uiUid], DEF_KMEANS_K);
            vecQDistance.push_back(dDistance);
            if (dDistance <= LSH_R){
                uiRight++;
            }
        }
        sort(vecQDistance.begin(), vecQDistance.end(), less<double>());
        //Work Out distance between all user
        for (uint32_t uiIdx = 0; uiIdx < ALL_USER_NUM; uiIdx++)
        {
            double dDistance = g_oE2lsh.ComputeL2(g_arUserBow[vecQueryUID[uiCur]], g_arUserBow[uiIdx], DEF_KMEANS_K);
            vecAllDistance.push_back(dDistance);
        }
        sort(vecAllDistance.begin(), vecAllDistance.end(), less<double>());
        //Work Out TOP K 's X
        for (uint32_t uiK = 0; uiK < TOP_K; uiK++)
        {
            double dX = 0;
            uint32_t uiZeroNum = 0;
            for (uint32_t uiIdx = 0; uiIdx < AR_TOP_K[uiK]; uiIdx++)
            {
                if (0 == vecAllDistance[uiIdx] && 0 == vecQDistance[uiIdx]){
                    dX += 1;
                }
                else
                {
                    if (0 == vecAllDistance[uiIdx] || 0 == vecQDistance[uiIdx])
                    {
                        uiZeroNum++;
                    }
                    else
                    {
                        dX += vecAllDistance[uiIdx] / vecQDistance[uiIdx];
                    }
                }
            }
            dX = dX / (AR_TOP_K[uiK] - uiZeroNum);
            //printf("uiCur = %u, AVG<%u> = %f \n", uiCur, AR_TOP_K[uiK], dX);
            dAllX[uiK] += dX;
        }
    }
    for (uint32_t uiK = 0; uiK < TOP_K; uiK++)
    {
        dAllX[uiK] = dAllX[uiK] / TEST_PLAINTEXT_OPERATOR;
        printf(" All K = %u AVG<dAllX> = %f \n", AR_TOP_K[uiK], dAllX[uiK]);
    }
    printf("Right Percent: %u / %u %f %% \n", uiRight, uiAll, (double)uiRight * 100 / (double)uiAll);
    //prepare for insert
    printf("Begin to test Insert %u times !\n", TEST_PLAINTEXT_OPERATOR);
    map<uint32_t, UserElement> mapInsert;
    while (mapInsert.size() != TEST_PLAINTEXT_OPERATOR)
    {
        uint32_t uiTmp;
        do
        {
            uiTmp = rand() % ALL_USER_NUM;
        }while(0 == uiTmp);
        mapInsert[uiTmp].uiUid = uiTmp;
        mapInsert[uiTmp].uiLabel = uiTmp;
    }
    //for more quickly use vector
    vector<UserElement> vecInsertElement;
    vector<uint32_t*> vecInsertLsh;
    vecInsertElement.clear();
    vecInsertLsh.clear();
    for (map<uint32_t, UserElement>::iterator it = mapInsert.begin(); it != mapInsert.end(); it++)
    {
        vecInsertElement.push_back(it->second);
        vecInsertLsh.push_back(g_arUserLSH[it->first]);
    }
    //insert the insert item for test.
    uiKickout = 0;
    DiffTime();
    for (uint32_t uiCur = 0; uiCur < TEST_PLAINTEXT_OPERATOR; uiCur++)
    {
        if (!insertIntoNest(vecInsertElement[uiCur].uiUid, vecInsertElement[uiCur].uiLabel, vecInsertLsh[uiCur], -1, NEST_MAXKICKOUT))
        {
            printf("With No.%u, Can not Kick after %u Times.\n", uiCur, NEST_MAXKICKOUT);
            return;
        }
    }
    uiEndTime = DiffTime();
    printf("Finish %u times Insert in %u us. Kickout %u times.\n", TEST_PLAINTEXT_OPERATOR, uiEndTime, uiKickout);
    map<uint32_t, UserElement> mapDelete;
    while (mapDelete.size() != TEST_PLAINTEXT_OPERATOR)
    {
        uint32_t uiTmp;
        do
        {
            uiTmp = rand() % ALL_USER_NUM;
            if (0 == uiTmp)
            {
                continue;
            }
            if (mapInsert.find(uiTmp) != mapInsert.end())
            {
                continue;
            }
            break;
        }while(true);
        mapDelete[uiTmp].uiUid = uiTmp;
        mapDelete[uiTmp].uiLabel = uiTmp;
    }
    //for more quickly use vector
    vector<UserElement> vecDeleteElement;
    vector<uint32_t*> vecDeleteLsh;
    vecDeleteElement.clear();
    vecDeleteLsh.clear();
    for (map<uint32_t, UserElement>::iterator it = mapDelete.begin(); it != mapDelete.end(); it++)
    {
        vecDeleteElement.push_back(it->second);
        vecDeleteLsh.push_back(g_arUserLSH[it->first]);
    }
    printf("Begin to test Delete %u times !\n", TEST_PLAINTEXT_OPERATOR);
    uint32_t uiDelete = 0;
    DiffTime();
    for (uint32_t uiCur = 0; uiCur < TEST_PLAINTEXT_OPERATOR; uiCur++)
    {
        if (0 == g_oNestHash.DeletePlace(vecDeleteLsh[uiCur], (char*)&vecDeleteElement[uiCur]))
        {
            uiDelete++;
        }
    }
    uiEndTime = DiffTime();
    printf("Finish %u/%u times Delete in %u us.\n", uiDelete, TEST_PLAINTEXT_OPERATOR, uiEndTime);
}

void ComputeE2LshValueAndSaveToFile_Search()
{
    printf("This Command will compute the LSH Value\n Now Alloc Memory...\n");
    g_oE2lsh.InitLSH(LSH_R, DEF_KMEANS_K, LSH_L, LSH_K, LSH_W);
    for (uint32_t uiCur = 0; uiCur < g_vecImgPath.size(); uiCur++)
    {
        g_arUserLSH[uiCur] = new uint32_t[LSH_L];
    }
    printf("Begin to Computing...\n");
    //16 Byte = 4 X 4
    uint32_t arMD5Left32BIT[4];
    DiffTime();
    for (uint32_t uiCur = 0; uiCur < g_vecImgPath.size(); uiCur++)
    {
        if (uiCur % 10000 == 0)
        {
            printf("Finish Compute Percent %u %%\n", uiCur / 10000);
        }
        g_oE2lsh.ComputeLSH(g_arUserBow[uiCur], g_arUserLSH[uiCur]);
    }
    uint32_t uiTimeDiff = DiffTime();
    printf("Finish Work Out the LSH. Use %u us.\n", uiTimeDiff);
    //MD5
    printf("Begin to Work MD5 for first 32BIT One Way HaSH !\N");
    DiffTime();
    for (uint32_t uiCur = 0; uiCur < g_vecImgPath.size(); uiCur++)
    {
        for (uint32_t uiIdx = 0; uiIdx < LSH_L; uiIdx++)
        {
            //MD5((unsigned char*)(g_arUserLSH[uiCur] + uiIdx), sizeof(uint32_t), (unsigned char*)arMD5Left32BIT);
            //g_arUserLSH[uiCur][uiIdx] = arMD5Left32BIT[0];
        }
    }
    uiTimeDiff = DiffTime();
    printf("Finish Work Out LSH MD5 One way hash !. Use %u us.\n Begin to Write into file\n", uiTimeDiff);
    //Write it into file
    ofstream fout;
    char szTmp[500];
    snprintf(szTmp, sizeof(szTmp), "/data/ppsn/UserLSHVector_%u_search.dat", LSH_L);
    string sPath(szTmp);
    fout.open(sPath);
    char szBuf[1000];
    for (uint32_t uiCur = 0; uiCur < g_vecImgPath.size(); uiCur++)
    {
        for (uint32_t uiIdx = 0; uiIdx < LSH_L; uiIdx++)
        {
            snprintf(szBuf, sizeof(szBuf), "%u ", g_arUserLSH[uiCur][uiIdx]);
            fout << szBuf;
        }
        fout << endl;
        fout << flush;
    }
    fout.close();
}

void ComputeE2LshValueAndSaveToFile()
{
    printf("This Command will compute the LSH Value\n Now Alloc Memory...\n");
    g_oE2lsh.InitLSH(LSH_R, DEF_KMEANS_K, LSH_L, LSH_K, LSH_W);
    for (uint32_t uiCur = 0; uiCur < ALL_USER_NUM; uiCur++)
    {
        g_arUserLSH[uiCur] = new uint32_t[LSH_L];
    }
    printf("Begin to Computing...\n");
    //16 Byte = 4 X 4
    uint32_t arMD5Left32BIT[4];
    DiffTime();
    for (uint32_t uiCur = 0; uiCur < ALL_USER_NUM; uiCur++)
    {
        if (uiCur % 10000 == 0)
        {
            printf("Finish Compute Percent %u %%\n", uiCur / 10000);
        }
        g_oE2lsh.ComputeLSH(g_arUserBow[uiCur], g_arUserLSH[uiCur]);
    }
    uint32_t uiTimeDiff = DiffTime();
    printf("Finish Work Out the LSH. Use %u us.\n", uiTimeDiff);
    //MD5
    printf("Begin to Work MD5 for first 32BIT One Way HaSH !\N");
    DiffTime();
    for (uint32_t uiCur = 0; uiCur < ALL_USER_NUM; uiCur++)
    {
        for (uint32_t uiIdx = 0; uiIdx < LSH_L; uiIdx++)
        {
            //MD5((unsigned char*)(g_arUserLSH[uiCur] + uiIdx), sizeof(uint32_t), (unsigned char*)arMD5Left32BIT);
            //g_arUserLSH[uiCur][uiIdx] = arMD5Left32BIT[0];
        }
    }
    uiTimeDiff = DiffTime();
    printf("Finish Work Out LSH MD5 One way hash !. Use %u us.\n Begin to Write into file\n", uiTimeDiff);
    //Write it into file
    ofstream fout;
    char szTmp[500];
    snprintf(szTmp, sizeof(szTmp), "/data/ppsn/UserLSHVector_%u.dat", LSH_L);
    string sPath(szTmp);
    fout.open(sPath);
    char szBuf[1000];
    for (uint32_t uiCur = 0; uiCur < ALL_USER_NUM; uiCur++)
    {
        for (uint32_t uiIdx = 0; uiIdx < LSH_L; uiIdx++)
        {
            snprintf(szBuf, sizeof(szBuf), "%u ", g_arUserLSH[uiCur][uiIdx]);
            fout << szBuf;
        }
        fout << endl;
        fout << flush;
    }
    fout.close();
}

void TestScoreMethodAccurary()
{
    printf("This Function need lots of memory and do not care time!\n");
    map<uint32_t, vector<uint32_t> > armapTrueLsh[LSH_L];
    //this circle input all the lsh into map. 
    for (uint32_t uiCur = 0; uiCur < ALL_USER_NUM; uiCur++)
    {
        for (uint32_t uiIdx = 0; uiIdx < LSH_L; uiIdx++)
        {
            uint32_t uiLsh = g_arUserLSH[uiCur][uiIdx];
            armapTrueLsh[uiIdx][uiLsh].push_back(uiCur);
        }
    }
    //init random query id
    map<uint32_t, uint32_t> mapQuery;
    vector<uint32_t> vecQuery;
    uint32_t uiQueryBackNum = NEST_L * (NEST_DELTA + 1);
    srand(g_uiQueryRandomSeed);
    while (mapQuery.size() != TEST_PLAINTEXT_OPERATOR)
    {
        uint32_t uiTmp;
        do
        {
            uiTmp = rand() % ALL_USER_NUM;
        }while(0 == uiTmp);
        mapQuery[uiTmp] = uiTmp;
    }
    for (map<uint32_t, uint32_t>::iterator it = mapQuery.begin(); it != mapQuery.end(); it++)
    {
        vecQuery.push_back(it->first);
    }
    //begin to test
    printf("Begin to Test True LSH Accurary\n");
    
    uint32_t uiTrueRight = 0;
    uint32_t uiRight = 0;
    uint32_t uiAll = 0;
    vector<double> dAllX;
    dAllX.assign(TOP_K, 0);
    uint32_t aruiShort[TOP_K];
    memset(aruiShort, 0, sizeof(aruiShort));

    for (uint32_t uiCur = 0; uiCur < TEST_PLAINTEXT_OPERATOR; uiCur++)
    {
        printf("Doing %u ...\n", uiCur);
        vector<double> vecQDistance;
        vector<double> vecAllDistance;
        //use for store the id
        vector<uint32_t> vecIDList;
        //use for make the id list unique.
        map<uint32_t, uint32_t> mapIDList;
        //query the id.
        uint32_t uiQueryId = vecQuery[uiCur];
        for (uint32_t uiIdx = 0; uiIdx < LSH_L; uiIdx++)
        {
            uint32_t uiQueryIdLsh = g_arUserLSH[uiQueryId][uiIdx];
            vector<uint32_t> *pVec = &armapTrueLsh[uiIdx][uiQueryIdLsh];
            for (vector<uint32_t>::iterator it = pVec->begin(); it != pVec->end(); it++)
            {
                mapIDList[*it]++;
            }
        }
        printf("uiCur %u has %u 's node with same lsh value. \n", uiQueryId, mapIDList.size());
        //use a map to work out the top (l * d) score
        map<uint32_t, vector<uint32_t> > mapScore;
        for (map<uint32_t, uint32_t>::iterator it = mapIDList.begin(); it != mapIDList.end(); it++)
        {
            mapScore[it->second].push_back(it->first);
        }
        printf("MapScore 's kinds : %u\n", mapScore.size());
        //make a flag for jump two circle.
        for (uint32_t uiK = 0; uiK < TOP_K; uiK++)
        {
            vecQDistance.clear();
            vecAllDistance.clear();
            uiQueryBackNum = AR_TOP_K[uiK];
            vecIDList.clear();
            bool bFull = false;
            for (map<uint32_t, vector<uint32_t> >::reverse_iterator it = mapScore.rbegin(); it != mapScore.rend() && !bFull; it++)
            {
                printf("The Score : %u \n", it->first);
                vector<uint32_t> *pVec = &it->second;
                for (vector<uint32_t>::iterator vit = pVec->begin(); vit != pVec->end(); vit++)
                {
                    vecIDList.push_back(*vit);
                    if (vecIDList.size() == uiQueryBackNum){
                        bFull = true;
                        break;
                    }
                }
            }
            //check the list
            if (vecIDList.size() != uiQueryBackNum)
            {
                printf("\n\n\n uiQueryBackNum Not Equal VecIDList.size() \n\n\n");
                aruiShort[uiK]++;
                break;
            }
            //work out distance
            for (uint32_t uiIdx = 0; uiIdx < vecIDList.size(); uiIdx++)
            {
                uiAll++;
                double dDistance = g_oE2lsh.ComputeL2(g_arUserBow[uiQueryId], g_arUserBow[vecIDList[uiIdx]], DEF_KMEANS_K);
                vecQDistance.push_back(dDistance);
                if (dDistance <= LSH_R){
                    uiRight++;
                }
            }
            sort(vecQDistance.begin(), vecQDistance.end(), less<double>());
            //Work Out distance between all user
            for (uint32_t uiIdx = 0; uiIdx < ALL_USER_NUM; uiIdx++)
            {
                double dDistance = g_oE2lsh.ComputeL2(g_arUserBow[uiQueryId], g_arUserBow[uiIdx], DEF_KMEANS_K);
                vecAllDistance.push_back(dDistance);
            }
            sort(vecAllDistance.begin(), vecAllDistance.end(), less<double>());
            //Work Out TOP K 's X
        
            double dX = 0;
            uint32_t uiZeroNum = 0;
            for (uint32_t uiIdx = 0; uiIdx < AR_TOP_K[uiK]; uiIdx++)
            {
                if (0 == vecAllDistance[uiIdx] && 0 == vecQDistance[uiIdx]){
                    dX += 1;
                }
                else
                {
                    if (0 == vecAllDistance[uiIdx] || 0 == vecQDistance[uiIdx])
                    {
                        uiZeroNum++;
                    }
                    else
                    {
                        dX += vecAllDistance[uiIdx] / vecQDistance[uiIdx];
                    }
                }
            }
            dX = dX / (AR_TOP_K[uiK] - uiZeroNum);
            //printf("uiCur = %u, AVG<%u> = %f \n", uiCur, AR_TOP_K[uiK], dX);
            dAllX[uiK] += dX;
        }
    }
    for (uint32_t uiK = 0; uiK < TOP_K; uiK++)
    {
        dAllX[uiK] = dAllX[uiK] / (TEST_PLAINTEXT_OPERATOR - aruiShort[uiK]);
        printf(" All K = %u AVG<dAllX> = %f \n", AR_TOP_K[uiK], dAllX[uiK]);
    }
    printf("Right Percent: %u / %u %f %% \n", uiRight, uiAll, (double)uiRight * 100 / (double)uiAll);

}


void TestTrueAccurary()
{
    printf("This Function need lots of memory and do not care time!\n");
    map<uint32_t, vector<uint32_t> > armapTrueLsh[LSH_L];
    //this circle input all the lsh into map. 
    for (uint32_t uiCur = 0; uiCur < ALL_USER_NUM; uiCur++)
    {
        for (uint32_t uiIdx = 0; uiIdx < LSH_L; uiIdx++)
        {
            uint32_t uiLsh = g_arUserLSH[uiCur][uiIdx];
            armapTrueLsh[uiIdx][uiLsh].push_back(uiCur);
        }
    }
    //init random query id
    map<uint32_t, uint32_t> mapQuery;
    vector<uint32_t> vecQuery;
    srand(g_uiQueryRandomSeed);
    while (mapQuery.size() != TEST_PLAINTEXT_OPERATOR)
    {
        uint32_t uiTmp;
        do
        {
            uiTmp = rand() % ALL_USER_NUM;
        }while(0 == uiTmp);
        mapQuery[uiTmp] = uiTmp;
    }
    for (map<uint32_t, uint32_t>::iterator it = mapQuery.begin(); it != mapQuery.end(); it++)
    {
        vecQuery.push_back(it->first);
    }
    //begin to test
    printf("Begin to Test True LSH Accurary\n");
    
    uint32_t uiTrueRight = 0;
    uint32_t uiRight = 0;
    uint32_t uiAll = 0;
    vector<double> dAllX;
    dAllX.assign(TOP_K, 0);
    uint32_t aruiShort[TOP_K];
    memset(aruiShort, 0, sizeof(aruiShort));
    for (uint32_t uiCur = 0; uiCur < TEST_PLAINTEXT_OPERATOR; uiCur++)
    {
        printf("Doing %u ...\n", uiCur);
        vector<double> vecQDistance;
        vector<double> vecAllDistance;
        //use for store the id
        vector<uint32_t> vecIDList;
        //use for make the id list unique.
        map<uint32_t, uint32_t> mapIDList;
        //query the id.
        uint32_t uiQueryId = vecQuery[uiCur];
        mapIDList.clear();
        for (uint32_t uiIdx = 0; uiIdx < LSH_L; uiIdx++)
        {
            uint32_t uiQueryIdLsh = g_arUserLSH[uiQueryId][uiIdx];
            vector<uint32_t> *pVec = &armapTrueLsh[uiIdx][uiQueryIdLsh];
            printf(" %u Add %u . ", uiQueryIdLsh, pVec->size());
            for (vector<uint32_t>::iterator it = pVec->begin(); it != pVec->end(); it++)
            {
                mapIDList[*it]++;
            }
        }
        vecIDList.clear();
        for (map<uint32_t, uint32_t>::iterator it = mapIDList.begin(); it != mapIDList.end(); it++)
        {
            vecIDList.push_back(it->first);
        }
        printf("Got Back Num vecIDList.size() = %u\n", vecIDList.size());
        //make the map to a distinct vector
        
        //work out distance
        for (uint32_t uiIdx = 0; uiIdx < vecIDList.size(); uiIdx++)
        {
            uiAll++;
            double dDistance = g_oE2lsh.ComputeL2(g_arUserBow[uiQueryId], g_arUserBow[vecIDList[uiIdx]], DEF_KMEANS_K);
            vecQDistance.push_back(dDistance);
            if (dDistance <= LSH_R){
                uiRight++;
            }
        }
        sort(vecQDistance.begin(), vecQDistance.end(), less<double>());
        //Work Out distance between all user
        for (uint32_t uiIdx = 0; uiIdx < ALL_USER_NUM; uiIdx++)
        {
            double dDistance = g_oE2lsh.ComputeL2(g_arUserBow[uiQueryId], g_arUserBow[uiIdx], DEF_KMEANS_K);
            vecAllDistance.push_back(dDistance);
            if (dDistance <= LSH_R){
                uiTrueRight++;
            }
        }
        printf("uiAll : %u  uiRight : %u  uiTrueRight : %u ... \n", uiAll, uiRight, uiTrueRight);
        sort(vecAllDistance.begin(), vecAllDistance.end(), less<double>());
        //Work Out TOP K 's X
        for (uint32_t uiK = 0; uiK < TOP_K; uiK++)
        {
            if (vecIDList.size() < AR_TOP_K[uiK])
            {
                printf("\n\n\n uiQueryBackNum < AR_TOP_K[uiK] \n\n\n");
                aruiShort[uiK]++;
                break;
            }
            double dX = 0;
            uint32_t uiZeroNum = 0;
            for (uint32_t uiIdx = 0; uiIdx < AR_TOP_K[uiK]; uiIdx++)
            {
                if (0 == vecAllDistance[uiIdx] && 0 == vecQDistance[uiIdx]){
                    dX += 1;
                }
                else
                {
                    if (0 == vecAllDistance[uiIdx] || 0 == vecQDistance[uiIdx])
                    {
                        uiZeroNum++;
                    }
                    else
                    {
                        dX += vecAllDistance[uiIdx] / vecQDistance[uiIdx];
                    }
                }
            }
            dX = dX / (AR_TOP_K[uiK] - uiZeroNum);
            //printf("uiCur = %u, AVG<%u> = %f \n", uiCur, AR_TOP_K[uiK], dX);
            dAllX[uiK] += dX;
        }
    }
    for (uint32_t uiK = 0; uiK < TOP_K; uiK++)
    {
        dAllX[uiK] = dAllX[uiK] / (TEST_PLAINTEXT_OPERATOR - aruiShort[uiK]);
        printf(" All K = %u AVG<dAllX> = %f \n", AR_TOP_K[uiK], dAllX[uiK]);
    }
    printf("Right Percent: %u / %u %f %% \n", uiRight, uiAll, (double)uiRight * 100 / (double)uiAll);
    printf("True Right Percent: %u / %u %f %% \n", uiRight, uiAll, (double)uiTrueRight * 100 / (double)uiAll);

}


void ComputeImgUserBOWAndSaveToFile()
{
    //do work out BOW
    //free the global mapset
    for (uint32_t uiCur = 0; uiCur < ALL_USER_NUM; uiCur++)
    {
        if (NULL != g_arUserBow[uiCur])
        {
            delete[] g_arUserBow[uiCur];
            g_arUserBow[uiCur] = NULL;
        }
    }
    vector<string> arUserPath[ALL_USER_NUM];
    for (uint32_t uiCur = 0; uiCur < g_vecImgPath.size(); uiCur++){
        //Random smaller than 50 pictures for each user

        arUserPath[uiCur].push_back(g_vecImgPath[uiCur]);

    }
    DiffTime();
    //Not only extract BOW Description but also work out the requency.
    oImgCtl.ExtractBOWDescriptor(arUserPath, g_arUserBow, g_vecImgPath.size());
    uint32_t uiTimeDiff = DiffTime();
    printf("Finish Work Out the User 's BOW Requency. Use %u us.\n", uiTimeDiff);
    //Write it into file
    ofstream fout;
    fout.open("/data/ppsn/UserBOWRequency_search.dat");
    char szBuf[1000];
    for (uint32_t uiCur = 0; uiCur < g_vecImgPath.size(); uiCur++)
    {
        for (uint32_t uiIdx = 0; uiIdx < DEF_KMEANS_K; uiIdx++)
        {
            snprintf(szBuf, sizeof(szBuf), "%.6lf ", g_arUserBow[uiCur][uiIdx]);
            fout << szBuf;
        }
        fout << endl;
        fout << flush;
    }
    fout.close();
}


void ComputeAllUserBOWAndSaveToFile()
{
    cout<<"Begin to Random "<<ALL_USER_NUM<<" Users AND compute there Matrix."<<endl;
    //free the global mapset
    for (uint32_t uiCur = 0; uiCur < ALL_USER_NUM; uiCur++)
    {
        if (NULL != g_arUserBow[uiCur])
        {
            delete[] g_arUserBow[uiCur];
            g_arUserBow[uiCur] = NULL;
        }
    }
    vector<string> arUserPath[ALL_USER_NUM];
    for (uint32_t uiCur = 0; uiCur < ALL_USER_NUM; uiCur++){
        //Random smaller than 50 pictures for each user
        uint32_t uiRandomNum = 10 + rand() % 50;
        for (uint32_t uiImg = 0; uiImg < uiRandomNum; uiImg++)
        {
            arUserPath[uiCur].push_back(g_vecImgPath[rand() % g_vecImgPath.size()]);
        }
    }
    DiffTime();
    //Not only extract BOW Description but also work out the requency.
    oImgCtl.ExtractBOWDescriptor(arUserPath, g_arUserBow, ALL_USER_NUM);
    uint32_t uiTimeDiff = DiffTime();
    printf("Finish Work Out the User 's BOW Requency. Use %u us.\n", uiTimeDiff);
    //Write it into file
    ofstream fout;
    fout.open("/data/ppsn/UserBOWRequency.dat");
    char szBuf[1000];
    for (uint32_t uiCur = 0; uiCur < ALL_USER_NUM; uiCur++)
    {
        for (uint32_t uiIdx = 0; uiIdx < DEF_KMEANS_K; uiIdx++)
        {
            snprintf(szBuf, sizeof(szBuf), "%.6lf ", g_arUserBow[uiCur][uiIdx]);
            fout << szBuf;
        }
        fout << endl;
        fout << flush;
    }
    fout.close();
}

void MakeUsersBowAndSendLshToSP(){
    cout<<"Begin to Random 100 Users AND compute there Matrix."<<endl;
    map<uint32_t, double*> mapUserBow;
    map<uint32_t, vector<string> > mapUserPath;
    DiffTime();
    for (uint32_t uiCur = 0; uiCur < 10; uiCur++){
        //Random smaller than 50 pictures for each user
        uint32_t uiRandomNum = rand() % 50;
        for (uint32_t uiImg = 0; uiImg < uiRandomNum; uiImg++)
        {
            mapUserPath[uiCur].push_back(g_vecImgPath[rand() % g_vecImgPath.size()]);
        }
    }
    //Not only extract BOW Description but also work out the requency.
    //oImgCtl.ExtractBOWDescriptor(mapUserPath, mapUserBow);
    //printf("Finish Work Out the User 's BOW Requency. Use %u us.\n", DiffTime());
    //printf("Begin to LSH(l) The User 's BOW");
    //lsh_fun lf(DEF_KMEANS_K, 5, 20);
    //lf.init_rnd_vector();
    //lf.build_lsh_funs();
    ////test the first user
    //for (uint32_t x = 0;x < 10; x++){
    //    vector<float> vectest;
    //    for (uint32_t i = 0; i < def_kmeans_k; i++)
    //    {
    //        vectest.push_back(mapuserbow[x][i] * 1000);
    //    }
    //    vector<vector<uint32_t>> veca = lf.compute_fea_lsh(vectest);
    //    vector<uint32_t> vecret;
    //    for (uint32_t i = 0; i < veca.size(); i++)
    //    {
    //        vecret.push_back(lf.compute_ulsh(veca[i]));
    //        cout<<"1 user value:"<<vecret[i]<<endl;
    //    }
    //}
    return;
}


void LoadFlickerPicturesAndTrainVocabularyOfKmeans()
{
    g_vecImgPath.clear();
    CollectFile("/data/ppsn/fliker_train", &g_vecImgPath, ".jpg");
    printf("Find Fliker_train Files : %u .\n", g_vecImgPath.size());
    printf("Begin To Train...\n");
    DiffLongTime();
    int iRet = oImgCtl.Train(g_vecImgPath, DEF_KMEANS_K);
    printf("Finish The Train. Takes %u us.\n", DiffLongTime());
    oImgCtl.Train(g_vecImgPath, DEF_KMEANS_K);
    oImgCtl.SaveVocabulary("/data/ppsn/kmeans_flicker.vob", DEF_KMEANS_K);
}

int main(int argc, char** argv)
{
    //Init the SURF
    initModule_nonfree();
    //Init AES key
    AES_set_encrypt_key((const unsigned char*)AES_UID_KEY.c_str(), AES_UID_KEY.length() * 8, &key_enc_uid);
    AES_set_encrypt_key((const unsigned char*)AES_LSH_KEY.c_str(), AES_LSH_KEY.length() * 8, &key_enc_lsh);
    AES_set_decrypt_key((const unsigned char*)AES_UID_KEY.c_str(), AES_UID_KEY.length() * 8, &key_dec_uid);
    AES_set_decrypt_key((const unsigned char*)AES_LSH_KEY.c_str(), AES_LSH_KEY.length() * 8, &key_dec_lsh);

    //Init Point Array
    for (uint32_t uiCur = 0; uiCur < ALL_USER_NUM; uiCur++)
    {
        g_arUserBow[uiCur] = NULL;
        g_arUserLSH[uiCur] = NULL;
    }
    //Begin Console
    int iCmd;
    do{
        PrintCmdList();
        iCmd = getchar();
        switch(iCmd)
        {
        case 49:    //1 Init the train vocabulary
            TrainFromPath();
            break;
        case 50:    //2 Save the Vocabulary
            oImgCtl.SaveVocabulary("/data/ppsn/kmeans.vob", DEF_KMEANS_K);
            break;
        case 51:    //3 Load the Vocabulary
            oImgCtl.LoadVocabulary("/data/ppsn/kmeans.vob", DEF_KMEANS_K);
            break;
        case 52:    //4 make 100 users and send BOW HASH to sp
            MakeUsersBowAndSendLshToSP();
            break;
        case 53:    //5 Train the SSH functions params
            TrainSshFunctionParams();
            break;
        case 54:    //6 Compute All the User 's BOW Frequency And Save to file
            ComputeAllUserBOWAndSaveToFile();
            break;
        case 55:    //7 Load User's BOW Frequency
            LoadAllUserBOWFromFile();
            break;
        case 56:    //8 Computing E2Lsh Value
            ComputeE2LshValueAndSaveToFile();
            break;
        case 57:    //9 Build a NestHash Table
            BuildNestHashTable();
            break;
        case 65:    //A Load E2Lsh from file
            LoadAllUserLSHFromFile();
            break;
        case 66:    //B Test N times operator and work out precision
            TestPlaintextOperator();
            break;
        case 67:    //C Build a Encryption NestHash table
            BuildEncryptionHashTable();
            break;
        case 68:    //D Test N times encryption operator and work out precision
            TestCiphertextOperator();
            break;
        case 69:    //E Build a Common Nest Hash and Encrypt it!
            BuildNestHashTableAndEncryptIt();
            break;
        case 70:    //F Try test Map LSH Accurary And Score Method 's Accurary!
            //TestTrueAccurary();
            TestScoreMethodAccurary();
            break;
        case 71:    //G Load the Flicker Features And train the vocabulary of KMeans ( K = 500 )
            LoadFlickerPicturesAndTrainVocabularyOfKmeans();
            break;
        case 72:    //H Work out BOW
            ComputeImgUserBOWAndSaveToFile();
            break;
        default:
            iPrintCmdListFlag = 0;
            break;
        };
    }while(1);
    //test the work
    
    
    
    //Finish The Train



    return 0;

}





