#include "StdAfx.h"
#include "XCSqlLog.h"
#include "XCGlobal.h"
#include "../Common/ptTime.h"


/*
#define LOG_DB_TABLE_FILE_WITH_PATH		_T("log_file_name")
#define LOG_DB_TABLE_TASK				_T("log_task")
#define LOG_DB_TABLE_RECORD				_T("log_record")


CXCSqlLog::CXCSqlLog(void):m_bOpened(false)
{
}


CXCSqlLog::~CXCSqlLog(void)
{
}

bool CXCSqlLog::Init() 
{
	if(m_bOpened)
	{
		return true ;
	}

	bool bRet = false ;

	try
	{
		const int nStepWait = 50 ;

		HANDLE hEvent = ::CreateEvent(NULL,FALSE,FALSE,_T("ExtremeCopy_Log_DB_Event")) ;

		DWORD dwResult = ::WaitForSingleObject(hEvent,nStepWait) ;

		m_SqliteDB.open(_T("x://my_log.db")) ;

		// ���� "·����"
		if(!m_SqliteDB.tableExists(LOG_DB_TABLE_FILE_WITH_PATH))
		{
			CptString strSQL ;
			strSQL.Format(_T("CREATE TABLE %s (ID INTEGER PRIMARY KEY,FileName NTEXT,FileSize BLOB,IsFolder BYTE,BelongToTaskID INTEGER,CreateTime DATETIME)"),LOG_DB_TABLE_FILE_WITH_PATH) ;
			m_SqliteDB.execDML(strSQL);
		}

		// ���� "�����"
		if(!m_SqliteDB.tableExists(LOG_DB_TABLE_TASK))
		{
			CptString strSQL ;
			strSQL.Format(_T("CREATE TABLE %s (ID INTEGER PRIMARY KEY,Source NTEXT,Destination NTEXT, Operation BYTE, Version CHAR(8), Configuration BLOB, LaunchType BYTE, IsNormalExit BYTE, BeginTime DATETIME, EndTime DATETIME)"),LOG_DB_TABLE_TASK) ;
			m_SqliteDB.execDML(strSQL);
		}

		// ���� "��¼��"
		if(!m_SqliteDB.tableExists(LOG_DB_TABLE_RECORD))
		{
			CptString strSQL ;
			strSQL.Format(_T("CREATE TABLE Customers(ID INTEGER PRIMARY KEY, TaskID INTEGER, Content NTEXT,CreateTime DATETIME)"),LOG_DB_TABLE_RECORD) ;
			m_SqliteDB.execDML(strSQL);
		}

		bRet = true ;
		
	}
	catch(...)
	{
		bRet = false ;
	}
	
	return bRet ;
}

bool CXCSqlLog::WriteStart(const SXCCopyTaskInfo& task,const SConfigData& config) 
{
	if(!this->Init())
	{
		return false ;
	}

	// ʼ�� v2.3.0 �汾
	struct SLogConfigBlob
	{
		bool bDefaultCopying ; // �Ƿ��ExtremeCopy ����ΪĬ�ϵ��ļ�������
		bool bPlayFinishedSound ; // �ļ�������󲥷�����
		bool bTopMost ;				// �����Ƿ�������ǰ����ʾ
		EUIType	UIType ;			// ��������
		int		nCopyBufSize ;		// �ļ����ݽ����������Ĵ�С
		bool	bMinimumToTray ;	// ��С��������
		int		nMaxFailedFiles ;	// ���ʧ�ܵ��ļ���
		bool	bWriteLog ;			// �Ƿ�д��־
		bool	bCloseWindowAfterDone ; //���������������Զ��رմ���
		bool	bAutoUpdate ;		// �Զ�������°汾
		bool	bAutoQueueMultipleTask ; // ������ʱ�Զ��Ŷ�
		time_t uLastCheckUpdateTime ; // �ϴμ�����°汾��ʱ��
	};

	CptString strSQL ;

	TCHAR szNow[64+1] = {0} ;

	CptString strVersion = ::MakeXCVersionString() ;
	CptTime::Now(szNow) ;

	// ����һ�����񵽡����������ȡ������ID
	strSQL.Format(_T("INSTERT INTO %s (Source,Destination,Operation,Version,LaunchType,IsNormalExit,BeginTime) VALUE ('','',%d,%s,%d,0,%s); SELECT ID FROM %s "),
		LOG_DB_TABLE_TASK,
		task.CopyType,
		strVersion,
		task.CreatedBy,
		szNow,
		LOG_DB_TABLE_TASK) ;

	m_SqliteDB.execDML(strSQL);

	return true ;
}

void CXCSqlLog::WriteErrorOccured(const SXCExceptionInfo& ErrorInfo,ErrorHandlingResult process) 
{
}

void CXCSqlLog::StepIntoFailedFileProcess(int nFailedFilesCount) 
{
}

void CXCSqlLog::FailedFileStatusChanged(const SFailedFileInfo& ffi) 
{
}

// 0: pause; 1: run; 2: exit
void CXCSqlLog::TaskRunningStateChanged(int NewState) 
{
}

void CXCSqlLog::FinishTask(const SStatisticalValue& sta) 
{
}
*/