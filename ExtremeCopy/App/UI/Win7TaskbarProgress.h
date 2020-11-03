/****************************************************************************

Copyright (c) 2008-2020 Kevin Wu (Wu Feng)

github: https://github.com/kevinwu1024/ExtremeCopy
site: http://www.easersoft.com

Licensed under the Apache License, Version 2.0 License (the "License"); you may not use this file except
in compliance with the License. You may obtain a copy of the License at

https://opensource.org/licenses/Apache-2.0

****************************************************************************/

#pragma once
#include <ShObjIdl.h>


class CWin7TaskbarProgress
{
public:
	CWin7TaskbarProgress() ;
	~CWin7TaskbarProgress() ;

	void SetProgressValue(HWND hWnd,int nValue) ; // nValueֵ�ķ�Χ0~ 100�� �԰ٷֱ����룬 ��70%����valueΪ 70
private:
	ITaskbarList3*			m_pTaskbarList ;
};