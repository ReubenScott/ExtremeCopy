/****************************************************************************

Copyright (c) 2008-2020 Kevin Wu (Wu Feng)

github: https://github.com/kevinwu1024/ExtremeCopy
site: http://www.easersoft.com

Licensed under the Apache License, Version 2.0 License (the "License"); you may not use this file except
in compliance with the License. You may obtain a copy of the License at

https://opensource.org/licenses/Apache-2.0

****************************************************************************/

#include "StdAfx.h"
#include "XCSourceFilter.h"
#include "XCCopyingEvent.h"
#include "XCWinStorageRelative.h"
#include "..\Common\ptGlobal.h"
#include "../App/XCGlobal.h"

#ifdef COMPILE_TEST_PERFORMANCE
extern DWORD	g_dwStartTick ;
#endif

CXCSourceFilter::CXCSourceFilter(CXCCopyingEvent* pEvent):
CXCFilter(pEvent),
	m_TaskType(XCTT_Copy),
	m_uFileIDCounter(0),
	m_pFileDataBuf(NULL),
	m_bIni(false),
	m_bSmallReadGran(false),
	m_pDownstreamFilter(NULL),
	m_hPauseWaitEvent(NULL),
	m_hWaitForStop(NULL),
	m_nSafeStopCount(2),
	m_hWaitDelayReleaseListEvent(NULL)
{
}

CXCSourceFilter::~CXCSourceFilter(void)
{
	if(NULL!=m_hWaitDelayReleaseListEvent)
	{
		::SetEvent(m_hWaitDelayReleaseListEvent) ;
		::CloseHandle(m_hWaitDelayReleaseListEvent) ;
		m_hWaitDelayReleaseListEvent = NULL ;
	}

	auto it = m_ConfirmReleaseFilePairInfoList.begin() ;

	while(it!=m_ConfirmReleaseFilePairInfoList.end())
	{
		if((*it).pFpi!=NULL)
		{
			SDataPack_SourceFileInfo::Free((*it).pFpi) ; 
		}
		++it ;
	}

	m_ConfirmReleaseFilePairInfoList.clear() ;
}


bool CXCSourceFilter::Connect(CXCFilter* pFilter,bool bUpstream)
{
	bool bRet = false ;

	if(!bUpstream && pFilter!=NULL && m_pDownstreamFilter==NULL)
	{
		m_pDownstreamFilter = pFilter ;
		pFilter->Connect(this,true) ;
		bRet = true ;
	}

	return bRet ;
}

void CXCSourceFilter::SetTask(const SXCSourceFilterTaskInfo& sfti) 
{
	m_SrcFileVer.assign(sfti.SrcFileVer.begin(),sfti.SrcFileVer.end()) ;
	m_TaskType = sfti.ExeType ;
	m_pFileDataBuf = sfti.pFileDataBuf ;
	m_pFileChangingBuffer = sfti.pFileChangingBuf ;
	m_nValidCachePointNum = sfti.nValidCachePointNum ;
	m_pImpactFileBehavior = sfti.pDefaultImpactFileBehavior ;
	m_pRunningState = sfti.pRunningState ;

	m_bIni = false ;
}

bool CXCSourceFilter::Run() 
{
	bool bRet = false ;

	_ASSERT(CFS_Stop!=NULL) ;

	if(*m_pRunningState==CFS_Stop)
	{// ��Ϊ�ڳ�ʼ��ʱ,�������ε�filter���� EDC_LinkIni ����,����ʱ *m_pRunningState Ϊ CFS_Stop �Ļ�,
	// ���ܻᵼ�����γ�ʼ�����漴�˳�,���������Ȱ� *m_pRunningState ״̬�޸�Ϊ CFS_Running
	// Ȼ���������η��� EDC_LinkIni ����
		*m_pRunningState = CFS_Running ;

		if(this->OnInitialize())
		{
			bRet = this->OnRun() ;

			::InterlockedDecrement(&m_nSafeStopCount) ; // ��ȫ�˳����ü���������1

			if(m_hWaitForStop!=NULL)
			{
				::SetEvent(m_hWaitForStop) ;
			}
		}
		else
		{
			this->InsideStop() ;
		}

	}
	
	return bRet ;
}

bool CXCSourceFilter::Pause()
{
	if(*m_pRunningState==CFS_Running)
	{
		if(m_hPauseWaitEvent==NULL)
		{
			m_hPauseWaitEvent = ::CreateEvent(NULL,FALSE,FALSE,NULL) ;
		}

		*m_pRunningState = CFS_Pause ;
		return this->OnPause() ;
	}

	return false ;
}

bool CXCSourceFilter::Skip() 
{
	_ASSERT(CFS_Pause==*m_pRunningState) ;

	if(*m_pRunningState==CFS_Pause)
	{
		return this->OnSkip() ;
	}

	return false ;
}

void CXCSourceFilter::Stop() 
{
	switch(*m_pRunningState)
	{
	case CFS_Pause:
		{
			this->Continue() ;
		}

	case CFS_ReadyStop:
	case CFS_Running:
		{
			PT_BREAK_IF(m_hWaitForStop!=NULL) ;

			// ������״̬����Ϊ CFS_ReadyStop�� �Ա�ʹ������ filter �����˳�
			// ���� filter �����˳���
			// �� filter ����ֹͣ�ǰ�ȫ���˳�����
			m_hWaitForStop = ::CreateEvent(NULL,FALSE,FALSE,NULL) ;
			*m_pRunningState = CFS_ReadyStop ;
			while(m_nSafeStopCount>0 && ::WaitForSingleObject(m_hWaitForStop,5*1000) ==WAIT_OBJECT_0)
			{
				NULL ;
			}
			::CloseHandle(m_hWaitForStop) ;
			m_hWaitForStop = NULL ;
			*m_pRunningState = CFS_Stop ;
		}
		break ;

	case CFS_Stop:
		break ;
	}
}

