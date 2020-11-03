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
#include "../Common/ptThreadLock.h"
#include "../Common/ptWinPath.h"

class CXCFilter ;

enum EFileHashType ;


// �ļ���������
enum EFilterCmd
{// S: source filter; D: destination filter; T: transform filter

	// ���ܣ��µ�FILTER LINK���������Ŀ���ļ��� URI ջ.
	// ����(S->D)��
	// ���壺��
	// ���ݰ��� SDataPack_LinkIni
	// ����ֵ�� ��
	EDC_LinkIni,	

	// ���ܣ��Ѵ���ͣ״̬�� FILTER ������������
	// ����(S->D)��
	// ���壺��
	// ���ݰ��� ��
	// ����ֵ�� ��
	EDC_Continue,

	// ���ܣ��Ѵ���ͣ״̬�� FILTER ������������
	// ����(S->D)��
	// ���壺��
	// ���ݰ��� ��
	// ����ֵ�� ��
	EDC_Pause,

	// ���ܣ�ֹͣ FILTER ���й������ͷ������Դ
	// ����(S->D)��
	// ���壺��
	// ���ݰ��� ��
	// ����ֵ�� ��
	EDC_Stop,

	// ���ܣ�������ǰ���ڸ��Ƶ��ļ���������ֻ��destinatin filter����������filter������
	// ����(S->D)��
	// ���壺��
	// ���ݰ��� ��
	// ����ֵ�� ��
	EDC_Skip,

	//// ���ܣ����� destination filter �����ļ���
	//// ����(S->D)
	//// ���壺��
	//// ���ݰ���SDataPack_CheckFileInfo
	//// ����ֵ���ɹ��� ErrorHandlingFlag_Success; ���ԣ�ErrorHandlingFlag_Ignore; �˳���ErrorHandlingFlag_Exit
	//EDC_CheckFile,

	// ���ܣ����� destination filter �����ļ�,�������destination filter���Ǵ��ж����ģ�
	//			Ҳ����˵����ǰ�кü����ļ���û��ȫд����̣�������ͻ����Ҫ�󴴽��µ��ļ���
	//			�� SDataPack_SourceFileInfo::
	// ����(S->D)
	// ���壺��
	// ���ݰ���SDataPack_SourceFileInfo
	// ����ֵ���ɹ��� ErrorHandlingFlag_Success; ���ԣ�ErrorHandlingFlag_Ignore; �˳���ErrorHandlingFlag_Exit
	//EDC_CreateFile,	

	// ���ܣ����� destination filter �����ļ�,�������destination filter���Ǵ��ж����ģ�
	//			Ҳ����˵����ǰ�кü����ļ���û��ȫд����̣�������ͻ����Ҫ�󴴽��µ��ļ���
	//			������ܱ�transfer filter �������������ڷ���ʱ��Ӧ��֪�����������Ҫô�ɹ���Ҫôʧ��
	// ����(S->D)
	// ���壺��
	// ���ݰ���SDataPack_CreateFileInfo
	// ����ֵ���ɹ��� ErrorHandlingFlag_Success; ���ԣ�ErrorHandlingFlag_Ignore; �˳���ErrorHandlingFlag_Exit
	EDC_BatchCreateFile,	

	// ���ܣ�(δʹ�ã�������) �ļ���Ϣ
	// ����(S->D)
	// ���ݰ�: SDataPack_SourceFileInfo
	// ����ֵ�� ��
	//EDC_FileInfo,   

	// ���ܣ��ļ�����, �������ڴ���transfer filter�ｫ����������������destination filter����ȥ��ֱ���յ� EDC_FlushData ����Ž�������������´�
	//			���������ڲ���transfer filter����ֱ����destination filter����ȥ
	//			�����ݰ�����ΪNULLʱ��˵�����δ����У����δ�������Щ���л�
	//			��SDataPack_FileData��Ա����pData==NULL����ô˵��������֪�����ļ�������
	// ����(S->T) ��T->D��
	// ���壺��
	// ���ݰ���SDataPack_FileData
	// ����ֵ���ɹ��� ErrorHandlingFlag_Success; ���ԣ�ErrorHandlingFlag_Ignore; �˳���ErrorHandlingFlag_Exit
	EDC_FileData, 

	// ���ܣ� Ҫ����������ļ����ݻ��幦�ܵ�FILTER�ѻ�����������д��洢����
	// ����(S->T)
	// ���壺��
	// ���ݰ�����
	// ����ֵ���ɹ��� ErrorHandlingFlag_Success; ���ԣ�ErrorHandlingFlag_Ignore; �˳���ErrorHandlingFlag_Exit
	EDC_FlushData, 

	// ���ܣ� �ļ���HASHֵ
	// ���򣺣�T->D��
	// ���壺��
	// ���ݰ�: SDataPack_FileHash
	// ����ֵ��(����)
	EDC_FileHash, 

	//EDC_InvalidBranch, //(δ���ã�������) (D->S)��Ϊ�ж��д���֧��������һ����֧д��ʧ���ҷ�������д���Filter������������source filter

