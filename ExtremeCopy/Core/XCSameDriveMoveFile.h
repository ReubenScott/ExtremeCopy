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
#include "..\Common\ptTypeDef.h"
#include <vector>
#include "XCCoreDefine.h"

class CXCSameDriveMoveFile
{
public:
	CXCSameDriveMoveFile(ECopyFileState* pRunState,EImpactFileBehaviorResult* pBehaviorResult);
	virtual ~CXCSameDriveMoveFile(void);

	bool Run(const SGraphTaskDesc& gtd) ;

private:
	int CheckAndUpdateImpactFileName(const SGraphTaskDesc& gtd,CptString strSrcFile,CptString& strDstFile) ;
	ErrorHandlingResult MoveItems(const SGraphTaskDesc& gtd,CptString strSubDir,CptString strParentDir) ; // ����Ŀ¼�ƶ������е��ļ����ļ��е���ͬ���ĸ�Ŀ¼��
	ErrorHandlingResult MoveFile(const SGraphTaskDesc& gtd,CptString strSrcFile,CptString strDstFolder,bool bFolder) ;

	// �ж�Ŀ¼��ϵ���Ƿ�ԴĿ¼�ĸ�Ŀ¼��Ŀ��Ŀ¼����Ŀ¼����ԴĿ¼��ø�Ŀ¼ͬ��
	inline bool IsSameNameAndSubdir(CptString strSrcDir,CptString strDstDir) ;

private:
	EImpactFileBehaviorResult	*m_pImpactFileBehavior ; // ��ͻ�ļ��Ĵ���ʽ
	ECopyFileState			*m_pRunningState ;	// ����״̬
};

