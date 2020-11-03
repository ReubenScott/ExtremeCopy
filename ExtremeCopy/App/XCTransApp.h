/****************************************************************************

Copyright (c) 2008-2020 Kevin Wu (Wu Feng)

github: https://github.com/kevinwu1024/ExtremeCopy
site: http://www.easersoft.com

Licensed under the Apache License, Version 2.0 License (the "License"); you may not use this file except
in compliance with the License. You may obtain a copy of the License at

https://opensource.org/licenses/Apache-2.0

****************************************************************************/

#pragma once

#include "CompileMacro.h"
#include "..\Core\XCCore.h"
#include "..\Common\ptFolderSize.h"
#include "XCCommandLine.h"
#include <list>
#include <deque>
#include "XCLog.h"
#include "..\Core\XCCopyingEvent.h"
#include "XCStatistics.h"
#include "XCFailedFile.h"
#include "..\Core\XCFileChangingBuffer.h"
#include "XCVerifyResult.h"
#include "..\Common\ptMsgQue.h"
#include "../Common/ptThreadLock.h"
#include "XCTaskQueue.h"



class CXCTransApp : public CXCCopyingEventReceiver,public CXCFailedFileObserver,public CXCVerifyExceptionCB,public CXCStatisticsCB
{
public:
	CXCTransApp(const SConfigData& config,const SXCCopyTaskInfo& sti);
	virtual ~CXCTransApp(void);

	bool Ini(CXCUIWnd* pUIWnd,CXCStatistics* pSta,CXCFailedFile* pFailedFile) ;

	void Run() ;
	
	void Continue() ;
	void Pause() ;
	void Stop() ;
	void Release() ;
	bool SkipCurrentFile() ;
	ECopyFileState GetXCState() ;
	EXCStatusType GetXCStatus() const ;
	const CXCStatistics* GetStatistics() const;

	void OnUITimer(int nTimerID) ;

	const SXCCopyTaskInfo* GetTaskInfo() const ;
	CptString GetCurWillSkipFile() ;

	// run time option
	bool SetSpeed(const int nSpeed) ;
	void SetVerifyData(const bool bVerify) ;
	void SetShutdown(const bool bIsShutdown) ;
	void SetCloseWindow(const bool bIsCloseWindow) ;

protected:

	virtual int OnCopyingEvent_Execute(CXCCopyingEvent::EEventType et,void* pParam1=NULL,void* pParam2=NULL) ;
	virtual void OnFailedFile_Update(const SFailedFileInfo& OldFfi,const pt_STL_vector(SFailedFileInfo)& FailedFileVer) ;

	virtual ECopyFileState OnVerifyFileDiff(CptString strSrc,CptString strDst,CXCVerifyResult::EFileDiffType fdt) ;
	virtual void OnVerifyProgress(const CptString& strSrc,const CptString& strDst,const WIN32_FIND_DATA* pWfd) ;
	virtual void OnVerifyProgressBeginOneFile(const CptString& strSrc,const CptString& strDst) ;
	virtual void OnVerifyProgressDataOccured(const DWORD& uFileSize) ;

	virtual void OnStatEventFolderSizeCompleted() ;
private:
	// Ӧ�ó������ڵ�����״̬
	enum EAppRunningState
	{
		ARS_Exit, // �˳������Դ��״̬
		ARS_StandardRunning, // ����������״̬
		ARS_Pause,
		ARS_FailedRestore, // �ָ�����ʧ�ܵ��ļ�
		ARS_ReadyToRun, // CXCTransApp::Run() ���ú󣬵�δ������ʼ����ǰ��״̬
		ARS_WaitForExit, // �ȴ��û���� exit ��ť�˳�������������� verify data ֮��
	};

private:
	inline SRichCopySelection::EErrorProcessType CalErrorProcType(const SRichCopySelection::EErrorProcessType& DefaultType, const int& SupportProcessType) const;

	void ProcessFailedFiles() ;

	inline void StartOneSecondTimer() ;
	inline void StopOneSecondTimer() ;
	inline void AddSkipFileToFileChangedBuf(const CptString& strSrcFile,const CptString& strDstFile) ;

	void BeginCopyOneFile() ;
	ErrorHandlingResult ProcessCopyError(SRichCopySelection::EErrorProcessType ept, const SXCExceptionInfo& ErrorInfo) ;

	static unsigned int __stdcall CopyThreadFunc(void*) ;
	void CopyWork() ;
	void ExecuteTask() ;
	void SpeedDelay() ;

	bool ExecuteTask2(const SXCCopyTaskInfo& TaskInfo) ;

