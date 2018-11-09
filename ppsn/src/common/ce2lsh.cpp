/* 
 * File  : ce2lsh.cpp
 * Author: rainewang kevinyuan
 * Info  : Use Euclidean Distance and use <u> function
 * Date  : 14-10-2013
 */

#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../inc/ce2lsh.h"

//#define USE_COSINE_LSH

CE2lsh::CE2lsh()
{
    m_pLSH = NULL;
    m_ppuiV = NULL;
    m_aruiRandom = NULL;
    m_dW = m_dR = m_uiD = m_uiL = m_uiK = 0;
}

CE2lsh::~CE2lsh()
{
    freeLSH();
}

void CE2lsh::freeLSH()
{
    //free m_pLSH
    if (NULL != m_pLSH)
    {
        for (uint32_t uiCurL = 0; uiCurL < m_uiL; uiCurL++)
        {
            free(m_pLSH[uiCurL]);
        }
        delete[] m_pLSH;
        m_pLSH = NULL;
    }
    //free m_ppuiV
    if (NULL != m_ppuiV)
    {
        for (uint32_t uiCurL = 0; uiCurL < m_uiL; uiCurL++)
        {
            delete[] m_ppuiV[uiCurL];
        }
        delete[] m_ppuiV;
        m_ppuiV = NULL;
    }
    //free m_aruiRandom
    delete[] m_aruiRandom;
    m_aruiRandom = NULL;
}

void CE2lsh::InitLSH(double r,uint32_t d, uint32_t l, uint32_t k, double w)
{
    m_dW = w;
    m_dR = r;
    m_uiD = d;
    m_uiL = l;
    m_uiK = k;
    srand(time(NULL));
    freeLSH();
    //init function params
    m_pLSH = new LSHFUNC[l];
    for (uint32_t uiCurL = 0; uiCurL < l; uiCurL++)
    {
        m_pLSH[uiCurL] = (LSHFUNC)malloc(sizeof(double) * (1 + d) * k);
        for (uint32_t uiCurK = 0; uiCurK < k; uiCurK++)
        {
            //init Gaussian Random Matrix a
            for (uint32_t uiCurD = 0; uiCurD < d; uiCurD++)
            {
                m_pLSH[uiCurL][uiCurK].a[uiCurD] = genGaussianRandom();
            }
            //init Uniform Random b [0 - w]
            m_pLSH[uiCurL][uiCurK].b = genUniformRandom(0, w);
        }
    }
    //init mid value storage
    m_ppuiV = new uint32_t*[l];
    for (uint32_t uiCurL = 0; uiCurL < l; uiCurL++)
    {
        m_ppuiV[uiCurL] = new uint32_t[k];
    } 
    //init uniform hash random array
    // MAX_HASH_RND = 2^29
    m_aruiRandom = new uint32_t[k];
    for (uint32_t uiCur = 0; uiCur < k; uiCur++)
    {
        m_aruiRandom[uiCur] = genUint32Random(1, MAX_HASH_RND);
    }

    //2 * K Random List Init

    m_arKRandom = new uint32_t[2 * k];
    for (uint32_t uiCur = 0; uiCur < 2 * k; uiCur++)
    {
        m_arKRandom[uiCur] = genUint32Random(0, (uint32_t)-1);
    }
}


