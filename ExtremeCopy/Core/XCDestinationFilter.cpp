/****************************************************************************

Copyright (c) 2008-2020 Kevin Wu (Wu Feng)

github: https://github.com/kevinwu1024/ExtremeCopy
site: http://www.easersoft.com

Licensed under the Apache License, Version 2.0 License (the "License"); you may not use this file except
in compliance with the License. You may obtain a copy of the License at

https://opensource.org/licenses/Apache-2.0

****************************************************************************/

#include "StdAfx.h"
#include "XCDestinationFilter.h"
#include "XCCopyingEvent.h"
#include "..\Common\ptWinPath.h"
#include "XCFileDataBuffer.h"
#include "..\Common\ptGlobal.h"
#include <deque>
#include "../Common/ptDebugView.h"


int CXCDestinationFilter::m_sDstFilterIDCounter = 0 ;

CXCDestinationFilter::CXCDestinationFilter(CXCCopyingEvent* pEvent,bool bIsRenameCopy):CXCFilter(pEvent),m_bIsRenameCopy(bIsRenameCopy)
{
	m_nDstFilterID = ++m_sDstFilterIDCounter ;
}


CXCDestinationFilter::~CXCDestinationFilter(void)
{
}


void CXCDestinationFilter::ResetCanCallbackMark() 
{
	m_sDstFilterIDCounter = 0 ;
}

// ��Ϊ�����ж�·�� destination filter ͬʱд�룬��ֻ�е�һ���ž��лص����ݵ��ʸ�
bool CXCDestinationFilter::CanCallbackFileInfo() const 
{
	return (m_nDstFilterID==1) ;
}


bool CXCDestinationFilter::OnInitialize() 
{
	return CXCFilter::OnInitialize() ;
}

bool CXCDestinationFilter::OnContinue() 
{
	return CXCFilter::OnContinue() ;
}

bool CXCDestinationFilter::OnPause()
{
	return CXCFilter::OnPause() ;
}

void CXCDestinationFilter::OnStop()
{
	CXCFilter::OnStop() ;
}

//===============================


CXCLocalFileDestnationFilter::CXCLocalFileDestnationFilter(CXCCopyingEvent* pEvent,const CptString strDestRoot,const SStorageInfoOfFile& siof,
	const bool bIsRenameCopy)
	:CXCDestinationFilter(pEvent,bIsRenameCopy),m_pUpstreamFilter(NULL)
	,m_hCurFileHandle(INVALID_HANDLE_VALUE),
	m_strDestRoot(strDestRoot),m_pImpactFileBehavior(NULL),m_StorageInfo(siof)
{
	m_bIsDriverRoot = (strDestRoot.GetAt(strDestRoot.GetLength()-2) == ':') ;

	m_CurFileIterator = m_FileInfoList.end() ;
}

CXCLocalFileDestnationFilter::~CXCLocalFileDestnationFilter(void)
{
	pt_STL_list(SDstFileInfo)::iterator DstFileIt = m_FileInfoList.begin() ;

	for(;DstFileIt!=m_FileInfoList.end();++DstFileIt)
	{
		if((*DstFileIt).hFile!=INVALID_HANDLE_VALUE)
		{
			::CloseHandle((*DstFileIt).hFile) ;
		}

		CptGlobal::ForceDeleteFile((*DstFileIt).strFileName) ;
	}

	m_FileInfoList.clear() ;
}


ErrorHandlingResult CXCLocalFileDestnationFilter::GetErrorHandleResult(SDataPack_SourceFileInfo& sfi,CptString strDestFile)
{
	SXCExceptionInfo ei ;
	ei.ErrorCode.nSystemError = ::GetLastError() ;
	ei.SupportType = ErrorHandlingFlag_RetryIgnoreCancel ;
	ei.strDstFile = strDestFile ;
	ei.uFileID = sfi.uFileID ;
	ei.strSrcFile = sfi.strSourceFile ;

	ErrorHandlingResult result = m_pEvent->XCOperation_CopyExcetption(ei) ;

	if(result==ErrorHandlingFlag_Ignore)
	{
		m_pEvent->XCOperation_RecordError(ei) ;	
	}

	return result ;
}

bool CXCLocalFileDestnationFilter::OnInitialize() 
{
	return CXCDestinationFilter::OnInitialize() ;
}

bool CXCLocalFileDestnationFilter::OnContinue() 
{
	return CXCDestinationFilter::OnContinue() ;
}

bool CXCLocalFileDestnationFilter::OnPause()
{
	return CXCDestinationFilter::OnPause() ;
}

