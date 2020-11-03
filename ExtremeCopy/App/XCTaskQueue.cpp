/****************************************************************************

Copyright (c) 2008-2020 Kevin Wu (Wu Feng)

github: https://github.com/kevinwu1024/ExtremeCopy
site: http://www.easersoft.com

Licensed under the Apache License, Version 2.0 License (the "License"); you may not use this file except
in compliance with the License. You may obtain a copy of the License at

https://opensource.org/licenses/Apache-2.0

****************************************************************************/

#include "StdAfx.h"
#include "XCTaskQueue.h"

#ifdef VERSION_HDD_QUEUE

// ���ݴ洢��ʽ
//[���м�¼��HDD����(BYTE)] [��0��HDD��ID(DWORD)] [��0��HDD�� remain time(DWORD)] [��0��HDD�� �ȴ�task����(BYTE)] 
// [��0��HDD�� ��0��task id(BYTE)] ... [��0��HDD�� ��N��task id] [��1��HDD��ID] ...[��N��HDD�� ��N��task id]



static const TCHAR* const TASK_QUEUE_MUTEXT_NAME =  _T("ExtremeCopy-Task-Queue-Mutext") ;


CXCTaskQueue::CXCTaskQueue():m_hMutex(NULL),m_byCurTaskID(0)
{

}

CXCTaskQueue::~CXCTaskQueue() 
{
	if(m_hMutex!=NULL)
	{
		::CloseHandle(m_hMutex) ;
		m_hMutex = NULL ;
	}
}

int CXCTaskQueue::AddToQueue(pt_STL_vector(DWORD) StorageIDVer) 
{
	int nPos = -1 ;

	if(!StorageIDVer.empty() && this->Lock())
	{
		if(m_byCurTaskID==0)
		{
			if(++s_byTaskIDCounter==0)
			{
				++s_byTaskIDCounter ;
			}

			m_byCurTaskID = s_byTaskIDCounter ;
		}

		SStorageQueInfo sqi ;
		pt_STL_list(SStorageQueInfo) sqiList ;

		this->LoadData(sqiList) ;

		{
			for(size_t i=0;i<StorageIDVer.size();++i)
			{
				pt_STL_list(SStorageQueInfo)::iterator it = sqiList.begin() ;

				for(;it!=sqiList.end();++it)
				{// �ڻ��������ҳ���Ӧ hdd id ��
					if((*it).dwStorID==StorageIDVer[i])
					{
						break ;
					}
				}

				if(it==sqiList.end())
				{// ���û�ҵ�����ֱ�Ӽ���
					sqi.TaskIDQue.clear() ;

					sqi.dwStorID = StorageIDVer[i] ;
					sqi.dwRemainTime = 0 ;
					sqi.TaskIDQue.push_back(m_byCurTaskID) ;

					sqiList.push_back(sqi) ;

					if(nPos<0)
					{
						nPos = 0 ;
					}
				}
				else
				{
					(*it).TaskIDQue.push_back(m_byCurTaskID) ;

					const int pos = (int)(*it).TaskIDQue.size() ;

					if(nPos<pos-1)
					{
						nPos = pos-1 ;
					}
				}	
			}
		}

		this->PutData(sqiList) ;

		this->Unlock() ;
	}

	return nPos ;
}


int CXCTaskQueue::Wait(DWORD dwInterval) 
{
	int nRet = 0 ;

	if(this->Lock())
	{
		nRet = -1 ;
		pt_STL_list(SStorageQueInfo) sqiList ;

		this->LoadData(sqiList) ;

		this->Unlock() ;

		if(sqiList.empty())
		{
			nRet = 0 ;
		}
		else
		{
			pt_STL_list(SStorageQueInfo)::const_iterator it = sqiList.begin() ;

			// ���ҵ�ǰ task id ���ڵ�λ��
			for(;it!=sqiList.end();++it)
			{
				if(!(*it).TaskIDQue.empty())
				{
					int nPos = -1 ;
					pt_STL_list(BYTE)::const_iterator it2 =  (*it).TaskIDQue.begin();

					for(;it2!=(*it).TaskIDQue.end();++it2)
					{// 
						++nPos ;
						if((*it2)==m_byCurTaskID)
						{
							break ;
						}
					}

					// ������λ��Ϊ׼
					if(nPos>0 && nPos>nRet)
					{
						nRet = nPos ;
					}
				}
			}
		}
	}

	if(nRet>0 && dwInterval>0 && dwInterval!=INFINITE)
	{
		::Sleep(dwInterval) ;
	}

	return nRet ;
}

