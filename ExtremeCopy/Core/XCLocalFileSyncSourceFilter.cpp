/****************************************************************************

Copyright (c) 2008-2020 Kevin Wu (Wu Feng)

github: https://github.com/kevinwu1024/ExtremeCopy
site: http://www.easersoft.com

Licensed under the Apache License, Version 2.0 License (the "License"); you may not use this file except
in compliance with the License. You may obtain a copy of the License at

https://opensource.org/licenses/Apache-2.0

****************************************************************************/

#include "StdAfx.h"
#include "XCLocalFileSyncSourceFilter.h"
#include "XCCopyingEvent.h"



CXCLocalFileSyncSourceFilter::CXCLocalFileSyncSourceFilter(CXCCopyingEvent* pEvent):CXCLocalFileSourceFilter(pEvent)
{
}


CXCLocalFileSyncSourceFilter::~CXCLocalFileSyncSourceFilter(void)
{
}

bool CXCLocalFileSyncSourceFilter::OnRun()
{
	return CXCLocalFileSourceFilter::OnRun() ;
}

bool CXCLocalFileSyncSourceFilter::OnInitialize() 
{
	m_nCreateFileFlag = FILE_FLAG_NO_BUFFERING|FILE_FLAG_SEQUENTIAL_SCAN ;

	return CXCLocalFileSourceFilter::OnInitialize() ;
}


bool CXCLocalFileSyncSourceFilter::OnPause() 
{
	return CXCLocalFileSourceFilter::OnPause() ;
}

void CXCLocalFileSyncSourceFilter::OnStop() 
{
	Debug_Printf(_T("CXCLocalFileSyncSourceFilter::OnStop()")) ;

	CXCLocalFileSourceFilter::OnStop() ;
}


