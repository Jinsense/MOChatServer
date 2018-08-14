#ifndef _CHATSERVER_SERVER_MATCHING_H_
#define _CHATSERVER_SERVER_MATCHING_H_

#include <iostream>
#include <list>
#include <map>
#include <stack>
#include <windows.h>
#include <chrono>

#include "Config.h"
#include "CpuUsage.h"
#include "EtherNet_PDH.h"
#include "LanGameClient.h"
#include "NetServer.h"
#include "Player.h"	

using namespace std;

enum en_RES_LOGIN
{
	LOGIN_SUCCESS = 1,
	SESSIONKEY_ERROR = 2,
	ACCOUNTNO_NOT_EXIST = 3,
	ETC_ERROR = 4,
	VER_ERROR = 5,
};

class CGameLanClient;

class CChatServer : public CNetServer
{
public:
	CChatServer();
	virtual ~CChatServer();

protected:
	//-----------------------------------------------------------
	// �����Լ���
	//-----------------------------------------------------------
	virtual void	OnClientJoin(st_SessionInfo Info);
	virtual void	OnClientLeave(unsigned __int64 ClientID);
	virtual void	OnConnectionRequest(WCHAR * pClientID);
	virtual void	OnError(int ErrorCode, WCHAR *pError);
	virtual bool	OnRecv(unsigned __int64 ClientID, CPacket *pPacket);
	virtual void	MonitorThread_Update();

	//-----------------------------------------------------------
	// ���Ӽ��� ���� üũ
	//-----------------------------------------------------------
	static unsigned int WINAPI LanGameCheckThread(LPVOID arg)
	{
		CChatServer *_pLanMasterCheckThread = (CChatServer *)arg;
		if (NULL == _pLanMasterCheckThread)
		{
			std::wprintf(L"[Server :: LanMonitoringThread] Init Error\n");
			return false;
		}
		_pLanMasterCheckThread->LanGameCheckThread_Update();
		return true;
	}
	void LanGameCheckThread_Update();
	//-----------------------------------------------------------
	//	��Ʈ��Ʈ ������
	//-----------------------------------------------------------
	static unsigned int WINAPI HeartbeatThread(LPVOID arg)
	{
		CChatServer *_pHeartbeatThread = (CChatServer *)arg;
		if (NULL == _pHeartbeatThread)
		{
			std::wprintf(L"[Server :: HeartbeatThread] Init Error\n");
			return false;
		}
		_pHeartbeatThread->HeartbeatThread_Update();
		return true;
	}
	void HeartbeatThread_Update();

public:
	//-----------------------------------------------------------
	//	����� �Լ�
	//-----------------------------------------------------------
	void	ConfigSet();
	void	UTF8toUTF16(const char *szText, WCHAR *szBuf, int BufLen);
	void	UTF16toUTF8(WCHAR *szText, char *szBuf, int BufLen);

	//-----------------------------------------------------------
	// OnClientJoin, OnClientLeave ���� ȣ��
	//-----------------------------------------------------------
	bool	InsertPlayer(unsigned __int64 ClientID, CPlayer* pPlayer);
	bool	RemovePlayer(unsigned __int64 ClientID);

	//-----------------------------------------------------------
	// TimeOut ���� ����
	//-----------------------------------------------------------
	bool	DisconnectPlayer(unsigned __int64 ClientID, INT64 AccountNo);

	//-----------------------------------------------------------
	// ���� ����� �� 
	//-----------------------------------------------------------
	int		GetPlayerCount(void);

	//-----------------------------------------------------------
	// ������ �÷��̾� ã�� 
	//-----------------------------------------------------------
	CPlayer*	FindPlayer_ClientID(unsigned __int64 ClientID);
	unsigned __int64	FindPlayer_AccountNo(INT64 AccountNo);

	//-----------------------------------------------------------
	// White IP ����
	//-----------------------------------------------------------
	

public:
	CLanGameClient * _pGameServer;
	CSystemLog *	_pLog;
	char	_CurConnectToken[32];
	char	_OldConnectToken[32];

protected:
	SRWLOCK	_DB_srwlock;
	HANDLE	_hHeartbeatThread;

	//-------------------------------------------------------------
	// ������ ����.
	// 
	// �����ڴ� �� ����Ʈ�� ������.  ������ ����ȭ�� ���� SRWLock �� ����Ѵ�.
	//-------------------------------------------------------------
	std::map<unsigned __int64, CPlayer*>	_PlayerMap;
	SRWLOCK				_PlayerMap_srwlock;

	CMemoryPool<CPlayer> *_PlayerPool;
	//-------------------------------------------------------------
	// ����͸��� ����
	//-------------------------------------------------------------
	long	_JoinSession;				//	�α��� ���� ���� ��

};

#endif