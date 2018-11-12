#ifndef 	OI_SHM_H_
#define  	OI_SHM_H_ 
#include <sys/shm.h>
 
#ifdef  __cplusplus
extern "C" {
#endif

#ifdef linux
union semun {
   int val;                    /* value for SETVAL */
   struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
   unsigned short int *array;  /* array for GETALL, SETALL */
   struct seminfo *__buf;      /* buffer for IPC_INFO */
};
#endif

int RWLock_Init(key_t key, int iLockNums,int);
int RWLock_GetValue(key_t key);
int RWLock_UnLock(void);
int RWLock_RDLock(void);
int RWLock_WDLock(void);

typedef struct
{
	int iSemID;
	char sErrMsg[255];
}SEMLOCKINFO;

int Lib_Semp_Init(SEMLOCKINFO *pstLockInfo,int iShmKey);
int Lib_Semp_Lock(SEMLOCKINFO *);
int Lib_Semp_Unlock(SEMLOCKINFO *);

char* GetShm(int iKey, size_t iSize, int iFlag);
//bzero
int GetShm2(void **pstShm, int iShmID, size_t iSize, int iFlag);
//no bzero 1 不需要init
int GetShm3(void **pstShm, int iShmID, size_t iSize, int iFlag);
/**
 *	支持开启大于2G的共享内存
 *  注意: 必须保证 /proc/sys/kernel/shmmax 也大于2G
 *  added by robintang 2013-06-17
 */
char* GetShmEx(int iKey, size_t Size, int iFlag);

#ifdef  __cplusplus
}
#endif

#endif






