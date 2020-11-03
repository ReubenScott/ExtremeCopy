
#pragma once

// �Զ�����Ϣ
#define WM_CONTROLSTATUSCHANGED		WM_USER+800  // �ؼ�״̬�ı� (wParam: �ؼ�ID; lParam:��״̬[EControlStatus ����])

// �ؼ����״̬
enum EControlMouseStatus
{
	ControlMouseStatus_Normal = 0,
	ControlMouseStatus_Leave = 1,
	ControlMouseStatus_Hover = 2,
	ControlMouseStatus_Down = 3,
};

// �ؼ���Ч״̬
enum EControlEnableStatus
{
	ControlEnableStatus_Enable = 0,
	ControlEnableStatus_Disable = 1,
};

struct SControlStatus
{//�ؼ�״̬
	union
	{
		struct SStatus
		{
			EControlMouseStatus MouseStatus : 4 ;
			int Enable : 1 ;
		} status ;
		unsigned int uStatusBlock ;
	};

	SControlStatus()
	{
		this->status.Enable = ControlEnableStatus_Enable ;
		this->status.MouseStatus = ControlMouseStatus_Normal ;
	}
};