bool CXCLocalFileSyncSourceFilter::FlushFileData() 
{
	Debug_Printf(_T("CXCLocalFileSyncSourceFilter::FlushFileData() begin")) ;

	if(!this->IsValideRunningState())
	{
		this->InsideStop() ;
		return false ;
	}

#ifdef COMPILE_TEST_PERFORMANCE
	DWORD dwkk = CptPerformanceCalcator::GetInstance()->BeginCal() ;
#endif

	bool bRet = true ;

	DWORD dwRead = 0 ;
	unsigned __int64 uRemainSize = 0 ;
	char*	pBuf = NULL ;

	SDataPack_FileData fd ;
	SFileIDNamePairsInfo fnpi ;

	pt_STL_list(SFileIDNamePairsInfo) fnpiList ;
	pt_STL_list(SSourceFileInfo)::iterator& it = m_OpenedFileInfoList.begin() ;

	BOOL bReadResult = FALSE ;
	const int& nFileDataBufPageSize = this->m_pFileDataBuf->GetPageSize() ;
	DWORD nReadAlignSize = 0 ;

	bool bStop = false ;

	Debug_Printf(_T("CXCLocalFileSyncSourceFilter::FlushFileData() 1")) ;

	while(bRet && it!=m_OpenedFileInfoList.end() && !(bStop=!this->IsValideRunningState()))
	{
		//if((*it).pFpi!=NULL && !(*it).pFpi->bDiscard)
		{// ������ļ��ڴ���ʱ�ڲ�������
			//continue ;
			if((*it).hFile==INVALID_HANDLE_VALUE)
			{// Ŀ¼

				// ���뵽"ȷ�ϻ������"
				// ע�⣺ (*it).pFpi==NULL ʱ�������� URI ��ջ
				if((*it).pFpi==NULL)
				{// ��Ҫ��ջ�ˣ���˵�����һ���ļ��е����в�������ɣ�����������ɶ���

					_ASSERT(!m_FolderStack.empty()) ;

					fnpiList.push_back(m_FolderStack.back()) ;
					m_FolderStack.pop_back() ;
				}
				else
				{
					fnpi.nSectorSize = (*it).nSectorSize ;
					fnpi.pFpi = (*it).pFpi ;

					m_FolderStack.push_back(fnpi) ;
				}
			}
			else if((*it).hFile==NULL || (*it).pFpi->IsError())
			{// ������ļ����ڴ���ʱ�ѳ��������� destination filter ����
			}
			else //if((*it).pFpi!=NULL && !(*it).pFpi->bDiscard)
			{// �ļ�
				_ASSERT((*it).pFpi!=NULL) ;

				uRemainSize = (*it).pFpi->nFileSize ;
				m_pCurSourceFileInfo = (*it).pFpi ;

				bool bFlush = false ;

				// file data buffer ��ҳ�� �Ƿ�Ϊ sector �ı���
				nReadAlignSize = this->CalculateAlignSize(uRemainSize,(*it).nSectorSize) ;

				SFileDataOccuredInfo fdoi ;
				fdoi.bReadOrWrite = true ;

				while(bRet && !(*it).pFpi->IsDiscard() && uRemainSize>0 && !(bStop=!this->IsValideRunningState()))
				{
					pBuf = NULL ;
					bFlush = false ;

					while(pBuf==NULL && !(bStop=(!this->IsValideRunningState())))
					{// �����ļ����ݻ���ѭ��

						pBuf = (char*)m_pFileDataBuf->Allocate(m_nValidCachePointNum,nReadAlignSize) ;

						if(pBuf==NULL)
						{// ��˵����������������

							if(!fnpiList.empty())
							{// �Ȱ�Ϊ�����ܶ���������������ɨβ��������������·��򱣴�����
								//Debug_Printf(_T("CXCLocalFileSyncSourceFilter::FlushFileData() 2a4")) ;

								this->RoundOffReadFile(fnpiList) ;
								fnpiList.clear() ;
							}

#ifdef COMPILE_TEST_PERFORMANCE
							DWORD dw = CptPerformanceCalcator::GetInstance()->BeginCal() ;
#endif
							m_pDownstreamFilter->OnDataTrans(this,EDC_FlushData,NULL) ;

#ifdef COMPILE_TEST_PERFORMANCE
							CptPerformanceCalcator::GetInstance()->EndCalAndSave(dw,3) ;
#endif

							bFlush = true ;
							if(!this->CheckPauseAndWait())
							{// ��ͣ
								bStop = true ;
								goto STOP_END ;
							}
						}
						else
						{
							break ;
						}
					}

					if(bStop)
					{// ���뻺����ʧ�ܣ������û�ѡ���˳�
						bStop = true ;
						bRet = false ;
						break ;
					}

EXCEPTION_RETRY_READFILE: // ���ԣ����ļ���

					dwRead = 0 ;

#ifdef COMPILE_TEST_PERFORMANCE
					DWORD dw = CptPerformanceCalcator::GetInstance()->BeginCal() ;
#endif

					bReadResult = ::ReadFile((*it).hFile,pBuf,nReadAlignSize,&dwRead,NULL) ;

#ifdef _DEBUG
					// ���ڲ��Զ�ȡ�������� case
					//::Sleep(10000) ;
#endif

#ifdef COMPILE_TEST_PERFORMANCE
					CptPerformanceCalcator::GetInstance()->EndCalAndSave(dw,1) ;
#endif
					if(!this->CheckPauseAndWait())
					{// ��ͣ
						bStop = true ;
						goto STOP_END ;
					}

					if(bReadResult && dwRead>0 )
					{// ��ȡ�ļ����ݳɹ�
						uRemainSize -= dwRead ;

						fd.nDataSize = dwRead ;
						fd.nBufSize = nReadAlignSize ;
						fd.pData = pBuf ;
						fd.uFileID = (*it).pFpi->uFileID ;

						// ע��: fd.nBufSize �п��ܱ� dwRead ��,
						// ������ dwRead ����fd.nDataSize,�����ͷ�ʱ�ͻ�
						// �� dwRead ��С�ͷ�,���������ǰ�fd.nBufSize ��С�����,
						// ��˻���ֿռ�й©�����. �������������б�Ҫ���µĶ���
						if(nReadAlignSize>uRemainSize)
						{// ���������ȹ���,������������
							nReadAlignSize = ALIGN_SIZE_UP((DWORD)uRemainSize,(DWORD)m_pFileDataBuf->GetPageSize()) ;
							nReadAlignSize = ALIGN_SIZE_UP(nReadAlignSize, (*it).nSectorSize);
						}

						//Debug_Printf(_T("CXCLocalFileSyncSourceFilter::FlushFileData() 2f")) ;

#ifdef COMPILE_TEST_PERFORMANCE
						DWORD dw = CptPerformanceCalcator::GetInstance()->BeginCal() ;
#endif
						m_pDownstreamFilter->OnDataTrans(this,EDC_FileData,&fd) ;

#ifdef COMPILE_TEST_PERFORMANCE
						CptPerformanceCalcator::GetInstance()->EndCalAndSave(dw,13) ;
#endif
					}
					else
					{
						const int nErrorCode = ::GetLastError() ;

						SXCExceptionInfo ei ;

						ei.uFileID = (*it).pFpi->uFileID ;
						ei.strSrcFile = (*it).pFpi->strSourceFile ;
						ei.ErrorCode.nSystemError = nErrorCode ;
						ei.SupportType = ErrorHandlingFlag_RetryIgnoreCancel ;

						// �쳣�ص�
						ErrorHandlingResult result = m_pEvent->XCOperation_CopyExcetption(ei) ;

						switch(result)
						{
						case ErrorHandlingFlag_Ignore: // ����
							{// ��������
								bRet = false ;

								// �� dstination filter �������ļ�
								(*it).pFpi->SetDiscard(true) ;
								(*it).pFpi->SetError(true) ; // ����Ϊ������ļ����Ա��� destination filter ����ص���app��ȥ

								m_pDownstreamFilter->OnDataTrans(this,EDC_FileData,&fd) ; // ������Ȼ���·����� destination filter �ͷ� fd.pData �ռ�
							}
							break ;

						case ErrorHandlingFlag_Retry: // ���ԣ����ļ���
							goto EXCEPTION_RETRY_READFILE ;

						default:
						case ErrorHandlingFlag_Exit: // �˳�
							bRet = false ;
							*this->m_pRunningState = CFS_ReadyStop ;
							break ;
						}
					}

				}// while(uRemainSize>0)

				//Debug_Printf(_T("CXCLocalFileSyncSourceFilter::FlushFileData() 3")) ;

				if(!this->CheckPauseAndWait())
				{// ��Ϊ�����ļ�Ϊ�����ļ�ʱ�������û�������������п��� source filter 
					// ��û�����ü����� pause ״̬�ʹ����ѭ���˳�����ͻᵼ�½���� Skip ���ٵ��� Contiune()�ͻ���ֱ���
					//����ҲӦ�ü���Ƿ�����ͣ״̬
					bStop = true ;
					goto STOP_END ;
				}

				//Debug_Printf(_T("CXCLocalFileSyncSourceFilter::FlushFileData() 4")) ;

				_ASSERT((*it).pFpi!=NULL) ; 

				// �����ļ��Ƿ�������Ӧ�ü��뵽"ȷ�ϻ������"��
				// �Ա� destination filter ���� EDC_FileDoneConfirm ����ʱ�ͷ� SDataPack_SourceFileInfo �ռ�
				fnpi.nSectorSize = (*it).nSectorSize ;
				fnpi.pFpi = (*it).pFpi ;

				fnpiList.push_back(fnpi) ;

				if((*it).pFpi->IsDiscard() && bRet && m_pEvent!=NULL)
				{// �����˳�������;������������
					//Release_Printf(_T("CXCLocalFileSyncSourceFilter::FlushFileData() discard 2")) ;
					m_pEvent->XCOperation_FileDiscard((*it).pFpi,uRemainSize) ;
				}

				
				if(XCTT_Move==m_TaskType && (*it).hFile!=INVALID_HANDLE_VALUE)
				{
					::CloseHandle((*it).hFile) ;
					(*it).hFile = INVALID_HANDLE_VALUE ;
				}
			}
		}

		++it ;
	}// while

	m_pCurSourceFileInfo = NULL ;

	if(!fnpiList.empty())
	{// �Ȱ�Ϊ�����ܶ���������������ɨβ��������������·��򱣴�����
		this->RoundOffReadFile(fnpiList) ;
		fnpiList.clear() ;
	}

STOP_END:
	if(bStop)
	{
		this->InsideStop() ;
		bRet = false ;
	}

	Debug_Printf(_T("CXCLocalFileSyncSourceFilter::FlushFileData() end")) ;

	return bRet ;
}

