/****************************************************************************

Copyright (c) 2008-2020 Kevin Wu (Wu Feng)

github: https://github.com/kevinwu1024/ExtremeCopy
site: http://www.easersoft.com

Licensed under the Apache License, Version 2.0 License (the "License"); you may not use this file except
in compliance with the License. You may obtain a copy of the License at

https://opensource.org/licenses/Apache-2.0

****************************************************************************/

#pragma once

#include <windows.h>
#include "../Common/ptString.h"
#include <list>
#include "../Common/ptTypeDef.h"
#include "XCFilterProtocol.h"
#include "..\Common\ptGlobal.h"
#include <deque>

// �������λ��
#define ExceptionCodePosition__XCDestnationFilter_CreateFile				1
#define ExceptionCodePosition__XCFileDataCacheTransFilter_OnPin_Data_AllocMem		2

// ����������
#define ErrorHandlingFlag_Null			0
#define ErrorHandlingFlag_Success		0
#define ErrorHandlingFlag_Retry			(1 << 1)
#define ErrorHandlingFlag_Ignore		(1 << 2)
#define ErrorHandlingFlag_Exit			(1 << 3)
#define ErrorHandlingFlag_List			(1 << 4)
//#define ErrorHandlingFlag_Skip			1 << 3

//#define ErrorHandlingFlag_Exit			1 << 8
#define ErrorHandlingFlag_RetryIgnoreListExit (ErrorHandlingFlag_List|ErrorHandlingFlag_Retry|ErrorHandlingFlag_Ignore|ErrorHandlingFlag_Exit)

#define ErrorHandlingFlag_RetryIgnoreCancel (ErrorHandlingFlag_Retry|ErrorHandlingFlag_Ignore|ErrorHandlingFlag_Exit)
#define ErrorHandlingFlag_IgnoreCancel (ErrorHandlingFlag_Ignore|ErrorHandlingFlag_Exit)
#define ErrorHandlingFlag_RetryCancel (ErrorHandlingFlag_Retry|ErrorHandlingFlag_Exit)

#define ERROR_HANDLE_PROCESS(nResult,RetryLabel) \
	{if((nResult)==ErrorHandlingFlag_Retry) goto RetryLabel ;\
	else return (nResult) ;}

typedef unsigned		ErrorHandlingResult;

// ExtremeCopy App ������
enum ECopyFileErrorCode
{
	CopyFileErrorCode_Unknown = -1,
	CopyFileErrorCode_Success = 0,
	CopyFileErrorCode_AppError,
	CopyFileErrorCode_InvaliableFileName,
	CopyFileErrorCode_CanotReadFileData,
	CopyFileErrorCode_WriteFileFailed,
};

// ������
struct SXCErrorCode
{
	ECopyFileErrorCode	AppError ;
	int					nSystemError ;

	SXCErrorCode():AppError(CopyFileErrorCode_Unknown),nSystemError(0)
	{
	}
};

// ExtremeCopy �����쳣ʱ����Ϣ��
struct SXCExceptionInfo
{
	SXCErrorCode	ErrorCode ;
	unsigned		uFileID ;
	CptString strSrcFile ;
	CptString strDstFile ;
	int				CodePosition ;
	int				SupportType;

	SXCExceptionInfo():uFileID(0),CodePosition(0),SupportType(0)
	{
	}
};

// ExtremeCopy ��������
enum EXCExecuteType
{
	XCTT_Copy = 1,
	XCTT_Move,
	XCTT_Delete
};

// ExtremeCopy ������״̬
enum ECopyFileState
{
	CFS_Stop = 1,
	CFS_Running = 2,
	CFS_Pause = 3,
	CFS_Exit = 4,
	CFS_ReadyStop = 5, // ��״̬Ϊcore�ڲ��˳�ʱʹ�ã������ⲿʹ��
};

// ��ͻ�ļ���ʱ�Ĵ�������
enum EImpactFileBehaviorResult
{
	SFDB_Unkown = -1,
	SFDB_Default = -2, // ����ѯ��
	SFDB_Skip = 0,
	SFDB_Replace = 1,
	SFDB_Rename = 2,
	SFDB_StopCopy = 3
};

// filter ����
enum EFilterType
{
	FilterType_Unknown,
	FilterType_Source,
	FilterType_Transform,
	FilterType_Destination
};

// "��"������
enum EPinType
{
	PinType_Unknown,
	PinType_Input,
	PinType_Output
};

