#include "StdAfx.h"
#include "XCMessageDlg.h"


CXCMessageDlg::CXCMessageDlg()
	:CptDialog(IDD_DIALOG_MESSAGE,NULL,CptMultipleLanguage::GetInstance()->GetResourceHandle())
{
}


CXCMessageDlg::~CXCMessageDlg(void)
{
	Debug_Printf(_T("CXCMessageDlg::~CXCMessageDlg()")) ;
}


BOOL CXCMessageDlg::OnInitDialog()
{
	int nScreenWidth = 0 ;
	m_nStayTimeCounter = 0 ;
	m_nScreenHeight = 0 ;

	CptGlobal::GetScreenSize(&nScreenWidth,&m_nScreenHeight) ;

	SptRect rect;
	::GetWindowRect(this->GetSafeHwnd(),rect.GetRECTPointer()) ;

	::SetWindowPos(this->GetSafeHwnd(),HWND_TOPMOST,nScreenWidth-rect.GetWidth(),m_nScreenHeight-rect.GetHeight(),0,0,SWP_NOSIZE) ;

	m_MovmentState = MS_Unknown ;

	this->ChangeState(MS_MoveUp) ;

	return FALSE ;
}

void CXCMessageDlg::OnClose()
{
	delete this ;// ����һ����ģ̬�Ի��������ڹر�ʱ���ͷ��Լ�
}

void CXCMessageDlg::SetMessage(const CptString& strMsg) 
{
	m_strMessage = strMsg ;
}

bool CXCMessageDlg::OnCancel()
{
	this->ChangeState(MS_MoveDown) ;

	return false ;
}


void CXCMessageDlg::OnButtonClick(int nButtonID)
{
	//if(nButtonID==IDC_BUTTON_CLOSE)
	//{
	//}
}

void CXCMessageDlg::OnTimer(int nTimerID) 
{
	switch(nTimerID)
	{
	case TIMER_ID_MOVEUP:// �����ƶ�
		{
			const int nStep = 2 ;
			SptRect rect;
			::GetWindowRect(this->GetSafeHwnd(),rect.GetRECTPointer()) ;

			if(rect.nBottom>m_nScreenHeight)
			{
				::SetWindowPos(this->GetSafeHwnd(),HWND_TOPMOST,rect.nLeft,rect.nTop-nStep,0,0,SWP_NOSIZE) ;
			}
			else
			{
				this->ChangeState(MS_Stay) ;
			}
		}
		break ;

	case TIMER_ID_MOVEDOWN: // �����ƶ�
		break ;

	case TIMER_ID_STAY: // ͣ��
		{
			// �������ͣ��ʱ�䣬�������˳�
			if(m_nStayTimeCounter++ >MESSAGE_WINDOW_STAY_LAST_TIME)
			{
				this->ChangeState(MS_MoveDown) ;
			}
		}
		break ;
	}
}

void CXCMessageDlg::ChangeState(const EMovmentState NewState)
{
	if(m_MovmentState==NewState)
	{
		return ;
	}

	switch(NewState)
	{
	case MS_MoveDown:
		{
			_ASSERT(m_MovmentState==MS_MoveUp || m_MovmentState==MS_Stay) ;

			m_MovmentState = MS_MoveDown ;

			// ������ move down ֱ�ӹرմ���
			this->Close() ; 
		}
		break ;

	case MS_MoveUp:
		{
			_ASSERT(m_MovmentState==MS_Unknown) ;

			m_MovmentState = MS_MoveUp ;

			this->SetTimer(TIMER_ID_MOVEUP,50) ;

		}
		break ;

	case MS_Stay:
		{
			_ASSERT(m_MovmentState==MS_MoveUp) ;

			m_MovmentState = MS_Stay ;

			this->SetTimer(TIMER_ID_STAY,1000) ;
		}
		break ;

	default:
		_ASSERT(FALSE) ;
		break ;
	}


	//if(bMoveUp)
	//{
	//	_ASSERT(MS_Unknown==m_MovmentState) ;

	//	if(m_MovmentState!=MS_MoveUp)
	//	{
	//		m_MovmentState =  MS_MoveDown ;

	//		this->SetTimer(TIMER_ID_MOVEUP) ;
	//	}
	//}
	//else
	//{
	//	if(m_MovmentState!=MS_MoveDown)
	//	{
	//		m_MovmentState =  MS_MoveDown ;

	//		// �ر����п������ڽ����е� timer
	//		this->KillTimer(TIMER_ID_MOVEUP) ;
	//		this->KillTimer(TIMER_ID_STAY) ;

	//		this->SetTimer(TIMER_ID_MOVEDOWN) ;
	//	}
	//}
	
}