void CXCLocalFileSyncSourceFilter::RoundOffReadFile(const pt_STL_list(SFileIDNamePairsInfo)& fnpiList) 
{
	Debug_Printf(_T("CXCLocalFileSyncSourceFilter::RoundOffReadFile() begin")) ;

#ifdef COMPILE_TEST_PERFORMANCE
	DWORD dw = CptPerformanceCalcator::GetInstance()->BeginCal() ;
#endif

	// ���뵽"ȷ�ϻ������"
	SDataPack_FileOperationCompleted FileCompletedPack ;

	{
		CptAutoLock lock(&m_FilePairListLock) ;

		pt_STL_list(SFileIDNamePairsInfo)::const_iterator it = fnpiList.begin() ;

		while(it!=fnpiList.end())
		{
			_ASSERT((*it).pFpi!=NULL) ;
			
			m_ConfirmReleaseFilePairInfoList.push_back(*it) ;
			
			FileCompletedPack.CompletedFileInfoList.push_back((*it).pFpi) ;
			++it ;
		}
	}

	// ���ļ��������,����뵽��ɶ��д�,������ EDC_FileOperationCompleted ����
	
	m_pDownstreamFilter->OnDataTrans(this,EDC_FileOperationCompleted,&FileCompletedPack) ;

#ifdef COMPILE_TEST_PERFORMANCE
	CptPerformanceCalcator::GetInstance()->EndCalAndSave(dw,99) ;
#endif

	Debug_Printf(_T("CXCLocalFileSyncSourceFilter::RoundOffReadFile() end")) ;
}