// �ļ�hash����
enum EFileHashType
{
	FHT_UNKNOWN,
	FHT_MD5,
	FHT_SHA1
};

// �ļ���ʼ����ʱ�ص����ϲ����Ϣ�ṹ��
struct SActiveFilesInfo
{
	unsigned uFileID ;
	unsigned __int64 uFileSize ;

	CptString	strSrcFile ;
	CptString	strDstFile ;
};

class CXCCopyingEvent ;

// Graph �����������Ϣ
struct SGraphTaskDesc
{
	 pt_STL_vector(CptString) SrcFileVer ; // Դ�ļ�����ȫ·����
	 pt_STL_vector(CptString) DstFolderVer; // Ŀ���ļ��У�����ǵ�

	 EXCExecuteType ExeType ;
	 EFileHashType	HashType ;
	 CXCCopyingEvent*	pCopyingEvent ;
	 int				nFileDataBufSize ;
	 EImpactFileBehaviorResult		ImpactFileBehavior ;
	 bool			bIsRenameDst ;

	 SGraphTaskDesc()
	 {
		 ExeType = XCTT_Copy ;
		 HashType = FHT_UNKNOWN ;
		 pCopyingEvent = NULL ;
		 nFileDataBufSize = 32*1024*1024 ;
		 ImpactFileBehavior = SFDB_Default ;
		 bIsRenameDst = false ;
	 }
};

//// �ļ�������ɣ���Ӧ EFilterCmd::EDC_FileOperationCompleted
//struct SDataPack_FileOpCompletedInfo
//{
//	unsigned	uID ; 		// �ļ�ID
//	bool		bDiscard ; 	// �Ƿ�����
//
//	SDataPack_FileOpCompletedInfo()
//	{
//		::memset(this,0,sizeof(*this)) ;
//	}
//};

// ���������¼�
struct SFileDataOccuredInfo
{
	unsigned	uFileID ;
	bool		bReadOrWrite ;
	int			nDataSize ;
};

// �ļ��������ʱ�Ļص�
struct SFileEndedInfo
{
	unsigned	uFileID ;
	//bool		bReadOrWrite ;
	//CptString	strFileName ;
};

// ��ͻ���ļ�����
enum EImpactFileType
{
	IT_SameFileName, // ��ͬ�����ļ�
	IT_ReadOnly, // ֻ���ļ�
	IT_ExecutableFile, // ��ִ���ļ�
};

struct SImpactFileInfo
{
	EImpactFileType ImpactType ;

	CptString strSrcFile ;
	CptString strDestFile ;
};

struct SImpactFileResult
{
	EImpactFileBehaviorResult	result ; // ����ֵ
	CptString strNewDstFileName ;  // ���û�ѡ�����ʱ��Ӧ�ṩ������
	bool bAlways ; // ����������,�����ֵΪtrue��˵���û�������ͻ�ļ�ʱ���ǰ�������ô���

	SImpactFileResult():result(SFDB_Default),bAlways(false)
	{
	}
};

// �ļ�����Ӧ�����ڴ��������Ϣ
struct SStorageInfoOfFile
{
	int			nSectorSize ;
	int			nPartitionIndex ; // ��Ϊ -1 ���DOS��
	DWORD		dwStorageID ;	// �洢����IDֵ
	unsigned	uDiskType ; // DRIVE_FIXED   �ο� ::GetDriveType()
							// DRIVE_REMOTE
							// DRIVE_CDROM 
							// DRIVE_RAMDISK
							// DRIVE_UNKNOWN
							// DRIVE_NO_ROOT_DIR
							// DRIVE_REMOVABLE

	bool IsSameStorage(const SStorageInfoOfFile& siof) const
	{
		//return (siof.nSectorSize==this->nSectorSize && siof.nSotrageIndex>=0 && siof.nSotrageIndex==this->nSotrageIndex && siof.uDiskType==this->uDiskType) ;
	return (siof.uDiskType==this->uDiskType && siof.nSectorSize==this->nSectorSize && siof.dwStorageID>0 && siof.dwStorageID==this->dwStorageID ) ;
	}

	SStorageInfoOfFile()
	{
		nSectorSize = 512 ;
		nPartitionIndex = -1 ;
		//nSotrageIndex = -1 ;
		dwStorageID = 0 ;
		uDiskType = DRIVE_UNKNOWN ;
	}
};

class CXCFileChangingBuffer ;