void CXCLocalFileDestnationFilter::OnStop()
{
	if(m_hCurFileHandle!=INVALID_HANDLE_VALUE)
	{
		::CancelIo(m_hCurFileHandle) ;
	}

	// �ѻ�û����ȫд����̵��ļ�ɾ����
	bool bDelete = false ;
	bool bOverCurIt = false ;

	pt_STL_list(SDstFileInfo)::iterator it = m_FileInfoList.begin() ;

	for(;it!=m_FileInfoList.end();)
	{
		bDelete = true ;

		// ��δ֪ΪʲôҪ�����Ե�ǰ�ļ���ɾ��
		// ���Ȱ����Լ������ bDelete = bOverCurIt ;һ��ע��
		//if(!bOverCurIt && it==m_CurFileIterator)
		//{
		//	bOverCurIt = true ;
		//}

		if((*it).hFile!=INVALID_HANDLE_VALUE)
		{
			if((*it).uRemainSize==0 && (*it).bNoBuf )
			{
				_ASSERT(!bOverCurIt) ;
				::SetFilePointer((*it).hFile, 0, NULL, FILE_BEGIN);
				::SetEndOfFile((*it).hFile) ;
				bDelete = false ;// ����ɵ��ļ���ɾ��
			}

			::CloseHandle((*it).hFile) ;
			(*it).hFile = INVALID_HANDLE_VALUE ;

			//bDelete = bOverCurIt ;
		}
		else
		{
			// ����ļ����ڴ���ǰ����Ϊͬ����ԭ�������������Ӧ��ɾ��
			if((*it).pSfi->IsDiscard() && (*it).uRemainSize==0)
			{
				bDelete = false ;
			}
		}

		if(bDelete)
		{
			CptGlobal::ForceDeleteFile((*it).strFileName.c_str()) ;
		}

		++it ;
	}

	m_FileInfoList.clear() ;

	Debug_Printf(_T("CXCLocalFileDestnationFilter::OnStop() end")) ;

	CXCDestinationFilter::OnStop() ;

	Debug_Printf(_T("CXCLocalFileDestnationFilter::OnStop() end")) ;
}

int CXCLocalFileDestnationFilter::OnCreateXCFile(SDataPack_CreateFileInfo& cfi) 
{
	int nRet = 0 ;

	SActiveFilesInfo bfi ;
	CptString strDstFile ;

	pt_STL_list(SActiveFilesInfo) ActiveFileInfoQue ;
	bool bFolder = false ;
	bool bDiscardInSrcOpen = false ;

	if(!cfi.SourceFileInfoList.empty())
	{
		pt_STL_list(SDataPack_SourceFileInfo*)::iterator it = cfi.SourceFileInfoList.begin() ;

		//for(size_t i=0;i<cfi.SourceFileInfoVer.size() && this->IsValideRunningState();++i)
		for(;it!=cfi.SourceFileInfoList.end() && this->IsValideRunningState();++it)
		{
			if((*it)==NULL)
			{
				this->m_DestURI.Pop() ; // ���� URI Ŀ¼ջ
			}
			else 
			{// �ļ�
				bDiscardInSrcOpen = (*it)->IsDiscard() ; //��Ϊ�� CreateXCFile() ����Ҳ���� bDiscard Ϊtrue��
														// ������������ж��Ƿ���source filter����������

				nRet = this->CreateXCFile(*(*it),strDstFile) ;

				bFolder = CptGlobal::IsFolder((*it)->dwSourceAttr) ;

				/**
				// �ݲ������ļ�������
				
				if(bFolder && cfi.SourceFileInfoVer[i]->bDiscard)
				{// ��Ϊ���������ļ���,���ڸ��ļ����ڵ������ļ������ļ��о���������
					// ��Ϊ���ļ��б�������������CreateXCFile()��û�б���Ŀ¼ջ����������Ҳ���ó�Ŀ¼ջ

					// �����¼�ڱ��������ļ��л��������Ա� EDC_BatchCreateFile �����ʱ��
					// source filter ֪������Щ�ļ��б�����
//					cfi.DiscardFolderVer.push_back(cfi.SourceFileInfoVer[i]->strSourceFile) ;

					int nDirStack = 1 ;

					for(size_t j=i+1;j<cfi.SourceFileInfoVer.size();++j)
					{
						if(cfi.SourceFileInfoVer[j]!=NULL)
						{
							if(cfi.SourceFileInfoVer[j]->strSourceFile.GetLength()>
								cfi.SourceFileInfoVer[i]->strSourceFile.GetLength())
							{// ��Ϊ��Ҫ������������Ŀ¼����ô�ļ������ȱ�Ȼ���ڸ�Ŀ¼�����ȣ�����·����
								if(cfi.SourceFileInfoVer[j]->strSourceFile.Left(cfi.SourceFileInfoVer[i]->strSourceFile.GetLength()).CompareNoCase(
									cfi.SourceFileInfoVer[i]->strSourceFile)!=0)
								{
									_ASSERT(FALSE) ;
								}
								else
								{
									if(CptGlobal::IsFolder(cfi.SourceFileInfoVer[j]->dwSourceAttr))
									{// ��Ҫ������Ŀ¼������Ŀ¼����Ŀ¼ջ��1
										++nDirStack ;
									}

									cfi.SourceFileInfoVer[j]->bDiscard = true ;
								}
							}
							else
							{
								_ASSERT(FALSE) ;
							}
						}
						else
						{// ��Ϊ��Ŀ¼ջ,ֱ��������Ҫ�������ļ���ƥ��ĳ�Ŀ¼ջ���˳���ѭ��
							if(--nDirStack<=0)
							{
								i = j -1;
								break ;
							}
						}
					}
				}
				else 
					/**/
				if(!bFolder && strDstFile.GetLength()>0 && !bDiscardInSrcOpen)
				{// ��Ϊ�ļ�,�Ҳ�����source filter���ѱ�����
					bfi.strDstFile = strDstFile ;
					bfi.strSrcFile = (*it)->strSourceFile ;
					bfi.uFileID = (*it)->uFileID ;
					bfi.uFileSize = (*it)->nFileSize ;

					ActiveFileInfoQue.push_back(bfi) ;
				}
			}
		}
	}

	if(nRet==0 && !ActiveFileInfoQue.empty())
	{
		if(this->CanCallbackFileInfo())
		{
			this->m_pEvent->XCOperation_FileBegin(ActiveFileInfoQue) ;
		}
	}

	return nRet ;
}

