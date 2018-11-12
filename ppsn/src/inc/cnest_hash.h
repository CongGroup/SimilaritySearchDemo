/******************************************************************************
 * 文件名称： cnest_hash.h
 * 类    名： CNestHash
 * 描    述:  指定数据结构
 * 创建日期:  2013-10-9
 * 作    者： rainewang
 * 修改历史：
 ******************************************************************************/

#include <arpa/inet.h>
#include <stdint.h>
#include <string>
#include <map>

using namespace std;

#define INSERT_DELTA_SPREAD

#define MAX_KICKOUT_NUM 10

template <class TData>
class CNestHash
{
public:

    CNestHash()
    {
        m_pZeroMem = NULL;
        m_pTable = NULL;
        m_arRandomSpread = NULL;
        m_uiKickoutSeed = 0;
    }

    ~CNestHash()
    {
        if (NULL != m_pZeroMem)
        {
            delete[] m_pZeroMem;
        }
    }

    //Init the Hash Table
    void InitNestHash(char *pMemory, uint32_t uiL, uint32_t uiDelta, uint32_t uiWidth, uint32_t uiEleSize)
    {
        m_pTable = pMemory;
        m_uiL = uiL;
        m_uiDelta = uiDelta;
        m_uiWidth = uiWidth;
        m_uiEleSize = uiEleSize;
        memset(m_pTable, 0, uiL * uiWidth * uiEleSize);
        //Make a Zero Memory for memcmp
        if (NULL != m_pZeroMem){
            delete[] m_pZeroMem;
        }
        m_pZeroMem = new char[uiEleSize];
        memset(m_pZeroMem, 0, uiEleSize);
        //Make a random spread array
        if (NULL != m_arRandomSpread)
        {
            delete[] m_arRandomSpread;
        }
        m_uiRandomSpreadSize = uiL * uiWidth * uiDelta;
        m_arRandomSpread = new uint32_t[m_uiRandomSpreadSize];



        srand(1);
        for (uint32_t uiCur = 0; uiCur < uiL * uiWidth; uiCur++)
        {
            vector<uint32_t> vecTmpR;
            map<uint32_t, uint32_t> mapTmpR;

            while(mapTmpR.size() != uiDelta)
            {
                uint32_t uiTmpRand;
                do
                {
                    uiTmpRand = rand() % m_uiWidth;
                }
                while (uiTmpRand == uiCur % m_uiWidth);
                mapTmpR[uiTmpRand] = uiTmpRand;
            }
            map<uint32_t, uint32_t>::iterator it = mapTmpR.begin();

            for (uint32_t uiIdx = 0; uiIdx < uiDelta; uiIdx++, it++)
            {
                m_arRandomSpread[uiCur * uiDelta + uiIdx] = it->second;
            }
            
        }

        return;
    }

    void CheckNestLoad()
    {
        uint32_t uiAllFull = 0;
        for (uint32_t uiL = 0; uiL < m_uiL; uiL++)
        {
            uint32_t uiFull = 0;
            for (uint32_t uiW = 0; uiW < m_uiWidth;uiW++)
            {
                if (0 != memcmp(m_pTable + (uiL * m_uiWidth + uiW) * m_uiEleSize, m_pZeroMem, m_uiEleSize)){
                    uiFull++;
                }
            }
            printf("The No. %u Floor : Element% = %.2f%%\n", uiL, (double)uiFull / m_uiWidth * 100);
            uiAllFull += uiFull;
        }
        printf("All The NEST Element% = %.2f%%\n", (double)uiAllFull / (m_uiL * m_uiWidth) * 100);
    }

    void InitNestHash(char *pMemory, uint32_t uiL, uint32_t uiDelta, uint32_t uiWidth)
    {
        InitNestHash(pMemory, uiL, uiDelta, uiWidth, sizeof(TData));
    }

    TData *Get(uint32_t uiL, uint32_t uiW)
    {
        return (TData*)(m_pTable + m_uiEleSize * (uiL * m_uiWidth + uiW % m_uiWidth));
    }

