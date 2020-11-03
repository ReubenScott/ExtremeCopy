/****************************************************************************

Copyright (c) 2008-2020 Kevin Wu (Wu Feng)

github: https://github.com/kevinwu1024/ExtremeCopy
site: http://www.easersoft.com

Licensed under the Apache License, Version 2.0 License (the "License"); you may not use this file except
in compliance with the License. You may obtain a copy of the License at

https://opensource.org/licenses/Apache-2.0

****************************************************************************/

#pragma once

#include "XCCoreDefine.h"

#include "XCPin.h"
#include "..\Common\ptTypeDef.h"
#include <list>
#include <deque>
#include "XCFileDataBuffer.h"
#include "..\Common\ptThreadLock.h"
#include <stack>

class CXCSourceFilter :
	public CXCFilter
{
public:
	CXCSourceFilter(CXCCopyingEvent* pEvent);
	virtual ~CXCSourceFilter(void);

	virtual EFilterType GetType() const
	{
		return FilterType_Source ;
	}

	void SetTask(const SXCSourceFilterTaskInfo& sfti) ;
	bool SetSmallReadGranularity(bool b) ;

	bool Run() ;
	bool Pause() ;
	void Stop() ;
	bool Continue() ;
	bool Skip() ;

	// ע�⣺ֻ�н��� pause ״̬ʱ���ܵ��øú���
	virtual SDataPack_SourceFileInfo* GetCurrentSFI() =0 ;

	virtual bool Connect(CXCFilter* pFilter,bool bUpstream) ;
	
	ECopyFileState GetState() const {return (*m_pRunningState==CFS_ReadyStop)?CFS_Stop:(*m_pRunningState);}

protected:
	virtual bool OnRun() = 0 ;
	virtual bool OnInitialize() ;
	virtual bool OnContinue() ;
	virtual bool OnPause();
	virtual void OnStop();
	virtual bool OnSkip() = 0 ;

	virtual int OnDataTrans(CXCFilterEventCB* pSender,EFilterCmd cmd,void* pFileData) ;
	//virtual int OnPin_Data(CXCPin* pOwnerPin,EFilterCmd cmd,void* pFileData) ;

	struct SFileIDNamePairsInfo
	{
		unsigned		nSectorSize ;
		SDataPack_SourceFileInfo*	pFpi ;


		SFileIDNamePairsInfo():pFpi(NULL),nSectorSize(512)
		{
		}
	};

	void AddToDelayReleaseFileList(const SFileIDNamePairsInfo& fnpi) ;
	bool RemoveFromDelayReleaseFileListByID(const pt_STL_list(unsigned)& FileIDList) ;
	bool GetFileFromDelayReleaseFileListByID(int nFileID,SFileIDNamePairsInfo& fnpi) ;

	bool IsIni() const {return m_bIni;}
	
	void InsideStop() ;

private:
	

protected:
	pt_STL_vector(CptString)	m_SrcFileVer ;
	EXCExecuteType				m_TaskType ;
	unsigned int				m_uFileIDCounter ;
	CXCFileDataBuffer*			m_pFileDataBuf ;
	int							m_nValidCachePointNum ; // ���ڴ�鱻���ô���
	
	CptCritiSecLock						m_FilePairListLock ;
	pt_STL_list(SFileIDNamePairsInfo)	m_ConfirmReleaseFilePairInfoList ;

	CXCFilterEventCB*						m_pDownstreamFilter ;

protected:
	bool						m_bSmallReadGran ; // ��ȡʱ����С��������ȡ��Ĭ���Ǵ����ȡ� �� 8M ���� 2M
	HANDLE						m_hPauseWaitEvent ;
	HANDLE						m_hWaitForStop ;
	HANDLE						m_hWaitDelayReleaseListEvent ;

private:
	bool						m_bIni ; // �ж��Ƿ��ʼ�����Ա㷢�� EDC_LinkIni ����
	
	LONG						m_nSafeStopCount ; // ��Ҫ��ȫֹͣ�˳�����ӦҪ���ٸ������Ѱ�ȫ�˳����ܰ�ȫ
	EImpactFileBehaviorResult*	m_pImpactFileBehavior ; // ��ͻ�ļ��Ĵ���ʽ
	
};