int CXCLocalFileDestnationFilter::CreateXCFile(SDataPack_SourceFileInfo& sfi,CptString& strOutDstFile) 
{
	int nRet = 0 ;

	strOutDstFile = _T("") ;
	//Debug_Printf(_T("CXCLocalFileDestnationFilter::CreateXCFile() 1 id=%u name=%s"),sfi.uFileID,sfi.strSourceFile) ;

	if(!this->IsValideRunningState())
	{
		return ErrorHandlingFlag_Exit ;
	}

	SDstFileInfo dfi ;

#ifdef COMPILE_TEST_PERFORMANCE
	DWORD dw53 = CptPerformanceCalcator::GetInstance()->BeginCal() ;
#endif
	CptString strDestFile ;

	if(this->IsRenameExe())
	{// ����ǡ�������ʽ���ƻ����ƶ�
		strDestFile = m_strDestRoot ;//MAKE_FILE_FULL_NAME(m_strDstFileName) ;
	}
	else
	{
		CptString strFileName ;

		if(sfi.strNewFileName.GetLength()>0)
		{
			strFileName = sfi.strNewFileName ;
		}
		else
		{
			strFileName = ::GetRawFileName(sfi.strSourceFile) ;
		}
		
		strDestFile = this->MakeFileFullName(strFileName) ;//MAKE_FILE_FULL_NAME(::GetRawFileName(sfi.strSourceFile)) ;
	}

#ifdef COMPILE_TEST_PERFORMANCE
	CptPerformanceCalcator::GetInstance()->EndCalAndSave(dw53,23) ;
#endif

	if(!sfi.IsDiscard())
	{// ��Ϊ�����ļ��������Ϊ�첽��
		// �����ļ���source filter����������ֻ���ڶ�ȡ��һ��ʱ���������
		// Ȼ����һ��ʱ���destination filter���յ����ļ��Ĵ��������������������
		
		if(*m_pImpactFileBehavior!=SFDB_Replace && !CptGlobal::IsFolder(sfi.dwSourceAttr))
		{// ���������ͻ�ļ����Ǹ��ǣ���ô�Ͳ�����Ƿ���ڳ�ͻ�ļ�
			bool bIsDestExist = IsFileExist(strDestFile.c_str()) ;

			if(bIsDestExist)
			{// Ŀ���ļ����Ѵ��ڣ���ص�����һ��ѯ��

				SImpactFileResult result ;
				
				if(this->m_pEvent!=NULL)
				{
					if(*m_pImpactFileBehavior==SFDB_Default || *m_pImpactFileBehavior==SFDB_Rename)
					{// ����Ĭ�ϣ������������ϻص�
						SImpactFileInfo ImpactInfo ;
						ImpactInfo.ImpactType = IT_SameFileName ;

						ImpactInfo.strSrcFile = sfi.strSourceFile ;
						ImpactInfo.strDestFile = strDestFile ;

						m_pEvent->XCOperation_ImpactFile(ImpactInfo,result) ;
					}
					else
					{
						result.result = *m_pImpactFileBehavior ;
					}
				}

				if(result.bAlways)
				{
					*m_pImpactFileBehavior = result.result ;
				}

				switch(result.result)
				{
				case SFDB_Skip: // ����
					{// ���������������˰�discard����Ϊtrue�⣬ҲӦ�ü��뵽 m_FileInfoList ��������
						//sfi.bDiscard = true ;
						sfi.SetDiscard(true) ;

						dfi.hFile = INVALID_HANDLE_VALUE ;
						dfi.bNoBuf = false ;
						dfi.strFileName = strDestFile ;
						dfi.pSfi = &sfi ; // ����ָ�뱣������

						m_FileInfoList.push_back(dfi) ;
					}
					
					return nRet ;

				case SFDB_Default:
				case SFDB_Rename: // ����
					strDestFile = result.strNewDstFileName ;
					break ;

				case SFDB_Replace: // ����
					break ;

				default:
				case SFDB_StopCopy: // �˳�
					*m_pRunningState = CFS_ReadyStop ;
					return ErrorHandlingFlag_Exit ;
				}
			}
		}
		
		if(CptGlobal::IsFolder(sfi.dwSourceAttr))
		{// �����ļ���

//EXCEPTION_RETRY_CREATEFOLDER:
			
			BOOL bDirectResult = FALSE ;

			if(sfi.strSourceFile.CompareNoCase(strDestFile)==0)
			{// ��������ļ��и����Ļ�
				SImpactFileInfo ImpactInfo ;
				SImpactFileResult result ;
				ImpactInfo.ImpactType = IT_SameFileName ;

				ImpactInfo.strSrcFile = sfi.strSourceFile ;
				ImpactInfo.strDestFile = strDestFile ;

				m_pEvent->XCOperation_ImpactFile(ImpactInfo,result) ;

				strDestFile = result.strNewDstFileName ;
			}

			if(sfi.IsLocal())
			{
				bDirectResult = ::CreateDirectoryEx(sfi.strSourceFile.c_str(),strDestFile.c_str(),NULL) ;
			}
			else
			{
				bDirectResult = ::CreateDirectory(strDestFile.c_str(),NULL) ;
			}
			
			if(!bDirectResult && ::GetLastError()!=ERROR_ALREADY_EXISTS)
			{// �����ļ���ʧ��, Ŀǰ���������ļ���ʧ�ܣ���ֱ���˳�

				SXCExceptionInfo ei ;
				ei.ErrorCode.nSystemError = ::GetLastError() ;
				ei.SupportType = ErrorHandlingFlag_Exit ;
				ei.strDstFile = strDestFile ;
				ei.uFileID = sfi.uFileID ;
				ei.strSrcFile = sfi.strSourceFile ;

				this->m_pEvent->XCOperation_CopyExcetption(ei) ;

				return ErrorHandlingFlag_Exit ;
			}

			if(!sfi.IsDiscard())
			{// �����ļ��д����ɹ����Ҳ�������,��ô������URI��ջ
				m_DestURI.Push(GetRawFileName(strDestFile)) ;
			}

			dfi.strFileName = strDestFile ;
			//dfi.pSfi->uFileID = sfi.uFileID ;
			dfi.uRemainSize = 0 ;
			dfi.hFile = INVALID_HANDLE_VALUE ;
			dfi.pSfi = &sfi ; // ����ָ�뱣������

		}
		else
		{// �����ļ�

			//Debug_Printf(_T("CXCLocalFileDestnationFilter::CreateXCFile() 4 create file")) ;

#ifdef COMPILE_TEST_PERFORMANCE
		DWORD dw52 = CptPerformanceCalcator::GetInstance()->BeginCal() ;
#endif

			dfi.bNoBuf = (m_StorageInfo.uDiskType!=DRIVE_REMOTE && (sfi.nFileSize >= 64*1024 || (sfi.nFileSize%m_StorageInfo.nSectorSize)==0)) ;

			dfi.strFileName = strDestFile ;
			dfi.pSfi = &sfi ; // ����ָ�뱣������

			DWORD dwFlag = (dfi.bNoBuf ? FILE_FLAG_NO_BUFFERING : 0) | m_nCreateFileFlag;// FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OVERLAPPED;

			strOutDstFile = strDestFile ;

#ifdef COMPILE_TEST_PERFORMANCE
		CptPerformanceCalcator::GetInstance()->EndCalAndSave(dw52,22) ;
#endif

EXCEPTION_RETRY_CREATEDESTFILE:// ���ԣ�����Ŀ���ļ���

			
#ifdef COMPILE_TEST_PERFORMANCE
		DWORD dw5 = CptPerformanceCalcator::GetInstance()->BeginCal() ;
#endif
			dfi.hFile = ::CreateFile(strDestFile.c_str(),GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,dwFlag,NULL) ;

			if(dfi.hFile==INVALID_HANDLE_VALUE && IsFileExist(strDestFile.c_str()))
			{
				::SetFileAttributes(strDestFile.c_str(), FILE_ATTRIBUTE_NORMAL);
				dfi.hFile = ::CreateFile(strDestFile.c_str(),GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,dwFlag,NULL) ;
			}

			if(dfi.hFile!=INVALID_HANDLE_VALUE)
			{
				if(dfi.bNoBuf)
				{
					int nOver = sfi.nFileSize%m_StorageInfo.nSectorSize ;

					dfi.uRemainSize = (nOver==0 ? sfi.nFileSize : sfi.nFileSize+m_StorageInfo.nSectorSize - nOver);
				}
				else
				{
					dfi.uRemainSize = sfi.nFileSize ;
				}

				if(dfi.uRemainSize>m_nSwapChunkSize)
				{
					unsigned __int64 dwLow = dfi.bNoBuf ? ALIGN_SIZE_UP(dfi.uRemainSize,m_StorageInfo.nSectorSize) : dfi.uRemainSize ;
					DWORD dwHi = (DWORD)(dwLow>>32) ;

EXCEPTION_RETRY_ALLOCATEFILESIZE:// ���ԣ������ļ���С��
					if(::SetFilePointer(dfi.hFile, (DWORD)dwLow, (PLONG)&dwHi, FILE_BEGIN)!=INVALID_SET_FILE_POINTER
						&& ::SetEndOfFile(dfi.hFile))
					{
						::SetFilePointer(dfi.hFile, 0, NULL, FILE_BEGIN);
					}
					else
					{
						int nError = ::GetLastError() ;

						if(this->m_pEvent!=NULL)
						{
							switch(this->GetErrorHandleResult(sfi,strDestFile))
							{
							case ErrorHandlingFlag_Ignore: // ����
								strOutDstFile = _T("") ;
								sfi.SetDiscard(true) ;
								nRet = ErrorHandlingFlag_Ignore ;
								break ;

							case ErrorHandlingFlag_Retry: // ���ԣ������ļ���С��
								goto EXCEPTION_RETRY_ALLOCATEFILESIZE ;

							default:
							case ErrorHandlingFlag_Exit: // �˳�
								strOutDstFile = _T("") ;
								sfi.SetDiscard(true) ;
								*this->m_pRunningState = CFS_ReadyStop ;

								// �������˳��Ļ�����Ӧ�Ѹոմ������ļ�ɾ����
								if(dfi.hFile!=INVALID_HANDLE_VALUE)
								{
									::CloseHandle(dfi.hFile) ;
									dfi.hFile = INVALID_HANDLE_VALUE ;
									::DeleteFile(strDestFile.c_str()) ;
								}
								return ErrorHandlingFlag_Exit ;
							}
						}
					}
				}
			}
			else
			{
				int nError = ::GetLastError() ;

				if(this->m_pEvent!=NULL)
				{
					switch(this->GetErrorHandleResult(sfi,strDestFile))
					{
					case ErrorHandlingFlag_Ignore: // ����
						//sfi.bDiscard = true ;
						sfi.SetDiscard(true) ;
						nRet = ErrorHandlingFlag_Ignore ;
						break ;

					case ErrorHandlingFlag_Retry: // ���ԣ�����Ŀ���ļ���
						goto EXCEPTION_RETRY_CREATEDESTFILE ;

					default:
					case ErrorHandlingFlag_Exit: // �˳�
						//sfi.bDiscard = true ;
						sfi.SetDiscard(true) ;
						*this->m_pRunningState = CFS_ReadyStop ;
						return ErrorHandlingFlag_Exit ;
					}
				}
			}

#ifdef COMPILE_TEST_PERFORMANCE
		CptPerformanceCalcator::GetInstance()->EndCalAndSave(dw5,5) ;
#endif
		}

		// �����Ǵ����ļ��ɹ�������ﶼ�����������
	}
	else
	{// ������ļ��Ǳ�������ҲӦ�÷��뵽 m_FileInfoList ��ȥ
		
		if(sfi.IsError())
		{// ������ļ��ڴ���ʱ�ѳ���,������¼����

			SXCExceptionInfo ei ;

			ei.ErrorCode.nSystemError = sfi.nSysErrCode ;
			ei.strSrcFile = sfi.strSourceFile ;
			ei.strDstFile = strDestFile ;
			ei.uFileID = sfi.uFileID ;

			this->m_pEvent->XCOperation_RecordError(ei) ;
		}

		dfi.uRemainSize = 0 ;
		dfi.hFile = INVALID_HANDLE_VALUE ;
		dfi.pSfi = &sfi ; // ����ָ�뱣������
	}

	if(CptGlobal::IsFolder(dfi.pSfi->dwSourceAttr))
	{
		m_FolderInfoList.push_back(dfi) ;
	}
	else
	{
		m_FileInfoList.push_back(dfi) ;
	}
	
	return nRet ;
}



