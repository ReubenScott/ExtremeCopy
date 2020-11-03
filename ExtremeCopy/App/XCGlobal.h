/****************************************************************************

Copyright (c) 2008-2020 Kevin Wu (Wu Feng)

github: https://github.com/kevinwu1024/ExtremeCopy
site: http://www.easersoft.com

Licensed under the Apache License, Version 2.0 License (the "License"); you may not use this file except
in compliance with the License. You may obtain a copy of the License at

https://opensource.org/licenses/Apache-2.0

****************************************************************************/

#pragma once

#include "../Core/XCCoreDefine.h"

#include <Winioctl.h>
#include "../Common/ptThread.h"
#include "../Common/ptGlobal.h"

#include "Language\XCRes_ENU\resource.h"

#include "ptMultipleLanguage.h"
#include <map>
#include <vector>
#include <list>


#include <string>

#include "../Common/sgi_stl_alloc.h"


#define LINK_HOME_WEBSITE		_T("www.easersoft.com")
#define LINK_EMAIL_SUPPORT		_T("support@easersoft.com")

#define WM_XC_COPYDATAOCCURED		WM_USER+1 // �������ݷ�������
#define WM_XC_BEGINCOPYONEFILE		WM_USER+2 // ��ʼһ���ļ�����
#define WM_XC_SHELLTRAY				WM_USER+3 // ������Ϣ


//#define UI_BG_COLOR		RGB(181,211,255)
//#define UI_TEXT_COLOR	RGB(10,10,230)

#define TIMER_ID_ONE_SECOND		1000 // ��ʱ��IDֵ

//#define DEFAULT_UISTARTUP_POSITION	0xfffffff 

//#define XC_STL_queue_t(TKey,TVal) std::queue<TKey,TVal,std::less<Tkey>,sgi_std::allocator<std::pair<const TKey&,TVal> > >

// ��վҳ����������
enum EWebLink
{
	WEBLINK_BUY_STANDARD,
	WEBLINK_BUY_PROFESSIONAL,
	WEBLINK_HELP_SAMEFILEDLG,
	WEBLINK_HELP_TASKDLG,
	WEBLINK_HELP_WILDCARDDLG,
	WEBLINK_SITE_HOME,
	WEBLINK_SITE_GETPROEDITION,
	WEBLINK_EMAIL_SUPPORT,
};


struct SGlobalData
{
	HBITMAP hSpeedBitmapNormal ;
	HBITMAP hSpeedBitmapHover ;
	HBITMAP hSpeedBitmapDown ;

	//HBRUSH	hDlgBkBrush ;

	HCURSOR hCursorHand ;

	int		nSwapBufSize ;

	SGlobalData()
	{
		::memset(this,0,sizeof(SGlobalData)) ;
	}

	void Release()
	{
		SAFE_DELETE_GDI(hSpeedBitmapNormal) ;
		SAFE_DELETE_GDI(hSpeedBitmapHover) ;
		SAFE_DELETE_GDI(hSpeedBitmapDown) ;

		SAFE_DELETE_GDI(hCursorHand) ;
	}
};

enum EUpdateUIType
{
	UUIT_OneSecond,
	UUIT_BeginCopyOneFile, // only for verification
	//UUIT_FinishCopyOneFile,
	UUIT_CopyDataOccured, // only for verification
	//UUIT_ShowIniDelayUI, // ��ʾ��ʼ��ʱ���ӳ���ʾ����
};

class CMainDialog ;

// GUI ��״̬�� ����normal window,tray
class CXCGUIState
{
public:
	CXCGUIState():m_pNextState(NULL),m_pMainDlg(NULL) {}

	virtual void UpdateUI(const EUpdateUIType uuit,void* pParam1=NULL,void* pParam2=NULL) = 0;
	virtual CptString GetCurFileName() {return _T("");}

	virtual void ProcessMessage(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)  = 0;
	virtual void OnEnter(CptString strCurFileName) {}
	virtual void OnLeave() {}
	//void OnStatusChanged(EXCStatusType OldStatus,EXCStatusType NewStatus) {}