void CXCSourceFilter::InsideStop() 
{
	if(*m_pRunningState!=CFS_Stop)
	{ 
		*m_pRunningState = CFS_Stop ;

		this->OnStop() ;

		::InterlockedDecrement(&m_nSafeStopCount) ; // ��ȫ�˳����ü���������2

		if(m_hWaitForStop!=NULL)
		{
			::SetEvent(m_hWaitForStop) ;
		}
	}
}

bool CXCSourceFilter::Continue() 
{
	bool bRet = false ;

	if(*m_pRunningState==CFS_Pause)
	{
		bRet = this->OnContinue() ;

		if(bRet)
		{
			*m_pRunningState = CFS_Running; 

			_ASSERT(m_hPauseWaitEvent!=NULL) ;

			::SetEvent(m_hPauseWaitEvent) ;
		}
	}
	
	return bRet;
}

// ������ CXCSourceFilter::Continue() �����
bool CXCSourceFilter::OnContinue() 
{
	_ASSERT(m_pDownstreamFilter!=NULL) ;
	return m_pDownstreamFilter->OnDataTrans(this,EDC_Continue,NULL)==0 ;
}


// ������ CXCSourceFilter::Pause() �����
bool CXCSourceFilter::OnPause() 
{
	_ASSERT(m_pDownstreamFilter!=NULL) ;
	return m_pDownstreamFilter->OnDataTrans(this,EDC_Pause,NULL)==0 ;
}


// ������ CXCSourceFilter::Stop() �����
void CXCSourceFilter::OnStop()
{
	_ASSERT(m_pDownstreamFilter!=NULL) ;
	m_pDownstreamFilter->OnDataTrans(this,EDC_Stop,NULL) ;
}

bool CXCSourceFilter::OnInitialize() 
{
	_ASSERT(!m_bIni) ;
	_ASSERT(m_pDownstreamFilter!=NULL) ;

	m_bIni = true ;

	// ���� link ��ʼ������
	SDataPack_LinkIni LinkIni ;

	LinkIni.pFileDataBuf = this->m_pFileDataBuf ;
	LinkIni.pCFState = m_pRunningState ;
	LinkIni.pFileChangingBuf = this->m_pFileChangingBuffer ;
	LinkIni.pDefaultImpactFileBehavior = m_pImpactFileBehavior ;

	return m_pDownstreamFilter->OnDataTrans(this,EDC_LinkIni,&LinkIni)==0 ;
}

bool CXCSourceFilter::OnRun() 
{
	*m_pRunningState = CFS_Running ;

	return true;
}

void CXCSourceFilter::AddToDelayReleaseFileList(const SFileIDNamePairsInfo& fnpi) 
{
	CptAutoLock lock(&m_FilePairListLock) ;

	m_ConfirmReleaseFilePairInfoList.push_back(fnpi) ;
}

bool CXCSourceFilter::RemoveFromDelayReleaseFileListByID(const pt_STL_list(unsigned)& FileIDList) 
{
	CptAutoLock lock(&m_FilePairListLock) ;

#ifdef _DEBUG
	size_t nCount = FileIDList.size() ;
#endif
	pt_STL_list(SFileIDNamePairsInfo)::iterator it ;
	pt_STL_list(unsigned)::const_iterator FileIt = FileIDList.begin() ;

	bool bMove = true ;

	while(FileIt!=FileIDList.end())
	{
		it = m_ConfirmReleaseFilePairInfoList.begin() ;

		while(it!=m_ConfirmReleaseFilePairInfoList.end())
		{
			if((*it).pFpi->uFileID==*FileIt)
			{
				if(this->m_TaskType==XCTT_Move)
				{// ��Ϊ�ƶ��ļ�����ɾ���ļ�
					bMove = true ;

					if((*it).pFpi!=NULL)
					{
						if((*it).pFpi->dwSourceAttr&FILE_ATTRIBUTE_READONLY && this->m_pEvent!=NULL)
						{// ��Ϊֻ���ļ�
							SImpactFileInfo ImpactInfo ;
							ImpactInfo.ImpactType = IT_ReadOnly ;
							ImpactInfo.strSrcFile = (*it).pFpi->strSourceFile ;

							SImpactFileResult result ;
							
							m_pEvent->XCOperation_ImpactFile(ImpactInfo,result) ;

							if(result.result==SFDB_Skip)
							{
								bMove = false ;
							}
						}

						if(bMove)
						{
							if(!(*it).pFpi->IsDiscard())
							{// �������������ļ���ɾ��
								if(CptGlobal::IsFolder((*it).pFpi->dwSourceAttr))
								{
									// �����ǹ���ʹ�� RemoveDirectory() ��
									// ��Ϊ����ļ��в�Ϊ�գ��������Ǽ��в�������˵�����ļ��������ȵ��±������ļ�����
									// ����ֻ���ļ�������ļ�ȫ���ɹ������к󣬲�ȫ���ֿ��ļ��У���ʱɾ���ļ��в�����ȷ��
									// �����л Daniel ����� bug
									::RemoveDirectory((*it).pFpi->strSourceFile.c_str()) ;
								}
								else
								{
									CptGlobal::ForceDeleteFile((*it).pFpi->strSourceFile) ;
								}
							}
						}
					}
				}

				if((*it).pFpi!=NULL)
				{
					SDataPack_SourceFileInfo::Free((*it).pFpi) ; 
					//delete (*it).pFpi ;
				}

				m_ConfirmReleaseFilePairInfoList.erase(it) ;
#ifdef _DEBUG
				--nCount ;
#endif
				break ;
			}
			++it ;
		}
		++FileIt ;
	}

	_ASSERT(nCount==0) ;

	if(NULL!=m_hWaitDelayReleaseListEvent && m_ConfirmReleaseFilePairInfoList.empty())
	{
		::SetEvent(m_hWaitDelayReleaseListEvent) ;
	}

	return true ;
}

