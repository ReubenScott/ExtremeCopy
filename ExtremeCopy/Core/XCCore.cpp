/****************************************************************************

Copyright (c) 2008-2020 Kevin Wu (Wu Feng)

github: https://github.com/kevinwu1024/ExtremeCopy
site: http://www.easersoft.com

Licensed under the Apache License, Version 2.0 License (the "License"); you may not use this file except
in compliance with the License. You may obtain a copy of the License at

https://opensource.org/licenses/Apache-2.0

****************************************************************************/

#include "StdAfx.h"
#include "XCCore.h"
#include "XCWinStorageRelative.h"
#include "XCAsyncFileDataTransFilter.h"
#include "XCSyncFileDataTransFilter.h"
#include "XCDuplicateOutputTransFilter.h"
#include "XCLocalFileSyncSourceFilter.h"
#include "XCLocalFileSyncDestnationFilter.h"
#include "XCLocalFileAsynSourceFilter.h"
#include "XCLocalFileAsynDestnationFilter2.h"
#include "XCCopyingEvent.h"

#include <shlobj.h>

#pragma comment(lib,"Winmm.lib")

//============================================


CXCCore::CXCCore(void):m_bIni(false)
{
}


CXCCore::~CXCCore(void)
{
	SDataPack_SourceFileInfo::ReleaseBuffer() ; // �ͷŻ������ڴ�
}

bool CXCCore::Run(const SGraphTaskDesc& gtd)
{
	bool bRet = false ;

	if(!m_bIni)
	{
		bRet = true ;
		m_bIni = true ;

		m_ImpactFileBehavior = gtd.ImpactFileBehavior ;
		m_RunningState = CFS_Stop ;
		CXCDestinationFilter::ResetCanCallbackMark() ;

		Debug_Printf(_T("CXCCore::Run() 1")) ;

		if(gtd.ExeType==XCTT_Move && gtd.DstFolderVer.size()==1)
		{// ��ͬһ�������ƶ��ļ�
			SStorageInfoOfFile SrcSiof ;

			SStorageInfoOfFile DstSiof ;

			CXCWinStorageRelative::GetFileStoreInfo(gtd.DstFolderVer[0],DstSiof) ;
			size_t i = 0 ;

			SGraphTaskDesc NormalGtd = gtd;
			SGraphTaskDesc SameMovingGtd = gtd;

			NormalGtd.SrcFileVer.clear() ;
			SameMovingGtd.SrcFileVer.clear() ;

			// ��Դ�ļ����Ƿ�ͬһ�������и��ļ���ֿ������Ա����Բ�ͬ�ĸ��Ʋ���
			for( i=0;i<gtd.SrcFileVer.size();++i)
			{
				if(CXCWinStorageRelative::GetFileStoreInfo(gtd.SrcFileVer[i],SrcSiof))
				{
					if(SrcSiof.dwStorageID!=DstSiof.dwStorageID 
						|| SrcSiof.nPartitionIndex!=DstSiof.nPartitionIndex
						|| SrcSiof.nSectorSize!=DstSiof.nSectorSize
						|| SrcSiof.uDiskType!=DstSiof.uDiskType)
					{
						NormalGtd.SrcFileVer.push_back(gtd.SrcFileVer[i]) ;
					}
					else
					{
						SameMovingGtd.SrcFileVer.push_back(gtd.SrcFileVer[i]) ;
					}
				}
				else
				{
					_ASSERT(FALSE) ;
				}
			}

			// ��ͬһ�������Խ����ƶ�����
			if(!SameMovingGtd.SrcFileVer.empty())
			{
				CXCSameDriveMoveFile same(&m_RunningState,&m_ImpactFileBehavior) ;

				bRet = same.Run(SameMovingGtd) ;
			}

			// ����ͨ����ģʽ���и���
			if(bRet && !NormalGtd.SrcFileVer.empty())
			{
				bRet = this->NormalRun(NormalGtd) ;
			}
		}
		else
		{
			bRet = this->NormalRun(gtd) ;
		}

		this->Stop() ;
	}

	return bRet ;
}

bool CXCCore::NormalRun(const SGraphTaskDesc& gtd) 
{
	bool bRet = this->CreateLinkGraph(gtd,m_GraphSet) ;

	if(bRet)
	{
		bRet = m_GraphSet.Start() ;
	}

	return bRet ;
}


void CXCCore::Stop() 
{
	CptAutoLock lock(&m_Lock) ;

	if(m_bIni)
	{
		m_bIni = false ;
		m_GraphSet.Stop() ;

		CXCDestinationFilter::ResetCanCallbackMark() ;
	}

}