void CE2lsh::ComputeLSH(double *ardMatrix, uint32_t *aruiRet)
{
    //for more effective, do not check memory. use carefully.
    //compute every LSH function
    for (uint32_t uiCurL = 0; uiCurL < m_uiL; uiCurL++)
    {
#ifndef USE_COSINE_LSH

        uint64_t ulTmp = 0;
        for (uint32_t uiCurK = 0; uiCurK < m_uiK; uiCurK++)
        {
            double dTmp = 0;
            for (uint32_t uiCurD = 0; uiCurD < m_uiD; uiCurD++)
            {
                dTmp += (ardMatrix[uiCurD] / m_dR) * m_pLSH[uiCurL][uiCurK].a[uiCurD];
            }
            m_ppuiV[uiCurL][uiCurK] = (uint32_t)(floor((dTmp + m_pLSH[uiCurL][uiCurK].b) / m_dW));
            //cross a uniform hash and combine k to 1
            ulTmp += (uint64_t)m_aruiRandom[uiCurK] * m_ppuiV[uiCurL][uiCurK];
            ulTmp = (ulTmp & TWO_TO_32_MINUS_1) + 5 * (ulTmp >> 32);
            ulTmp = ulTmp >= UH_PRIME_DEFAULT ? ulTmp - UH_PRIME_DEFAULT : ulTmp;
        }
        aruiRet[uiCurL] = ulTmp;

#else

        uint64_t ulTmp = 0;
        for (uint32_t uiCurK = 0; uiCurK < m_uiK; uiCurK++)
        {
            double dTmp = 0;
            for (uint32_t uiCurD = 0; uiCurD < m_uiD; uiCurD++)
            {
                dTmp += ardMatrix[uiCurD] * m_pLSH[uiCurL][uiCurK].a[uiCurD];
            }
            if (dTmp >= 0)
            {
                ulTmp += m_arKRandom[uiCurK * 2 + 1];
            }
            else
            {
                ulTmp += m_arKRandom[uiCurK * 2];
            }
        }
        aruiRet[uiCurL] = ulTmp;

#endif

    }
}

double CE2lsh::ComputeL2(double *ardX, double *ardY, uint32_t uiD)
{
    double dRet = 0;
    for(uint32_t uiCur = 0; uiCur < uiD; uiCur++)
    {
        dRet += (ardY[uiCur] - ardX[uiCur]) * (ardY[uiCur] - ardX[uiCur]);
    }
    return sqrt(dRet);
}

double CE2lsh::ComputeCos(double *ardX, double *ardY, uint32_t uiD)
{
    double dRet = 0;
    double dA = 0;
    double dB = 0;
    for(uint32_t uiCur = 0; uiCur < uiD; uiCur++)
    {
        dRet += ardY[uiCur] * ardX[uiCur];
        dA += ardY[uiCur] * ardY[uiCur];
        dB += ardX[uiCur] * ardX[uiCur];
    }
    dRet = dRet / (sqrt(dA) * sqrt(dB));
    if (dRet > 1)
    {
        dRet = 1;
    }
    dRet = acos(dRet);
    return floor(dRet * 10000000) / 100000000;
}

uint32_t CE2lsh::genUint32Random(uint32_t uiRangeStart, uint32_t uiRangeEnd)
{

#ifdef CPP_ELEVEN

    static random_device rd;
    static mt19937 mt(rd());
    uniform_int_distribution<uint32_t> gr(uiRangeStart, uiRangeEnd);
    return gr(mt);

#else

    return RAND_MAX >= uiRangeEnd - uiRangeStart ? \
        uiRangeStart + (uint32_t)((uiRangeEnd - uiRangeStart + 1.0) * random() / (RAND_MAX + 1.0)) : \
        uiRangeStart + (uint32_t)((uiRangeEnd - uiRangeStart + 1.0) * ((uint64_t)random() * ((uint64_t)RAND_MAX + 1) + (uint64_t)random()) / ((uint64_t)RAND_MAX * ((uint64_t)RAND_MAX + 1) + (uint64_t)RAND_MAX + 1.0));

#endif

}

double CE2lsh::genGaussianRandom()
{
    
#ifdef CPP_ELEVEN

    static random_device rd;
    static mt19937 mt(rd());
    static normal_distribution<float> gr(0.0, 1.0);
    return gr(mt);

#else


    double x1, x2;
    do
    {
        x1 = genUniformRandom(0.0, 1.0);
    }
    while(0 == x1);
    x2 = genUniformRandom(0.0, 1.0);
    return sqrt(-2.0 * log(x1)) * cos(2.0 * M_PI * x2);

#endif

}

double CE2lsh::genUniformRandom(double dRangeStart, double dRangeEnd)
{

#ifdef CPP_ELEVEN

    static random_device rd;
    static mt19937 mt(rd());
    uniform_real_distribution<double> gr(dRangeStart, dRangeEnd);
    return gr(mt);

#else

    return dRangeStart + (dRangeEnd - dRangeStart) * (double)random() / (double)RAND_MAX;

#endif

}