// source filter ������Ϣ
struct SXCSourceFilterTaskInfo
{
	pt_STL_vector(CptString) SrcFileVer ;
	EXCExecuteType					ExeType ;
	CXCFileDataBuffer*		pFileDataBuf ;
	CXCFileChangingBuffer*	pFileChangingBuf ;
	int						nValidCachePointNum ; // �ļ����ݻ���鱻���ô�����Ҳ���Ǻ�η�֧����Ŀ
	EImpactFileBehaviorResult*	pDefaultImpactFileBehavior ; // Ĭ�ϵĳ�ͻ�ļ������������� destination filter �Լ��� XCCore�����õ�MoveFileEx���õ�
	ECopyFileState*				pRunningState ;
};

class CXCCopyingEvent ;
class CXCPin ;


class CXCFilterEventCB
{
public:
	virtual int OnDataTrans(CXCFilterEventCB* pSender,EFilterCmd cmd,void* pFileData) = 0 ;
};


class CXCFilter : public CXCFilterEventCB
{
public:
	CXCFilter(CXCCopyingEvent* pEvent=NULL):m_pEvent(pEvent),m_pRunningState(NULL),m_pFileChangingBuffer(NULL) {}

	virtual ~CXCFilter() {}

	virtual EFilterType GetType() const {return FilterType_Unknown ;}
	virtual bool Initialize() {return true ;}
	inline bool IsValideRunningState() const {return (m_pRunningState!=NULL && *m_pRunningState!=CFS_Stop && *m_pRunningState!=CFS_Exit && *m_pRunningState!=CFS_ReadyStop);}

	virtual bool Connect(CXCFilter* pFilter,bool bUpstream) =0 ;


protected:
	struct SXCAsynOverlapped
	{
		OVERLAPPED ov ;
		DWORD		dwOperSize ;
		void*		pBuf ;
		int			nBufSize ;
	};
	virtual bool OnInitialize() {return true;}
	virtual bool OnPause() {return true ;}
	virtual bool OnContinue() {return true ;}
	virtual void OnStop() {}
	
protected:
	CXCCopyingEvent*	m_pEvent ;
	ECopyFileState*		m_pRunningState ;
	CXCFileChangingBuffer*	m_pFileChangingBuffer ;
	//ECopyFileState		m_RunningState ;
};


//======================================================= �����ķֽ��� =================


enum EOperationResult
{
	OperationResult_Success = 0 ,
	OperationResult_Cancel,
	OperationResult_Ignore,
	OperationResult_TryAgain,
};

enum ECopyBehavior
{
	CopyBehavior_ReplaceUncondition = 7, // ����ͬ�ļ���ʱ,������
	CopyBehavior_SkipCondition, // ����ͬ�ļ�����������ʱ,������
	CopyBehavior_Rename, // �Զ�����,
	CopyBehavior_AskUser, // ѯ���û�,����β���
};

enum EInterestFileType
{
	IFT_Invalid,
	//IFT_Unknown,
	IFT_File,
	IFT_Folder,
	IFT_FolderWithWildcard
};

#define SameFileCondition_SameSize				(1<<1) 
#define SameFileCondition_SameCreateTime		(1<<2)
#define SameFileCondition_SameModifyTime		(1<<3)


class CXCTransObject ;

struct STransformDataDesc
{
	void*	pAllocBuf ;
	int		nAllocSize ;
	void*	pDataBuf ;
	int		nBufSize ;
	DWORD	nOperatorSize ;
	DWORD	nOffset ;
	void*	pExtra ;

	STransformDataDesc()
	{
		::memset(this,0,sizeof(STransformDataDesc)) ;
	}
};

struct SFileOrDirectoryDiskInfo
{
	unsigned __int64 nFileSize ;
	int nPartitionIndex ;
	int nHardDriverIndex ;
	int nSector ;
	int nDriverType ;
	DWORD dwFileAttr ;
	CptString strFileOrDirectory ;

	SFileOrDirectoryDiskInfo()
	{
		dwFileAttr = INVALID_FILE_ATTRIBUTES ; // GetFileAttributes
		nDriverType = DRIVE_UNKNOWN ; // GetDriveType
		nSector = 512 ;
		nHardDriverIndex = 0 ;
		nPartitionIndex = 0 ;
		nFileSize = 0 ;
	}

};


bool IsContainWildcardChar(const TCHAR* pStr) ;
void RenameNewFileOrFolder(CptString& strFileOrFolder) ;
//bool IsFolder(const TCHAR* lpDir) ;
EInterestFileType GetInterestFileType(const TCHAR* pFileName) ;
CptString GetRawFileName(CptString strFullFileName) ;