int CXCTaskQueue::GetWaitingTaskCount()
{
	int nRet = 0;

	do
	{
		PT_BREAK_IF(!this->Lock());

		nRet = s_pShareQueue[0];

		this->Unlock();
	}
	while(0);

	return nRet;
}

// �Ӷ������Ƴ���ǰ����
void CXCTaskQueue::RemoveFromQueue() 
{
	if(m_byCurTaskID>0 && this->Lock())
	{
		struct CRemoveEmptyHDD
		{
			bool operator()(const SStorageQueInfo& sqi)
			{
				return sqi.TaskIDQue.empty() ;
			}
		};

		pt_STL_list(SStorageQueInfo) sqiList ;

		this->LoadData(sqiList) ;

		if(!sqiList.empty())
		{
			pt_STL_list(SStorageQueInfo)::iterator it = sqiList.begin() ;

			for(;it!=sqiList.end();++it)
			{
				(*it).TaskIDQue.remove(m_byCurTaskID) ;
			}

			sqiList.remove_if(CRemoveEmptyHDD()) ;
		}

		this->PutData(sqiList) ;

		this->Unlock() ;
	}
	m_byCurTaskID = 0 ;
}

DWORD CXCTaskQueue::GetTimeLeftOfRunningTask() 
{
	DWORD dwRet = 0 ;

	if(this->Lock())
	{
		pt_STL_list(SStorageQueInfo) sqiList ;

		this->LoadData(sqiList) ;
		this->Unlock() ;

		if(!sqiList.empty())
		{// �������������
			pt_STL_list(SStorageQueInfo)::const_iterator it = sqiList.begin() ;

			for(;it!=sqiList.end();++it)
			{
				if(std::find((*it).TaskIDQue.begin(),(*it).TaskIDQue.end(),m_byCurTaskID)!=(*it).TaskIDQue.end())
				{// �� HDD �е�ǰ�����ڵȴ������ҳ���ʣ��ʱ��Ķ��٣��Եȴ���õ�Ϊ׼
					if((*it).dwRemainTime>dwRet)
					{
						dwRet = (*it).dwRemainTime ;
					}
				}
			}
		}
	}

	return dwRet ;
}

// �ѵ�ǰ�����ڶ������ƶ�
int CXCTaskQueue::MovePriority(bool bForwardOrBackward) 
{
	int nRet = -1 ;

	if(this->Lock())
	{
		pt_STL_list(SStorageQueInfo) sqiList ;

		this->LoadData(sqiList) ;

		if(!sqiList.empty())
		{
			pt_STL_list(SStorageQueInfo)::iterator it = sqiList.begin() ;

			int nPos = -1 ;
			bool bContinue = true ;
			for(;it!=sqiList.end() && bContinue;++it)
			{// �������� HDD ID
				nPos = 0 ;
				pt_STL_list(BYTE)::iterator it2 = (*it).TaskIDQue.begin() ;

				for(;it2!=(*it).TaskIDQue.end() && bContinue;++it2)
				{
					if((*it2)==m_byCurTaskID)
					{// ������ֵ�ǰ�������ڵĶ���
						if(nPos>0)
						{
							pt_STL_list(BYTE)::iterator it3 = it2 ;
							if(bForwardOrBackward)
							{
								if(nPos>1)
								{
									--nPos ;
									--it3 ;
									int tem = (*it2) ;
									(*it2) = (*it3) ;
									(*it3) = tem ;
								}
							}
							else
							{
								if(++it3!=(*it).TaskIDQue.end())
								{
									++nPos ;
									int tem = (*it2) ;
									(*it2) = (*it3) ;
									(*it3) = tem ;
								}
							}
						}

						if(nPos>nRet)
						{
							nRet = nPos ;
						}

						break ;
					}

					++nPos ;
				}
			}

			this->PutData(sqiList) ;
		}

		this->Unlock() ;
	}

	return nRet ;
}

void CXCTaskQueue::UpdateRunningTaskRemainTime(DWORD dwRemainTime)
{
	if(this->Lock())
	{
		pt_STL_list(SStorageQueInfo) sqiList ;

		this->LoadData(sqiList) ;

		if(!sqiList.empty())
		{
			pt_STL_list(SStorageQueInfo)::iterator it = sqiList.begin() ;

			for(;it!=sqiList.end();++it)
			{// �������� HDD ID
				if(!(*it).TaskIDQue.empty() && (*(*it).TaskIDQue.begin())==m_byCurTaskID)
				{
					(*it).dwRemainTime = dwRemainTime ;
				}
			}
		}

		this->PutData(sqiList) ;

		this->Unlock() ;
	}
}

