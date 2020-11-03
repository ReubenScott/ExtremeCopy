/****************************************************************************

Copyright (c) 2008-2020 Kevin Wu (Wu Feng)

github: https://github.com/kevinwu1024/ExtremeCopy
site: http://www.easersoft.com

Licensed under the Apache License, Version 2.0 License (the "License"); you may not use this file except
in compliance with the License. You may obtain a copy of the License at

https://opensource.org/licenses/Apache-2.0

****************************************************************************/

#pragma once

#include "XCCoreDefine.h"
#include <map>
#include "..\Common\ptTypeDef.h"
#include "..\Common\ptThreadLock.h"
#include "../Common/ptDebugView.h"
#include <deque>


class CXCCopyingEventReceiver ;

class CXCCopyingEvent
{
public:
	struct SRenamedFileNameInfo
	{
		CptString strOldFileName ;
		bool	bSameFile ;
	};

	enum EEventType
	{
		ET_GetState,
		ET_Exception,
		ET_ImpactFile,
		ET_WriteLog,
		ET_GetRenamedFileName,

		ET_CopyBatchFilesBegin,
		ET_CreateDestinationFile,
		ET_CopyOneFileBegin,
		ET_CopyFileEnd,
		ET_CopyFileDataDone, // file data done �� file end �ǲ�ͬ�ģ�
							// done��д�뵽�ļ���������ȫ��д���ˣ�����û�н������ļ��Ĳ�����
							// ��end ����ȫ�������˸��ļ��Ĳ���
		ET_CopyFileDataOccur,
		ET_CopyFileDiscard,
		ET_RecordError,
	};

	CXCCopyingEvent(void);
	~CXCCopyingEvent(void);

	CXCCopyingEventReceiver* SetReceiver(EEventType et,CXCCopyingEventReceiver* pRecv) ;

	// д��־
	void WriteLog(const TCHAR* pcFormat,...) ;

	// ��ȡ��ǰExtremeCopy ����״̬
	ECopyFileState GetState() ;

	// ��ExtremeCopy���� Pause ״̬ʱ���ú�����ֱ�����Pause״̬�ŷ���
	bool WaitForXCContinue() ;
	
	void XCOperation_ImpactFile(const SImpactFileInfo& ImpactInfo,SImpactFileResult& ifr) ;

	void XCOperation_FileBegin(const pt_STL_list(SActiveFilesInfo)& FilePairInfoQue) ;
	void XCOperation_FileEnd(const pt_STL_vector(SFileEndedInfo)& FileEndedInfoVer) ;
	void XCOperation_FileDataOccured(const SFileDataOccuredInfo& fdoi) ;
	void XCOperation_FileDiscard(const SDataPack_SourceFileInfo* pSfi,const unsigned __int64& uDiscardFileSize) ;
	void XCOperation_RecordError(const SXCExceptionInfo& ec) ;

	ErrorHandlingResult XCOperation_CopyExcetption(const SXCExceptionInfo& ec) ;

private:
	inline CXCCopyingEventReceiver* GetRecer(const EEventType et) ;
private:
	pt_STL_map(EEventType,CXCCopyingEventReceiver*) 	m_RecerMap ;
	CptCritiSecLock			m_MapLock ;
	CXCCopyingEventReceiver*						m_pDataOccuredRecever ; // �������ܿ��ǣ������ݱ仯�ص�ָ��ר�Ŵ���
};

class CXCCopyingEventReceiver
{
public:
	virtual int OnCopyingEvent_Execute(CXCCopyingEvent::EEventType et,void* pParam1=NULL,void* pParam2=NULL) = 0 ;
};