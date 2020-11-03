
#pragma once

#include "..\Core\XCCore.h"
#include "..\Core\XCCopyingEvent.h"
#include <vector>
#include <list>
#include "..\Core\XCCoreDefine.h"
#include "..\Common\ptTypeDef.h"

#define ROUTINE_CMD_BEGINONEFILE		1	// ��ʼ����һ�ļ�
#define ROUTINE_CMD_FINISHONEFILE		2	// ��������һ�ļ�
#define ROUTINE_CMD_FILEFAILED			3	// ʧ�ܵ��ļ�
#define ROUTINE_CMD_SAMEFILENAME		4	// �ļ�����ͬ
#define ROUTINE_CMD_DATAWROTE			5	// ����д��ɹ�
#define ROUTINE_CMD_TASKFINISH			6	// �������


#define START_ERROR_CODE_SUCCESS					0 // �ɹ�
#define START_ERROR_CODE_INVALID_DSTINATION			1 // ��Ч��Ŀ���ļ���
#define START_ERROR_CODE_INVALID_SOURCE				2 // ��Ч��Դ�ļ�
#define START_ERROR_CODE_INVALID_RECURISE			3 // �ݹ�����ļ���
#define START_ERROR_CODE_CANOT_LAUNCHTASK			4 // ��ǰ״̬�����������µĸ�������
#define START_ERROR_CODE_UNKNOWN					999 // δ֪����

//#define ROUTINE_CMD_STATECHANGED		6	// ״̬�����仯

#define XCRunType_Copy		1
#define XCRunType_Move		2

typedef int (__stdcall *lpfnCopyRoutine_t)(int nCmd,int nParam1,int nParam2,const TCHAR* pSrcStr,const TCHAR* pDstStr) ;

class CExtremeCopyApp : public CXCCopyingEventReceiver
{
public:

	CExtremeCopyApp() ;
	~CExtremeCopyApp() ;

	void SetLicenseKey(CptString strSN) ;

	void Stop() ;
	bool Pause() ;
	bool Continue() ;
	int Start(int nRunType,bool bSyncOrAsync) ;
	ECopyFileState GetState() ;
	int AttachSrc(const CptString strSrcFile) ;
	int AttachDst(const CptString strDstFile) ;

	bool SetDestinationFolder(const CptString strDstFolder) ;
	void SetRoutine(lpfnCopyRoutine_t) ;
	int SetCopyBufferSize(int nBufSize) ;
protected:

	int CheckTaskParameter() ;
		// ���ƻص��¼�
	virtual int OnCopyingEvent_Execute(CXCCopyingEvent::EEventType et,void* pParam1=NULL,void* pParam2=NULL) ;

	bool DoesIncludeRecuriseFolder(const pt_STL_vector(CptString)& SrcVer,const pt_STL_vector(CptString)& DstVer,int& nSrcIndex, int& nDstIndex) ;

	bool IsDstRenameFileName(const pt_STL_vector(CptString)& SrcVer,const pt_STL_vector(CptString)& DstVer)  ;

	inline void RemoveCityFromActiveList(unsigned uCityID) ;
	void PostBeginOneFileCommand(const SActiveFilesInfo& afi) ;

private:
	static unsigned int __stdcall CopyThreadFunc(void*) ;
	void CopyWork(bool bCopyOrMove) ;

	struct SThreadParamData
	{
		CExtremeCopyApp*	pThis ;
		bool				bCopyOrMove ;
	};

	//class CActiveFileComparetor
	//{
	//public:
	//	bool operator ()(const SActiveFilesInfo& afi)
	//	{
	//		return (afi.uFileID==m_uFileID) ;
	//	}
	//	
	//	void SetCompareFileID(unsigned uFileID)
	//	{
	//		m_uFileID = uFileID ;
	//	}
	//private:
	//	unsigned int	m_uFileID ;
	//}  ;

private:
	CXCCore					m_XCCore ;
	CXCCopyingEvent			m_XCCopyEvent ;

	pt_STL_vector(CptString)		m_strSrcVer ;
	pt_STL_vector(CptString)	m_strDstFolderVer ;

	lpfnCopyRoutine_t			m_lpfnRoutine ;

	HANDLE						m_hThread ;
	CptString					m_strSeriesNumber ;

	bool					m_bIni ;

	pt_STL_list(SActiveFilesInfo)		m_ActiveFileInfoList ;
	CptCritiSecLock			m_FdoListLock ;
	unsigned int				m_uCurFileID ;

	//CActiveFileComparetor			m_ActiveFileComparetor ;
	int							m_nBufSize ;
	unsigned __int64			m_uCurFileWrittenSize ;
};