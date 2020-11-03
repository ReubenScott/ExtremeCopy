/****************************************************************************

Copyright (c) 2008-2020 Kevin Wu (Wu Feng)

github: https://github.com/kevinwu1024/ExtremeCopy
site: http://www.easersoft.com

Licensed under the Apache License, Version 2.0 License (the "License"); you may not use this file except
in compliance with the License. You may obtain a copy of the License at

https://opensource.org/licenses/Apache-2.0

****************************************************************************/

#include "StdAfx.h"
//#include "XCLocalFileAsynSourceFilter.h"
//#include "XCCopyingEvent.h"
//#include "XCFileChangingBuffer.h"
//
//
//CXCLocalFileAsynSourceFilter::CXCLocalFileAsynSourceFilter(CXCCopyingEvent* pEvent):CXCLocalFileSourceFilter(pEvent),m_APCEvent(NULL)
//{
//	::memset(&m_NextAllocBuf,0,sizeof(m_NextAllocBuf)) ;
//}
//
//
//CXCLocalFileAsynSourceFilter::~CXCLocalFileAsynSourceFilter(void)
//{
//	Debug_Printf(_T("CXCLocalFileAsynSourceFilter::~CXCLocalFileAsynSourceFilter()")) ;
//}
//
//bool CXCLocalFileAsynSourceFilter::OnInitialize() 
//{
//	m_nCreateFileFlag = FILE_FLAG_NO_BUFFERING|FILE_FLAG_SEQUENTIAL_SCAN|FILE_FLAG_OVERLAPPED ;
//
//	if(m_APCEvent==NULL)
//	{
//		m_APCEvent = ::CreateEvent(NULL,FALSE,FALSE,NULL) ;
//	}
//
//	return CXCLocalFileSourceFilter::OnInitialize() ;
//}
//
//bool CXCLocalFileAsynSourceFilter::OnRun()
//{
//	return CXCLocalFileSourceFilter::OnRun() ;
//}
//
//
//bool CXCLocalFileAsynSourceFilter::OnPause() 
//{
//	//*m_pRunningState = CFS_Pause ;
//	//return true ;
//
//	return CXCLocalFileSourceFilter::OnPause() ;
//}
//
//void CXCLocalFileAsynSourceFilter::OnStop() 
//{
//	//*m_pRunningState = CFS_Stop ;
//	if(m_NextAllocBuf.pBuf!=NULL)
//	{
//		this->m_pFileDataBuf->Free(m_NextAllocBuf.pBuf,m_NextAllocBuf.dwBufSize) ;
//		::memset(&m_NextAllocBuf,0,sizeof(m_NextAllocBuf)) ;
//	}
//
//	if(m_APCEvent!=NULL)
//	{
//		::SetEvent(m_APCEvent) ;
//		::CloseHandle(m_APCEvent) ;
//		m_APCEvent = NULL ;
//	}
//
//	CXCLocalFileSourceFilter::OnStop() ;
//}
//
//// ���� false �Ļ�����˵��Ҫ�˳���������
//bool CXCLocalFileAsynSourceFilter::FlushFileData() 
//{
//	if(!this->IsValideRunningState())
//	{
//		this->InsideStop() ;
//		return false ;
//	}
//
//	bool bRet = true ;
//
//	unsigned __int64 uRemainSize = 0 ;
//
//	SDataPack_FileData fd ;
//	SFileIDNamePairsInfo fnpi ;
//
//	pt_STL_list(SSourceFileInfo)::iterator& it = m_OpenedFileInfoList.begin() ;
//
//	BOOL bReadResult = FALSE ;
//	const int nFileDataBufPageSize = this->m_pFileDataBuf->GetPageSize() ;
//	//const int nPageSizeTimes = (8*1024*1024)/nFileDataBufPageSize ;
//
//	SDataPack_FileOperationCompleted FileCompletedPack ;
//	DWORD nReadAlignSize = 0 ;
//	bool bMultiple = false ;
//	bool bEOF = false ;
//
//	//pt_STL_list(HANDLE) CloseFileList ;
//	SAllocBufInfo CurAlloc ;
//	m_hCurFileHandle = INVALID_HANDLE_VALUE ;
//	SXCAsynOverlapped xcOverlap[2] ;
//	int nCurOverlapIndex = 0 ;
//	int nPreOverlapIndex = 0 ;
//
//	SFileEndedInfo fei ;
//
//	fei.bReadOrWrite = true ;
//	bool bStop = false ;
//
//	for(;bRet && it!=m_OpenedFileInfoList.end() && !(bStop=!this->IsValideRunningState());++it)
//	{
//		//if((*it).pFpi!=NULL && !(*it).pFpi->bDiscard)
//		{// ������ļ��ڴ���ʱ�ڲ�������
//			//continue ;
//			if((*it).hFile==INVALID_HANDLE_VALUE)
//			{// Ŀ¼
//				if((*it).pFpi==NULL)
//				{// �����ڸ��ļ��е����ļ����ļ��в��������, �� destination filter �� URI��ջ
//
//					// ���ļ��������,����뵽��ɶ��д�,������ EDC_FileOperationCompleted ����
//
//					//Win_Debug_Printf(_T("CXCLocalFileAsynSourceFilter::FlushFileData() m_FilePairListLock 1")) ;
//
//					pt_STL_list(SDataPack_FileOperationCompleted) FOCList ;
//
//					{// �����������ռ䣬��ȡ m_FilePairListLock �����ͷ�
//						CptAutoLock lock(&m_FilePairListLock) ;
//						pt_STL_list(SFileIDNamePairsInfo)::iterator FolderIt2 = m_ConfirmReleaseFilePairInfoList.begin() ;
//
//						for(;FolderIt2!=m_ConfirmReleaseFilePairInfoList.end();++FolderIt2)
//						{
//							if((*FolderIt2).pFpi->uFileID==(*it).nSectorSize)
//							{// nSectorSize ��ֵΪ��URI �ļ��е�IDֵ
//								FileCompletedPack.CompletedFileInfoList.push_back((*FolderIt2).pFpi) ;
//
//								FOCList.push_back(FileCompletedPack) ;
//								//							m_OutputPin.PushData(EDC_FileOperationCompleted,&FileCompletedPack) ;
//								FileCompletedPack.CompletedFileInfoList.clear() ;
//								break ;
//							}
//						}
//					}
//
//					if(!FOCList.empty())
//					{
//						pt_STL_list(SDataPack_FileOperationCompleted)::iterator focit = FOCList.begin() ;
//
//						for(;focit!=FOCList.end();++focit)
//						{
//							m_pDownstreamFilter->OnDataTrans(this,EDC_FileOperationCompleted,&(*focit)) ;
//						}
//					}
//
//					//Win_Debug_Printf(_T("CXCLocalFileAsynSourceFilter::FlushFileData() m_FilePairListLock 2")) ;
//				}
//				else
//				{
//					if((*it).pFpi!=NULL && !(*it).pFpi->bDiscard)
//					{// ���뵽"ȷ�ϻ������"
//						fnpi.nSectorSize = (*it).nSectorSize ;
//						fnpi.pFpi = (*it).pFpi ;
//
//						this->AddToDelayReleaseFileList(fnpi) ;
//					}
//				}
//			}
//			else if((*it).pFpi!=NULL && !(*it).pFpi->bDiscard)
//			{// �ļ�
//				if((*it).pFpi->nFileSize>0)
//				{
//					uRemainSize = (*it).pFpi->nFileSize ;
//					//fd.uReadBeginPos = 0 ;
//					nCurOverlapIndex = 0 ;
//					nPreOverlapIndex = 0 ;
//
//					nReadAlignSize = this->CalculateAlignSize(uRemainSize,(*it).nSectorSize) ;
//					/**
//					// file data buffer ��ҳ�� �Ƿ�Ϊ sector �ı���
//					nReadAlignSize = (DWORD)min((unsigned __int64)m_pFileDataBuf->GetChunkSize()/4,(*it).pFpi->nFileSize) ;
//
//					nReadAlignSize = ALIGN_SIZE_UP(nReadAlignSize,(*it).nSectorSize) ;
//
//					if(m_bSmallReadGran && nReadAlignSize>1024*1024)
//					{// ʹ��С��������ȡ����
//						nReadAlignSize /= 4 ;
//					}
//
//					nReadAlignSize = ALIGN_SIZE_UP(nReadAlignSize,(DWORD)m_pFileDataBuf->GetPageSize()) ;
//					/**/
//
//					bEOF = false ;
//
//					::memset(&xcOverlap[nCurOverlapIndex],0,sizeof(xcOverlap[nCurOverlapIndex])) ;
//
//					m_hCurFileHandle = (*it).hFile ;
//					bool bFlush = false ;
//
//					SFileDataOccuredInfo fdoi ;
//
//					fdoi.bReadOrWrite = true ;
//					
//					while(bRet && !(*it).pFpi->bDiscard && !bEOF && uRemainSize>0 && !(bStop=!this->IsValideRunningState()))
//					{
//						//pBuf = pTemBuf ;
//						bFlush = false ;
//						CurAlloc = m_NextAllocBuf ;
//
//						while(CurAlloc.pBuf==NULL && !(bStop=!this->IsValideRunningState()))
//						{
//							CurAlloc.pBuf = this->AllocateFileDataBuf(CurAlloc.dwBufSize,nReadAlignSize,uRemainSize,(*it).nSectorSize) ;//(char*)m_pFileDataBuf->Allocate(m_nValidCachePointNum,dwBufSize) ;//(char*)m_OutputPin.Allocate(dwReadSize) ;
//
//							if(CurAlloc.pBuf==NULL)
//							{// ��˵����������������
//								//Debug_Printf(_T("CXCLocalFileAsynSourceFilter::FlushFileData() EDC_FlushData ")) ;
//								
//								m_pDownstreamFilter->OnDataTrans(this,EDC_FlushData,NULL) ;
//
//								bFlush = true ;
//								//Debug_Printf(_T("CXCLocalFileAsynSourceFilter::FlushFileData()remain: %I64u  IsRunning=%d"),
//								//	uRemainSize,this->IsValideRunningState()?1:0) ;
//
//								if(!this->CheckPauseAndWait())
//								{// ��ͣ
//									//return false ;
//									bStop = true ;
//									goto STOP_END ;
//								}
//							}
//							else
//							{
//								break ;
//							}
//						}
//
//						if(bStop || CurAlloc.pBuf==NULL)
//						{// ���뻺����ʧ�ܣ������û�ѡ���˳�
//							Debug_Printf(_T("CXCLocalFileAsynSourceFilter::FlushFileData() 555")) ;
//							bStop = true ;
//							bRet = false ;
//							break ;
//						}
//
//EXCEPTION_RETRY_READFILE: // ���ԣ����ļ���
//
//						CptGlobal::Int64ToDoubleWord(fd.uReadBeginPos,xcOverlap[nCurOverlapIndex].ov.OffsetHigh,xcOverlap[nCurOverlapIndex].ov.Offset) ;
//
//						if(!this->CheckPauseAndWait())
//						{// ��ͣ
//							bStop = true ;
//							goto STOP_END ;
//						}
//
//						xcOverlap[nCurOverlapIndex].ov.hEvent = m_APCEvent ;
//
//						//Debug_Printf(_T("CXCLocalFileAsynSourceFilter::FlushFileData() ReadFileEx")) ;
//
//						bReadResult = ::ReadFileEx((*it).hFile,CurAlloc.pBuf,CurAlloc.dwBufSize,(OVERLAPPED*)&xcOverlap[nCurOverlapIndex],FileIOCompletionRoutine) ;
//
//						const int nErrorCode = ::GetLastError() ;
//
//						::memset(&m_NextAllocBuf,0,sizeof(m_NextAllocBuf)) ;
//
//						if(!bReadResult && ERROR_HANDLE_EOF==nErrorCode)
//						{
//							Debug_Printf(_T("ERROR_HANDLE_EOF")) ;
//							bEOF = true ;
//							bReadResult = TRUE ;
//						}
//
//						if(bReadResult)
//						{
//							if(uRemainSize>CurAlloc.dwBufSize)
//							{
//								m_NextAllocBuf.pBuf = this->AllocateFileDataBuf(m_NextAllocBuf.dwBufSize,nReadAlignSize,uRemainSize-CurAlloc.dwBufSize,(*it).nSectorSize) ;
//							}
//							
//							// ����������
//							nPreOverlapIndex = nCurOverlapIndex ;
//							nCurOverlapIndex = nCurOverlapIndex==0 ? 1 : 0 ;
//
//							::memset(&xcOverlap[nCurOverlapIndex],0,sizeof(xcOverlap[nCurOverlapIndex])) ;
//
//							const DWORD dwResult = ::WaitForSingleObjectEx(m_APCEvent,INFINITE,TRUE) ;
//							bReadResult = (dwResult==WAIT_IO_COMPLETION) ;
//							
//						}
//
//						if(bReadResult)
//						{// ��ȡ�ļ����ݳɹ�
//
//							if(xcOverlap[nPreOverlapIndex].dwOperSize==0 && !(bStop=!this->IsValideRunningState()))
//							{// ������صĳ���Ϊ0����˵����ȡʧ��
//								//Debug_Printf(_T("������صĳ���Ϊ0����˵����ȡʧ�� %d"),nErrorCode) ;
//
//								if(this->m_pEvent!=NULL)
//								{
//									SXCExceptionInfo ei ;
//
//									ei.strSrcFile = (*it).pFpi->strSourceFile ;
//									ei.ErrorCode.nSystemError = 0 ;
//									ei.ErrorCode.AppError = CopyFileErrorCode_CanotReadFileData ;
//									ei.SupportType = ErrorHandlingFlag_Exit ;
//
//									ErrorHandlingResult result = m_pEvent->XCOperation_CopyExcetption(ei) ;
//
//									bRet = false ;
//
//									switch(result)
//									{
//									case ErrorHandlingFlag_Ignore: // ����
//										break ;
//
//									default:
//									case ErrorHandlingFlag_Exit: // �˳�
//										bStop = true ;
//										*this->m_pRunningState = CFS_ReadyStop ;
//										break ;
//									}
//								}
//								
//								goto STOP_END ;
//								//return false ;
//							}
//
//							//Debug_Printf(_T("CXCLocalFileAsynSourceFilter::FlushFileData() 5")) ;
//
//							/**
//							//�������ܿ����ݲ��ص�������
//							if(m_pEvent!=NULL)
//							{// ���ϻص�����
//								fdoi.nDataSize = xcOverlap[nPreOverlapIndex].dwOperSize ;
//								fdoi.uFileID = (*it).pFpi->uFileID ;
//								fdoi.strFileName = (*it).pFpi->strSourceFile ;
//
//								m_pEvent->XCOperation_FileDataOccured(fdoi) ;
//							}
//							/**/
//
//							//Debug_Printf(_T("CXCLocalFileAsynSourceFilter::FlushFileData() 6")) ;
//
//							uRemainSize -= xcOverlap[nPreOverlapIndex].dwOperSize  ;
//							fd.uReadBeginPos = (*it).pFpi->nFileSize - uRemainSize  ;
//
//							
//							//fd.nDataSize = CurAlloc.dwBufSize ;//xcOverlap[nPreOverlapIndex].dwOperSize  ;
//							fd.nDataSize = xcOverlap[nPreOverlapIndex].dwOperSize  ;
//							fd.pData = CurAlloc.pBuf ;
//							fd.nBufSize = CurAlloc.dwBufSize ;
//							fd.uFileID = (*it).pFpi->uFileID ;
//
//							// ע��: CurAlloc.dwBufSize �п��ܱ� xcOverlap[nPreOverlapIndex].dwOperSize ��,
//							// ������ xcOverlap[nPreOverlapIndex].dwOperSize ����nDataSize,�����ͷ�ʱ�ͻ�
//							// �� xcOverlap[nPreOverlapIndex].dwOperSize ��С�ͷ�,���������ǰ�CurAlloc.dwBufSize ��С�����,
//							// ��˻���ֿռ�й¶�����. �������������б�Ҫ���µĶ���
//							if(nReadAlignSize>uRemainSize)
//							{// ���������ȹ���,������������
//								//nReadAlignSize = this->CalculateAlignSize(uRemainSize,(*it).nSectorSize) ;
//								nReadAlignSize = ALIGN_SIZE_UP((DWORD)uRemainSize,(DWORD)m_pFileDataBuf->GetPageSize()) ;
//							}
//
//							//Win_Debug_Printf(_T("CXCLocalFileAsynSourceFilter::FlushFileData() EDC_FileData ")) ;
//
//							m_pDownstreamFilter->OnDataTrans(this,EDC_FileData,&fd) ;
//
//							//Debug_Printf(_T("CXCLocalFileAsynSourceFilter::FlushFileData() 7")) ;
//						}
//						else
//						{
//							SXCExceptionInfo ei ;
//
//							ei.strSrcFile = (*it).pFpi->strSourceFile ;
//							ei.uFileID = (*it).pFpi->uFileID ;
//							ei.ErrorCode.nSystemError = nErrorCode ;
//							ei.SupportType = ErrorHandlingFlag_RetryCancel ;
//
//							// �쳣�ص�
//							ErrorHandlingResult result = this->m_pEvent->XCOperation_CopyExcetption(ei) ;
//
//							switch(result)
//							{
//							case ErrorHandlingFlag_Ignore: // ����
//								{// ��������
//									bRet = false ;
//
//									m_pEvent->XCOperation_FileDiscard((*it).pFpi,uRemainSize) ;
//
//									// �� dstination filter �������ļ�
//									fd.pData = NULL ;
//									fd.nDataSize = 0 ;
//									m_pDownstreamFilter->OnDataTrans(this,EDC_FileData,&fd) ;
//								}
//								break ;
//
//							case ErrorHandlingFlag_Retry: // ���ԣ����ļ���
//								goto EXCEPTION_RETRY_READFILE ;
//
//							default:
//							case ErrorHandlingFlag_Exit: // �˳�
//								bRet = false ;
//								bStop = true ;
//								*this->m_pRunningState = CFS_ReadyStop ;
//								break ;
//							}
//						}
//					}// while(uRemainSize>0)
//
//					m_hCurFileHandle = INVALID_HANDLE_VALUE ;
//				}
//
//				// ������ļ���������� CXCLocalFileSourceFilter::FreeFileInfo() ���ͷ�
//
//				if(bRet)
//				{// �����˳�
//					if(uRemainSize==0)
//					{// �ⲻ�Ǳ�"����"���ļ�
//						if((*it).pFpi!=NULL)
//						{// ���뵽"ȷ�ϻ������"
//							fnpi.nSectorSize = (*it).nSectorSize ;
//							fnpi.pFpi = (*it).pFpi ;
//
//							this->AddToDelayReleaseFileList(fnpi) ;
//						}
//
//						if(m_pEvent!=NULL)
//						{// ����ɶ�ȡ���ļ����ϻص�
//							fei.strFileName = (*it).pFpi->strSourceFile ;
//							fei.uFileID = (*it).pFpi->uFileID ;
//
//							pt_STL_vector(SFileEndedInfo) FeiVer ;
//
//							FeiVer.push_back(fei) ;
//
//							m_pEvent->XCOperation_FileEnd(FeiVer) ;
//						}
//
//						// ���ļ��������,����뵽��ɶ��д�,������ EDC_FileOperationCompleted ����
//						FileCompletedPack.CompletedFileInfoList.push_back((*it).pFpi) ;
////						m_OutputPin.PushData(EDC_FileOperationCompleted,&FileCompletedPack) ;
//						m_pDownstreamFilter->OnDataTrans(this,EDC_FileOperationCompleted,&FileCompletedPack) ;
//						FileCompletedPack.CompletedFileInfoList.clear() ;
//					}
//					else 
//					{// ������;������������
//						m_pEvent->XCOperation_FileDiscard((*it).pFpi,uRemainSize) ;
//						// ���ﲻ�ùر��ļ������������� CXCLocalFileSourceFilter::FlushFileData() ���ͷ�
//
//						SDataPack_SourceFileInfo::Free((*it).pFpi) ; // �������ͷ� source file info ��Ϣ��
//						(*it).pFpi = NULL ;
//					}
//				}
//			}
//		}
//
//	}// for 
//
//STOP_END:
//	if(bStop)
//	{
//		//this->FreeFileInfo() ;
//		this->InsideStop() ;
//		bRet = false ;
//	}
//
//	return bRet ;
//}
//
//
//void CXCLocalFileAsynSourceFilter::FileIOCompletionRoutine(DWORD dwErrorCode,DWORD dwNumberOfBytesTransfered,OVERLAPPED* lpOverlapped) 
//{
//	//Debug_Printf(_T("Read FileIOCompletionRoutine() begin")) ;
//
//	if(lpOverlapped!=NULL)
//	{
//		SXCAsynOverlapped* pXCOverlap = (SXCAsynOverlapped*)lpOverlapped ;
//
//		//Debug_Printf(_T("Read FileIOCompletionRoutine() %u"),dwNumberOfBytesTransfered) ;
//		pXCOverlap->dwOperSize = dwNumberOfBytesTransfered ;
//		//pXCOverlap->pThis->FileIOCompleteWork(dwNumberOfBytesTransfered) ;
//	}
//
//	//Debug_Printf(_T("Read FileIOCompletionRoutine() end")) ;
//}
//
////void CXCLocalFileAsynSourceFilter::FileIOCompleteWork(const DWORD& dwNumberOfBytesTransfered)
////{
////	ECopyFileState cfs = this->GetState() ;
////	if(this->GetState()!=CFS_Stop)
////	{
////		if(dwNumberOfBytesTransfered==0)
////		{// HANDLE CLOSED
////		}
////		else
////		{
////		}
////	}
////}
//
////bool CXCLocalFileAsynSourceFilter::FlushFile() 
////{
////	if(m_OpenedFileInfoList.empty())
////	{
////		return true ;
////	}
////
////	if(!m_OutputPin.IsConnected())
////	{
////		return false ;
////	}
////
////	if(!this->FlushCreateFile())
////	{
////		return false ;
////	}
////
////	if(this->FlushFileData())
////	{
////		return CXCLocalFileSourceFilter::FlushFile() ; // ������һ���ģ��Ա��ͷ� m_OpenedFileInfoList ��Դ
////	}
////	
////	return false ;
////}
////
////bool CXCLocalFileAsynSourceFilter::AddFile(const CptString strFileName,const WIN32_FIND_DATA& wfd,int nSectorSize,unsigned uSpecifyFileID) 
////{
////	bool bRet = false ;
////	bool bAdd = true ;
////	
////EXCEPTION_RETRY_OPENFILE: // ���ԣ����ļ���
////	HANDLE hFile = ::CreateFile(strFileName.c_str(),GENERIC_READ,0,NULL,OPEN_EXISTING,
////		FILE_FLAG_NO_BUFFERING|FILE_FLAG_SEQUENTIAL_SCAN|FILE_FLAG_OVERLAPPED,NULL) ;
////
////	if(hFile==INVALID_HANDLE_VALUE)
////	{// ���ļ�ʧ��
////		if(this->m_pEvent!=NULL)
////		{
////			SXCExceptionInfo ei ;
////			ei.ErrorCode.nSystemError = ::GetLastError() ;
////			ei.strSrcFile = strFileName ;
////			ei.SupportType = ErrorHandlingFlag_RetryIgnoreCancel ;
////
////			ErrorHandlingResult result = this->m_pEvent->XCOperation_CopyExcetption(ei) ;
////
////			switch(result)
////			{
////			case ErrorHandlingFlag_Ignore: // ����
////				bAdd = false ; // �����
////				m_pEvent->XCOperation_FileDiscard(NULL,0) ;
////				break ;
////
////			case ErrorHandlingFlag_Retry: // ���ԣ����ļ���
////				goto EXCEPTION_RETRY_OPENFILE ;
////				break ;
////
////			case ErrorHandlingFlag_Exit: // �˳�
////			default:
////				return false ;
////			}
////		}
////		else
////		{
////			bAdd = false ;
////		}
////	}
////
////	if(bAdd)
////	{
////		return this->AddOpenedFileToList(hFile,strFileName,&wfd,nSectorSize,uSpecifyFileID) ;
////	}
////
////	return true ;
////}
////
////int CXCLocalFileAsynSourceFilter::OnDataTrans(CXCFilterEventCB* pSender,EFilterCmd cmd,void* pFileData) 
////{
////	return CXCLocalFileSourceFilter::OnDataTrans(pSender,cmd,pFileData) ;
////}
//
////int CXCLocalFileAsynSourceFilter::OnPin_Data(CXCPin* pOwnerPin,EFilterCmd cmd,void* pFileData) 
////{
////	return CXCLocalFileSourceFilter::OnPin_Data(pOwnerPin,cmd,pFileData) ;
////}