void CXCCore::Pause() 
{
	CptAutoLock lock(&m_Lock) ;

	if(m_bIni)
	{
		m_GraphSet.Pause() ;
	}
}

void CXCCore::Continue() 
{
	CptAutoLock lock(&m_Lock) ;

	if(m_bIni)
	{
		m_GraphSet.Continue() ;
	}
}

bool CXCCore::Skip()
{
	CptAutoLock lock(&m_Lock) ;

	return (m_bIni && m_GraphSet.Skip()) ;
}

CptString CXCCore::GetCurWillSkipFile() 
{
	CptAutoLock lock(&m_Lock) ;

	return m_GraphSet.GetCurWillSkipFile() ;
}

ECopyFileState CXCCore::GetState()  
{
	CptAutoLock lock(&m_Lock) ;
	return m_GraphSet.GetState() ;
}

// �����쳣
bool CXCCore::ProcessException(CXCCopyingEvent* pEvent)
{
	SXCExceptionInfo ei ;
	ei.ErrorCode.nSystemError = ::GetLastError() ;

	ei.SupportType = ErrorHandlingFlag_Exit|ErrorHandlingFlag_Retry ;

	ErrorHandlingResult result = pEvent->XCOperation_CopyExcetption(ei) ;

	return (result==ErrorHandlingFlag_Retry) ;
}

// �����ļ����ݻ�����
bool CXCCore::AllocFileDataBuffer(const SGraphTaskDesc& gtd)
{
	if(!m_FileDataBuffer.IsAllocateChunk()
		|| m_FileDataBuffer.GetChunkSize()!=gtd.nFileDataBufSize)
	{// ֻ����һ�Σ��Ե�һ������Ϊ׼
		while(!m_FileDataBuffer.AllocateChunk(gtd.nFileDataBufSize,2*1024))
		{
			// �����ڴ�ʧ�ܺ�,������û���δ���
			// �û����ԣ����� �� �˳�����
			if(!this->ProcessException(gtd.pCopyingEvent))
			{
				return false ;
			}
		}
	}

	return true ;
}

