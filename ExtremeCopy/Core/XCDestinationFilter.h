/****************************************************************************

Copyright (c) 2008-2020 Kevin Wu (Wu Feng)

github: https://github.com/kevinwu1024/ExtremeCopy
site: http://www.easersoft.com

Licensed under the Apache License, Version 2.0 License (the "License"); you may not use this file except
in compliance with the License. You may obtain a copy of the License at

https://opensource.org/licenses/Apache-2.0

****************************************************************************/

#pragma once
#include "xccoredefine.h"
#include "XCPin.h"
#include <deque>
#include <map>

class CXCDestinationFilter : public CXCFilter
{
public:
	CXCDestinationFilter(CXCCopyingEvent* pEvent,bool bIsRenameCopy);
	virtual ~CXCDestinationFilter(void);

	virtual EFilterType GetType() const
	{
		return FilterType_Destination ;
	}

	static void ResetCanCallbackMark() ;

protected:
	virtual bool OnInitialize() ;
	virtual bool OnContinue() ;
	virtual bool OnPause();
	virtual void OnStop();

	inline const bool IsRenameExe() const {return m_bIsRenameCopy;}

	inline bool CanCallbackFileInfo() const ;

	class CXCDirectoryURI
	{
	public:
		CXCDirectoryURI() ;
		void Clean() ;
		void Push(CptString strFolderName) ;
		void Pop() ;
		CptString GetURI() ;

	private:
		CptString				m_strLastURI ;
		pt_STL_deque(CptString)	m_URIDeque ;
		bool					m_bNewestURI ;
	};

	class CCreateDestFileCache
	{
	public: 
		bool CanAdd(CptString strPath,const SDataPack_SourceFileInfo& sfi) ;
		void Push(const SDataPack_SourceFileInfo& sfi) ;
		SDataPack_SourceFileInfo* Pop() ;
		bool IsEmpty() const ;

	private:
		CXCDirectoryURI	m_URI ;
	};

private:
	int				m_nDstFilterID ; // ��IDֵ��Ϊ��,���ж��dstination filterʱ,ֻ�е�һ��filter�������ϻص�����
	static	int		m_sDstFilterIDCounter ;

	bool			m_bIsRenameCopy ; // �Ƿ��� �������������ƣ�������CRT�� rename()
};

class CXCLocalFileDestnationFilter :
	public CXCDestinationFilter
{
public:
	CXCLocalFileDestnationFilter(CXCCopyingEvent* pEvent,const CptString strDestRoot,const SStorageInfoOfFile& siof,const bool bIsRenameCopy);
	virtual ~CXCLocalFileDestnationFilter(void);

	virtual bool Connect(CXCFilter* pFilter,bool bUpstream) ;

protected:
	struct SDstFileInfo
	{
		HANDLE				hFile ;
		bool				bNoBuf ;
		unsigned __int64	uRemainSize ; // �����һ�����ļ�ʵ�ʵĴ�С������ʹ�û���ʱ����sector��С���϶����С��
										// ��ʹ�û���ʱ��Ϊ�ļ���ʵ�ʴ�С
		
		//unsigned __int64 uActFileSize ; // �ļ���ʵ�ʴ�С,�Թ���ɵĵ���֮��
		CptString strFileName ; // ȫ·���ļ���
		SDataPack_SourceFileInfo*	pSfi ;

		SDstFileInfo()
		{
			hFile = INVALID_HANDLE_VALUE ;
			bNoBuf = false ;
			uRemainSize = 0 ;
			strFileName = _T("") ;			
			pSfi = NULL ;
		}
	};

	virtual int OnDataTrans(CXCFilterEventCB* pSender,EFilterCmd cmd,void* pFileData) ;
	virtual int OnCreateXCFile(SDataPack_CreateFileInfo& cfi) ;
	int CreateXCFile(SDataPack_SourceFileInfo& sfi,CptString& strDstFile)  ;
	virtual int WriteFileData(SDataPack_FileData& fd)  = 0 ;

	virtual void OnLinkEnded(pt_STL_list(SDataPack_SourceFileInfo*)& FileList) {}

	// ��һ��ROUND���ļ�������ɺ�,�������ROUND��ɨβ����
	void RoundOffFile(pt_STL_list(SDataPack_SourceFileInfo*)& FileList ) ;

	SDstFileInfo* AllocDFI() ;
	void FreeDFI(SDstFileInfo* dfi) ;

protected:

	virtual bool OnInitialize() ;
	virtual bool OnContinue() ;
	virtual bool OnPause();
	virtual void OnStop();

	ErrorHandlingResult GetErrorHandleResult(SDataPack_SourceFileInfo& sfi,CptString strDestFile) ;

private:
	inline CptString MakeFileFullName(CptString strRawFileName) ;

	HANDLE m_hCurFileHandle ;

protected:
	
	CXCFilterEventCB*						m_pUpstreamFilter ;
	CXCDirectoryURI							m_DestURI ;

	pt_STL_list(SDstFileInfo)				m_FolderInfoList ; // �ļ��е�ɾ�����Դ����ķ�˳����У�����ջ��˳��
	pt_STL_list(SDstFileInfo)				m_FileInfoList ; // д����ļ���Ϣ����������˳�򴴽���д��
	pt_STL_list(SDstFileInfo)::iterator		m_CurFileIterator ; // ��ǰ���ڲ������ļ�ָ�룬
																// ��ָ���Ǹ��ֽ�㣬ָ��ǰ�� m_FileInfoList Ԫ��Ϊ��д�뵽���̵��ļ��ȴ� round off ��
																// ָ���� m_FileInfoList Ԫ��Ϊ�����ˣ�����ûд�뵽���̵��ļ�

	
	CptString		m_strDestRoot ;
	bool			m_bIsDriverRoot ; // m_strDestRoot �Ƿ�Ϊ��Ŀ¼	
	int				m_nSwapChunkSize ;
	SStorageInfoOfFile m_StorageInfo ;
	DWORD							m_nCreateFileFlag ;

	CXCFileDataBuffer*			m_pFileDataBuf ;
	EImpactFileBehaviorResult*	m_pImpactFileBehavior ; // ��ͻ�ļ��Ĵ���ʽ
};