void CXCLocalFileDestnationFilter::RoundOffFile(pt_STL_list(SDataPack_SourceFileInfo*)& FileList )
{
	pt_STL_list(SDataPack_SourceFileInfo*)::const_iterator it = FileList.begin() ;

	SDataPack_FileDoneConfirm fdc ;
	
	bool bIsRunning = true ;

	SFileEndedInfo fei ;

	bool bFound = false ;

	pt_STL_vector(SFileEndedInfo) FeiVer ;
	pt_STL_list(HANDLE) ReadyCloseHandleList ;

	pt_STL_list(SDstFileInfo)::iterator DstFileIt ;

	for(;it!=FileList.end() && bIsRunning;++it )
	{
		bIsRunning = this->IsValideRunningState() ;

		fdc.FileDoneConfirmList.push_back((*it)->uFileID) ; // �ռ�������source filter�ͷ���Ӧ�ļ���Դ���ļ�ID

		bFound = false ;

		{// ���Ҷ�Ӧ�� file ID
			if(CptGlobal::IsFolder((*it)->dwSourceAttr))
			{// �ļ���
				_ASSERT(!m_FolderInfoList.empty()) ;

				//DstFileIt = m_FolderInfoList.rbegin() ;
				DstFileIt = m_FolderInfoList.begin() ;

				if((*DstFileIt).pSfi->uFileID!=(*it)->uFileID)
				{
					bFound = false ;

					for(;DstFileIt!=m_FolderInfoList.end();++DstFileIt)
					{
						if((*DstFileIt).pSfi->uFileID==(*it)->uFileID)
						{
							bFound = true ;
							break ;
						}
					}

					_ASSERT(bFound) ;
				}
				else
				{
					bFound = true ;
				}
			}
			else
			{// �ļ�

				_ASSERT(!m_FileInfoList.empty()) ;

				DstFileIt = m_FileInfoList.begin() ;

				if((*DstFileIt).pSfi->uFileID!=(*it)->uFileID)
				{
					bFound = false ;

					for(;DstFileIt!=m_CurFileIterator;++DstFileIt)
					{
						if((*DstFileIt).pSfi->uFileID==(*it)->uFileID)
						{
							bFound = true ;
							break ;
						}
					}

					_ASSERT(bFound) ;
				}
				else
				{
					bFound = true ;
				}
			}
		}
		

		if(!(*it)->IsDiscard())
		{
			//Win_Debug_Printf(_T("CXCLocalFileDestnationFilter::RoundOffFile() id=%u"),(*it)->uFileID) ;

			//if(it2!=m_FileInfoMap.end())
			if(bFound)
			{
				if(CptGlobal::IsFolder((*it)->dwSourceAttr))
				{// �ļ���
					//if(!(*it)->bLocal)
					if(!(*it)->IsLocal())
					{
						(*DstFileIt).hFile = ::CreateFile((*DstFileIt).strFileName,GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL) ;

						_ASSERT((*DstFileIt).hFile!=NULL) ;

						if((*DstFileIt).hFile!=INVALID_HANDLE_VALUE)
						{   
							BOOL b = ::SetFileTime((*DstFileIt).hFile,&(*it)->SrcFileTime.CreateTime,&(*it)->SrcFileTime.LastAccessTime,&(*it)->SrcFileTime.LastWriteTime) ;

							BOOL b2 = ::CloseHandle((*DstFileIt).hFile) ;

							//it2->second.hFile = INVALID_HANDLE_VALUE ;
							//if(!(*it)->bLocal)
							if(!(*it)->IsLocal())
							{// Դ�ļ��зǱ����ļ�
								BOOL b3 = ::SetFileAttributes((*DstFileIt).strFileName.c_str(),(*it)->dwSourceAttr) ;

								//DWORD dwNew = ::GetFileAttributes(it2->second.strFileName.c_str()) & ~FILE_ATTRIBUTE_COMPRESSED ;

							}
							//::SHChangeNotify(SHCNE_ALLEVENTS,SHCNF_PATH|SHCNF_FLUSH,it2->second.strFileName.c_str(),NULL) ;

						}
					}

					//if(m_FolderInfoList.back()==DstFileIt)
					//{
					//	m_FolderInfoList.pop_back() ;
					//}
					//else
					{
						m_FolderInfoList.erase(DstFileIt) ;
					}
					
				}
				else
				{
					if(INVALID_HANDLE_VALUE!=(*DstFileIt).hFile && 0==(*DstFileIt).uRemainSize && bIsRunning)
					{// ���ļ������д�뵽���̲���Ϊ��Ч�ľ������Ϊ���Դ�ļ���СΪ0�Ļ����� uRemainSize Ҳ��Ϊ0��

						{// �����д���ļ����뻺�����������洦�ص�
//							fei.strFileName = (*DstFileIt).strFileName ;
							fei.uFileID = (*DstFileIt).pSfi->uFileID;

							FeiVer.push_back(fei) ;
						}

#ifdef COMPILE_TEST_PERFORMANCE
							DWORD dw = CptPerformanceCalcator::GetInstance()->BeginCal() ;
#endif

							/**
						if(it2->second.bNoBuf && (*it)->nFileSize%m_StorageInfo.nSectorSize)
						{// ������Ҫ���´��ļ����
							::CloseHandle(it2->second.hFile) ;

							it2->second.hFile = ::CreateFile(it2->second.strFileName.c_str(),GENERIC_WRITE,FILE_SHARE_READ,NULL,OPEN_EXISTING,
								FILE_FLAG_SEQUENTIAL_SCAN,NULL) ;
						}

						if(m_StorageInfo.nSectorSize>0 && it2->second.uRemainSize!=0)
						{// �����ļ���Ҫ��������

							_ASSERT(it2->second.hFile!=INVALID_HANDLE_VALUE) ;

							DWORD dwLow = (DWORD)(*it)->nFileSize;
							DWORD dwHi = (DWORD)((*it)->nFileSize>>32) ;

							BOOL b2 = (::SetFilePointer(it2->second.hFile, dwLow, (PLONG)&dwHi, FILE_BEGIN)==INVALID_SET_FILE_POINTER);
							BOOL b = ::SetEndOfFile(it2->second.hFile) ;
						}
						/**/

						/**/
						if((*DstFileIt).bNoBuf && (*it)->nFileSize>0 && m_StorageInfo.nSectorSize>0 
							&& (*it)->nFileSize%m_StorageInfo.nSectorSize )
						{// �����ļ���Ҫ��������
							//HANDLE hFile = it2->second.hFile ;
							//::FlushFileBuffers(it2->second.hFile) ;

							if((*DstFileIt).bNoBuf && (*it)->nFileSize%m_StorageInfo.nSectorSize)
							{
								::CloseHandle((*DstFileIt).hFile) ;

								(*DstFileIt).hFile = ::CreateFile((*DstFileIt).strFileName.c_str(),GENERIC_WRITE,FILE_SHARE_READ,NULL,OPEN_EXISTING,
									FILE_FLAG_SEQUENTIAL_SCAN,NULL) ;
							}

							if((*DstFileIt).hFile!=INVALID_HANDLE_VALUE)
							{
								DWORD dwLow = (DWORD)(*it)->nFileSize;
								DWORD dwHi = (DWORD)((*it)->nFileSize>>32) ;

								BOOL b2 = (::SetFilePointer((*DstFileIt).hFile, dwLow, (PLONG)&dwHi, FILE_BEGIN)==INVALID_SET_FILE_POINTER);
								BOOL b = ::SetEndOfFile((*DstFileIt).hFile) ;

								//Release_Printf(_T("SetFilePointer()=%d SetEndOfFile()=%d"),b2,b) ;
							}
						}
/**/
						::SetFileTime((*DstFileIt).hFile,&(*it)->SrcFileTime.CreateTime,&(*it)->SrcFileTime.LastAccessTime,&(*it)->SrcFileTime.LastWriteTime) ;
						//
						//ReadyCloseHandleList.push_back(it2->second.hFile) ;
						::CloseHandle((*DstFileIt).hFile) ;

						if ((*it)->dwSourceAttr & (FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_SYSTEM)) 
						{
							::SetFileAttributes((*DstFileIt).strFileName.c_str(),(*it)->dwSourceAttr) ;
						}
						
#ifdef COMPILE_TEST_PERFORMANCE
							CptPerformanceCalcator::GetInstance()->EndCalAndSave(dw,6) ;
#endif
					}
					else
					{// �ļ���δ��ȫд�����

						if((*DstFileIt).hFile!=INVALID_HANDLE_VALUE)
						{
							::CloseHandle((*DstFileIt).hFile) ;
						}

						//ReadyCloseHandleList.push_back(it2->second.hFile) ;
						CptGlobal::ForceDeleteFile((*DstFileIt).strFileName) ;
					}

					if(DstFileIt==m_CurFileIterator) 
					{
						++m_CurFileIterator ;
					}

					m_FileInfoList.erase(DstFileIt) ;
				}
			}
			else
			{// ���ǲ�Ӧ���Ҳ����ģ��������������ļ�����m_FileInfoLis��Ҳ���ҵ�����μ� 'ExtremeCopy ԭ��.docx' ����㷨����
				_ASSERT(FALSE) ;
				//Release_Printf(_T("RoundOffFile() not found")) ;
				//fdc.FileDoneConfirmList.pop_back() ;
			}
		}
		else
		{
			//Release_Printf(_T("CXCLocalFileDestnationFilter::RoundOffFile() discard  111")) ;

			if(bFound)
			{
				//Release_Printf(_T("CXCLocalFileDestnationFilter::RoundOffFile() discard  222")) ;

				if(!CptGlobal::IsFolder((*it)->dwSourceAttr))
				{
					//Release_Printf(_T("CXCLocalFileDestnationFilter::RoundOffFile() discard  333")) ;

					if((*DstFileIt).hFile!=INVALID_HANDLE_VALUE)
					{
						::CloseHandle((*DstFileIt).hFile) ;

						// ֻ�����Ѵ򿪵��ļ�����ɾ������Ϊ���ܵ�����ʱ������ͬ�ļ�������ʱ�û����'skip'�Ļ�����Ŀ���ļ��еĸ��ļ�����Ч�ģ������ǲ�Ӧ��ɾ����
						CptGlobal::ForceDeleteFile((*DstFileIt).strFileName) ; 
					}

				}

				if(DstFileIt==m_CurFileIterator) 
				{
					++m_CurFileIterator ;
				}

				m_FileInfoList.erase(DstFileIt) ;

			}
			else
			{
				_ASSERT(FALSE) ;

			}
			//Release_Printf(_T("RoundOffFile() meet dicards file")) ;
		}

		
	}// end for

	if(m_pEvent!=NULL && !FeiVer.empty())
	{// �����д���ļ����ϻص�
		m_pEvent->XCOperation_FileEnd(FeiVer) ;
	}

	if(!fdc.FileDoneConfirmList.empty())
	{// �����ļ����ȷ������
		m_pUpstreamFilter->OnDataTrans(this,EDC_FileDoneConfirm,&fdc) ;
	}

	//Win_Debug_Printf(_T("CXCLocalFileDestnationFilter::RoundOffFile() end")) ;
}


