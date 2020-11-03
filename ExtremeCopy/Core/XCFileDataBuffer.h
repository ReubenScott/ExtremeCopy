/****************************************************************************

Copyright (c) 2008-2020 Kevin Wu (Wu Feng)

github: https://github.com/kevinwu1024/ExtremeCopy
site: http://www.easersoft.com

Licensed under the Apache License, Version 2.0 License (the "License"); you may not use this file except
in compliance with the License. You may obtain a copy of the License at

https://opensource.org/licenses/Apache-2.0

****************************************************************************/

#pragma once

#include "..\Common\ptTypeDef.h"
#include <list>

#include "../Common/ptThreadLock.h"

class CXCFileDataBuffer
{
public:

	enum EWaitForEventType
	{
		WFET_WaitForBufEmpty = 0,
		WFET_WaitForBufFull = 1,
		WFET_WaitForBufNotFull = 2,
		WFET_WaitForBufNotEmpty = 3,
		WFET_END = 4
	};

	CXCFileDataBuffer(const CXCFileDataBuffer&) ;

	CXCFileDataBuffer(void);
	~CXCFileDataBuffer(void);

	CXCFileDataBuffer& operator=(const CXCFileDataBuffer&) ;

	void ResetRef() ;

	bool AllocateChunk(int nChunkSize,int nAlignSize) ;
	bool IsAllocateChunk() const ;

	void* Allocate(BYTE nRefCount,int nSize) ;
	void Free(void* pBuf,int nSize) ;

	int GetRemainSpace() ;
	int GetBottomRemainSpace() ;

	int GetPageSize() const ;
	int GetChunkSize() const ;

	bool IsEmpty()  ;
	bool IsFull()  ;

	void CheckBufAlloc() ;

	void* GetBlockAddress() const {return m_pBlockBuf;}

#ifdef _DEBUG
	void SaveAllocIDRecored() ;
#endif

private:
	void Release() ;

private:
	BYTE*	m_pBlockBuf ;	// ���ڴ�
	int		m_nChunkSize ;	// ���ڴ��Ĵ�С

	BYTE*	m_pRefCountBuf ; // ��Ӧ������ҳ�汻���õļ���������
	int		m_nRefNum ;		// ���ڴ���ڰ����ܵ�ҳ����Ŀ

	int		m_nPageSize ; // һ��ҳ��Ĵ�С

	int		m_nCurAllocIndex ; // ��ǰ���䵽��ҳ������,��ָ�������Ϊ��û�����ȥ��
	
	pt_STL_list(HANDLE)			m_BufEvent[(int)WFET_END] ;

	int		m_nRefPageCount ;		// �ж��ٸ�ҳ�汻��ǰ������

#ifdef _DEBUG
	pt_STL_list(int)		m_AllocSizeList ;
	unsigned*				m_pAllocIDRecordBuf ;
	int						m_nIDCount ;
	unsigned						m_nLastFreeID ;
#endif
	CptCritiSecLock	m_Lock ;
};

