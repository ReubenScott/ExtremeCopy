/****************************************************************************

Copyright (c) 2008-2020 Kevin Wu (Wu Feng)

github: https://github.com/kevinwu1024/ExtremeCopy
site: http://www.easersoft.com

Licensed under the Apache License, Version 2.0 License (the "License"); you may not use this file except
in compliance with the License. You may obtain a copy of the License at

https://opensource.org/licenses/Apache-2.0

****************************************************************************/

#include "stdafx.h"
#include  <wininet.h> 
#include  <stdlib.h>
#include <Dbghelp.h>

#include "XCRunMode.h"
#include "XCNewsChecker.h"
#include "UI/XCStartupPosDlg.h"

#pragma comment(lib,"Wininet.lib")
#pragma comment(lib,"Dbghelp.lib")


void XCRunModeXtremeRun(const SXCCopyTaskInfo& task,SConfigData& config,bool bAutoRun)
{

// ������ǲ��԰棬���ҷǵ����ã���ô���Ǹ��û����õģ���Ӧ�и���������	
#if (defined(VERSION_TEST_BETA) || defined(VERSION_TEST_ALPHA)) && !defined(_DEBUG)
	TCHAR szNowTime[64] = {0} ;
	CptTime::Now(szNowTime) ;

	if(!CptTime::IsInRange(szNowTime,_T("2012-06-01 10:00:00"),BETA_VERSION_EXPIRE_DATE))
	{// ������ں�������÷�Χ�ڣ����ṩ����
		::MessageBox(NULL,_T("This is test version and trial was expired. If you want to contiue use please access www.easersoft.com to download the latest version. Thank you !"),_T("Warning"),0) ;
		return ;
	}

#endif

	int nMainUIID = IDD_DIALOG_MAIN_SIMPLY ;

	switch(config.UIType)
	{
	case UI_Normal: nMainUIID = IDD_DIALOG_MAIN ; break ;

	default:
	case UI_Simple: nMainUIID = IDD_DIALOG_MAIN_SIMPLY ;break ;
	}

#ifdef VERSION_CHECKREGSITER
	{// ���ע����
		CAppRegister reg ;

		reg.CheckRegister(NULL) ;
	}
#endif

	if(config.bAutoUpdate && !CXCNewChecker::IsChecked(config.uLastCheckUpdateTime))
	{// ������°汾�������
		CptString str;

		str.Format(_T("%s\\XCUpdate.exe"),CptWinPath::GetStartupPath()) ;
		
		::ShellExecute(NULL,_T("open"),str.c_str(),_T("-check_update"),NULL,0) ;
		/*
		// ʹ�� ExtremeCopy.exe ��� update �����أ� ���� XCUpdate.exe 
		TCHAR ExtremeCopyFile[MAX_PATH] = {0} ;

		::GetModuleFileName(NULL,ExtremeCopyFile,sizeof(ExtremeCopyFile)/sizeof(TCHAR)) ;

		::ShellExecute(NULL,_T("open"),ExtremeCopyFile,_T("-check_update"),NULL,0) ;
		*/
	}

	CMainDialog dlg(bAutoRun,nMainUIID) ;

	CXCStatistics sta ;
	CXCFailedFile FailedFile ;

	CXCTransApp xcApp(config,task) ;

	if(xcApp.Ini(&dlg,&sta,&FailedFile))
	{
		xcApp.Run() ;
	}
}