bool CXCSourceFilter::GetFileFromDelayReleaseFileListByID(int nFileID,SFileIDNamePairsInfo& fnpi) 
{
	CptAutoLock lock(&m_FilePairListLock) ;

	pt_STL_list(SFileIDNamePairsInfo)::iterator it = m_ConfirmReleaseFilePairInfoList.begin() ;

	while(it!=m_ConfirmReleaseFilePairInfoList.end() )
	{
		if((*it).pFpi->uFileID==nFileID)
		{
			fnpi = (*it) ;
			return true ;
		}

		++it ;
	}

	return false ;
}

int CXCSourceFilter::OnDataTrans(CXCFilterEventCB* pSender,EFilterCmd cmd,void* pFileData) 
{
	int nRet = 0 ;

	switch(cmd)
	{

	case EDC_FileDoneConfirm:
		{
			_ASSERT(pFileData!=NULL) ;

			SDataPack_FileDoneConfirm* pFdc = (SDataPack_FileDoneConfirm*)pFileData ;

			this->RemoveFromDelayReleaseFileListByID(pFdc->FileDoneConfirmList) ;
		}
		break ;

	}

	return nRet ;
}

bool CXCSourceFilter::SetSmallReadGranularity(bool b)
{
	bool bRet = m_bSmallReadGran ;
	m_bSmallReadGran = b ;
	return bRet ;
}


//==================

CXCLocalFileSourceFilter::CXCLocalFileSourceFilter(CXCCopyingEvent* pEvent=NULL)
	:CXCSourceFilter(pEvent),m_nCreateFileFlag(0),m_hCurFileHandle(INVALID_HANDLE_VALUE),m_pCurSourceFileInfo(NULL),m_bIsPauseWaited(false)
{
	m_nTotalFileCounter = 0 ;
	m_nTotalFileSize = 0 ;
}


CXCLocalFileSourceFilter::~CXCLocalFileSourceFilter(void)
{
	this->ReleaseOpenFiledList() ;

}

bool CXCLocalFileSourceFilter::OnSkip()
{
	if(m_pCurSourceFileInfo!=NULL)
	{
		m_pCurSourceFileInfo->SetDiscard() ;
		return true ;
	}

	return false ;
}

SDataPack_SourceFileInfo* CXCLocalFileSourceFilter::GetCurrentSFI() 
{
	int nCount = 0 ;

	SDataPack_SourceFileInfo* pRet = NULL ;

	const int nMaxCount = 30 ;

	while(!m_bIsPauseWaited && nCount<nMaxCount)
	{
		::Sleep(100) ;
		++nCount ;
	}
	
	if(nCount<nMaxCount)
	{
		if(m_pCurSourceFileInfo==NULL && !m_OpenedFileInfoList.empty())
		{
			pt_STL_list(SSourceFileInfo)::iterator it = m_OpenedFileInfoList.begin() ;

			for(;it!=m_OpenedFileInfoList.end();++it)
			{
				if((*it).pFpi!=NULL)
				{
					break ;
				}
			}

			m_pCurSourceFileInfo = (*it).pFpi;
		}
	}

	return m_pCurSourceFileInfo ;
}


void CXCLocalFileSourceFilter::ReleaseOpenFiledList() 
{
	if(!m_OpenedFileInfoList.empty())
	{
		pt_STL_list(SSourceFileInfo)::iterator& it = m_OpenedFileInfoList.begin() ;

		while(it!=m_OpenedFileInfoList.end())
		{
			if((*it).hFile!=NULL && (*it).hFile!=INVALID_HANDLE_VALUE)
			{
				::CloseHandle((*it).hFile) ;
			}

			if((*it).pFpi!=NULL)
			{
				SDataPack_SourceFileInfo::Free((*it).pFpi) ; 
			}

			++it ;
		}

		m_OpenedFileInfoList.clear() ;
	}
}

bool CXCLocalFileSourceFilter::AddFile(const CptString strFileName,const WIN32_FIND_DATA& wfd,int nSectorSize,unsigned uSpecifyFileID) 
{
	bool bRet = false ;
	bool bAdd = true ;
	int nSystemErrorCode = 0 ;

	_ASSERTE( _CrtCheckMemory( ) );
	
EXCEPTION_RETRY_OPENFILE: // ���ԣ����ļ���

#ifdef COMPILE_TEST_PERFORMANCE
	DWORD dw = CptPerformanceCalcator::GetInstance()->BeginCal() ;
#endif

	HANDLE hFile = ::CreateFile(strFileName.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,m_nCreateFileFlag,NULL) ;
	//HANDLE hFile = ::CreateFile(strFileName.c_str(),GENERIC_READ,0,NULL,OPEN_EXISTING,m_nCreateFileFlag,NULL) ;

#ifdef COMPILE_TEST_PERFORMANCE
	CptPerformanceCalcator::GetInstance()->EndCalAndSave(dw,4) ;
#endif

	_ASSERTE( _CrtCheckMemory( ) );

	if(hFile==INVALID_HANDLE_VALUE)
	{// ���ļ�ʧ��
		if(this->m_pEvent!=NULL)
		{
			SXCExceptionInfo ei ;

			nSystemErrorCode = ::GetLastError() ;
			ei.ErrorCode.nSystemError = nSystemErrorCode ;
			ei.strSrcFile = strFileName ;
			ei.SupportType = ErrorHandlingFlag_RetryIgnoreCancel ;

			ErrorHandlingResult result = m_pEvent->XCOperation_CopyExcetption(ei) ;

			switch(result)
			{
			case ErrorHandlingFlag_Ignore: // ����
				{
					hFile = NULL ;
					//bAdd = false ; // �����

					if(m_pEvent!=NULL)
					{
						SDataPack_SourceFileInfo sfi ;

						sfi.dwSourceAttr = wfd.dwFileAttributes ;
						//sfi.bDiscard = true ;
						sfi.SetDiscard() ;
						sfi.nFileSize = CptGlobal::DoubleWordTo64(wfd.nFileSizeLow,wfd.nFileSizeHigh) ;

						sfi.SrcFileTime.CreateTime = wfd.ftCreationTime ;
						sfi.SrcFileTime.LastAccessTime = wfd.ftLastAccessTime ;
						sfi.SrcFileTime.LastWriteTime = wfd.ftLastWriteTime ;

						m_pEvent->XCOperation_FileDiscard(&sfi,sfi.nFileSize) ;
					}
					/**/
				}
				
				break ;

			case ErrorHandlingFlag_Retry: // ���ԣ����ļ���
				goto EXCEPTION_RETRY_OPENFILE ;
				break ;

			case ErrorHandlingFlag_Exit: // �˳�
			default:
				*m_pRunningState = CFS_ReadyStop ;
				return false ;
			}
		}
		else
		{
			bAdd = false ;
		}
	}

	if(bAdd)
	{
		return this->AddOpenedFileToList(hFile,strFileName,&wfd,nSectorSize,uSpecifyFileID,nSystemErrorCode) ;
	}

	return true ;
}