	void SetParameter(CXCGUIState* pNextState,CMainDialog* pDlg) 
	{
		m_pNextState = pNextState ;
		m_pMainDlg = pDlg ;
	}

protected:
	CXCGUIState*	m_pNextState ;
	CMainDialog*	m_pMainDlg ;
};

//// ------------------����ͳ�������ò���------------------- ��ʼ----

struct SRichCopySelection
{
	enum EFileDifferenceType
	{
		FDT_Newer = 0,
		FDT_Older = 1,
		FDT_Bigger = 2,
		FDT_Smaller = 3,
		FDT_SameTimeAndSize = 4,
		FDT_Last,
	};

	enum ESameFileOperationType
	{
		SFOT_Skip = 0,
		SFOT_Replace = 1,
		SFOT_Ask = 2,
		SFOT_Rename = 3,
		SFOT_Last,
	};

	enum ESameFileProcessType
	{
		SFPT_Skip, // ����
		SFPT_Replace, // ����
		SFPT_Ask, // ѯ��
		SFPT_Rename, // ������
		//SFPT_ReplaceNewer, // �� v2.1.1�Ͳ����ø������ˣ���ֵ��Ϊ SFPT_IfCondition ����
		SFPT_IfCondition, // ������Ϊ��ǰ��SFPT_ReplaceNewer�� �����������в����������� EFileDifferenceType �����
							// Ĭ�ϵ�����ǣ� Replace it if newer, otherwise ask me
	};

	// ����ֻ���ļ��Ĵ�������
	enum EReadOnlyFileProcessType
	{
		ROFPT_Skip, // ����
		ROFPT_Ask, // ѯ��
		ROFPT_Replace // ���ǣ��������ƶ����� 
	};

	// ����Ĵ���ʽ
	enum EErrorProcessType
	{
		EPT_Retry, // ����
		EPT_Exit, // ȡ�� (���˳�����)
		EPT_Ignore, // ����
		EPT_Ask, // ѯ��
	};
};

enum EUIType
{
	UI_Simple,
	UI_Normal,
	UI_Advance
};

// ��ͬ�ļ����������Ĵ���ʽ
struct SSameFileIfConditionInfo
{
	SRichCopySelection::EFileDifferenceType		IfCondition ;
	SRichCopySelection::ESameFileOperationType	ThenOperation ;
	SRichCopySelection::ESameFileOperationType	OtherwiseOperation ;
};

#define CONFIG_DEFAULT_SAME_FILE_NAME_PROCESS		(SRichCopySelection::SFPT_Ask)
#define CONFIG_DEFAULT_ERROR_PROCESS				(SRichCopySelection::EPT_Ignore)
#define CONFIG_DEFAULT_RETRY_FAILED_THEN_PROCESS	(SRichCopySelection::EPT_Ignore)
#define CONFIG_DEFAULT_RETRY_TIMES					1
#define CONFIG_DEFAULT_MAINDIALOG_POS_X				140  // �����ڵ�Ĭ������λ�� X
#define CONFIG_DEFAULT_MAINDIALOG_POS_Y				30 // �����ڵ�Ĭ������λ�� Y

struct SXCTaskAndConfigShare
{

	SptPoint				ptStartupPos ;
	SRichCopySelection::EErrorProcessType		epc ; // ����ʱ�Ĵ���ʽ
	SRichCopySelection::ESameFileProcessType	sfpt ; // ��ͬ�ļ�ʱ�Ĵ���ʽ
	bool					bVerifyData ; // �Ƿ�������
	bool					bShutdownAfterDone ; // ������ɺ��Զ��ػ�
	SRichCopySelection::EErrorProcessType		RetryFailThen ; // ����ʧ�ܺ�Ĵ���ʽ
	int						nRetryTimes ; // ���Դ���. �� epc ָ��Ϊ����ʱ,��ֵ��Ч
	SSameFileIfConditionInfo	sfic ; // ������� sfpt ��ֵΪ SFPT_IfCondition ʱ����ֵ����Ч

