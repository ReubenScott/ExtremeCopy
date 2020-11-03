/****************************************************************************

Copyright (c) 2008-2020 Kevin Wu (Wu Feng)

github: https://github.com/kevinwu1024/ExtremeCopy
site: http://www.easersoft.com

Licensed under the Apache License, Version 2.0 License (the "License"); you may not use this file except
in compliance with the License. You may obtain a copy of the License at

https://opensource.org/licenses/Apache-2.0

****************************************************************************/

#include "StdAfx.h"
#include "XCAsyncFileDataTransFilter.h"
#include "XCFileDataBuffer.h"

CXCAsyncFileDataTransFilter::CXCAsyncFileDataTransFilter(CXCCopyingEvent* pEvent)
	:CXCFileDataCacheTransFilter(pEvent),m_hWaitForFileBufNotFull(NULL),
	m_hThread(NULL),m_hWriteDataReadyEvent(NULL),m_hLinkEndForQueEmpty(NULL),m_uFirstCompleteFileID(0),m_bWriteThreadEnd(false)
{
}


CXCAsyncFileDataTransFilter::~CXCAsyncFileDataTransFilter(void)
{
	m_bWriteThreadEnd = true;

	if(m_hThread!=NULL)
	{
		::SetEvent(m_hWriteDataReadyEvent) ;
		::CloseHandle(m_hWriteDataReadyEvent) ;
		m_hWriteDataReadyEvent = NULL ;

		if(m_hWaitForFileBufNotFull!=NULL)
		{
			::SetEvent(m_hWaitForFileBufNotFull) ;
			::CloseHandle(m_hWaitForFileBufNotFull) ;
			m_hWaitForFileBufNotFull = NULL ;
		}

		if(m_hLinkEndForQueEmpty!=NULL)
		{
			::SetEvent(m_hLinkEndForQueEmpty) ;
			::CloseHandle(m_hLinkEndForQueEmpty) ;
			m_hLinkEndForQueEmpty = NULL ;
		}

		::WaitForSingleObject(m_hThread,3*1000) ;
		::CloseHandle(m_hThread) ;
		m_hThread = NULL ;
	}
}

bool CXCAsyncFileDataTransFilter::OnContinue() 
{
	return CXCFileDataCacheTransFilter::OnContinue() ;
}

bool CXCAsyncFileDataTransFilter::OnPause() 
{
	return CXCFileDataCacheTransFilter::OnPause() ;
}


void CXCAsyncFileDataTransFilter::OnStop() 
{
	m_bWriteThreadEnd = true;
	if(m_hThread!=NULL)
	{
		::SetEvent(m_hWriteDataReadyEvent) ;
		::CloseHandle(m_hWriteDataReadyEvent) ;
		m_hWriteDataReadyEvent = NULL ;

		if(m_hWaitForFileBufNotFull!=NULL)
		{
			::SetEvent(m_hWaitForFileBufNotFull) ;
			::CloseHandle(m_hWaitForFileBufNotFull) ;
			m_hWaitForFileBufNotFull = NULL ;
		}

		if(m_hLinkEndForQueEmpty!=NULL)
		{
			::SetEvent(m_hLinkEndForQueEmpty) ;
			::CloseHandle(m_hLinkEndForQueEmpty) ;
			m_hLinkEndForQueEmpty = NULL ;
		}

		::WaitForSingleObject(m_hThread,3*1000) ;
		::CloseHandle(m_hThread) ;
		m_hThread = NULL ;
	}

	CXCFileDataCacheTransFilter::OnStop() ;
}

bool CXCAsyncFileDataTransFilter::OnInitialize() 
{
	m_bLinkEnded = false ;
	if(m_hWriteDataReadyEvent==NULL)
	{
		m_hWriteDataReadyEvent = ::CreateEvent(NULL,FALSE,FALSE,NULL) ;
	}

	if(m_hThread==NULL)
	{
		m_bWriteThreadEnd = false;
		m_hThread = (HANDLE)::_beginthreadex(NULL,0,ThreadFunc,this,0,NULL) ;
	}

	return CXCFileDataCacheTransFilter::OnInitialize() ;
}