bool CXCLocalFileSourceFilter::FlushCreateFile() 
{
	pt_STL_list(SSourceFileInfo)::iterator& it = m_OpenedFileInfoList.begin() ;

	bool bRet = true ;
	bool bCreateDestinationFile = false ;

	SDataPack_CreateFileInfo cfi ;

	while(it!=m_OpenedFileInfoList.end())
	{// ���������������ݰ�
		if((*it).pFpi!=NULL) 
		{
			//_ASSERT(!(*it).pFpi->bDiscard) ;
			cfi.SourceFileInfoList.push_back((*it).pFpi) ;
		}
		else
		{// Ҫ���� destination filter URI ջ
			cfi.SourceFileInfoList.push_back(NULL) ;
		}

		++it ;
	}

	if(!cfi.SourceFileInfoList.empty())
	{
#ifdef COMPILE_TEST_PERFORMANCE
		DWORD dw2 = CptPerformanceCalcator::GetInstance()->BeginCal() ;
#endif

		// ���� EDC_BatchCreateFile ����
		m_pDownstreamFilter->OnDataTrans(this,EDC_BatchCreateFile,&cfi) ;
#ifdef COMPILE_TEST_PERFORMANCE
		CptPerformanceCalcator::GetInstance()->EndCalAndSave(dw2,14) ;
#endif
		
		//if(cfi.bExistDiscard)
		//{// ��Ҫ�������ļ��� m_OpenedFileInfoList �Ƴ�
		//	CRemoveDiscardFile rdf(m_pEvent,&m_DiscardedFolder,&cfi.SourceFileInfoList) ;

		//	m_OpenedFileInfoList.remove_if(rdf) ;
		//}
	}

	if(!this->IsValideRunningState())
	{
		this->InsideStop() ;
		return false ;
	}

	return true ;
}

bool CXCLocalFileSourceFilter::ExecuteFiles()
{
	bool bRet = false ;

#ifdef COMPILE_TEST_PERFORMANCE
	CptPerformanceCalcator::GetInstance()->EndCalAndSave(g_dwStartTick,0) ;
#endif

	Debug_Printf(_T("CXCLocalFileSourceFilter::ExecuteFiles() begin")) ;

	for(size_t i=0;i<m_SrcFileVer.size() && this->IsValideRunningState();++i)
	{
		if(!this->OpenXCFile(m_SrcFileVer[i]))
		{
			Debug_Printf(_T("CXCLocalFileSourceFilter::ExecuteFiles() false")) ;
			//this->InsideStop() ;
			return false ;
		}
	}

	Debug_Printf(_T("CXCLocalFileSourceFilter::ExecuteFiles() 2")) ;

	// ����ʣ�µ��ļ�����ǿ��д��destination filter
	bRet = this->FlushFile() ;

	Debug_Printf(_T("CXCLocalFileSourceFilter::ExecuteFiles() 3 result=%s"),bRet?_T("true"):_T("false")) ;

	if(bRet)
	{// ����ʣ�µ��ļ�����ǿ��д��destination filter

		Debug_Printf(_T("CXCLocalFileSourceFilter::ExecuteFiles() 4")) ;

		m_pDownstreamFilter->OnDataTrans(this,EDC_FlushData,NULL) ;

		if(!this->IsValideRunningState())
		{
			this->InsideStop() ;
			return false ;
		}

		SDataPack_FileOperationCompleted foc ;

		{
			CptAutoLock lock(&m_FilePairListLock) ;

			Debug_Printf(_T("CXCLocalFileSourceFilter::ExecuteFiles() 4a")) ;

			if(!m_ConfirmReleaseFilePairInfoList.empty())
			{
				pt_STL_list(SFileIDNamePairsInfo)::const_iterator it = m_ConfirmReleaseFilePairInfoList.begin() ;

				while(it!=m_ConfirmReleaseFilePairInfoList.end())
				{
					foc.CompletedFileInfoList.push_back((*it).pFpi) ;
					++it ;
				}

				Debug_Printf(_T("CXCLocalFileSourceFilter::ExecuteFiles() 4b")) ;

				m_hWaitDelayReleaseListEvent = ::CreateEvent(NULL,FALSE,FALSE,NULL) ;
			}
		}

		// ���·��� EDC_LinkEnded ����,�Ա�ʾ link ������
		m_pDownstreamFilter->OnDataTrans(this,EDC_LinkEnded,&foc) ;

		if(NULL!=m_hWaitDelayReleaseListEvent)
		{
			Debug_Printf(_T("CXCLocalFileSourceFilter::ExecuteFiles() 5")) ;

			::WaitForSingleObject(m_hWaitDelayReleaseListEvent,5*1000) ;
			::CloseHandle(m_hWaitDelayReleaseListEvent) ;
			m_hWaitDelayReleaseListEvent = NULL ;
		}

		//Release_Printf(_T("CXCLocalFileSourceFilter::ExecuteFiles() release_file_size= %u"),m_ConfirmReleaseFilePairInfoList.size()) ;

#ifdef _DEBUG
		if(this->IsValideRunningState())
		{
			_ASSERT(m_ConfirmReleaseFilePairInfoList.empty()) ;
			//m_pFileDataBuf->SaveAllocIDRecored() ;
			_ASSERT(this->m_pFileDataBuf->IsEmpty()) ;
		}
#endif
	}

	Debug_Printf(_T("CXCLocalFileSourceFilter::ExecuteFiles() end")) ;

	return bRet ;
}