	SXCTaskAndConfigShare()
	{
		this->SetDefaultValue() ;
	}

	void SetDefaultValue()
	{
		bVerifyData = false ;
		bShutdownAfterDone = false ;
		ptStartupPos.nX = CONFIG_DEFAULT_MAINDIALOG_POS_X ;
		ptStartupPos.nY = CONFIG_DEFAULT_MAINDIALOG_POS_Y ;
		epc = CONFIG_DEFAULT_ERROR_PROCESS ;
		sfpt = CONFIG_DEFAULT_SAME_FILE_NAME_PROCESS ;

		RetryFailThen = CONFIG_DEFAULT_RETRY_FAILED_THEN_PROCESS ;
		nRetryTimes = CONFIG_DEFAULT_RETRY_TIMES ;

		sfic.IfCondition = SRichCopySelection::FDT_Newer ;
		sfic.ThenOperation = SRichCopySelection::SFOT_Replace ;
		sfic.OtherwiseOperation = SRichCopySelection::SFOT_Ask ;
	}
};

// ���������ṹ
struct SXCCopyTaskInfo
{
	enum EXCCmd
	{
		XCMD_Unknown, // δ֪
		XCMD_Null, // �ղ���
		XCMD_Register, // ����ע����
		XCMD_About, // about
		XCMD_XtremeRun, // ��������
		XCMD_Shell, // SHELL ������
		XCMD_TaskDlg, // ����Ի���
		XCMD_Config, // ����
		XCMD_CheckUpdate, // ������
	};

	// ִ�в���������
	enum EExecuteType
	{
		RT_Copy = 20, // ����
		RT_Move // �ƶ�
	};

	enum EShowMode
	{
		ST_Window, // ����
		ST_Tray // ����
	};

	// �������ĸ����ܴ�����
	enum ETaskCreatedByType
	{
		TCT_TaskDialog, // ������Ի��򴦴���������
		TCT_ShellExtension, // SHELL ������������
	};

	EExecuteType CopyType ; // ��������

	pt_STL_vector(CptString) strSrcFileVer ;
	pt_STL_vector(CptString) strDstFolderVer ;
	EXCCmd cmd ;

	CptString				strLogFile ;
	EShowMode				ShowMode ;
	CptString				strFinishEvent ; // �������ʱ���Դ������¼�
	ETaskCreatedByType		CreatedBy ; // �ò���Ŀǰ���� verification ���������ĸ�Ϊ׼���������жϸ����������ĸ�������

	SXCTaskAndConfigShare		ConfigShare ; // �� SConfigData ������ͬ���͵��������ݣ����Ǳ��� shell extension ֱ�Ӹ�ֵ

	SXCCopyTaskInfo()
	{
		this->SetDefaultValue() ;
	}

	void SetDefaultValue()
	{
		cmd = XCMD_Unknown ;

		strSrcFileVer.clear() ;
		strDstFolderVer.clear() ;
		strLogFile = _T("") ;

		CopyType = RT_Copy ;
		CreatedBy = TCT_TaskDialog ;

		ShowMode = SXCCopyTaskInfo::ST_Window ;

		strFinishEvent = _T("") ;
	}
};

struct SConfigData
{
	bool bDefaultCopying ; // �Ƿ��ExtremeCopy ����ΪĬ�ϵ��ļ�������
	bool bPlayFinishedSound ; // �ļ�������󲥷�����
	bool bTopMost ;				// �����Ƿ�������ǰ����ʾ
	CptString strSoundFile ;	// ������Ҫ���ŵ������ļ�
	EUIType	UIType ;			// ��������
	int		nCopyBufSize ;		// �ļ����ݽ����������Ĵ�С
	bool	bMinimumToTray ;	// ��С��������
	int		nMaxFailedFiles ;	// ���ʧ�ܵ��ļ���
	bool	bWriteLog ;			// �Ƿ�д��־
	bool	bCloseWindowAfterDone ; //���������������Զ��رմ���
	CptString	strResourceDLL ; // ʹ�õ����԰�
	bool	bAutoUpdate ;		// �Զ�������°汾
	bool	bAutoQueueMultipleTask ; // ������ʱ�Զ��Ŷ�
	time_t uLastCheckUpdateTime ; // �ϴμ�����°汾��ʱ��

	SConfigData()
	{
		bDefaultCopying = true ;
		bPlayFinishedSound = false ;
		bTopMost = true ;
		UIType = UI_Simple ;
		nCopyBufSize = 32 ;
		bMinimumToTray = true ;
		nMaxFailedFiles = 1000 ;
		bWriteLog = false ;
		bCloseWindowAfterDone = true ;

		bAutoUpdate = true ;
		bAutoQueueMultipleTask = true ;
		uLastCheckUpdateTime = 0 ;
	}
};

// log��־�����õ��ֶ�
//struct SLogConfigData
//{
//	unsigned int totalBoolConfig ;
//	EUIType		UIType ;			// ��������
//	BYTE		nCopyBufSize ;		// �ļ����ݽ����������Ĵ�С
//	int		nMaxFailedFiles ;	// ���ʧ�ܵ��ļ���
//
//	SXCTaskAndConfigShare ShareConfig ;
//
//	
//};

//// ------------------����ͳ�������ò���------------------- ����

// ʧ���ļ�״̬����
enum EFailFileStatusType
{
	EFST_Unknown ,
	EFST_Failed,
	EFST_Waitting,
	EFST_Running,
	EFST_Success,
};

// ������ͬ��ͻ���͵��ļ�ʱ����
struct SImpactFileBehaviorSetting
{
	bool	bActived ; // �ýṹ����Ϣ�Ƿ��� 
						//��'same file name dialog' ��������
						// ����û�ѡ�������磺 replace all, skip all ֮���
						// app Ӧ�ü�ס���ѡ���Ա��´����� same file name ʱ�����ٵ�������Ի���
						// �Ӷ�֪��Ӧ��β����� ������������Ƿ��û�����ѡ�������

	bool bApplyForReadOnly ;
	ECopyBehavior Behavior ;
	int SkipCondition ; // SameFileCondition_SameSize 
						// SameFileCondition_SameCreateTime
						// SameFileCondition_SameModifyTime

	SImpactFileBehaviorSetting():bActived(false)
	{
	}
};

// ʧ���ļ���Ϣ�����ṹ
struct SFailedFileInfo
{
	int			nIndex ;
	unsigned	uFileID ;
	EFailFileStatusType		Status ;
	CptString	strSrcFile ;
	CptString	strDstFile ;
	SXCErrorCode	ErrorCode ;

	SFailedFileInfo()
	{
		nIndex = 0 ;
		uFileID = 0 ;
		Status = EFST_Unknown ;
		strSrcFile = _T("") ;
		strDstFile = _T("") ;
	}
};

// ������������XCTransApp �� UUIT_OneSecond ����ص�������Ĳ����Ľṹ��
struct SXCUIOneSecondUpdateDisplay
{
	CptString	strCurSrcFileName ; // ��ǰԴ�ļ���
	CptString	strCurDstFileName ;	// ��ǰĿ���ļ���
	//int			nPercentOfCurFile ; // ��ǰ�ļ��Ľ��Ȱٷֱȣ����Ը�ֵ�����100����С��0
};

struct SXCUIDisplayConfig
{
	bool	bTopMost ;
	bool	bMinimumToTray ;
	bool	bVerifyData ;
	bool	bShutDown ;
	bool	bCloseWindow ;
	SXCCopyTaskInfo::EShowMode	ShowMode ;
	int		nMaxIgnoreFailedFiles ;
	SptPoint	DlgStartupPos ;

	SXCUIDisplayConfig()
	{
	}
};

enum EXCStatusType
{
	XCST_Unkown,
	XCST_Ready,
	XCST_Copying,
	XCST_Pause,
	XCST_Verifying,
	XCST_Finished,
};