// ����������·
bool CXCCore::CreateLinkGraph(const SGraphTaskDesc& gtd,CXCGraphSet& GraphSet)
{
	SLinkDesc ld ;

	bool bRet = false ;

	do
	{
		// ����������Ϣ�Ӷ����� ���ƹ�����
		PT_BREAK_IF(!this->ParseTaskToGraphDesc(gtd,ld)) ;

		// �����ļ������ڴ�
		PT_BREAK_IF(!this->AllocFileDataBuffer(gtd)) ;

		// ��λ �ļ������ڴ�
		m_FileDataBuffer.ResetRef() ;

		CXCSourceFilter* pSourceFilter = NULL ;
		CXCDestinationFilter* pDestFilter = NULL ;

		CXCTransformFilter* pTransFilter = NULL ;
		CXCDuplicateOutputTransFilter* pDupTransFilter = NULL ;

		int nIndex = 0 ;

		CXCGraph LinkGraph ;
		SXCSourceFilterTaskInfo sfti ;

		sfti.pFileDataBuf = &m_FileDataBuffer ;
		sfti.pFileChangingBuf = &m_FileChangingBuffer ;
		sfti.nValidCachePointNum = (int)ld.BranchLinkVer.size() ;
		sfti.ExeType = gtd.ExeType ;
		sfti.pDefaultImpactFileBehavior = &m_ImpactFileBehavior ;
		sfti.pRunningState = &m_RunningState ;

		int nSameDeviceCount = 0 ;
		int nDiffDeviceCount = 0 ;

		auto it = ld.SrcLinkVer.begin() ;

		// Ĭ��Ϊ�ɹ�
		bRet = true ;

		for(;it!=ld.SrcLinkVer.end();++it)
		{
			const SFileStorageInfo& fsi = (*it) ;
			// ���� source filter 
			while(bRet && (pSourceFilter = new CXCLocalFileSyncSourceFilter(gtd.pCopyingEvent)) ==NULL)
			{
				if(!this->ProcessException(gtd.pCopyingEvent))
				{
					LinkGraph.Release() ;
					GraphSet.Stop() ;
					bRet = false ;
				}
			}

			PT_BREAK_IF(!bRet) ; // ������ source filter ʧ�ܣ����˳�

			sfti.SrcFileVer = fsi.strFileOrFolderVer ;

			pSourceFilter->SetTask(sfti) ; // ָ���������

			_ASSERT(pSourceFilter!=NULL) ;

			LinkGraph.AddFilter(pSourceFilter) ; // ��� source filter �� graph link

			if(ld.BranchLinkVer.size()>1)
			{// �ж����֧, �򴴽� duplicate transfer filter

				// ���� duplicate transfer filter
				while(bRet && (pDupTransFilter = new CXCDuplicateOutputTransFilter(gtd.pCopyingEvent)) ==NULL)
				{
					if(!this->ProcessException(gtd.pCopyingEvent))
					{
						LinkGraph.Release() ;
						GraphSet.Stop() ;
						bRet = false ;
					}
				}

				PT_BREAK_IF(!bRet) ; // ������ dulplicate ��֧ʧ�ܣ����˳�

				LinkGraph.AddFilter(pDupTransFilter) ; // ��� duplicate transfer filter �� graph link
			}

			for(size_t j=0;bRet && j<ld.BranchLinkVer.size();++j)
			{// ���Ӹ�����֧
				if(ld.BranchLinkVer[j].siof.IsSameStorage(fsi.siof))
				{// ͬһ������洢��
					++nSameDeviceCount ;

					// ����ͬ���� transfer filter
					while(bRet && (pTransFilter = new CXCSyncFileDataTransFilter(gtd.pCopyingEvent)) ==NULL)
					{
						if(!this->ProcessException(gtd.pCopyingEvent))
						{
							LinkGraph.Release() ;
							GraphSet.Stop() ;
							bRet = false ;
						}
					}

					PT_BREAK_IF(!bRet) ; // ������ filter ʧ�ܣ����˳�
				}
				else
				{// ��ͬ������洢��
					++nDiffDeviceCount ;

					// �����첽�� transfer filter
					while(bRet && (pTransFilter = new CXCAsyncFileDataTransFilter(gtd.pCopyingEvent)) ==NULL)
					{
						if(!this->ProcessException(gtd.pCopyingEvent))
						{
							LinkGraph.Release() ;
							GraphSet.Stop() ;
							bRet = false ;
						}
					}

					PT_BREAK_IF(!bRet) ; // ������ transfer filter ʧ�ܣ����˳�
				}

				//  ���� destination filter
				while(bRet && (pDestFilter = new CXCLocalFileSyncDestnationFilter(gtd.pCopyingEvent,
					ld.BranchLinkVer[j].strFileOrFolderVer[0],ld.BranchLinkVer[j].siof,
					gtd.bIsRenameDst)) ==NULL)
				{
					if(!this->ProcessException(gtd.pCopyingEvent))
					{
						LinkGraph.Release() ;
						GraphSet.Stop() ;
						bRet = false ;
					}
				}

				PT_BREAK_IF(!bRet) ; // ������ destination filter ʧ�ܣ����˳�

				_ASSERT(pTransFilter!=NULL) ;

				LinkGraph.AddFilter(pTransFilter) ; // ��� transfer filter �� graph link

				_ASSERT(pDestFilter!=NULL) ;

				pDestFilter->Connect(pTransFilter,true) ;

				LinkGraph.AddFilter(pDestFilter) ; // ��� destination filter �� graph link

				if(pDupTransFilter!=NULL)
				{// ������֧��duplicate transfer filter�������֧����
					pDupTransFilter->Connect(pTransFilter,false) ;
				}
			}// for() ��������֧

			PT_BREAK_IF(!bRet) ; // ������֧ʧ�ܣ����˳�
			
			if(nSameDeviceCount==0)
			{
				pSourceFilter->SetSmallReadGranularity(true) ;
			}

			if(pDupTransFilter!=NULL)
			{// ������duplicate,�� �� source filter �� duplicate transfer filter ����
				pSourceFilter->Connect(pDupTransFilter,false) ;
			}
			else
			{// ��source filter �� ��һ��transfer ����
				pSourceFilter->Connect(pTransFilter,false) ;
			}

			GraphSet.AddGraph(LinkGraph) ;
			LinkGraph.Clear() ;
		}
	}
	while(0);

	return bRet ;
}