bool CXCLocalFileSourceFilter::WaitForConfirmDone() 
{
	return false ;
}

//bool CXCLocalFileSourceFilter::FlushFile() 
//{
//	if(!this->IsIni())
//	{// ���� link ��ʼ������
//		this->SendLinkIniCmd() ;
//	}
//
//	this->FreeFileInfo() ;
//	return true ;
//}

bool CXCLocalFileSourceFilter::OpenXCFile(const CptString strFileName,unsigned nSpecifyFileID)
{
	bool bRet = false ;

	HANDLE hFileFind = INVALID_HANDLE_VALUE ;
	WIN32_FIND_DATA wfd ;

EXCEPTION_RETRY_ATTR: // ����(����)

	hFileFind = ::FindFirstFile(strFileName.c_str(),&wfd) ;

	unsigned uFileID = 0 ;

	if(hFileFind!=INVALID_HANDLE_VALUE)
	{
		::FindClose(hFileFind) ;
		SStorageInfoOfFile siof ;

		CXCWinStorageRelative::GetFileStoreInfo(strFileName,siof) ;

		hFileFind = INVALID_HANDLE_VALUE ;

		if(FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes)
		{// Ŀ¼
			uFileID = 0 ;
			if(this->AddOpenedFileToList(INVALID_HANDLE_VALUE,strFileName,&wfd,siof.nSectorSize,uFileID))
			{
				bRet = this->AddFolder(strFileName,siof.nSectorSize,uFileID) ;
			}
			else
			{
				_ASSERT(FALSE) ;
			}
		}
		else
		{// �ļ�
			bRet = this->AddFile(strFileName,wfd,siof.nSectorSize,nSpecifyFileID) ;
		}
	}
	else
	{// �Ҳ������ļ�
		if(this->m_pEvent!=NULL)
		{
			SXCExceptionInfo ei ;
			ei.ErrorCode.nSystemError = ::GetLastError() ;
			ei.SupportType = ErrorHandlingFlag_RetryIgnoreCancel ;
			ei.strSrcFile = strFileName ;
			ErrorHandlingResult result = this->m_pEvent->XCOperation_CopyExcetption(ei) ;

			switch(result)
			{
			case ErrorHandlingFlag_Ignore: // ����
				bRet = true ; // ��Ϊ���� false������ɳ����˳�������Ӧ���� true ;
				break ;

			case ErrorHandlingFlag_Retry: // ���ԣ����ԣ�
				goto EXCEPTION_RETRY_ATTR ;

			default:
			case ErrorHandlingFlag_Exit: // �˳�
				*this->m_pRunningState = CFS_ReadyStop ;
				bRet = false ;
				break ;
			}
		}
	}

	return bRet ;
}

bool CXCLocalFileSourceFilter::FlushFile() 
{
	//Debug_Printf(_T("CXCLocalFileSourceFilter::FlushFile()  1")) ;

	if(m_OpenedFileInfoList.empty())
	{
		return true ;
	}

	//Debug_Printf(_T("CXCLocalFileSourceFilter::FlushFile()  2")) ;

	#ifdef COMPILE_TEST_PERFORMANCE
		DWORD dw2 = CptPerformanceCalcator::GetInstance()->BeginCal() ;
#endif

	if(!this->FlushCreateFile())
	{
		return false ;
	}

	#ifdef COMPILE_TEST_PERFORMANCE
		CptPerformanceCalcator::GetInstance()->EndCalAndSave(dw2,19) ;
#endif

	//Debug_Printf(_T("CXCLocalFileSourceFilter::FlushFile()  3")) ;

	if(this->FlushFileData())
	{
		this->FreeFileInfo() ;
		return true ;
		//return CXCLocalFileSourceFilter::FlushFile() ; // ������һ���ģ��Ա��ͷ� m_OpenedFileInfoList ��Դ
	}
	
	return false ;
}