// shell mode �������ǽ��մ� shell extension �������������ݣ�
// ������ task editor dialog ����������������޸�Ӧ 2 �������Ҫ���ǵ�
void XCRunModeShell(const SXCCopyTaskInfo& task,SConfigData& config)
{
	{
		SXCCopyTaskInfo sti ;

		DWORD dwRead = 0 ;
		HANDLE hRead = INVALID_HANDLE_VALUE ;

		{// named pipe
			const TCHAR* pPipeName = _T("\\\\.\\pipe\\XCShell2AppPipeName") ;

			hRead = ::CreateFile(pPipeName,GENERIC_WRITE|GENERIC_READ,0,NULL, 
				OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

			DWORD dwMode = PIPE_READMODE_MESSAGE;
			::SetNamedPipeHandleState(hRead,&dwMode,NULL,NULL) ; 
		}

		int nDataLen = 0 ;

		if(hRead!=INVALID_HANDLE_VALUE && ::ReadFile(hRead,&nDataLen,4,&dwRead,NULL) && dwRead==4)
		{// ��ȡXML�����ݳ���
			char* pUTF8XML = new char[nDataLen] ;

			if(pUTF8XML!=NULL && ::ReadFile(hRead,pUTF8XML,nDataLen,&dwRead,NULL) && dwRead==nDataLen)
			{// ��ȡXML����

				pt_STL_vector(SXCCopyTaskInfo) TaskVer ;

				const ETaskXMLErrorType result = CXCCopyTask::ConvertXML2TaskInfo(pUTF8XML,TaskVer) ;
				if(result==TXMLET_Success && !TaskVer.empty())
				{
					SAFE_DELETE_MEMORY(pUTF8XML) ;

					// ������ ���ݹ����� rich copy selection ѡ��Ϊ�������� INI �ļ�����Ϊ��
					// ��Ϊͨ����MODE�����Ĳ�����shell��Ҳ����ExtremeCopy����Ի������������ﴫ�ݽ����Ĳ���Ϊ׼��
					// ������� shell extension ��� ini �ļ�������ö����������������XML�ѱ���ini���ô��ݹ����ģ�
					// �������ﲻӦ�ö�ȡ ini �ļ���� verify ����
					// ����� shell ��Ӧ��ȡ verification �������ٴ��ݹ��� 

					const SXCCopyTaskInfo& TaskInfo = TaskVer[0] ; 

#ifndef VERSION_PROFESSIONAL
					// �����רҵ�棬����Ĭ��ֵ���δ������� config share ����
					SXCCopyTaskInfo NewTaskInfo = TaskInfo ;

					NewTaskInfo.ConfigShare.SetDefaultValue() ;

					// verify data ��֧�����а汾�ģ�����������Ӧ�ñ�������
					NewTaskInfo.ConfigShare.bVerifyData = TaskInfo.ConfigShare.bVerifyData ;

					::XCRunModeXtremeRun(NewTaskInfo,config,true) ;
#else
					::XCRunModeXtremeRun(TaskInfo,config,true) ;
#endif
					
				}
			}

			SAFE_DELETE_MEMORY(pUTF8XML) ;
		}

		if(hRead!=INVALID_HANDLE_VALUE)
		{
			::DisconnectNamedPipe(hRead) ;
			::CloseHandle(hRead) ;
			hRead = INVALID_HANDLE_VALUE ;
		}
	}
}

// ���öԻ���
void XCRunModeConfiguration()
{
	CConfigurationDialog dlg ;

	dlg.ShowDialog() ;
}

// ע��Ի���
void XCRunModeRegister()
{
	CAppRegister reg ;

	if(!reg.IsAppRegisted())
	{
		reg.ShowRegisterDlg(NULL) ;
	}
	else
	{
		CptString strText = CptMultipleLanguage::GetInstance()->GetString(IDS_MESSAGEBOX_ALREADREGISTED) ; // 
		CptString strTitle = CptMultipleLanguage::GetInstance()->GetString(IDS_TITLE_REGISTER) ;

		CptMessageBox::ShowMessage(NULL,strText,strTitle,CptMessageBox::Button_OK) ;
	}
}

// ABOUT �Ի���
void XCRunModeAbout()
{
	CAboutDlg dlg ;
	dlg.ShowDialog() ;
}

// ����Ի���
void XCRunModeXtremeTaskDlg(const SXCCopyTaskInfo& task,SConfigData& config)
{
#if defined(VERSION_PROFESSIONAL) || defined(VERSION_PORTABLE)
	{
		CXCTaskEditDlg SpeDlg(task) ;

		if(SpeDlg.ShowDialog()==CptDialog::DialogResult_OK)
		{
			CptString strStartupPath = CptWinPath::GetStartupPath() ;

			const SXCCopyTaskInfo& ti = SpeDlg.GetTaskInfo() ;

#ifdef _DEBUG
			XCRunModeXtremeRun(ti,config,true) ;
#else
#ifdef VERSION_PORTABLE
			//��һ�ζ�ȡ���ã���Ϊ������task dialogʱ���û��޸�������
			CXCConfiguration::GetInstance()->LoadConfigDataFromFile(config) ;

			XCRunModeXtremeRun(ti,config,true) ;
#else
			CXCCopyTask::LaunchXCTaskInstanceViaNamePipe(ti,strStartupPath.c_str(),NULL,false) ;
#endif
#endif
		}

	}
#endif
	
}



void XCRunModeNull(const SXCCopyTaskInfo& task,SConfigData& config)
{
	//::XCRunModeConfiguration() ;
	//return ;	  
	
	/**/
	// �����Ĵ���
#if defined(VERSION_PROFESSIONAL) || defined(VERSION_PORTABLE)
	XCRunModeXtremeTaskDlg(task,config) ;
#else 
	CXCStdLaunchDlg dlg ;

	dlg.ShowDialog() ;
#endif

	return ;
	/**/
#ifdef _DEBUG
	
	SXCCopyTaskInfo sti = task ; 

	sti.ConfigShare.sfpt = SRichCopySelection::SFPT_IfCondition;
	sti.ConfigShare.sfic.IfCondition = SRichCopySelection::FDT_Older ;
	sti.ConfigShare.sfic.ThenOperation = SRichCopySelection::SFOT_Ask ;
	sti.ConfigShare.sfic.OtherwiseOperation = SRichCopySelection::SFOT_Rename ;

	sti.strSrcFileVer.clear() ;
	sti.strDstFolderVer.clear() ;

	sti.CopyType = SXCCopyTaskInfo::RT_Copy ;
	//sti.CopyType = SXCCopyTaskInfo::RT_Move ;

	sti.strSrcFileVer.push_back(_T("x:\\phone charge.txt")) ; 

	sti.strDstFolderVer.push_back(_T("x:\\a")) ;

	config.UIType = UI_Normal ;
	//config.UIType = UI_Simple ;

	::XCRunModeXtremeRun(sti,config,false) ;

#endif
}


void XCRunCheckAndUpdate(const SXCCopyTaskInfo& task,SConfigData& config)
{
	CXCNewChecker nc ;

	nc.CheckAndUpdateExtremeCopy(config.uLastCheckUpdateTime) ; 

	// ����������������¼���������
	SConfigData cd ;
	CXCConfiguration::GetInstance()->LoadConfigDataFromFile(cd) ;
	cd.uLastCheckUpdateTime = CptTime::Now() ;

	CXCConfiguration::GetInstance()->SaveConfigDataToFile(cd) ;
}


// ������Դ
bool LoadXCResource(SConfigData* pConfigData) 
{
	_ASSERT(pConfigData) ;

	// ������Դ DLL
	CptString strStartUpPath = CptWinPath::GetStartupPath() ;

	CptString strLanguagePath = strStartUpPath+ _T("\\Language") ;

	/**
	// �������� Polish ���԰��
	CptString strDll = strLanguagePath + _T("\\XCRes_CHN.dll") ;

	bool bLoad = false ;
	if(IsFileExist(strDll.c_str()))
	{
		HMODULE hInst = ::LoadLibraryEx(strDll.c_str(),NULL,LOAD_LIBRARY_AS_DATAFILE) ;

		if(hInst!=NULL)
		{ 
			TCHAR szBuf[128+1] = {0} ;
			::LoadString(hInst,IDS_LANGUAGE_NAME,szBuf,sizeof(szBuf)/sizeof(TCHAR)) ;

			if(szBuf[0]!=0 && ::_tcscmp(_T("����"),szBuf)==0)
			{// ����������Ϊ��������������
				CptMultipleLanguage::GetInstance(_T("XCRes_CHN.dll")) ;
				bLoad = true ;
			}

			//strDLLVer.push_back(szBuf) ;
			::FreeLibrary(hInst) ;

			hInst = NULL ;
		}
	}

	if(!bLoad)
	{
		::MessageBox(NULL,_T("Can't load language resource file !"),NULL,MB_OK) ;
		return  false;
	}

	/**/

	CptString strDll = strLanguagePath + _T("\\") + pConfigData->strResourceDLL ;

	if(!IsFileExist(strDll.c_str()))
	{
		typedef pt_STL_map(std::basic_string<TCHAR>,std::basic_string<TCHAR>)	Str2StrMap_t ; 

		Str2StrMap_t LanguageName2DLLNameMap ;

		::SearchResourceDLL(LanguageName2DLLNameMap,strLanguagePath) ;

		if(!LanguageName2DLLNameMap.empty())
		{
			pConfigData->strResourceDLL = (*LanguageName2DLLNameMap.begin()).second.c_str() ;

			CXCConfiguration::GetInstance()->SaveConfigDataToFile(*pConfigData) ;
		}
		else
		{
			::MessageBox(NULL,_T("Can't load resource file !"),NULL,MB_OK) ;
			return false ;
		}
	}

	// ��������
	CptMultipleLanguage::GetInstance(pConfigData->strResourceDLL.c_str()) ;
	
	return true ;
}


LONG WINAPI XCReportBug(struct _EXCEPTION_POINTERS* pExceptionInfo)
{
	CptString strBugReportApp = CptWinPath::GetStartupPath() +_T("\\XCBugReport.exe") ;

	if(CptGlobal::IsFileExist2(strBugReportApp))
	{
		TCHAR szTemPath[MAX_PATH] = {0} ;

		::GetTempPath(sizeof(szTemPath)/sizeof(TCHAR),szTemPath) ;

		int nLen = (int)::_tcslen(szTemPath) ;

		if(szTemPath[nLen-1]!='\\' && szTemPath[nLen-1]!='/')
		{
			::_tcscat(szTemPath,_T("\\")) ;
		}

		::_tcscat(szTemPath,_T("ExtremeCopyCrash.dmp")) ;

		// Create the file
		HANDLE hMiniDumpFile = ::CreateFile(
			szTemPath,
			GENERIC_WRITE,
			0,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
			NULL);

		if(hMiniDumpFile!=INVALID_HANDLE_VALUE)
		{
			MINIDUMP_EXCEPTION_INFORMATION eInfo;

			eInfo.ThreadId = GetCurrentThreadId();
			eInfo.ExceptionPointers = pExceptionInfo;
			eInfo.ClientPointers = FALSE;

			::MiniDumpWriteDump(
				GetCurrentProcess(),
				GetCurrentProcessId(),
				hMiniDumpFile,
				MiniDumpNormal,
				&eInfo,
				NULL,
				NULL);

			::CloseHandle(hMiniDumpFile) ;

			::ShellExecute(NULL,_T("open"),strBugReportApp.c_str(),szTemPath,NULL,SW_SHOW) ;
		}

		return EXCEPTION_EXECUTE_HANDLER ;
	}

	return EXCEPTION_CONTINUE_SEARCH ;
}

#ifdef _DEBUG
void WriteTaskInfo2File(const SXCCopyTaskInfo& task,CptString strTargetFile)
{
	HANDLE hFile = ::CreateFile(strTargetFile.c_str(),GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL) ;

	if(hFile!=INVALID_HANDLE_VALUE)
	{
		CptString strText ;

		switch(task.cmd)
		{
		case SXCCopyTaskInfo::XCMD_Null:
			strText += _T("run type: XCMD_Null\r\n") ;
			break ;

		case SXCCopyTaskInfo::XCMD_About:
			strText += _T("run type: XCMD_About\r\n") ;
			break ;

		case SXCCopyTaskInfo::XCMD_Config:
			strText += _T("run type: XCMD_Config\r\n") ;
			break ;

		case SXCCopyTaskInfo::XCMD_Register:
			strText += _T("run type: XCMD_Register\r\n") ;
			break ;

		case SXCCopyTaskInfo::XCMD_Shell:
			strText += _T("run type: XCMD_Shell\r\n") ;
			break ;

		case SXCCopyTaskInfo::XCMD_XtremeRun:
			strText += _T("run type: XCMD_XtremeRun\r\n") ;
			break ;

		case SXCCopyTaskInfo::XCMD_Unknown:
			strText += _T("run type: XCMD_Unknown\r\n") ;
			break ;

		default:
			strText += _T("run type: -\r\n") ;
			break ;
		}

		// ��������
		strText += _T("copy type: ") ;

		switch(task.CopyType)
		{
		case SXCCopyTaskInfo::RT_Copy:
			strText += _T("RT_Copy \r\n") ;
			break ;

		case SXCCopyTaskInfo::RT_Move:
			strText += _T("RT_Copy \r\n") ;
			break ;

		default:
			strText += _T("- \r\n") ;
			break ;
		}

		// Դ�ļ�
		strText += _T("src files:") ;

		if(task.strSrcFileVer.empty())
		{
			strText += _T(" -\r\n") ;
		}
		else
		{
			for(unsigned int i=0;i<task.strSrcFileVer.size();++i)
			{
				strText += task.strSrcFileVer[i] + _T("\r\n") ;
			}
		}

		// Ŀ��Ŀ¼
		strText += _T("dst files: ") ;
		if(task.strDstFolderVer.empty())
		{
			strText += _T(" -\r\n") ;
		}
		else
		{
			for(unsigned int i=0;i<task.strDstFolderVer.size();++i)
			{
				strText += task.strDstFolderVer[i] + _T("\r\n") ;
			}
		}

		// ��ͬ�ļ�����ʽ
		strText += _T("same file process: ") ;
		switch(task.ConfigShare.sfpt)
		{
		case SRichCopySelection::SFPT_Skip:
			strText += _T("SFPT_Skip \r\n") ;
			break ;

		case SRichCopySelection::SFPT_Replace:
			strText += _T("SFPT_Replace \r\n") ;
			break ;

		case SRichCopySelection::SFPT_Ask:
			strText += _T("SFPT_Ask \r\n") ;
			break ;

		case SRichCopySelection::SFPT_Rename:
			strText += _T("SFPT_Rename \r\n") ;
			break ;

		default:
			strText += _T("- \r\n") ;
			break ;
		}

		// ����������
		strText += _T("error process: ") ;
		switch(task.ConfigShare.epc)
		{
		case SRichCopySelection::EPT_Retry:
			strText += _T("EPT_Retry \r\n") ;
			break ;

		case SRichCopySelection::EPT_Exit:
			strText += _T("EPT_Exit \r\n") ;
			break ;

		case SRichCopySelection::EPT_Ignore:
			strText += _T("EPT_Ignore \r\n") ;
			break ;

		case SRichCopySelection::EPT_Ask:
			strText += _T("EPT_Ask \r\n") ;
			break ;
		default:
			strText += _T("- \r\n") ;
			break ;
		}

		// ���Դ���
		CptString strTem ;
		strTem.Format(_T("retry times: %d \r\n"),task.ConfigShare.nRetryTimes) ;
		strText += strTem ;

		// ��ʾ����
		strText += _T("show mode: ") ;
		switch(task.ShowMode)
		{
		case SXCCopyTaskInfo::ST_Window:
			strText += _T("ST_Window \r\n") ;
			break ;

		case SXCCopyTaskInfo::ST_Tray:
			strText += _T("ST_Tary \r\n") ;
			break ;

		default:
			strText += _T("- \r\n") ;
			break ;
		}

		// ��־�ļ�
		strTem.Format(_T("log file: %s \r\n"),task.strLogFile) ;
		strText += strTem ;

		DWORD dwWrite = 0 ;

		::WriteFile(hFile,strText.c_str(),strText.GetLength()*sizeof(TCHAR),&dwWrite,NULL) ;

		::CloseHandle(hFile) ;
	}
}
#endif
