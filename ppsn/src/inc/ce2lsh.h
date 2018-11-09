/* 
 * File  : ce2lsh.h
 * Author: rainewang
 * Info  : Use Euclidean Distance and use <u> function
 * Date  : 14-10-2013
 */

#ifndef __CE2LSH_H__
#define __CE2LSH_H__

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include <random>
#include <algorithm>
#include <vector>
#include <iostream>


#define CPP_ELEVEN

using namespace std;

// LSH function coefficients 

//====================
//if you use the E2lsh train script. you need to known that: 
//    our l = train's M
//    our k = train's K / 2
//====================

//2^29
const uint32_t MAX_HASH_RND = 536870912;
//2^32 - 1
const uint32_t TWO_TO_32_MINUS_1 = 4294967295;
// 2^32 - 5
const uint32_t UH_PRIME_DEFAULT = 4294967291;

#define DDDDD 1000

class CE2lsh 
{
    
public:

    CE2lsh();
    ~CE2lsh();
    
    void InitLSH(double r,uint32_t d, uint32_t l, uint32_t k, double w);

    //non-thread safe
    //this method use a share temp rather than alloc a new memory
    void ComputeLSH(double *ardMatrix, uint32_t *aruiRet);
    
    //compute the L2 distance of X,Y
    double ComputeL2(double *ardX, double *ardY, uint32_t uiD);

    double ComputeCos(double *ardX, double *ardY, uint32_t uiD);

    double genGaussianRandom();

    uint32_t genUint32Random(uint32_t uiRangeStart, uint32_t uiRangeEnd);

    double genUniformRandom(double dRangeStart, double dRangeEnd);

    //Inner define the LshFunction
    typedef struct _stLshFunction
    {
        double b;
        double a[DDDDD];
    }stLshFunction, *LSHFUNC;

    LSHFUNC *m_pLSH;
private:
    


//LSH Params
double m_dW;
double m_dR;
uint32_t m_uiL;
uint32_t m_uiK;
uint32_t m_uiD;

uint32_t *m_arKRandom;

uint32_t **m_ppuiV;
uint32_t *m_aruiRandom;

void freeLSH();

};


#endif
