/****************************************************************************

Copyright (c) 2008-2020 Kevin Wu (Wu Feng)

github: https://github.com/kevinwu1024/ExtremeCopy
site: http://www.easersoft.com

Licensed under the Apache License, Version 2.0 License (the "License"); you may not use this file except
in compliance with the License. You may obtain a copy of the License at

https://opensource.org/licenses/Apache-2.0

****************************************************************************/

#include "StdAfx.h"
#include "XCSyncFileDataTransFilter.h"
#include "XCFileDataBuffer.h"
#include "../Common/ptDebugView.h"

CXCSyncFileDataTransFilter::CXCSyncFileDataTransFilter(CXCCopyingEvent* pEvent)
	:CXCFileDataCacheTransFilter(pEvent)
{
	m_bCacheCmdThreadSafe = false ; // Ϊ������ܣ��ʲ��̰߳�ȫ
}


CXCSyncFileDataTransFilter::~CXCSyncFileDataTransFilter(void)
{
}

//void* CXCSyncFileDataTransFilter::Allocate(CXCPin* pOwnerPin,int nSize) 
//{
//	void* pRet = NULL ;
//
//	if(pOwnerPin==&m_InputPin)
//	{
//	}
//
//	return pRet ;
//}
//
//bool CXCSyncFileDataTransFilter::CanAllocate(CXCPin* pOwnerPin) const 
//{
//	return (pOwnerPin==&m_InputPin) ;
//}
//
//bool CXCSyncFileDataTransFilter::GetPins(pt_STL_vector(CXCPin*)& PinVer)  
//{
//	PinVer.clear() ;
//
//	PinVer.push_back(&m_InputPin) ;
//	PinVer.push_back(&m_OutputPin) ;
//
//	return true ;
//}

bool CXCSyncFileDataTransFilter::OnInitialize() 
{
	return CXCFileDataCacheTransFilter::OnInitialize() ;
}

bool CXCSyncFileDataTransFilter::OnContinue() 
{
	return CXCFileDataCacheTransFilter::OnContinue() ;
}

bool CXCSyncFileDataTransFilter::OnPause() 
{
	return CXCFileDataCacheTransFilter::OnPause() ;
}


void CXCSyncFileDataTransFilter::OnStop() 
{
	CXCFileDataCacheTransFilter::OnStop() ;
}