inline CptString CXCLocalFileDestnationFilter::MakeFileFullName(CptString strRawFileName)
{
	CptString strURI = m_DestURI.GetURI() ;
	CptString strRet ;

	if(m_bIsDriverRoot && strURI.GetLength()==0)
	{
		strRet = m_strDestRoot + strRawFileName  ;
	}
	else
	{
		CptString strRootWithoutSlash = m_strDestRoot ;

		strRootWithoutSlash.TrimRight('\\') ;
		strRootWithoutSlash.TrimRight('/') ;

		strRet = strRootWithoutSlash + strURI + _T("\\") + strRawFileName ;
	}

	return strRet ;
}

bool CXCLocalFileDestnationFilter::Connect(CXCFilter* pFilter,bool bUpstream) 
{
	bool bRet = false ;

	if(bUpstream && pFilter!=NULL && m_pUpstreamFilter==NULL)
	{
		m_pUpstreamFilter = pFilter ;
		pFilter->Connect(this,false) ;
		bRet = false ;
	}

	return bRet ;
}

int CXCLocalFileDestnationFilter::OnDataTrans(CXCFilterEventCB* pSender,EFilterCmd cmd,void* pFileData)
{
	int nRet = 0 ;

	switch(cmd)
	{

	case EDC_Continue:
		{
			return this->OnContinue() ? 0 : 1 ;
		}
		break ;

	case EDC_Pause:
		{
			return this->OnPause() ? 0 : 1 ;
		}
		break ;

	case EDC_Stop:
		this->OnStop() ;
		break ;

	case EDC_LinkIni: // �µ�FILTER LINK
		{
			SDataPack_LinkIni* pLinkIni = (SDataPack_LinkIni*)pFileData ;
			m_DestURI.Clean() ;

			_ASSERT(pLinkIni->pDefaultImpactFileBehavior!=NULL) ;

			//m_nSwapChunkSize = pLinkIni->pFileDataBuf->GetPageSize() ;
			m_nSwapChunkSize = pLinkIni->pFileDataBuf->GetChunkSize() ;

			m_pRunningState = pLinkIni->pCFState ;
			m_pFileDataBuf = pLinkIni->pFileDataBuf ;
			m_pFileChangingBuffer = pLinkIni->pFileChangingBuf ;
			m_pImpactFileBehavior = pLinkIni->pDefaultImpactFileBehavior ;

			this->OnInitialize() ;
		}
		break ;

	//case EDC_CreateFile:// �����ļ�
	//	{
	//		_ASSERT(pFileData!=	NULL) ;
	//		SDataPack_SourceFileInfo* pSfi = (SDataPack_SourceFileInfo*)pFileData ;

	//		nRet = this->CreateXCFile(*pSfi) ;
	//	}
	//	break ;

	case EDC_BatchCreateFile: // ���������ļ�
		{
			_ASSERT(pFileData!=NULL) ;

			//Release_Printf(_T("CXCLocalFileDestnationFilter::OnDataTrans() EDC_BatchCreateFile")) ;

			SDataPack_CreateFileInfo* pCfi = (SDataPack_CreateFileInfo*)pFileData ;

#ifdef COMPILE_TEST_PERFORMANCE
			DWORD dw = CptPerformanceCalcator::GetInstance()->BeginCal() ;
#endif
			this->OnCreateXCFile(*pCfi) ;

#ifdef COMPILE_TEST_PERFORMANCE
			CptPerformanceCalcator::GetInstance()->EndCalAndSave(dw,16) ;
#endif
		}
		break ;

	case EDC_FileData: // �ļ�����
		{
			if(pFileData!=NULL)
			{
				SDataPack_FileData* pFD = (SDataPack_FileData*)pFileData ;
				//_ASSERT(pFD!=NULL) ;

				int nRet = this->WriteFileData(*pFD) ;
				
				//if(nRet==0 && m_pCurDstFileInfo!=NULL && !pFD->bDiscard)
				//{
				//	if((*m_CurFileIterator).uFileID==pFD->uFileID && (*m_CurFileIterator).uRemainSize<=pFD->nDataSize)
				//	{
				//		m_pEvent->XCOperation_FileDataDone(pFD->uFileID) ;
				//	}
				//}

				return nRet ;
			}
		}
		break ;

	case EDC_FileHash: // �ļ���HASH ֵ
		break ;


	case EDC_FileOperationCompleted: // �����ڸ��ļ��Ĳ��������
		{// ���ļ���ɨβ����

			Debug_Printf(_T("CXCLocalFileDestnationFilter::OnDataTrans() EDC_FileOperationCompleted 1")) ;

			SDataPack_FileOperationCompleted* pFOC = (SDataPack_FileOperationCompleted*)pFileData ;

			_ASSERT(pFOC!=NULL) ;
			this->RoundOffFile(pFOC->CompletedFileInfoList) ;

			Debug_Printf(_T("CXCLocalFileDestnationFilter::OnDataTrans() EDC_FileOperationCompleted 2")) ;
			return 0 ;
		}
		break ;

	//case EDC_FolderChildrenOperationCompleted: // �����ڸ��ļ��е����ļ����ļ��в��������
	//	{
	//		this->m_DestURI.Pop() ; // ���� URI ջ
	//	}
	//	break ;

	case EDC_LinkEnded: // ���еĸ��ƹ���׼����ɣ�������ɨβ����
		{
			SDataPack_FileOperationCompleted* pFOC = (SDataPack_FileOperationCompleted*)pFileData ;

			_ASSERT(pFOC!=NULL) ;

			this->OnLinkEnded(pFOC->CompletedFileInfoList) ;
		}
		
		break ;
	}

	return nRet ;
}