class CXCLocalFileSourceFilter :
	public CXCSourceFilter
{
public:
	CXCLocalFileSourceFilter(CXCCopyingEvent* pEvent);
	virtual ~CXCLocalFileSourceFilter(void);

protected:
	struct SFindFileAttr
	{
		DWORD dwFileAttributes;
		FILETIME ftCreationTime;
		FILETIME ftLastAccessTime;
		FILETIME ftLastWriteTime;
		DWORD nFileSizeHigh;
		DWORD nFileSizeLow;

		SFindFileAttr& operator=(const WIN32_FIND_DATA& wfd)
		{
			::memcpy(this,&wfd,sizeof(*this)) ;

			return *this ;
		}

		void CopyTo(WIN32_FIND_DATA& wfd)
		{
			::memcpy(&wfd,this,sizeof(*this)) ;
		}
	};

	struct SFindFolderInfo
	{
		CptString FolderName ;
		SFindFileAttr wfd ;
		unsigned		uFileID ;

		SFindFolderInfo()
		{
			::memset(&wfd,0,sizeof(wfd)) ;
		}
	};

protected:
	virtual bool OnInitialize() ;
	virtual bool OnContinue() ;
	virtual bool OnRun() ;
	virtual bool OnPause();
	virtual void OnStop() ;

	//int CXCLocalFileSourceFilter::OnPin_Data(CXCPin* pOwnerPin,EFilterCmd cmd,void* pFileData) ;
	//virtual int OnDataTrans(CXCFilterEventCB* pSender,EFilterCmd cmd,void* pFileData) ;
	struct SSourceFileInfo
	{
		SDataPack_SourceFileInfo*	pFpi ;
		HANDLE		hFile ; // �����ļ�Ϊ�ļ��У����ֵΪ INVALUD_HANDLE_VALUE
		unsigned			nSectorSize ; // �� pFpi==NULL�� Ҳ�͵���URIʱ����ֵ��ʾ�������ļ���ID
		//bool		bLastFile ; // �Ƿ�Ϊ�ļ��������һ���ļ����У�������Ϊͬ��Ŀ���ļ���ʱ��������

		SSourceFileInfo():pFpi(NULL),hFile(INVALID_HANDLE_VALUE),nSectorSize(512)
		{
		}
	};

	bool CheckPauseAndWait() ;

	bool FlushFile() ;
	//void RemoveDiscardFileFromOpenedFileList(pt_STL_vector(SDataPack_SourceFileInfo*)& SourceFileVer) ;

	inline void ReleaseOpenFiledList() ;
	bool ExecuteFiles() ;
	void FreeFileInfo() ;
	bool WaitForConfirmDone() ; // �� FILTER ������ж��ļ���,���˳�ǰ�ȵȴ� "ȷ�ϻ������" �ͷ����

	char* AllocateFileDataBuf(DWORD& dwBufSize,const int nReadAlignSize,
		const unsigned __int64& uRemainSize,const unsigned& nSectorSize) ;

	int CalculateAlignSize(const unsigned __int64& uRemainSize,const unsigned int& uSectorSize) ;

protected:
	virtual bool FlushFileData() = 0;
	virtual bool OnSkip() ;

private:
	bool AddFolder(const CptString strFileName,unsigned nSectorSize,unsigned uFileID=0) ;

	bool OpenXCFile(const CptString strFileName,unsigned nSpecifyFileID=0) ;

	// uSpecifyFileID ��Ϊ��������ָ���ü�����е��ļ�IDֵ���粻ָ��������Ϊ0�� 
	// ��Ϊ���Σ���ֵ���¼�������ļ�ID��ֵ
	bool AddOpenedFileToList(HANDLE hFile,CptString strFile,const WIN32_FIND_DATA* pWdf,
		unsigned nSectorSize,unsigned& uSpecifyFileID,const int nSysErrCode=0) ;

	void ResendedFileAddToList() ;

	bool FlushCreateFile() ;
	bool AddFile(const CptString strFileName,const WIN32_FIND_DATA& wfd,int nSectorSize,unsigned uSpecifyFileID) ;

	virtual SDataPack_SourceFileInfo* GetCurrentSFI() ;
protected:
	DWORD							m_nCreateFileFlag ;

	pt_STL_list(SSourceFileInfo)	m_OpenedFileInfoList ;
	//pt_STL_list(SFindFolderInfo)	m_DiscardedFolder ;
	HANDLE					m_hCurFileHandle ;
	SDataPack_SourceFileInfo*			m_pCurSourceFileInfo ;
	bool							m_bIsPauseWaited ;
	pt_STL_stack(SFileIDNamePairsInfo)			m_FolderStack ; // ��¼��folder��ջ��
													// ��Ϊsource filter ���·��ļ��� EDC_FileOperationCompleted ʱӦ���������ģ�
													// ���ȴ����ģ���complete,�󴴽��ģ���complete
	

private:
	int						m_nTotalFileCounter ;
	unsigned __int64		m_nTotalFileSize ;
	
	
};

