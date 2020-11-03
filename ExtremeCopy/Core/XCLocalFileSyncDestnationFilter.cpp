/****************************************************************************

Copyright (c) 2008-2020 Kevin Wu (Wu Feng)

github: https://github.com/kevinwu1024/ExtremeCopy
site: http://www.easersoft.com

Licensed under the Apache License, Version 2.0 License (the "License"); you may not use this file except
in compliance with the License. You may obtain a copy of the License at

https://opensource.org/licenses/Apache-2.0

****************************************************************************/

#include "StdAfx.h"
#include "XCLocalFileSyncDestnationFilter.h"
#include "XCCopyingEvent.h"
#include "..\Common\ptGlobal.h"
#include "XCFileDataBuffer.h"
#include "..\Common\ptPerformanceCalcator.h"

CXCLocalFileSyncDestnationFilter::CXCLocalFileSyncDestnationFilter(CXCCopyingEvent* pEvent,const CptString strDestRoot,const SStorageInfoOfFile& siof,
	const bool bIsRenameCopy):CXCLocalFileDestnationFilter(pEvent,strDestRoot,siof,bIsRenameCopy)
{
}


CXCLocalFileSyncDestnationFilter::~CXCLocalFileSyncDestnationFilter(void)
{
	//Release_Printf(_T("~CXCLocalFileSyncDestnationFilter()")) ;
}

bool CXCLocalFileSyncDestnationFilter::OnContinue() 
{
	return CXCLocalFileDestnationFilter::OnContinue() ;
}

bool CXCLocalFileSyncDestnationFilter::OnPause()
{
	return CXCLocalFileDestnationFilter::OnPause() ;
}

void CXCLocalFileSyncDestnationFilter::OnStop()
{
	CXCLocalFileDestnationFilter::OnStop() ;
}

bool CXCLocalFileSyncDestnationFilter::OnInitialize() 
{
	m_nCreateFileFlag = FILE_FLAG_SEQUENTIAL_SCAN ;

	return CXCLocalFileDestnationFilter::OnInitialize() ;
}