int CXCAsyncFileDataTransFilter::OnDataTrans(CXCFilterEventCB* pSender,EFilterCmd cmd,void* pFileData) 
{
	int nRet = 0 ;

	switch(cmd)
	{
	case EDC_FlushData:
		::Sleep(1) ;
		break ;

	case EDC_BatchCreateFile:
		{
			CptAutoLock lock(&m_CreateBatchFileLock) ;

			nRet = CXCFileDataCacheTransFilter::OnDataTrans(pSender,cmd,pFileData) ;

			{
				CptAutoLock lock(&m_FileDataQueLock) ;

				bool bNeedWaitUp = m_FileDataQue.empty() ;

				SDataPack_FileData fd ;

				fd.pData = NULL ;
				fd.uFileID = 0 ;
				fd.nBufSize = 1 ; // create file cmd

				m_FileDataQue.push_back(fd) ;

				if(bNeedWaitUp)
				{// ���ѵȴ���д�߳�
					::SetEvent(m_hWriteDataReadyEvent) ;
				}
			}

			return nRet ;
		}
		break ;

	case EDC_FileData: // �ļ�����
		{
			SDataPack_FileData* pFD = (SDataPack_FileData*)pFileData ;

			_ASSERT(pFD!=NULL) ;

			bool bNeedWaitUp = false ;

			{
				CptAutoLock lock(&m_FileDataQueLock) ;

				bNeedWaitUp = m_FileDataQue.empty() ;

				CXCFileDataCacheTransFilter::OnDataTrans(pSender,cmd,pFileData) ;
			}
			
			if(bNeedWaitUp)
			{// ���ѵȴ���д�߳�
				::SetEvent(m_hWriteDataReadyEvent) ;
			}
		}

		return 0 ;

	case EDC_FileOperationCompleted: // �����ڸ��ļ��Ĳ��������
		{
			SDataPack_FileOperationCompleted* pFD = (SDataPack_FileOperationCompleted*)pFileData ;

			SCompleteCmdCacheInfo ccci ;

			CptAutoLock lock(&m_FileOperCompleteCacheLock) ;

			ccci.CacheCompleteCmdList = pFD->CompletedFileInfoList ;

			m_CacheCompleteCmdList.push_back(ccci) ; // ֻ�����������

			{
				CptAutoLock lock(&m_FileDataQueLock) ;

				bool bNeedWaitUp = m_FileDataQue.empty() ;

				SDataPack_FileData fd ;

				fd.pData = NULL ;
				fd.uFileID = 0 ;
				fd.nBufSize = 2 ; // complete cmd

				m_FileDataQue.push_back(fd) ;

				if(bNeedWaitUp)
				{// ���ѵȴ���д�߳�
					::SetEvent(m_hWriteDataReadyEvent) ;
				}
			}

			return 0 ;
		}

		return nRet ;


	case EDC_LinkEnded:
		{
			SMsgPack mp ;

			mp.cmd = EDC_LinkEnded ;
			mp.pFileData = pFileData ;

			HANDLE hMsg = NULL ;

			// ������У��ȴ���OutputWorkThread���

			{
				CptAutoLock lock(&m_FileDataQueLock) ;

				hMsg = m_IdelMsgQue.AsynSendMsg(&mp) ;

				if(m_FileDataQue.empty())
				{// ��� OutputWorkdThread �Ѿ�ֹ����ô���份��
					::SetEvent(m_hWriteDataReadyEvent) ;
				}
			}

			m_IdelMsgQue.WaitForAsynResult(hMsg,&nRet) ;

		}
		return nRet ;

	}

	return CXCFileDataCacheTransFilter::OnDataTrans(pSender,cmd,pFileData) ;
}

int CXCAsyncFileDataTransFilter::ProcessLinkedEnd(SDataPack_FileOperationCompleted* pFoc) 
{// �Ƴ���Ϊsource filter�ӳ��յ� EDC_FileDoneConfirm ���
	//�������� EDC_LinkEnded Я������Ҫ�����ٴδ��� EDC_FileOperationCompleted ������������

	CptAutoLock lock(&m_FileOperCompleteCacheLock) ;
	CptAutoLock lock2(&m_FileDataQueLock) ;

	SDataPack_FileOperationCompleted fdc ;

	if(!m_CacheCompleteCmdList.empty())
	{
		pt_STL_list(SDataPack_SourceFileInfo*)& CompleteList = (*m_CacheCompleteCmdList.begin()).CacheCompleteCmdList ;

		pt_STL_list(SDataPack_SourceFileInfo*)::iterator it = CompleteList.begin() ;//m_CacheFileOperCompList.begin() ;
		pt_STL_list(SDataPack_SourceFileInfo*)::iterator it2 = pFoc->CompletedFileInfoList.begin() ;

		for(;it!=CompleteList.end() && !pFoc->CompletedFileInfoList.empty();++it)
		{
			it2 = pFoc->CompletedFileInfoList.begin() ;

			for(;it2!=pFoc->CompletedFileInfoList.end();++it2)
			{
				if((*it2)->uFileID==(*it)->uFileID)
				{
					pFoc->CompletedFileInfoList.erase(it2) ;
					break ;
				}
			}
		}

		it2 = pFoc->CompletedFileInfoList.begin() ;

		for(;it2!=pFoc->CompletedFileInfoList.end();++it2)
		{
			CompleteList.push_back(*it2) ;
		}

		if(!CompleteList.empty())
		{
			fdc.CompletedFileInfoList = CompleteList ;
			m_CacheCompleteCmdList.clear() ;
		}
	}

	return m_pDownstreamFilter->OnDataTrans(this,EDC_LinkEnded,&fdc) ;
}