	// ���ܣ� �����ڸ�Դ�ļ��Ĳ�������ɣ� destination filter Ӧ���� EDC_FileDoneConfirm ���
	//			��Ϊtransfer filter���л������ã����� EDC_FileDoneConfirm ����������첽����
	// ���򣺣�S->T->D��
	// ���壺��
	// ���ݰ��� SDataPack_FileOperationCompleted
	// ����ֵ�� ��������
	EDC_FileOperationCompleted, 

	// ���ܣ� ��destination filter����д���ļ����ֻ�ȫ��ʧ�ܣ�����Ҫ��source filter�ط����ֻ�ȫ������
	// ���� ��D->S��
	// ���ݰ��� SDataPack_ResendFileData
	// ����ֵ��(����)
	EDC_ResendFileData,

	// ���ܣ� ���ļ������ѳɹ�д����̣����͸���� transfer filter
	// ���� ��D->S��
	// ���ݰ��� SDataPack_FileDataIOComplete
	// ����ֵ��(����)
	//EDC_DstFileDataIOComplete,

	// ���ܣ� destination filter ȷ���ѳɹ�д���ˡ���destination filterд����ʳɹ����source filter���ص�ȷ�ϣ���ʹsource filter�رո��ļ���Դ
	// ���� ��D->S��
	// ���ݰ���SDataPack_FileDoneConfirm
	// ����ֵ�� ��
	EDC_FileDoneConfirm, // (D->S)�ļ�д����� . 
	
	// ���ܣ� �����ڸ��ļ��е����ļ����ļ��в��������
	// ���� ��S->D��
	// ���ݰ�����
	// ����ֵ����
	//EDC_FolderChildrenOperationCompleted,

	// ���ܣ� ��link�����в��������
	// ���� ��S->D��
	// ���ݰ���SDataPack_FileOperationCompleted
	// ����ֵ����
	EDC_LinkEnded
};

class CXCFileDataBuffer ;
class CXCFileChangingBuffer ;
enum ECopyFileState ;
enum EImpactFileBehaviorResult ;

// ����LINK�����ݰ�
struct SDataPack_LinkIni
{
	CXCFileDataBuffer*		pFileDataBuf ;
	ECopyFileState*			pCFState ;
	CXCFileChangingBuffer*	pFileChangingBuf ;
	EImpactFileBehaviorResult*	pDefaultImpactFileBehavior ; // Ĭ�ϵĳ�ͻ�ļ������������� destination filter �Լ��� XCCore�����õ�MoveFileEx���õ�
};

// filter �� LINK ��Ϣ
struct SFilterLinkInfo
{
	unsigned	uID ;
	pt_STL_list(CXCFilter*)	FilterList ;
};

// Ҫ���Ƶ�"Դ�ļ�"��Ϣ, ��Ӧ EFilterCmd::EDC_FileInfo
// ����Ϣ����source filter ���ļ�ʱ��������source filter�� destinatin filter���� 
// EDC_CreateFile �� EDC_FileOperationCompleted ʱ��Я������Ϣ�壬 ��Ϊ EDC_FileOperationCompleted
// �л��幦�ܣ����Ը���Ϣ�弴ʹ�� source filter ��ȫ����ȡ�ļ����ݺ�Ҳ���ἴʱ�ͷţ����Ǽ����� ��ȷ����ɻ�����С�
// �ֱ�� source filter �յ����� EDC_FileDoneConfirm ����ʱ���ͷŻػ����� ��
// ������Ϣ��Ҳ����һЩ�����;�ͷţ����磺 ���ļ�����;����������
struct SDataPack_SourceFileInfo
{
#define DP_SFI_PROPERTY_MASK_LOCAL				1<<0 // �Ƿ�Ϊ�����ļ�
#define DP_SFI_PROPERTY_MASK_DISCARD			1<<1 // �Ƿ������ļ�
#define DP_SFI_PROPERTY_MASK_ERROR				1<<3 // �Ƿ�Ϊ�����ļ�


	unsigned			uFileID ;		// �ļ�ID
	CptString			strSourceFile ; // Դ�ļ�����������·����
	CptString			strNewFileName ; // �µ��ļ����������Ϊ�գ�������ʹ�ø��ļ�����Ϊdestination���ļ���
										// ���ļ���Ϊ raw �ļ���
	DWORD				dwSourceAttr ; // �ļ�������
	SFileTimeInfo		SrcFileTime ; // �ļ�ʱ��
	unsigned __int64	nFileSize ; // �ļ���С
	int					nSysErrCode ; // �� source filter ��������error code
	BYTE				byProperty ;

	static pt_STL_list(SDataPack_SourceFileInfo*) CacheList ; // ��������ڴ���ظ�ʹ��
	static CptCritiSecLock			m_ThreadLock ;

	SDataPack_SourceFileInfo()
	{
		this->Ini() ;
	}

	bool IsDiscard() const
	{
		return (byProperty&DP_SFI_PROPERTY_MASK_DISCARD) ? true : false ;
	}

	bool IsError() const
	{
		return (byProperty&DP_SFI_PROPERTY_MASK_ERROR) ? true : false ;
	}