bool CXCLocalFileSourceFilter::AddFolder(const CptString strFileName,unsigned nSectorSize,unsigned uFileID)
{
	bool bRet = true ;
	bool bDiscardCurrentFolder = false ;
	pt_STL_deque(SFindFolderInfo) FolderQue ;

	{
		WIN32_FIND_DATA wfd1 ;

		CptString strTemFile ;
		CptString strFolderFiles = strFileName + _T("\\*.*") ;

		_ASSERTE( _CrtCheckMemory( ) );

#ifdef COMPILE_TEST_PERFORMANCE
		DWORD dw = CptPerformanceCalcator::GetInstance()->BeginCal() ;
#endif

		HANDLE hFileFind = ::FindFirstFile(strFolderFiles.c_str(),&wfd1) ;

#ifdef COMPILE_TEST_PERFORMANCE
		CptPerformanceCalcator::GetInstance()->EndCalAndSave(dw,12) ;
#endif
		_ASSERTE( _CrtCheckMemory( ) );

		unsigned uFileID1 = 0 ;
		SFindFolderInfo ffi ;
		BOOL bFindNext ;
		//bool bFirstFolder = true ;

		if(hFileFind!=INVALID_HANDLE_VALUE)
		{
			bool bDotFound1 = false ;
			bool bDotFound2 = false ;

			do
			{
				_ASSERTE( _CrtCheckMemory( ) );

				bRet = true ;

				if(!bDotFound1 && ::_tcscmp(wfd1.cFileName,_T("."))==0)
				{
					//_ASSERTE( _CrtCheckMemory( ) );
					bDotFound1 = true ;
					//continue ;
					goto KKKKKKKK ;
				}
				else if(!bDotFound2 && ::_tcscmp(wfd1.cFileName,_T(".."))==0)
				{
					//_ASSERTE( _CrtCheckMemory( ) );
					bDotFound2 = true ;
					//continue ;
					goto KKKKKKKK ;
				}
				else
				{
					strTemFile = strFileName + _T("\\") ;

					strTemFile += wfd1.cFileName ;

					if(wfd1.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{// ��ΪĿ¼
						ffi.FolderName = strTemFile ;
						//ffi.uFileID = bFirstFolder ? -1 : 0 ;
						ffi.wfd = wfd1 ;

						FolderQue.push_front(ffi) ; // ����ջ��

						//_ASSERTE( _CrtCheckMemory( ) );

					}
					else
					{// �ļ�
						if(!this->AddFile(strTemFile,wfd1,nSectorSize,0))
						{
							//_ASSERTE( _CrtCheckMemory( ) );

							bRet = false ;
							break ;
						}

						_ASSERTE( _CrtCheckMemory( ) );
					}
				}
KKKKKKKK:

#ifdef COMPILE_TEST_PERFORMANCE
				dw = CptPerformanceCalcator::GetInstance()->BeginCal() ;
#endif
				bFindNext = ::FindNextFile(hFileFind,&wfd1) ;

#ifdef COMPILE_TEST_PERFORMANCE
				CptPerformanceCalcator::GetInstance()->EndCalAndSave(dw,12) ;
#endif
			}
			while(bFindNext && bRet && this->IsValideRunningState());

#ifdef COMPILE_TEST_PERFORMANCE
			dw = CptPerformanceCalcator::GetInstance()->BeginCal() ;
#endif
			::FindClose(hFileFind) ;
#ifdef COMPILE_TEST_PERFORMANCE
			CptPerformanceCalcator::GetInstance()->EndCalAndSave(dw,12) ;
#endif
		}
		else
		{
		}
	}

	if(bRet && !bDiscardCurrentFolder)
	{// ���ø��ļ����ڵ�������ȫ�����뵽�����������Ե���URIջ��

		SFindFolderInfo ffi ;

		unsigned uFileID2 = 0 ;//����URIջ��
		//bool bIsRunning = true ;

		while(bRet && !FolderQue.empty() && (bRet = this->IsValideRunningState()))
		{
			ffi = FolderQue.front() ; // ��������ջ�Ľṹ��ʹ��
			FolderQue.pop_front() ;

			uFileID2 = 0 ;
			// 
			this->AddOpenedFileToList(INVALID_HANDLE_VALUE,ffi.FolderName,(WIN32_FIND_DATA*)&ffi.wfd,nSectorSize,uFileID2) ;
			bRet = this->AddFolder(ffi.FolderName,nSectorSize,uFileID2) ;
		}

		if(bRet)
		{
			uFileID2 = 0 ;
			this->AddOpenedFileToList(INVALID_HANDLE_VALUE,_T(""),NULL,uFileID,uFileID2) ;
		}
		else
		{// �����˳������ͷ� opened file list
			//this->ReleaseOpenFiledList() ;
			this->InsideStop() ;
		}
	}

	return bRet ;
}


char* CXCLocalFileSourceFilter::AllocateFileDataBuf(DWORD& dwBufSize,const int nReadAlignSize,
	const unsigned __int64& uRemainSize,const unsigned& nSectorSize) 
{
	char* pRet = NULL ;

	if(uRemainSize>nReadAlignSize)
	{
		dwBufSize = nReadAlignSize ;
	}
	else
	{
		dwBufSize = (DWORD)ALIGN_SIZE_UP(uRemainSize,nSectorSize) ;
	}

	pRet = (char*)m_pFileDataBuf->Allocate(m_nValidCachePointNum,dwBufSize) ;

	if(pRet==NULL)
	{
		const int nRemainSpace = m_pFileDataBuf->GetBottomRemainSpace() ;

		if(nRemainSpace>64*1024)
		{
			dwBufSize = (DWORD)ALIGN_SIZE_DOWN(min(nRemainSpace,nReadAlignSize),nSectorSize) ;
			pRet = (char*)m_pFileDataBuf->Allocate(m_nValidCachePointNum,dwBufSize) ;
		}
	}

	return pRet ;
}

int CXCLocalFileSourceFilter::CalculateAlignSize(const unsigned __int64& uRemainSize,const unsigned int& uSectorSize) 
{
	// file data buffer ��ҳ�� �Ƿ�Ϊ sector �ı���
	int nReadAlignSize = (DWORD)min((unsigned __int64)m_pFileDataBuf->GetChunkSize()/4,uRemainSize) ;

	nReadAlignSize = ALIGN_SIZE_UP(nReadAlignSize,uSectorSize) ;

	if(m_bSmallReadGran && nReadAlignSize>1024*1024)
	{// ʹ��С��������ȡ����
		nReadAlignSize /= 4 ;
	}

	nReadAlignSize = ALIGN_SIZE_UP(nReadAlignSize, (DWORD)m_pFileDataBuf->GetPageSize());
	nReadAlignSize = ALIGN_SIZE_UP(nReadAlignSize, uSectorSize);

	return nReadAlignSize ;
}


bool CXCLocalFileSourceFilter::AddOpenedFileToList(HANDLE hFile,CptString strFile,const WIN32_FIND_DATA* pWdf,unsigned nSectorSize,unsigned& nSpecifyFileID,const int nSysErrCode) 
{
				
#ifdef COMPILE_TEST_PERFORMANCE
		DWORD dw5 = CptPerformanceCalcator::GetInstance()->BeginCal() ;
#endif

	SSourceFileInfo sfi ;

	sfi.hFile = hFile ;
	sfi.nSectorSize = nSectorSize ;

	if(hFile==INVALID_HANDLE_VALUE && pWdf==NULL)
	{// ��Ϊ�� destination filter ���� URIջ������
		sfi.pFpi = NULL ;
	}
	else
	{
		_ASSERT(pWdf!=NULL) ;

		if(m_TaskType==XCTT_Move && pWdf->dwFileAttributes&FILE_ATTRIBUTE_READONLY)
		{// ��Ϊ�ƶ�����Ϊֻ���ļ�����ѯ���ϲ�
			if(m_pEvent!=NULL)
			{
				SImpactFileInfo ifi ;

				ifi.ImpactType = IT_ReadOnly ;
				ifi.strSrcFile = strFile ;

				SImpactFileResult ifr ;
				m_pEvent->XCOperation_ImpactFile(ifi,ifr) ;

				if(ifr.result==SFDB_Skip)
				{// �������ļ�

					if(hFile!=INVALID_HANDLE_VALUE)
					{
						::CloseHandle(hFile) ;
					}

					return false ;
				}
			}
		}

		sfi.pFpi = SDataPack_SourceFileInfo::Allocate() ;
		//sfi.pFpi = new SDataPack_SourceFileInfo() ;

		sfi.pFpi->nSysErrCode = nSysErrCode ;
		sfi.pFpi->dwSourceAttr = pWdf->dwFileAttributes ;
		
		bool bError = (hFile==NULL) ;
		
		sfi.pFpi->strSourceFile = strFile ;
		sfi.pFpi->SetDiscard(bError) ; // ����Ϊ��Ϊ��������
		sfi.pFpi->SetError(bError) ; // ������ 

		sfi.pFpi->SrcFileTime.CreateTime = pWdf->ftCreationTime ;
		sfi.pFpi->SrcFileTime.LastAccessTime = pWdf->ftLastAccessTime ;
		sfi.pFpi->SrcFileTime.LastWriteTime = pWdf->ftLastWriteTime ;

		sfi.pFpi->uFileID = nSpecifyFileID==0 ? ++m_uFileIDCounter : nSpecifyFileID;

		nSpecifyFileID = sfi.pFpi->uFileID ;

		sfi.pFpi->nFileSize = CptGlobal::DoubleWordTo64(pWdf->nFileSizeLow,pWdf->nFileSizeHigh) ;

		CptString strNewFileName ;

		// �ж�Դ�ļ��Ƿ�Ϊ������վ���ļ�������ǣ�������ֻ���ԭ��
		// ��Ϊ�� ������վ���ļ�������ļ������Ǳ��޸Ĺ��ģ�����ȡ����ʱӦ�޸Ļ���
		if(::IsRecycleFile(sfi.pFpi->strSourceFile.c_str(),strNewFileName))
		{
			sfi.pFpi->strNewFileName = strNewFileName ;
		}

		if(!(pWdf->dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
		{// ��ΪĿ¼
			++m_nTotalFileCounter ;
			m_nTotalFileSize += sfi.pFpi->nFileSize ;
		}
	}

	m_OpenedFileInfoList.push_back(sfi) ;

	_ASSERTE( _CrtCheckMemory( ) );

	#ifdef COMPILE_TEST_PERFORMANCE
		CptPerformanceCalcator::GetInstance()->EndCalAndSave(dw5,17) ;
#endif
	
		//Debug_Printf(_T("CXCLocalFileSourceFilter::AddOpenedFileToList()  1 m_nTotalFileCounter=%d m_nTotalFileSize=%u"),m_nTotalFileCounter,m_nTotalFileSize) ;

	if(m_nTotalFileCounter>63 || m_nTotalFileSize>this->m_pFileDataBuf->GetChunkSize())
	{// ���򿪵��ļ����ﵽ 512 �ݣ� �������ļ����ݻ������� 16 ��
		//Debug_Printf(_T("CXCLocalFileSourceFilter::AddOpenedFileToList()  2")) ;

		m_nTotalFileCounter = 0 ;
		m_nTotalFileSize = 0 ;

		return this->FlushFile() ;
	}
	
	return true ;
}

//void CXCLocalFileSourceFilter::ResendedFileAddToList()
//{
//	CptAutoLock lock(&m_ResendListLock) ;
//
//	if(!m_ResendFileList.empty())
//	{
//		pt_STL_list(SFileIDNamePairsInfo)::iterator& it = m_ResendFileList.begin() ;
//
//		for(;it!=m_ResendFileList.end();++it)
//		{
//			_ASSERT((*it).pFpi!=NULL) ;
//
//			this->OpenXCFile((*it).pFpi->strSourceFile,(*it).pFpi->uFileID) ;
//		}
//	}
//}

//int CXCLocalFileSourceFilter::OnDataTrans(CXCFilterEventCB* pSender,EFilterCmd cmd,void* pFileData) 
//{
//	return CXCSourceFilter::OnDataTrans(pSender,cmd,pFileData) ;
//}


bool CXCLocalFileSourceFilter::OnContinue() 
{
	return CXCSourceFilter::OnContinue() ;
}

bool CXCLocalFileSourceFilter::OnPause() 
{
	if(m_hPauseWaitEvent!=NULL)
	{
		m_hPauseWaitEvent = ::CreateEvent(NULL,FALSE,FALSE,NULL) ;
	}

	return CXCSourceFilter::OnPause() ;
}

void CXCLocalFileSourceFilter::OnStop() 
{
	Debug_Printf(_T("CXCLocalFileSourceFilter::OnStop() begin")) ;

	CXCSourceFilter::OnStop() ;

	Debug_Printf(_T("CXCLocalFileSourceFilter::OnStop() 1")) ;

	if(m_hCurFileHandle!=INVALID_HANDLE_VALUE)
	{
		::CancelIo(m_hCurFileHandle) ;
		m_hCurFileHandle = INVALID_HANDLE_VALUE ;
	}

	Debug_Printf(_T("CXCLocalFileSourceFilter::OnStop() 2")) ;

	if(m_hPauseWaitEvent!=NULL)
	{
		::SetEvent(m_hPauseWaitEvent) ;
		::CloseHandle(m_hPauseWaitEvent) ;
	}

	Debug_Printf(_T("CXCLocalFileSourceFilter::OnStop() 3")) ;

	this->FreeFileInfo() ;

	Debug_Printf(_T("CXCLocalFileSourceFilter::OnStop() end")) ;
}

bool CXCLocalFileSourceFilter::OnInitialize() 
{
		// ���� link ��ʼ������
	return CXCSourceFilter::OnInitialize() ;
}

bool CXCLocalFileSourceFilter::OnRun() 
{
	CXCSourceFilter::OnRun() ;

	bool bRet = this->ExecuteFiles() ;

	if(bRet)
	{// ��������˳�ҲҪ������� filter ���� OnStop
		this->OnStop() ;
	}

	*m_pRunningState=CFS_Stop ;

	return bRet ;
}


bool CXCLocalFileSourceFilter::CheckPauseAndWait() 
{
	bool bRet = true ;

	if(*m_pRunningState==CFS_Pause)
	{// ��ͣ
		_ASSERT(m_hPauseWaitEvent!=NULL) ;

		m_bIsPauseWaited = true ;
		Debug_Printf(_T("CXCLocalFileSourceFilter::CheckPauseAndWait()  enter pause")) ;

		::WaitForSingleObject(m_hPauseWaitEvent,INFINITE) ;

		Debug_Printf(_T("CXCLocalFileSourceFilter::CheckPauseAndWait()  leave pause")) ;

		m_bIsPauseWaited = false ;
		bRet = this->IsValideRunningState() ;
	}

	return bRet ;
}


void CXCLocalFileSourceFilter::FreeFileInfo() 
{
#ifdef COMPILE_TEST_PERFORMANCE
			DWORD dwBegin = CptPerformanceCalcator::GetInstance()->BeginCal() ;
			int count = 0 ;
#endif

	pt_STL_list(SSourceFileInfo)::iterator it = m_OpenedFileInfoList.begin() ;

	while(it!=m_OpenedFileInfoList.end())
	{
		if((*it).hFile!=NULL && (*it).hFile!=INVALID_HANDLE_VALUE)
		{// �ر������ļ����
			//::CancelIo((*it).hFile) ;
			::CloseHandle((*it).hFile) ;
		}

		++it ;
	}

	m_OpenedFileInfoList.clear() ;

#ifdef COMPILE_TEST_PERFORMANCE
		DWORD dwDiff = CptPerformanceCalcator::GetInstance()->EndCalAndSave(dwBegin,7) ;
		//Release_Printf(_T("count = %d  dwDiff = %u"),count,dwDiff) ;
#endif
}

/**
CXCLocalFileSourceFilter::CRemoveDiscardFile::CRemoveDiscardFile(CXCCopyingEvent* pEvent,pt_STL_list(SFindFolderInfo)* pDiscardedFolder,pt_STL_list(SDataPack_SourceFileInfo*)* pSrcFileInfoList)
	:m_pSrcFileInfoVer(pSrcFileInfoVer),m_nDirStackCount(0),m_nSrcFileVerBeginIndex(0),m_pDiscardedFolder(pDiscardedFolder),
	m_pEvent(pEvent)
{
}

bool CXCLocalFileSourceFilter::CRemoveDiscardFile::operator()(SSourceFileInfo& sfi)
{
	_ASSERT(m_pSrcFileInfoVer!=NULL) ;

	bool bRet = false ;

	if(sfi.pFpi==NULL)
	{// ����ջ
		if(m_nDirStackCount>0)
		{// ˵����source file���ڱ��������ļ��������ڣ���ô�ý���Ȼ�Ǳ��������ļ����ļ���
			--m_nDirStackCount ;
			bRet = true ; // �ý����Ҫ�Ƴ�
		}
	}
	else
	{
		pt_STL_list(SDataPack_SourceFileInfo*)::iterator = m_pSrcFileInfoList->begin() ;

		//for(size_t i=m_nSrcFileVerBeginIndex;i<m_pSrcFileInfoVer->size();++i)
		for(;it!=m_pSrcFileInfoList->end();++it)
		{
			if((*it)!=NULL && (*it)->uFileID==sfi.pFpi->uFileID)
			{// �ҵ�����ļ�IDƥ���destination filter ���ص�create file�Ľ�����ж��Ƿ�Ϊ����
			 // ��Ϊд�� EDC_BatchCreateFile ����� source file vector ��source file info ˳����һ���ģ�
			 // ���Կ��Բ��ҹ��Ľ�㲻�����жϣ�ֱ������
				m_nSrcFileVerBeginIndex = i+1 ;

				if((*it)->bDiscard)
				{// �����ļ�Ϊ����
					if(sfi.hFile!=NULL && sfi.hFile!=INVALID_HANDLE_VALUE)
					{// ������һ�ļ��У���Ŀ¼��1
						if(m_nDirStackCount==0)
						{// ���Ǳ������ĸ��ļ�������Ҫ��ӵ����������������һ�� AddFolder() ���ͷ�����ļ��н��
							SFindFolderInfo ffi ;

							ffi.FolderName = (*it)->strSourceFile ;
							ffi.uFileID = (*it)->uFileID ;
							m_pDiscardedFolder->push_back(ffi) ;
						}
						++m_nDirStackCount ;
					}
					else
					{
						::CloseHandle(sfi.hFile) ;
						m_pEvent->XCOperation_FileDiscard(sfi.pFpi,sfi.pFpi->nFileSize) ;
					}

					SDataPack_SourceFileInfo::Free(sfi.pFpi) ;
					sfi.pFpi = NULL ;
					bRet = true ;

				}

				break ;
			}
		}
	}

	return bRet ;
}
/**/