int CXCSyncFileDataTransFilter::OnDataTrans(CXCFilterEventCB* pSender,EFilterCmd cmd,void* pFileData) 
{
	int nRet = 0 ;

	switch(cmd)
	{
	case EDC_FileOperationCompleted: // �����ڸ��ļ��Ĳ��������
		{
			_ASSERT(pFileData!=NULL) ;

			SDataPack_FileOperationCompleted* pFD = (SDataPack_FileOperationCompleted*)pFileData ;

			pt_STL_list(SDataPack_SourceFileInfo*)::const_iterator it = pFD->CompletedFileInfoList.begin() ;

			//CptAutoLock lock(&m_CacheCmdLock) ;

			for(;it!=pFD->CompletedFileInfoList.end();++it)
			{
				m_CacheFileOperCompList.push_back(*it) ;
			}
		}
		return 0 ;

	case EDC_FlushData: // ���ļ�����д��Ӳ��
		{
			Debug_Printf(_T("CXCSyncFileDataTransFilter::OnDataTrans() EDC_FlushData 1")) ;
			if(this->IsHaveCreateFileCache())
			{
				this->SendCacheBathCreateCommand() ;

				if(!this->IsValideRunningState())
				{
					return 0 ;
				}
			}
			
			Debug_Printf(_T("CXCSyncFileDataTransFilter::OnDataTrans() EDC_FlushData 2")) ;

			pt_STL_deque(SDataPack_FileData)::iterator it = m_FileDataQue.begin() ;

			unsigned uLastID = 0 ;
			unsigned uIgnoreID = 0 ;

			
			while(it!=m_FileDataQue.end())
			{
				uLastID = (*it).uFileID ;

				if(uLastID!=uIgnoreID)
				{
EXCEPTION_RETRY_LOCALSYNFILEDATA:
					//switch(m_OutputPin.PushData(EDC_FileData,&(*it)))
					switch(m_pDownstreamFilter->OnDataTrans(this,EDC_FileData,&(*it)))
					{
					case 0:
						break ;

					case ErrorHandlingFlag_Exit: // �˳�
						*this->m_pRunningState = CFS_ReadyStop ;
						return ErrorHandlingFlag_Exit ;

					case ErrorHandlingFlag_Ignore: // ����
						uIgnoreID = (*it).uFileID ;
						break ;

					case ErrorHandlingFlag_Retry: // ����
						goto EXCEPTION_RETRY_LOCALSYNFILEDATA ;
						break ;
					}
				}

				++it ;
//				m_pFileDataBuf->Free((*it).pData,(*it).nDataSize) ;
			}

			Debug_Printf(_T("CXCSyncFileDataTransFilter::OnDataTrans() EDC_FlushData 2")) ;

			m_FileDataQue.clear() ;

			// �ѻ��������� EDC_FileOperationCompleted ����ͳ�ȥ
			this->FlushFileOperCompletedCommand(m_CacheFileOperCompList) ;

			Debug_Printf(_T("CXCSyncFileDataTransFilter::OnDataTrans() EDC_FlushData 3")) ;

			m_CacheFileOperCompList.clear() ;

			if(uIgnoreID!=0 && uLastID==uIgnoreID)
			{// ��Ҫ�����ԡ����ļ���û��ȡ��ɣ����Ժ��Է���
				return ErrorHandlingFlag_Ignore ; // 
			}
		}
		return 0 ;

	//case EDC_FileHash: // �ļ���HASH ֵ
	//	break ;



	//case EDC_FolderChildrenOperationCompleted: // �����ڸ��ļ��е����ļ����ļ��в��������
	//	break ;
	}

	return CXCFileDataCacheTransFilter::OnDataTrans(pSender,cmd,pFileData) ; // �����������ֱ��ת����ȥ
}
//
//int CXCSyncFileDataTransFilter::OnPin_Data(CXCPin* pOwnerPin,EFilterCmd cmd,void* pFileData) 
//{
//	int nRet = 0 ;
//
//	switch(cmd)
//	{
//	case EDC_FileData: // �ļ�����
//		{// ��������
//			SDataPack_FileData* pFD = (SDataPack_FileData*)pFileData ;
//			
//			if(m_nCurFileID!=pFD->uFileID)
//			{
//				m_FileDataQue.push_back(*pFD) ;
//				m_nCurFileID = pFD->uFileID ;
//
//				//this->PushToCacheCmdQue<CNullObject>(cmd,NULL,pFileData) ;
//			}
//			else
//			{
//				SDataPack_FileData& fd = m_FileDataQue.back() ;
//
//				_ASSERT(pFD->uFileID!=fd.uFileID || pFD->pData!=(char*)fd.pData+fd.nDataSize || pFD->uReadBeginPos!=fd.uReadBeginPos) ;
//
//				fd.nDataSize += pFD->nDataSize ;
//			}
//		}
//		return 0 ;
//
//	case EDC_FlushData: // ���ļ�����д��Ӳ��
//		{
//			pt_STL_deque(SDataPack_FileData)::iterator it = m_FileDataQue.begin() ;
//
//			unsigned uLastID = 0 ;
//			unsigned uIgnoreID = 0 ;
//
//			for(;it!=m_FileDataQue.end();++it)
//			{
//				uLastID = (*it).uFileID ;
//
//				if(uLastID!=uIgnoreID)
//				{
//EXCEPTION_RETRY_LOCALSYNFILEDATA:
//					switch(m_OutputPin.PushData(EDC_FileData,&(*it)))
//					{
//					case 0:
//						break ;
//
//					case ErrorHandlingFlag_Exit: // �˳�
//						return ErrorHandlingFlag_Exit ;
//
//					case ErrorHandlingFlag_Ignore: // ����
//						uIgnoreID = (*it).uFileID ;
//						break ;
//
//					case ErrorHandlingFlag_Retry: // ����
//						goto EXCEPTION_RETRY_LOCALSYNFILEDATA ;
//						break ;
//					}
//				}
//
//				m_pFileDataBuf->Free((*it).pData,(*it).nDataSize) ;
//			}
//
//			m_FileDataQue.clear() ;
//
//			{
//				// �ѻ��������� EDC_FileOperationCompleted ����ͳ�ȥ
//				CptAutoLock lock(&m_CacheCmdLock) ;
//
//				if(!m_CacheFileOperCompList.empty())
//				{
//					this->FlushFileOperCompletedCommand() ;
//				}
//			}
//
//			m_nCurFileID = 0 ;
//
//			if(uIgnoreID!=0 && uLastID==uIgnoreID)
//			{// ��Ҫ�����ԡ����ļ���û��ȡ��ɣ����Ժ��Է���
//				return ErrorHandlingFlag_Ignore ; // 
//			}
//		}
//		return 0 ;
//
//	//case EDC_FileHash: // �ļ���HASH ֵ
//	//	break ;
//
//	case EDC_FileOperationCompleted: // �����ڸ��ļ��Ĳ��������
//		{
//			_ASSERT(pFileData!=NULL) ;
//
//			SDataPack_FileDoneConfirm* pFD = (SDataPack_FileDoneConfirm*)pFileData ;
//
//			pt_STL_list(unsigned)::const_iterator it = pFD->FileDoneConfirmList.begin() ;
//
//			CptAutoLock lock(&m_CacheCmdLock) ;
//
//			for(;it!=pFD->FileDoneConfirmList.end();++it)
//			{
//				m_CacheFileOperCompList.push_back(*it) ;
//			}
//		}
//		return 0 ;
//
//	//case EDC_FolderChildrenOperationCompleted: // �����ڸ��ļ��е����ļ����ļ��в��������
//	//	break ;
//	}
//
//	return CXCFileDataCacheTransFilter::OnPin_Data(pOwnerPin,cmd,pFileData) ; // �����������ֱ��ת����ȥ
//}
//
//int CXCSyncFileDataTransFilter::FlushCmd() 
//{
//	int nRet = 0 ;
//
//	while(!m_CacheCmdQue.empty())
//	{
//		SCacheCommandInfo& cci = m_CacheCmdQue.front() ;
//
//		switch(cci.cmd)
//		{
//		case EDC_FileHash:
//			this->m_OutputPin.PushData(cci.cmd,cci.pCmdData) ;
//			break ;
//
//		case EDC_FileData:
//			if(!m_FileDataQue.empty())
//			{
//				if(this->WriteFileData(cci.cmd,cci.pCmdData)==ErrorHandlingFlag_Exit)
//				{
//					return ErrorHandlingFlag_Exit ;
//				}
//			}
//			//this->m_OutputPin.PushData(cci.cmd,cci.pCmdData) ;
//			break ;
//		}
//
//		if(cci.pCmdData!=NULL)
//		{
//			this->FreeCmdPackMem(cci.pCmdData) ;
//		}
//	}
//	
//	return nRet ;
//}