void CXCAsyncFileDataTransFilter::OutputWorkThread() 
{
	DWORD dwWaitResult = WAIT_OBJECT_0 ;
	SDataPack_FileData fd ;
	bool bWait = false ;
	unsigned uCurFileID = 0 ;

	::memset(&fd,0,sizeof(fd)) ;

	bool bCreateOrComplete = true ;
	void* pLastDataPointer = NULL ;

	do
	{
		PT_BREAK_IF(m_bWriteThreadEnd);
		PT_BREAK_IF(!this->IsValideRunningState());

		{// ��������Ų���ȥ���� ��Ϊ��δ������̰߳�ȫ��
			CptAutoLock lock(&m_FileDataQueLock) ;

			bWait = m_FileDataQue.empty() ;

			if(!bWait)
			{
				_ASSERTE( _CrtCheckMemory( ) );

				fd = m_FileDataQue.front() ;
				m_FileDataQue.pop_front() ;
			}
		}

		// ��Ϊ ����complete command����ʱ��֪ͨ source filter���ͷŴ����Ҿ�������������������Ի�����������
		if(bWait && m_uFirstCompleteFileID+1<fd.uFileID && m_uFirstCompleteFileID>0)
		{// �������source file �ļ�����������µ��ļ�IDһ�µĻ��������ļ����������
			m_uFirstCompleteFileID = 0 ;
		}

		PT_BREAK_IF(m_bWriteThreadEnd || !this->IsValideRunningState()); // ��ʱ�����˳��߳�

		if(bWait)
		{
			SMsgPack* pMp = (SMsgPack*)m_IdelMsgQue.BeginMsg() ;

			if(pMp!=NULL)
			{
				if(pMp->cmd==EDC_LinkEnded)
				{
					SDataPack_FileOperationCompleted* pFoc = (SDataPack_FileOperationCompleted*)pMp->pFileData ;

					int nResult = this->ProcessLinkedEnd(pFoc) ;

					m_IdelMsgQue.EndMsg(&nResult) ;

					PT_BREAK_IF(true); // ��Ϊ���������ѷ��������ԾͿ���ֱ���˳��߳�
				}
				else
				{
					_ASSERT(FALSE) ;
					m_IdelMsgQue.EndMsg(NULL) ;
				}
			}

			PT_BREAK_IF(m_bWriteThreadEnd || !this->IsValideRunningState()); // ��ʱ�����˳��߳�

			if(fd.nDataSize>0)
			{// �����Filter��һЩ���еĹ���������ȴ��첽IO���
				m_pDownstreamFilter->OnDataTrans(this,EDC_FileData,NULL) ;

				PT_BREAK_IF(m_bWriteThreadEnd || !this->IsValideRunningState()); // ��ʱ�����˳��߳�

				CptAutoLock lock(&m_FileDataQueLock) ;

				if(!m_FileDataQue.empty())
				{
					continue ;
				}
			}
			
			dwWaitResult = ::WaitForSingleObject(m_hWriteDataReadyEvent,2000) ;

			PT_BREAK_IF(m_bWriteThreadEnd || !this->IsValideRunningState()); // ��ʱ�����˳��߳�
		}
		else
		{
			if(fd.uFileID==0 && fd.pData==NULL)
			{
				if(fd.nBufSize==1)
				{// create file
					CptAutoLock lock(&m_CreateBatchFileLock) ;

					this->SendCacheBathCreateCommand() ;
				}
				else if(fd.nBufSize==2)
				{// complete cmd
					if(!m_CacheCompleteCmdList.empty())
					{
						this->FlushFileOperCompletedCommand(m_CacheCompleteCmdList.front().CacheCompleteCmdList) ;

						m_CacheCompleteCmdList.pop_front() ;
					}
				}

				continue ;
			}
			_ASSERTE( _CrtCheckMemory( ) );

			//CptAutoLock lock(&m_CreateFileLock) ; // ��Ϊ�����ļ���д�ļ�������������ͬ���̣߳���������Ϊ��HD�����Ч�ʣ���ֻ��ͨ������һ��

EXCEPTION_RETRY_LOCALASYFILEDATA: // ����

			PT_BREAK_IF(m_bWriteThreadEnd || !this->IsValideRunningState()); // ��ʱ�����˳��߳�

			pLastDataPointer = fd.pData ;
			// �����δ�����
			switch(m_pDownstreamFilter->OnDataTrans(this,EDC_FileData,&fd))
			{
			case 0:
				break ;

			case ErrorHandlingFlag_Exit: // �˳�
				dwWaitResult= WAIT_FAILED ;
				break ;

			case ErrorHandlingFlag_Ignore: // ����
				break ;

			case ErrorHandlingFlag_Retry: // ����
				goto EXCEPTION_RETRY_LOCALASYFILEDATA ;
				break ;
			}
		}

		_ASSERTE( _CrtCheckMemory( ) );

	}
	while(!m_bWriteThreadEnd || this->IsValideRunningState());

}

UINT CXCAsyncFileDataTransFilter::ThreadFunc(void* pParam)
{
	CXCAsyncFileDataTransFilter* pThis = (CXCAsyncFileDataTransFilter*)pParam ;
	pThis->OutputWorkThread() ;

	return 0 ;
}