int CXCLocalFileSyncDestnationFilter::WriteFileData(SDataPack_FileData& fd)
{
	int nRet = 0 ;

	if(!this->IsValideRunningState())
	{
		return ErrorHandlingFlag_Exit ;
	}

	// �����Ǳ��������ļ�����Ҳ���� m_FileInfoList ��
	_ASSERT(!m_FileInfoList.empty()) ;


	if(m_CurFileIterator==m_FileInfoList.end())
	{
		m_CurFileIterator = m_FileInfoList.begin() ;
	}

	_ASSERT(fd.uFileID>=(*m_CurFileIterator).pSfi->uFileID) ;

	while((*m_CurFileIterator).pSfi->nFileSize==0 || (*m_CurFileIterator).pSfi->uFileID<fd.uFileID)
	{
		++m_CurFileIterator ;
	}

	_ASSERT((*m_CurFileIterator).pSfi->uFileID==fd.uFileID) ;
	_ASSERT(m_CurFileIterator!=m_FileInfoList.end()) ;


	if((*m_CurFileIterator).pSfi->IsDiscard())
	{// �Ǵ���������ܵģ���Ҫ�������ļ����ݻ�û����destination filter��
		//����ʱreading �쳣������������������destination filter��
		// ����destination�������ļ��������ļ����ݿ���Ȼ���� destination filter���ͷ�
		// ��μ� ��ExtremeCopy ԭ��.docx�� ���ļ��������֡����2�µ�״��2
		_ASSERT(fd.pData!=NULL) ;
		_ASSERT(fd.uFileID>0) ;
		//_ASSERT(fd.pSfi!=NULL && fd.pSfi->bDiscard==true) ;
		//--m_CurFileIterator ;
		m_pFileDataBuf->Free(fd.pData,fd.nBufSize) ;

		return 0 ;
	}

	_ASSERT(!(*m_CurFileIterator).pSfi->IsDiscard()) ;

	{
		DWORD dwWrite = 0 ;

		int nSize = 0 ;

		if((*m_CurFileIterator).bNoBuf)
		{//��ʹ��ϵͳ����,�����������С����
			nSize = ALIGN_SIZE_UP(fd.nDataSize,m_StorageInfo.nSectorSize) ;
		}
		else
		{
			nSize = fd.nDataSize ;
		}

EXCEPTION_RETRY_WRITEDATA:// ���ԣ�д�ļ���

#ifdef COMPILE_TEST_PERFORMANCE
		DWORD dw = CptPerformanceCalcator::GetInstance()->BeginCal() ;
#endif

		BOOL b = ::WriteFile((*m_CurFileIterator).hFile,fd.pData,nSize,&dwWrite,NULL) ;

#ifdef COMPILE_TEST_PERFORMANCE
		CptPerformanceCalcator::GetInstance()->EndCalAndSave(dw,2) ;
#endif

		if(!this->IsValideRunningState())
		{
			return ErrorHandlingFlag_Exit ;
		}
		
		if(b)
		{
			m_pFileDataBuf->Free(fd.pData,fd.nBufSize) ;

			_ASSERT((*m_CurFileIterator).uRemainSize>=dwWrite) ;

			(*m_CurFileIterator).uRemainSize -= dwWrite ;

			if(m_pEvent!=NULL && this->CanCallbackFileInfo())
			{// ���ϻص�����
				SFileDataOccuredInfo fdoi ;

				fdoi.bReadOrWrite = false ;
				fdoi.nDataSize = dwWrite ;
				fdoi.uFileID = (*m_CurFileIterator).pSfi->uFileID ;//(*m_CurFileIterator).uFileID ;
				//fdoi.strFileName = (*m_CurFileIterator).strFileName ;

				m_pEvent->XCOperation_FileDataOccured(fdoi) ;
			}


			if((*m_CurFileIterator).uRemainSize==0)
			{
				while((++m_CurFileIterator)!=m_FileInfoList.end() && (*m_CurFileIterator).pSfi->nFileSize==0)
				{
					NULL ;
				}

			}
		}
		else
		{
			Debug_Printf(_T("CXCLocalFileDestnationFilter::WriteFileData() failed")) ;

			if(m_pEvent!=NULL)
			{
				SXCExceptionInfo ei ;
				ei.uFileID =(*m_CurFileIterator).pSfi->uFileID ;
				ei.ErrorCode.nSystemError = ::GetLastError() ;
				ei.SupportType = ErrorHandlingFlag_RetryIgnoreCancel ;
				ei.strDstFile = (*m_CurFileIterator).strFileName ;
				ei.strSrcFile = (*m_CurFileIterator).pSfi->strSourceFile ;

				ErrorHandlingResult result = m_pEvent->XCOperation_CopyExcetption(ei) ;

				switch(result)
				{
				case ErrorHandlingFlag_Ignore: // ����
					{
						::CloseHandle((*m_CurFileIterator).hFile) ;
						::DeleteFile((*m_CurFileIterator).strFileName) ;
						//(*m_CurFileIterator).pSfi->bDiscard = true ;
						(*m_CurFileIterator).pSfi->SetDiscard(true) ;
						
						if(m_pEvent!=NULL)
						{
							m_pEvent->XCOperation_FileDiscard((*m_CurFileIterator).pSfi,(*m_CurFileIterator).uRemainSize) ;
						}

						++m_CurFileIterator ;

						m_pEvent->XCOperation_RecordError(ei) ; // ��������������¼����

						nRet = ErrorHandlingFlag_Ignore ;
					}
					break ;

				case ErrorHandlingFlag_Retry: // ���ԣ�д�ļ���
					goto EXCEPTION_RETRY_WRITEDATA ;

				default:
				case ErrorHandlingFlag_Exit: // �˳�
					::CloseHandle((*m_CurFileIterator).hFile) ;
					::DeleteFile((*m_CurFileIterator).strFileName) ;
//					m_FileInfoMap.erase(fd.uFileID) ;
					m_pFileDataBuf->Free(fd.pData,fd.nBufSize) ;
					*this->m_pRunningState = CFS_ReadyStop ;
					return ErrorHandlingFlag_Exit ;
				}
			}
		}

		//Debug_Printf(_T("file id: %u  remain size: %u "),fd.uFileID,dfi.uRemainSize) ;
	}

	return nRet ;
}

void CXCLocalFileSyncDestnationFilter::OnLinkEnded(pt_STL_list(SDataPack_SourceFileInfo*)& FileList) 
{
	if(!FileList.empty())
	{
		// ��Ϊ���첽������LinkEnded�����ʱ���������ļ�δ��ȷ���ͷţ���ʵ�ϣ��������һ˲����ȷ���ͷ��ˣ���
		// ���Ծͻ���� FileList ���ļ�ID����ȴ���ͷ��˵����
		if(!m_FileInfoList.empty())
		{
			this->RoundOffFile(FileList) ;
		}
	}
}