bool CXCCore::ParseTaskToGraphDesc(const SGraphTaskDesc& gtd,SLinkDesc& ld)
{
	bool bRet = false ;

	if(!gtd.SrcFileVer.empty() && !gtd.DstFolderVer.empty())
	{
		SFileStorageInfo fsi ;

		for(size_t j=0;j<gtd.DstFolderVer.size();++j)
		{
			fsi.strFileOrFolderVer.clear() ;
			if(CXCWinStorageRelative::GetFileStoreInfo(gtd.DstFolderVer[j],fsi.siof))
			{// ����destination filter��֧��
				if(fsi.siof.uDiskType==DRIVE_CDROM)
				{// ��Ŀ�����Ϊ��������ô����ʧ���˳�

					if(gtd.pCopyingEvent!=NULL)
					{
						SXCExceptionInfo ei ;

						ei.ErrorCode.nSystemError = ERROR_BAD_COMMAND ;
						ei.strDstFile = gtd.DstFolderVer[j] ;
						ei.SupportType = ErrorHandlingFlag_Exit ;

						gtd.pCopyingEvent->XCOperation_CopyExcetption(ei) ;
					}

					return false ;
				}

				fsi.strFileOrFolderVer.push_back(gtd.DstFolderVer[j]) ;
				ld.BranchLinkVer.push_back(fsi) ;
			}
			else
			{
				_ASSERT(FALSE) ;
			}
		}

		bool bAdded = false ;

		for(size_t i=0;i<gtd.SrcFileVer.size();++i)
		{// ���� link ��Ŀ
			bAdded = false ;

			const bool& bSuccess = CXCWinStorageRelative::GetFileStoreInfo(gtd.SrcFileVer[i],fsi.siof) ;

			_ASSERT(bSuccess) ;
			
			fsi.strFileOrFolderVer.clear() ;

			for(size_t j=0;j<ld.SrcLinkVer.size();++j)
			{
				if(ld.SrcLinkVer[j].siof.IsSameStorage(fsi.siof))
				{
					ld.SrcLinkVer[j].strFileOrFolderVer.push_back(gtd.SrcFileVer[i]) ;
					bAdded = true ;
					break ;
				}
			}

			if(!bAdded)
			{
				fsi.strFileOrFolderVer.push_back(gtd.SrcFileVer[i]) ;
				ld.SrcLinkVer.push_back(fsi) ;
			}
		}

		bRet = true ;
	}

	return bRet ;
}



//====================== CXCGraphSet ============

CXCCore::CXCGraphSet::CXCGraphSet():m_nCurIndex(0)
{
}

CXCCore::CXCGraphSet::~CXCGraphSet() 
{
	this->Stop() ;
}

int CXCCore::CXCGraphSet::GetCount()
{
	return (int)m_GraphList.size() ;
}

void CXCCore::CXCGraphSet::Release()
{
	pt_STL_list(CXCGraph)::iterator it = m_GraphList.begin() ;

	while(it!=m_GraphList.end())
	{
		(*it).Stop() ;
		(*it).Release() ;
		++it ;
	}

	m_GraphList.clear() ;
}

bool CXCCore::CXCGraphSet::Start() 
{
	if(this->GetState()!=CFS_Stop)
	{
		return false ;
	}

	if(this->GetState()==CFS_Stop)
	{
		CXCGraph* pGraph = NULL ;
		const size_t& nGraphCount = m_GraphList.size() ;

		// ��ÿһ��������·���г�ʼ��
		for(size_t i=0;i<nGraphCount;++i)
		{
			pGraph = this->GetCurGraph() ;

			_ASSERT(pGraph!=NULL) ;

			if(pGraph!=NULL && !pGraph->Initialize())
			{
				return false ;
			}

			++m_nCurIndex ; // ��������һ�� Graph
		}

		m_nCurIndex = 0 ;

		// �������������·
		for(size_t i=0;i<nGraphCount;++i)
		{
			pGraph = this->GetCurGraph() ;

			_ASSERT(pGraph!=NULL) ;

			if(pGraph!=NULL && !pGraph->Run())
			{
				return false ;
			}

			// ���������� release �� m_GraphList.clear(), ��ΪCXCCore::CXCGraphSet::Start() �Ǳ� CXCCore::Run() ���õ�
			// �����������һ���ļ��պ����ʱ���û������Skip()������ÿ����Ϊ���������������̱߳���
			// ��ǡ�����CXCCore::Run()û�У����Ծͻ����Skip()�����󲢲���֤source filter������
			// ���Բ��������� Release()�� ��Ӧ�� CXCCore::Stop() ��Release����ΪStop()���̰߳�ȫ��
			//pGraph->Release() ;

			++m_nCurIndex ; // ��������һ�� Graph
		}

		//m_GraphList.clear() ;
	}

	return true ;
}

