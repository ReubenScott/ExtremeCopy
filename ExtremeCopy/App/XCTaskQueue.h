/****************************************************************************

Copyright (c) 2008-2020 Kevin Wu (Wu Feng)

github: https://github.com/kevinwu1024/ExtremeCopy
site: http://www.easersoft.com

Licensed under the Apache License, Version 2.0 License (the "License"); you may not use this file except
in compliance with the License. You may obtain a copy of the License at

https://opensource.org/licenses/Apache-2.0

****************************************************************************/

#pragma once

#ifdef VERSION_HDD_QUEUE

#include "../Common/ptTypeDef.h"

#include <vector>
#include <list>


const int s_ShareMemTaskQueSize = 1024 ;

#pragma data_seg("XCTaskQueueAA") // "XCTaskQueue"  Ϊ�Զ���
static BYTE s_pShareQueue[s_ShareMemTaskQueSize] = {0} ;
static BYTE	s_byTaskIDCounter = 0 ;
#pragma data_seg()
#pragma comment(linker,"/SECTION:XCTaskQueueAA,RWS")


class CXCTaskQueue
{
public:

	CXCTaskQueue() ;
	CXCTaskQueue(const CXCTaskQueue&) ;

	virtual ~CXCTaskQueue() ;

	// �ѵ�ǰ�������漰��HDD ID��������У����ص����ڶ��е�˳��
	int AddToQueue(pt_STL_vector(DWORD) StorageIDVer) ;

	// �ȴ��ڶ������Ƿ��ֵ��Լ��������ڶ����е�λ��
	int Wait(DWORD dwInterval) ;

	// �Ӷ������Ƴ���ǰ����
	void RemoveFromQueue() ;

	// �ڶ�����ǰ����������е�����ʣ��ʱ�䣬����ʣ��ʱ�䣬����Ϊ��λ
	DWORD GetTimeLeftOfRunningTask() ;

	// �ѵ�ǰ�����ڶ������ƶ������ص����ڶ��е�λ��
	int MovePriority(bool bForwardOrBackward) ;

	int GetWaitingTaskCount();

	//
	void UpdateRunningTaskRemainTime(DWORD dwRemainTime) ;

private:
	struct SStorageQueInfo
	{
		DWORD dwStorID ;
		DWORD dwRemainTime ;
		pt_STL_list(BYTE)	TaskIDQue ;
	};

public:
	bool LoadData(pt_STL_list(SStorageQueInfo)& sqiList) ;
	int PutData(const pt_STL_list(SStorageQueInfo)& sqiList) ;

	bool Lock() ;
	void Unlock() ;

private:
	HANDLE		m_hMutex;
	BYTE		m_byCurTaskID ;
};

#endif