    //Update All l * (d + 1)
    void SetGroup(uint32_t *parKey, TData *pData)
    {
        char *p = (char*)pData;
        uint32_t uiIdx, uiPos;
        uint32_t *parRand;
        for (uint32_t uiCur = 0; uiCur < m_uiL; uiCur++)
        {
            //uiIdx is the first of the cur floor
            uiIdx = uiCur * m_uiWidth;
            uiPos = parKey[uiCur] % m_uiWidth;
            parRand = m_arRandomSpread + (uiIdx + uiPos) * m_uiDelta;
            //memcpy the first Node
            memcpy(m_pTable + m_uiEleSize * (uiIdx + uiPos), p + m_uiEleSize * uiCur *(m_uiDelta + 1), m_uiEleSize);
            //memcpy the delta Node
            for (uint32_t uiD = 0; uiD < m_uiDelta; uiD++)
            {
                memcpy(m_pTable + m_uiEleSize * (uiIdx + *parRand++), p + m_uiEleSize * (uiCur * (m_uiDelta + 1) + uiD + 1), m_uiEleSize);
            }
        }
    }

    //Update All L
    void SetCore(uint32_t *parKey, TData *pData)
    {
        char *p = (char*)pData;
        for (uint32_t uiCur = 0; uiCur < m_uiL; uiCur++)
        {
            memcpy(m_pTable +  m_uiEleSize * (uiCur * m_uiWidth + parKey[uiCur] % m_uiWidth), p + m_uiEleSize * uiCur * (m_uiDelta + 1), m_uiEleSize);
        }
    }

    TData *NewPlace(int &iKickL, uint32_t *parKey, uint32_t uiBeg = 0)
    {
        //Normal Insert
        //check the hit L.
        /*if (iKickL == -1)
        {
            srand(time(NULL));
            uiBeg = rand();
        }*/
        uint32_t uiCur, uiIdx, uiPos, uiDeltaIdx;
        uint32_t *puiR, *parRand;
        for (uint32_t uiN = 0; uiN < m_uiL; uiN++)
        {
            uiCur = (uiBeg + uiN) % m_uiL;
            uiIdx = m_uiEleSize * (uiCur * m_uiWidth + parKey[uiCur] % m_uiWidth);
            if (0 == memcmp(m_pTable + uiIdx, m_pZeroMem, m_uiEleSize)){
                iKickL = -1;
                return (TData*)(m_pTable + uiIdx);
            }
        }
        //check the hit Delta
        //put it into right
#ifndef INSERT_DELTA_SPREAD
        for (uint32_t uiN = 0; uiN < m_uiL; uiN++)
        {
            uiCur = (uiBeg + uiN) % m_uiL;
            uiIdx = m_uiEleSize * (uiCur * m_uiWidth + parKey[uiCur] % m_uiWidth);
            for (uint32_t uiDelta = 1; uiDelta <= m_uiDelta; uiDelta++)
            {
                if (0 == memcmp(m_pTable + uiIdx + uiDelta * m_uiEleSize, m_pZeroMem, m_uiEleSize)){
                    iKickL = -1;
                    return (TData*)(m_pTable + uiIdx + uiDelta * m_uiEleSize);
                }
            }
        }
#else
        for (uint32_t uiN = 0; uiN < m_uiL; uiN++)
        {
            uiCur = (uiBeg + uiN) % m_uiL;
            uiIdx = uiCur * m_uiWidth;
            uiPos = parKey[uiCur] % m_uiWidth;
            parRand = m_arRandomSpread + (uiIdx + uiPos) * m_uiDelta;

            //begin to insert
            for (uint32_t uiDelta = 0; uiDelta < m_uiDelta; uiDelta++)
            {
                uiDeltaIdx = m_uiEleSize * (uiIdx + *parRand++);
                if (0 == memcmp(m_pTable + uiDeltaIdx, m_pZeroMem, m_uiEleSize)){
                    iKickL = -1;
                    return (TData*)(m_pTable + uiDeltaIdx);
                }
            }
        }

#endif
        //Need Kick out
        //srand(time(NULL));
        //这里可以考虑优化一下，用已经生成的随机数
        /*uint32_t uiRand;
        do
        {
            uiRand = m_arRandomSpread[m_uiKickoutSeed++ % m_uiRandomSpreadSize] % m_uiL;
        }while(iKickL == uiRand);
        iKickL = uiRand;*/

        srand(m_uiKickoutSeed++);
        //uint32_t uiKickL = rand() % NEST_L;
        iKickL = rand() % m_uiL;
        //printf("Need Kick Out :L:%u", iKickL);
        return (TData*)(m_pTable +  m_uiEleSize * (iKickL * m_uiWidth + parKey[iKickL] % m_uiWidth));
    }