	bool IsLocal() const
	{
		return (byProperty&DP_SFI_PROPERTY_MASK_LOCAL) ? true : false ;
	}

	void SetDiscard(const bool bDiscard=true)
	{
		if(bDiscard)
		{
			byProperty |= DP_SFI_PROPERTY_MASK_DISCARD ;
		}
		else
		{
			byProperty &= ~DP_SFI_PROPERTY_MASK_DISCARD ;
		}
		
	}

	void SetError(const bool bError=true)
	{
		if(bError)
		{
			byProperty |= DP_SFI_PROPERTY_MASK_ERROR ;
		}
		else
		{
			byProperty &= ~DP_SFI_PROPERTY_MASK_ERROR ;
		}
		
	}

	void Ini()
	{
		uFileID = 0 ;
		strSourceFile = _T("") ;
		dwSourceAttr = 0 ;
		nFileSize = 0 ;
		byProperty = 0 ;
		nSysErrCode = 0 ;

		byProperty |= DP_SFI_PROPERTY_MASK_LOCAL ;
	}

	static SDataPack_SourceFileInfo* Allocate()
	{
		SDataPack_SourceFileInfo* pRet = NULL ;

		CptAutoLock lock(&m_ThreadLock) ;
		if(!CacheList.empty())
		{
			pRet = CacheList.front();
			CacheList.pop_front() ;
		}
		else
		{
			pRet = new SDataPack_SourceFileInfo() ;
		}

		return pRet ;
	}

	static void Free(SDataPack_SourceFileInfo* pFpi)
	{
		if(pFpi!=NULL)
		{
			pFpi->Ini() ;

			CptAutoLock lock(&m_ThreadLock) ;
			CacheList.push_back(pFpi) ;
		}
	}

	static void ReleaseBuffer()
	{
		CptAutoLock lock(&m_ThreadLock) ;

		pt_STL_list(SDataPack_SourceFileInfo*)::iterator it = CacheList.begin() ;

		for(;it!=CacheList.end();++it)
		{
			if((*it)!=NULL)
			{
				delete (*it) ;
			}
		}

		CacheList.clear() ;
	}
};

// ���� EDC_BatchCreateFile ���������
struct SDataPack_CreateFileInfo
{
	pt_STL_list(SDataPack_SourceFileInfo*) SourceFileInfoList ; // Ҫ�������ļ�
	//bool bExistDiscard ; // �Ƿ����Ҫ���������ļ����ļ��У����������������destination filter��д��
	//pt_STL_vector(CptString)	DiscardFolderVer ; // ���������ļ��У����������������destination filter��д��
};

//�����ļ����� 
struct SDataPack_CheckFileInfo
{
	CptString	strFileName ; // �������ļ�·��
	bool		bFileOrFolder ;
	bool		bDiscard ;
};

// �ļ�����, ��Ӧ EFilterCmd::EDC_FileData
struct SDataPack_FileData
{
	unsigned	uFileID ;
	//unsigned __int64	uReadBeginPos ; // ���Ŀ�ʼλ��
	//unsigned __int64	uWriteBeginPos ; // д�Ŀ�ʼλ��
	int			nDataSize ;	// ��Ч���ݴ�С
	int			nBufSize ; // nBufSize �� nDataSize ��һ��һ�£����ļ�β�εĳ��Ȳ���sector�ı���ʱ���ͻ���ֲ�һ��
							// ����֮�����ٿ�һ��������¼������Ϊ��ֹ XCFileDataBuffer allocʱ��BufSize,freeʱ��DataSize
							// �������ܹ���ֹ�ռ�й©
	void*		pData ;		// �ļ�����
	//bool		bDiscard ;

	SDataPack_FileData()
	{
		::memset(this,0,sizeof(*this)) ;
	}
};

// �����ڸ��ļ����������, ��Ӧ�� EFilterCmd::EDC_FileOperationCompleted
struct SDataPack_FileOperationCompleted
{
	pt_STL_list(SDataPack_SourceFileInfo*)	CompletedFileInfoList ;
};

// �ļ�HASHֵ����Ӧ EFilterCmd::EDC_FileHash
struct SDataPack_FileHash
{
	unsigned		uFileID ;			// �ļ�IDֵ
	EFileHashType	HashType ;		// HASHֵ����
	int				nDataSize ; 	// HASH��Ч���ݴ�С
	void*			pHashValue ; 	// HASH����

	SDataPack_FileHash()
	{
		::memset(this,0,sizeof(*this)) ;
	}
};

// ���·���ָ���ļ������ݣ� ��Ӧ���� EDC_ResendFileData
struct SDataPack_ResendFileData
{
	unsigned uFileID ;
	CptString		strFileNameWithURI ;
	CptString		strDestRoot ;
	unsigned __int64	uFileBeginPos ;
};

// ��Ӧ���� EDC_FileDoneConfirm
struct SDataPack_FileDoneConfirm
{
	pt_STL_list(unsigned) FileDoneConfirmList ; // ������file ID
	//unsigned uFileID ;
};