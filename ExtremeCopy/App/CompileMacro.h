/****************************************************************************

Copyright (c) 2008-2020 Kevin Wu (Wu Feng)

github: https://github.com/kevinwu1024/ExtremeCopy
site: http://www.easersoft.com

Licensed under the Apache License, Version 2.0 License (the "License"); you may not use this file except
in compliance with the License. You may obtain a copy of the License at

https://opensource.org/licenses/Apache-2.0

****************************************************************************/

#pragma once

#define		VERSION_PROFESSIONAL	// רҵ�汾

#ifndef VERSION_PROFESSIONAL
//#define		VERSION_PORTABLE		// portable version
#endif

//#define		VERSION_TEST_BETA
//#define		VERSION_TEST_ALPHA

#define		TEST_VERSION_NUMBER		5	// ���԰汾��

// �������ڹ����Ĳ��԰�ļ�����ڵģ�����������Ч�Ĳ��԰���������
#define		BETA_VERSION_EXPIRE_DATE	_T("2012-10-01 11:00:00")

//#define COMPILE_NOTDATAOCCUREDREAD		//�������ݷ���ʱ���ص�

#define		VERSION_UNLIMIT_FILE_NAME_LENGTH	// �Ƿ�Ӧ�������Ƴ��ȵ��ļ���

//#define		COMPILE_TEST_PERFORMANCE		// ���ü���ض�����ģ�������

#if defined(VERSION_TEST_BETA) || defined(VERSION_TEST_ALPHA)
#define VERSION_TEST // �Ƿ���԰汾
#endif


#if		defined(VERSION_PROFESSIONAL) && !defined(VERSION_TEST) && !defined(_DEBUG)
//#define		VERSION_CHECKREGSITER // �Ƿ�ʹ��ע����
#endif

#if defined(VERSION_PROFESSIONAL)
#define		VERSION_HDD_QUEUE // �Ƿ��HDD���ж��и���
#endif

/////---------------------------------------

// message ������ȫ�������ͣ��ʱ��
#define		MESSAGE_WINDOW_STAY_LAST_TIME		8