	// �¼���Ӧ������
	void ProcessEvent_CopyFileBegin(const pt_STL_list(SActiveFilesInfo)& SrcFileVer) ;
	void ProcessEvent_CopyFileDataDone(unsigned int uFileID) ;
	void ProcessEvent_CopyFileDiscard(const SDataPack_SourceFileInfo& fd,const unsigned __int64& uDiscardSize) ;
	ErrorHandlingResult ProcessEvent_Exception(const SXCExceptionInfo& ExceptionInfo) ;
	void ProcessEvent_ImpactFile(const SImpactFileInfo& ImpactInfo,SImpactFileResult& ifr) ;
	CptString ProcessEvent_GetRenamedFileName(CptString strOldFileName,bool bSameFile,bool bFolder) ;

	EImpactFileBehaviorResult ImpactFile_SameFile(const SImpactFileInfo& pImpactInfo) ;

private:
	class CCompareDOI
	{
	public:
		CCompareDOI(const unsigned uFileID):m_uFileID(uFileID) {}
		bool operator() ( const SFileDataOccuredInfo& val ) 
		{
			return ( val.uFileID==m_uFileID );
		}

	private:
		const unsigned m_uFileID ;
	};

private:
	bool MakeUnlimitedFileName() ;
	bool ExpandWildcardName() ;
	void InsertWildcardToRootFolder() ;
	bool IsSpecialFolderMovement() ;
	inline bool ExpandErrorCallback(CptString strFileName) ;
	EImpactFileBehaviorResult AskUserForSameFile(SImpactFileInfo ifi) ;
	EImpactFileBehaviorResult ImpactFile_SameFileByUserChoose(const SImpactFileInfo& ImpactInfo) ;

	bool IsMatchSameFileCondition(const CptString& strSrcFile,const CptString& strDestFile,SRichCopySelection::EFileDifferenceType) ;

#ifdef VERSION_PROFESSIONAL
	inline void AddToFailedFile(const SXCExceptionInfo& ErrorInfo) ;
	bool WaitForHDQueue(const SXCCopyTaskInfo& gtd) ;
#endif
	void GetUIOneSecondData(SXCUIOneSecondUpdateDisplay& osud) ;

private:

	HANDLE					m_hCopyThread ;
	
	int						m_nSpeed ;	// �����ڼ��û����ڵ��ٶ�ֵ
	int						m_nLastSpeedRate ;
	int						m_nLastSpeedDelayTime ;
	unsigned				m_LastUIOnceSecondFileID ;

	SConfigData				m_ConfigData ;

	CXCStatistics*			m_pXCSta ;
	
	SFailedFileInfo*		m_pCurFailedFile ;
	CptMsgQue				m_FailedFileMsgQue ;

	CXCUIWnd*				m_pUIWnd ;
	SXCCopyTaskInfo			m_CopyTask ;
	int						m_nRemainRetryCount ;

	EAppRunningState		m_CurRunState ;

	SImpactFileBehaviorSetting	m_BehaviorSetting ;

	///////////////////

	CXCFileLog					m_LogFile ; // ��־�ļ�

	CXCCore					m_XCCore ;
	CXCCopyingEvent			m_XCCopyEvent ;

	CptCritiSecLock						m_ActiveMapLock ;
	pt_STL_map(unsigned,SActiveFilesInfo)	m_ActiveFilesInfoMap ;
	SActiveFilesInfo*						m_pCurActiveFile ;

	CXCFileChangingBuffer	m_FileChangingBuf ;

	CptString				m_strCopyOfText ;

	HANDLE					m_hClearStopEvent ;
	EXCStatusType			m_CurStatus ;

	unsigned int			m_uCurUIFileID ; // ��ǰUI��ʾ�Ķ�Ӧ�ļ�ID����Ϊ��ʾ��ID��ʵ�����ڸ����е�ID��һ��һ�£���������ü�¼����

	pt_STL_list(SXCUIOneSecondUpdateDisplay) m_FriendlyUIList ; // ��Ϊ�ļ������������򿪺͹رգ�Ϊ�����û��о���ʾ��ƽ�����Ѻã����������ļ�����������ƽ��֮��

	CptCritiSecLock			m_FdoListLock ;
	pt_STL_list(SFileDataOccuredInfo) m_fdoiList[2] ; // �������屣���� data occur ���������������ݣ�Ȼ����1��ʱ�������ķ�ʽ�����ȡ��Щ����
	int		m_nCurFdoListIndex ; // fdoiList ������

	HANDLE					m_hLaunchCompleteEvent ;

	pt_STL_list(SActiveFilesInfo)		m_CreatedDestinationFileList ;

#ifdef VERSION_PROFESSIONAL
	CXCFailedFile*			m_pFailedFile ;
	CXCTaskQueue			m_TaskQueue ;
#endif
};