bool CXCCore::CXCGraphSet::Pause() 
{
	CXCGraph* pGraph = this->GetCurGraph() ;

	return ((pGraph!=NULL) && pGraph->Pause()) ;
}

bool CXCCore::CXCGraphSet::Skip() 
{
	CXCGraph* pGraph = this->GetCurGraph() ;

	return ((pGraph!=NULL) && pGraph->Skip()) ;
}

bool CXCCore::CXCGraphSet::Continue()
{
	CXCGraph* pGraph = this->GetCurGraph() ;

	return ((pGraph!=NULL) && pGraph->Continue()) ;
}

void CXCCore::CXCGraphSet::Stop() 
{
	CXCGraph* pGraph = this->GetCurGraph() ;

	if(pGraph!=NULL)
	{
		pGraph->Stop() ;
	}

	m_nCurIndex = 0 ;

	this->Release() ;
}

CptString CXCCore::CXCGraphSet::GetCurWillSkipFile() 
{
	CptString strRet ;

	CXCGraph* pGraph = this->GetCurGraph() ;

	if(pGraph!=NULL)
	{
		strRet = pGraph->GetCurWillSkipFile() ;
	}

	return strRet ;
}

ECopyFileState CXCCore::CXCGraphSet::GetState()  
{
	CXCGraph* pGraph = this->GetCurGraph() ;

	ECopyFileState ret = CFS_Stop ;

	if(pGraph!=NULL)
	{
		ret = pGraph->GetState() ;
	}
	//_ASSERT(pGraph!=NULL) ;

	return ret ;
}

void CXCCore::CXCGraphSet::AddGraph(CXCGraph& graph) 
{
	m_GraphList.push_back(graph) ;
}

CXCCore::CXCGraph* CXCCore::CXCGraphSet::GetCurGraph() 
{
	int nCount = 0 ;

	pt_STL_list(CXCGraph)::iterator it = m_GraphList.begin() ;

	while(it!=m_GraphList.end())
	{
		if(m_nCurIndex==nCount++)
		{
			return &(*it) ;
		}
		++it ;
	}

	return NULL ;
}

//======================= CXCGraph ================

CXCCore::CXCGraph::CXCGraph():pSourceFilter(NULL)
{
}

CXCCore::CXCGraph::~CXCGraph()
{
}

void CXCCore::CXCGraph::Release()
{
	auto it = FilterList.begin() ;

	while(it!=FilterList.end())
	{
		CXCFilter* pp = *it ;

		_ASSERT(pp!=NULL) ;

		delete pp ;
		++it ;
	}

	FilterList.clear() ;
}

void CXCCore::CXCGraph::AddFilter(CXCFilter* pFilter)
{
	FilterList.push_back(pFilter) ;

	if(pFilter->GetType()==FilterType_Source)
	{// ��Ϊ source filter
		pSourceFilter = (CXCSourceFilter*)pFilter ;
	}
}

void CXCCore::CXCGraph::Clear() 
{
	FilterList.clear() ;
}

bool CXCCore::CXCGraph::Initialize() 
{
	return true ;
}

bool CXCCore::CXCGraph::Run() 
{
	if(!FilterList.empty())
	{
		if(pSourceFilter!=NULL)
		{
			return pSourceFilter->Run() ;
		}
	}

	return false ;
}

bool CXCCore::CXCGraph::Pause()
{
	if(pSourceFilter!=NULL)
	{
		return pSourceFilter->Pause() ;
	}

	return false ;
}

void CXCCore::CXCGraph::Stop()
{
	if(pSourceFilter!=NULL)
	{
		pSourceFilter->Stop() ;
	}
}

bool CXCCore::CXCGraph::Skip() 
{
	if(pSourceFilter!=NULL)
	{
		return pSourceFilter->Skip() ;
	}
	return false ;
}

bool CXCCore::CXCGraph::Continue() 
{
	if(pSourceFilter!=NULL)
	{
		return pSourceFilter->Continue() ;
	}
	return false ;
}

CptString CXCCore::CXCGraph::GetCurWillSkipFile() 
{
	CptString strRet ;

	if(pSourceFilter!=NULL)
	{
		SDataPack_SourceFileInfo* pSfi = pSourceFilter->GetCurrentSFI() ;

		if(pSfi!=NULL)
		{
			strRet = pSfi->strSourceFile ;
		}
	}

	return strRet ;
}

ECopyFileState CXCCore::CXCGraph::GetState() const
{
	if(pSourceFilter!=NULL)
	{
		return pSourceFilter->GetState() ;
	}

	return CFS_Stop ;
}