    //输入为一个长度为L的结果KeyMatrix.逐层查找,找出 L * ( Delta + 1 ) * uiEleSize 的数据,并依次取值拷贝到pDest指针中,请确保pDest可用且分配了足够大的空间.
    //用于查询
    int QueryToMemory(uint32_t *parKey, char *pDest)
    {
        uint32_t uiIdx, uiPos, uiDeltaIdx;
        uint32_t *parRand;

#ifndef INSERT_DELTA_SPREAD
        for (uint32_t uiCur = 0; uiCur < m_uiL; uiCur++)
        {
            uiIdx = m_uiEleSize * (uiCur * m_uiWidth + parKey[uiCur] % m_uiWidth);
            memcpy(pDest, m_pTable + uiIdx, m_uiEleSize * (m_uiDelta + 1));
            pDest += m_uiEleSize * (m_uiDelta + 1);
        }
#else
        for (uint32_t uiCur = 0; uiCur < m_uiL; uiCur++)
        {
            uiIdx = uiCur * m_uiWidth;
            uiPos = parKey[uiCur] % m_uiWidth;
            parRand = m_arRandomSpread + (uiIdx + uiPos) * m_uiDelta;
            //memcpy the first Node.
            memcpy(pDest + m_uiEleSize * (uiCur * (m_uiDelta + 1)), m_pTable + m_uiEleSize * (uiIdx + uiPos), m_uiEleSize);
            //deal with the Delta Node.
            for (uint32_t uiD = 0; uiD < m_uiDelta; uiD++)
            {
                uiDeltaIdx = m_uiEleSize * (uiIdx + *parRand++);
                memcpy(pDest + m_uiEleSize * (uiCur * (m_uiDelta + 1) + 1 + uiD), m_pTable + uiDeltaIdx, m_uiEleSize);
            }
            
        }
#endif
        return 0;
    }

    //for delete node use memcpy to check if it is the same one.
    int DeletePlace(uint32_t *parKey, char *pVal)
    {
        uint32_t uiIdx, uiPos, uiDeltaIdx, uiDelPos;
        uint32_t *parRand;

#ifndef INSERT_DELTA_SPREAD
        for (uint32_t uiCur = 0; uiCur < m_uiL; uiCur++)
        {
            uiIdx = m_uiEleSize * (uiCur * m_uiWidth + parKey[uiCur] % m_uiWidth);
            for (uint32_t uiDelta = 0; uiDelta <= m_uiDelta; uiDelta++)
            {
                if (0 == memcmp(m_pTable + uiIdx + uiDelta * m_uiEleSize, pVal, m_uiEleSize)){
                    memset(m_pTable + uiIdx + uiDelta * m_uiEleSize, 0, m_uiEleSize);
                    return 0;
                }
            }
        }
#else
        for (uint32_t uiCur = 0; uiCur < m_uiL; uiCur++)
        {
            uiIdx = uiCur * m_uiWidth;
            uiPos = parKey[uiCur] % m_uiWidth;
            parRand = m_arRandomSpread + (uiIdx + uiPos) * m_uiDelta;
            uiDelPos = m_uiEleSize * (uiIdx + uiPos);
            //deal with the first node
            if (0 == memcmp(m_pTable + uiDelPos, pVal, m_uiEleSize))
            {
                memset(m_pTable + uiDelPos, 0, m_uiEleSize);
                
                return 0;
            }
            //deal with the delta node
            for (uint32_t uiDelta = 0; uiDelta < m_uiDelta; uiDelta++)
            {
                uiDeltaIdx = m_uiEleSize * (uiIdx + *parRand++);
                if (0 == memcmp(m_pTable + uiDeltaIdx, pVal, m_uiEleSize)){
                    memset(m_pTable + uiDeltaIdx, 0, m_uiEleSize);
                    return 0;
                }
            }
        }
#endif
        return -1;
    }


private:

    uint32_t *m_arDeltaRandom;

    //Global Seed
    uint32_t m_uiKickoutSeed;

    //Hash Floor
    uint32_t m_uiL;

    //Delta for Left and Right
    uint32_t m_uiDelta;

    //The Count of Bucket
    uint32_t m_uiWidth;

    //One Element Size
    uint32_t m_uiEleSize;

    //for cache the random spread
    uint32_t *m_arRandomSpread;
    uint32_t m_uiRandomSpreadSize;

    //for a memory place zero
    char *m_pZeroMem;

    //point to the table
    char *m_pTable;

    //true mean use heap allocation, need to be release.
    //false mean use other memory, need not care
    bool bMalloc;

};
