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
	LOGIN_FAIL = 0,
};

enum en_RES_ENTERROOM
{
	SUCCESS = 1,
	TOKEN_ERROR = 2,
	NOT_FIND = 3,
};

typedef struct st_RoomPlayer
{
	unsigned __int64 ClientID;
	INT64 AccountNo;
}RoomPlayerInfo;

typedef struct st_BattleRoom
{
	int		ServerNo;
	int		RoomNo;
	int		MaxUser;
	char	EnterToken[32];
	std::list<RoomPlayerInfo> RoomPlayer;
	SRWLOCK		Room_lock;
}BATTLEROOM;

class CGameLanClient;

class CChatServer : public CNetServer
{
public:
	CChatServer();
	virtual ~CChatServer();

protected:
	//-----------------------------------------------------------
	// 가상함수들
	//-----------------------------------------------------------
	virtual void	OnClientJoin(st_SessionInfo Info);
	virtual void	OnClientLeave(unsigned __int64 ClientID);
	virtual void	OnConnectionRequest(WCHAR * pClientID);
	virtual void	OnError(int ErrorCode, WCHAR *pError);
	virtual bool	OnRecv(unsigned __int64 ClientID, CPacket *pPacket);
	virtual void	MonitorThread_Update();

	//-----------------------------------------------------------
	// 게임서버 연결 체크
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
	//	하트비트 스레드
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
	//	사용자 함수
	//-----------------------------------------------------------
	void	ConfigSet();
	void	UTF8toUTF16(const char *szText, WCHAR *szBuf, int BufLen);
	void	UTF16toUTF8(WCHAR *szText, char *szBuf, int BufLen);

	//-----------------------------------------------------------
	// OnClientJoin, OnClientLeave 에서 호출
	//-----------------------------------------------------------
	bool	InsertPlayer(unsigned __int64 ClientID, CPlayer* pPlayer);
	bool	RemovePlayer(unsigned __int64 ClientID);

	//-----------------------------------------------------------
	// TimeOut 유저 끊기
	//-----------------------------------------------------------
	bool	DisconnectPlayer(unsigned __int64 ClientID, INT64 AccountNo);

	//-----------------------------------------------------------
	// 배틀 서버와 연결 끊겼을 경우 모든 방과 유저 끊기
	//-----------------------------------------------------------
	void	BattleDisconnect();

	//-----------------------------------------------------------
	// 배틀 방 찾기
	//-----------------------------------------------------------
	BATTLEROOM * FindBattleRoom(int RoomNo);

	//-----------------------------------------------------------
	// 배틀 방 갯수
	//-----------------------------------------------------------
	int		GetBattleRoomCount(void);
	//-----------------------------------------------------------
	// 접속 사용자 수 
	//-----------------------------------------------------------
	int		GetPlayerCount(void);

	//-----------------------------------------------------------
	// 접속한 플레이어 찾기 
	//-----------------------------------------------------------
	CPlayer*	FindPlayer_ClientID(unsigned __int64 ClientID);
	unsigned __int64	FindPlayer_AccountNo(INT64 AccountNo);

	//-----------------------------------------------------------
	// White IP 관련
	//-----------------------------------------------------------
	
	//-----------------------------------------------------------
	// 패킷처리 
	//-----------------------------------------------------------
	void	ReqChatLogin(CPacket * pPacket, CPlayer * pPlayer);
	void	ReqEnterRoom(CPacket * pPacket, CPlayer * pPlayer);
	void	ReqSendMsg(CPacket * pPacket, CPlayer * pPlayer);

public:
	CLanGameClient * _pGameServer;
	CSystemLog *	_pLog;
	char	_CurConnectToken[32];
	char	_OldConnectToken[32];

	//-------------------------------------------------------------
	// BattleRoom 방 관리
	// 
	// 방은 본 Map으로 관리함. 스레드 동기화를 위해 SRWLock을 사용한다.
	//-------------------------------------------------------------
	std::map<int, BATTLEROOM*> _BattleRoomMap;
	SRWLOCK		_BattleRoom_lock;

	CMemoryPool<BATTLEROOM>	*_BattleRoomPool;

protected:
	HANDLE	_hHeartbeatThread;

	//-------------------------------------------------------------
	// 접속자 관리.
	// 
	// 접속자는 본 Map으로 관리함.  스레드 동기화를 위해 SRWLock을 사용한다.
	//-------------------------------------------------------------
	std::map<unsigned __int64, CPlayer*>	_PlayerMap;
	SRWLOCK				_PlayerMap_srwlock;

	CMemoryPool<CPlayer> *_PlayerPool;
	//-------------------------------------------------------------
	// 모니터링용 변수
	//-------------------------------------------------------------
	long	_JoinSession;				//	로그인 성공 유저 수

};

#endif