enum EHelpFilePage
{
	HFP_MainMenu,
	HFP_SameFileNameDlg,
	HFP_ConfigurationDlg,
	HFP_TaskEditDlg,
	HFP_WildcardDlg,
	HFP_TaskQueue,
	HFP_MainDlg,
};

enum EXCVerificationResult
{
	XCST_Same,
	XCST_Different,
};

class CXCTransApp ;
class CXCFailedFile ;

class CXCUIWnd
{
public:
	CXCUIWnd(){}

	virtual void XCUI_SetTimer(int nTimerID,int nInterval) {}
	virtual void XCUI_KillTimer(int nTimerID) {}

	virtual void XCUI_OnRunningStateChanged(ECopyFileState NewState) {}
	virtual void XCUI_OnUIVisibleChanged(bool bVisible) {}
	virtual ErrorHandlingResult XCUI_OnError(const SXCExceptionInfo& ErrorInfo) = 0 ;

	virtual EImpactFileBehaviorResult XCUI_OnImpactFile(const SImpactFileInfo& ImpactInfo,SImpactFileBehaviorSetting& Setting) = 0 ;

	virtual void XCUI_OnIni(CXCTransApp* pTransApp,CXCFailedFile* pFailedFile,const SXCUIDisplayConfig& config,const SXCCopyTaskInfo& TaskInfo) {}

	virtual void XCUI_UpdateUI(EUpdateUIType uuit,void* pParam1,void* pParam2) {}
	
	virtual CptString XCUI_GetCopyOfText() const {return _T("Copy Of");}

	virtual void XCUI_OnStatusChanged(EXCStatusType OldStatus,EXCStatusType NewStatus)  {}
	virtual void XCUI_OnVerificationResult(EXCVerificationResult result) {}
	virtual void XCUI_OnRecuriseFolder(const CptString& strSrcFolder, const CptString& strDstFolder) {}
};

	struct SSateDisplayStrings
	{
		enum EMask
		{
			nSppedMask = 1<<0 ,
			nTotalFilesMask = 1<<1 ,
			nDoneFilesMask =  1<<2 ,
			nTotalSizeMask = 1<<3 ,
			nRemainSizeMask = 1<<4 ,
			nTotalTimeMask = 1<<5 ,
			nRemainTimeMask = 1<<6 ,
			nLapseTimeMask = 1<<7 ,
			nRemainFileMask = 1<<8 ,
		};

		UINT uFlag ;

		CptString strSpeed ;

		CptString strTotalFiles ;
		CptString strDoneFiles ;
		CptString strRemainFiles ;

		CptString strTotalSize ;
		CptString strRemainSize ;

		CptString strTotalTime ;
		CptString strRemainTime ;
		CptString strLapseTime ;
	};

CptString GetSizeString(unsigned __int64 nValue) ;

bool ConfirmExitApp(HWND hParentWnd=NULL) ;
unsigned __int64 DoubleWordTo64(const DWORD dwLow,const DWORD dwHight) ;

void TransparenWnd(HWND hWnd,int nPercent) ;

bool CheckRegisterCode(const CptStringList& sl) ;
void GetRegisterCode(CptStringList& sl) ;

void OnHyperLinkHomePageCallBack2(void* pVoid);

bool DoesIncludeRecuriseFolder(const SXCCopyTaskInfo& sti,int& nSrcIndex, int& nDstIndex) ;
CptString MakeXCVersionString() ;
int SearchResourceDLL(pt_STL_map(std::basic_string<TCHAR>,std::basic_string<TCHAR>)& LanguageName2DLLNameMap,CptString strFolder) ;

enum ETaskXMLErrorType ;
CptString GetTaskXMLErrorString(const ETaskXMLErrorType& ErrorCode) ;

bool OpenLink(const TCHAR* pLink) ;
bool OpenLink(const EWebLink LinkType) ;
bool IsDstRenameFileName(const SGraphTaskDesc& gtd) ;

bool IsRecycleFile(const CptString& strFile,CptString& strOriginName) ;

void LaunchHelpFile(const EHelpFilePage hfp) ;