CXCLocalFileDestnationFilter::CXCDirectoryURI::CXCDirectoryURI() 
{
	m_bNewestURI = true ;
}

void CXCLocalFileDestnationFilter::CXCDirectoryURI::Clean() 
{
	m_URIDeque.clear() ;
	m_bNewestURI = false ;
}

void CXCLocalFileDestnationFilter::CXCDirectoryURI::Push(CptString strFolderName) 
{
	m_URIDeque.push_back(strFolderName) ;
	m_bNewestURI = false ;
}

void CXCLocalFileDestnationFilter::CXCDirectoryURI::Pop() 
{
	if(!m_URIDeque.empty())
	{
		m_URIDeque.pop_back() ;
		m_bNewestURI = false ;
	}
}

CptString CXCLocalFileDestnationFilter::CXCDirectoryURI::GetURI() 
{
	if(!m_bNewestURI)
	{
		m_strLastURI = _T("") ;

		if(!m_URIDeque.empty())
		{
			pt_STL_deque(CptString)::const_iterator it = m_URIDeque.begin() ;

			while(it!=m_URIDeque.end())
			{
				m_strLastURI += _T("\\") ;
				m_strLastURI += (*it) ;

				++it ;
			}
		}

		m_bNewestURI = true ;
	}

	return m_strLastURI ;
}

//CXCLocalFileDestnationFilter::CDstFileInfo::CDstFileInfo(const bool bSequence):m_bSequence(bSequence)
//{
//}
//
//SDstFileInfo* CXCLocalFileDestnationFilter::CDstFileInfo::GetFileInfoByID(const unsigned& uFileID) 
//{
//	
//}
//
//SDstFileInfo* CXCLocalFileDestnationFilter::CDstFileInfo::GetCurFileInfo() 
//{
//}
//
//void CXCLocalFileDestnationFilter::CDstFileInfo::AddDstFileInfo(const SDstFileInfo& dfi) 
//{
//}
//
//void CXCLocalFileDestnationFilter::CDstFileInfo::RemoveDstFileInfoByID(const unsigned& uFileID) 
//{
//}