bool CXCTaskQueue::LoadData(pt_STL_list(SStorageQueInfo)& sqiList) 
{
	bool bRet = false ;

	sqiList.clear() ;
	bRet = true ;

	if(s_pShareQueue[0]>0)
	{
		SStorageQueInfo sqi ;

		int nOffset = 0 ;
		BYTE byHDDNum = 0 ;
		BYTE byTaskNum = 0 ;
		BYTE byTaskID = 0 ;
		byHDDNum = s_pShareQueue[0] ;
		nOffset += sizeof(BYTE) ;

		for(BYTE i=0;i<byHDDNum;++i)
		{
			byTaskNum = 0 ;

			sqi.dwStorID = *(DWORD*)(s_pShareQueue+nOffset) ; 
			nOffset += sizeof(DWORD) ;

			sqi.dwRemainTime = *(DWORD*)(s_pShareQueue+nOffset) ;
			nOffset += sizeof(DWORD) ;

			byTaskNum = *(BYTE*)(s_pShareQueue+nOffset) ;
			nOffset += sizeof(BYTE) ;

			if(byTaskNum>0)
			{
				sqi.TaskIDQue.clear() ;

				for(BYTE j=0;j<byTaskNum;++j)
				{
					byTaskID = *(BYTE*)(s_pShareQueue+nOffset) ;
					nOffset += sizeof(BYTE) ;

					sqi.TaskIDQue.push_back(byTaskID) ;
				}

				sqiList.push_back(sqi) ;
			}
		}
	}

	return bRet ;
}

int CXCTaskQueue::PutData(const pt_STL_list(SStorageQueInfo)& sqiList) 
{
	bool bRet = false ;

	int nTotalSize = 0 ;
	// ����ռ���ڴ�ռ����
	int nHDDNum = 0 ;
	int nTotalTaskIDNum = 0 ;

	if(!sqiList.empty())
	{
		pt_STL_list(SStorageQueInfo)::const_iterator it = sqiList.begin() ;

		for(;it!=sqiList.end();++it)
		{
			++nHDDNum ;
			nTotalTaskIDNum += (int)(*it).TaskIDQue.size() ;
		}

		nTotalSize = sizeof(BYTE) + nHDDNum*(2*sizeof(DWORD)+sizeof(BYTE)) + nTotalTaskIDNum*sizeof(BYTE) ;

		if(nTotalSize<=s_ShareMemTaskQueSize)
		{// ������㹻�Ŀռ䣬��д��ȥ
			bRet = true ;
		}
	}

	if(bRet )
	{
		int nOffset = 0 ;
		if(nHDDNum==0)
		{
			s_pShareQueue[0] = 0 ;
		}
		else
		{
			BYTE byTaskNum = 0 ;
			s_pShareQueue[0] = nHDDNum ;
			nOffset += sizeof(BYTE) ;

			pt_STL_list(SStorageQueInfo)::const_iterator it = sqiList.begin() ;

			for(;it!=sqiList.end();++it)
			{
				byTaskNum = (BYTE)(*it).TaskIDQue.size() ;

				if(byTaskNum>0)
				{
					*(DWORD*)(s_pShareQueue+nOffset) = (*it).dwStorID ;
					nOffset += sizeof(DWORD) ;

					*(DWORD*)(s_pShareQueue+nOffset) = (*it).dwRemainTime ;
					nOffset += sizeof(DWORD) ;

					*(BYTE*)(s_pShareQueue+nOffset) = byTaskNum ;
					nOffset += sizeof(BYTE) ;

					pt_STL_list(BYTE)::const_iterator it2 = (*it).TaskIDQue.begin() ;

					for(;it2!=(*it).TaskIDQue.end();++it2)
					{
						*(BYTE*)(s_pShareQueue+nOffset) = (*it2) ;
						nOffset += sizeof(BYTE) ;
					}
				}

			}
		}

	}

	return bRet ;
}

bool CXCTaskQueue::Lock() 
{

OWNERSHIP_AGAIN:
	m_hMutex = ::CreateMutex(NULL,TRUE,TASK_QUEUE_MUTEXT_NAME) ;

	if(m_hMutex!=NULL && ::GetLastError()==ERROR_ALREADY_EXISTS)
	{// ��ǰ�߳�δ�ܷ������ݣ�ֱ����һ�����ͷŸû�����
		::WaitForSingleObject(m_hMutex,100) ;
		::CloseHandle(m_hMutex) ;
		m_hMutex = NULL ;
		goto OWNERSHIP_AGAIN ;
	}

	return (m_hMutex!=NULL) ;
}

void CXCTaskQueue::Unlock() 
{
	if(m_hMutex!=NULL)
	{
		::ReleaseMutex(m_hMutex) ;
		::CloseHandle(m_hMutex) ;
		m_hMutex = NULL ;
	}
}

#endif 