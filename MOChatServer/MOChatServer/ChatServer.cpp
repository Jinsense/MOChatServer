#include "ChatServer.h"

CChatServer::CChatServer()
{
	InitializeSRWLock(&_BattleRoom_lock);
	InitializeSRWLock(&_PlayerMap_srwlock);
	ZeroMemory(&_CurConnectToken, sizeof(_CurConnectToken));
	ZeroMemory(&_OldConnectToken, sizeof(_OldConnectToken));

	_BattleRoomPool = new CMemoryPool<BATTLEROOM>();
	_PlayerPool = new CMemoryPool<CPlayer>();
	_pGameServer = new CLanGameClient;
	_pGameServer->Constructor(this);

	_pLog = _pLog->GetInstance();

	_JoinSession = NULL;
}

CChatServer::~CChatServer()
{
	delete _BattleRoomPool;
	delete _PlayerPool;
	delete _pGameServer;
}

void CChatServer::OnClientJoin(st_SessionInfo Info)
{
	//-------------------------------------------------------------
	//	맵에 유저 추가
	//-------------------------------------------------------------
	CPlayer *pPlayer = _PlayerPool->Alloc();
	pPlayer->Init();
	pPlayer->_ClientID = Info.iClientID;
	pPlayer->_Time = GetTickCount64();
	InsertPlayer(pPlayer->_ClientID, pPlayer);
	return;
}

void CChatServer::OnClientLeave(unsigned __int64 ClientID)
{
	CPlayer * pPlayer = FindPlayer_ClientID(ClientID);
	if (nullptr == pPlayer)
	{
		_pLog->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM, const_cast<WCHAR*>(L"FindClientID Fail [ClientID : %d]"), ClientID);
		return;
	}
	//-------------------------------------------------------------
	//	맵에 유저 삭제
	//-------------------------------------------------------------
	RemovePlayer(ClientID);
	_PlayerPool->Free(pPlayer);
	return;
}

void CChatServer::OnConnectionRequest(WCHAR * pClientID)
{
	//-------------------------------------------------------------
	//	화이트 IP 처리
	//	현재 사용 X
	//-------------------------------------------------------------
	return;
}

void CChatServer::OnError(int ErrorCode, WCHAR *pError)
{
	//-------------------------------------------------------------
	//	에러 처리 - 로그 남김
	//	별도의 합의된 로그 코드를 통해 로그를 txt에 저장
	//	현재 사용 X
	//-------------------------------------------------------------
	return;
}

bool CChatServer::OnRecv(unsigned __int64 ClientID, CPacket *pPacket)
{
	//-------------------------------------------------------------
	//	모니터링 측정 변수
	//-------------------------------------------------------------
	m_iRecvPacketTPS++;
	//-------------------------------------------------------------
	//	예외 처리 - 현재 존재하는 유저인지 검사
	//-------------------------------------------------------------
	CPlayer *pPlayer = FindPlayer_ClientID(ClientID);
	if (nullptr == pPlayer)
	{
		Disconnect(ClientID);
		return false;
	}
	//-------------------------------------------------------------
	//	패킷 처리 - 컨텐츠 처리
	//-------------------------------------------------------------
	WORD Type;
	*pPacket >> Type;
	//-------------------------------------------------------------
	//	패킷 처리 - 채팅 서버로 로그인 요청
	//-------------------------------------------------------------
	if (Type == en_PACKET_CS_CHAT_REQ_LOGIN)
	{
		ReqChatLogin(pPacket, pPlayer);
		pPlayer->_Time = GetTickCount64();
	}
	//-------------------------------------------------------------
	//	패킷 처리 - 채팅 서버 방 입장
	//-------------------------------------------------------------
	else if (Type == en_PACKET_CS_CHAT_REQ_ENTER_ROOM)
	{
		ReqEnterRoom(pPacket, pPlayer);
		pPlayer->_Time = GetTickCount64();
	}
	//-------------------------------------------------------------
	//	패킷 처리 - 채팅 보내기 요청
	//-------------------------------------------------------------
	else if (Type == en_PACKET_CS_CHAT_REQ_MESSAGE)
	{
		ReqSendMsg(pPacket, pPlayer);
		pPlayer->_Time = GetTickCount64();
	}
	//-------------------------------------------------------------
	//	패킷 처리 - 하트 비트
	//-------------------------------------------------------------
	else if (Type == en_PACKET_CS_CHAT_REQ_HEARTBEAT)
	{
		pPlayer->_Time = GetTickCount64();
	}
	return true;
}

void CChatServer::HeartbeatThread_Update()
{
	//-------------------------------------------------------------
	//	Heartbeat Thread
	//	StatusDB에 일정시간마다 현재 매치서버 Timestamp 값 갱신
	//	전체 유저 대상으로 Heartbeat 기준으로 TimeOut 체크 
	//	현재 유저가 범위 이상 변화가 발생했을 때 DB에 connectuser 값 갱신
	//-------------------------------------------------------------
	UINT64 start = GetTickCount64();
	int count = GetPlayerCount();
	int usercount = NULL;
	std::stack<unsigned __int64> temp;

	while (1)
	{
		Sleep(1000);
		UINT64 now = GetTickCount64();

		/*
		AcquireSRWLockExclusive(&_PlayerMap_srwlock);
		for (auto i = _PlayerMap.begin(); i != _PlayerMap.end(); i++)
		{
		if (now - i->second->_Time > _Config.USER_TIMEOUT)
		{
		_pLog->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM, const_cast<WCHAR*>(L"USER TIMEOUT : %d [AccountNo : %d]"), now - i->second->_Time, i->second->_AccountNo);
		//	바로 Disconnect 하면 데드락 위험성
		//	임시 stack에 넣고 마지막에 Disconnect 호출할 것
		temp.push(i->second->_ClientID);
		}
		}
		ReleaseSRWLockExclusive(&_PlayerMap_srwlock);
		while (!temp.empty())
		{
		unsigned __int64 ClientID = temp.top();
		Disconnect(ClientID);
		temp.pop();
		}
		*/
	}
}

void CChatServer::LanGameCheckThread_Update()
{
	//-------------------------------------------------------------
	//	게임 서버 연결 주기적으로 확인
	//-------------------------------------------------------------
	while (1)
	{
		Sleep(5000);
		if (false == _pGameServer->IsConnect())
		{
			_pGameServer->Connect(_Config.BATTLE_IP, _Config.BATTLE_PORT, true, LANCLIENT_WORKERTHREAD);
			continue;
		}
	}
	return;
}

void CChatServer::ConfigSet()
{
	//-------------------------------------------------------------
	//	Config 파일 불러오기
	//-------------------------------------------------------------
	_Config.Set();
	return;
}

void CChatServer::UTF8toUTF16(const char *szText, WCHAR *szBuf, int iBufLen)
{
	//-------------------------------------------------------------
	//	UTF8 타입을 UTF16으로 변환
	//-------------------------------------------------------------
	int iRe = MultiByteToWideChar(CP_UTF8, 0, szText, strlen(szText), szBuf, iBufLen);
	if (iRe < iBufLen)
		szBuf[iRe] = L'\0';
	return;
}

void CChatServer::UTF16toUTF8(WCHAR *szText, char *szBuf, int iBufLen)
{
	//-------------------------------------------------------------
	//	UTF16 타입을 UTF8로 변환
	//-------------------------------------------------------------
	int iRe = WideCharToMultiByte(CP_UTF8, 0, szText, lstrlenW(szText), szBuf, iBufLen, NULL, NULL);
	return;
}

bool CChatServer::InsertPlayer(unsigned __int64 ClientID, CPlayer* pPlayer)
{
	//-------------------------------------------------------------
	//	Player Map에 클라이언트 추가
	//-------------------------------------------------------------
	AcquireSRWLockExclusive(&_PlayerMap_srwlock);
	_PlayerMap.insert(make_pair(ClientID, pPlayer));
	ReleaseSRWLockExclusive(&_PlayerMap_srwlock);
	return true;
}

bool CChatServer::RemovePlayer(unsigned __int64 ClientID)
{
	//-------------------------------------------------------------
	//	Player Map에 클라이언트 제거
	//-------------------------------------------------------------
	AcquireSRWLockExclusive(&_PlayerMap_srwlock);
	_PlayerMap.erase(ClientID);
	ReleaseSRWLockExclusive(&_PlayerMap_srwlock);
	return true;
}

bool CChatServer::DisconnectPlayer(unsigned __int64 ClientID, INT64 AccountNo)
{
	//-------------------------------------------------------------
	//	TimeOut 유저 끊기
	//	Log 남기고 Disconnect 호출
	//-------------------------------------------------------------
	_pLog->Log(const_cast<WCHAR*>(L"TimeOut"), LOG_SYSTEM,
		const_cast<WCHAR*>(L"TimeOut [AccountNo : %d]"), AccountNo);
	Disconnect(ClientID);
	return true;
}

void CChatServer::BattleDisconnect()
{
	//	모든 방의 유저들 끊고 해당 방 파괴
	AcquireSRWLockExclusive(&_BattleRoom_lock);
	for (auto Map = _BattleRoomMap.begin(); Map != _BattleRoomMap.end();)
	{
		AcquireSRWLockExclusive(&(*Map).second->Room_lock);
		for (auto RoomPlayer = (*Map).second->RoomPlayer.begin(); RoomPlayer != (*Map).second->RoomPlayer.end();)
		{
			Disconnect((*RoomPlayer).ClientID);
			RoomPlayer = (*Map).second->RoomPlayer.erase(RoomPlayer);
		}
		ReleaseSRWLockExclusive(&(*Map).second->Room_lock);

		_BattleRoomPool->Free((*Map).second);
		Map = _BattleRoomMap.erase(Map);
	}
	ReleaseSRWLockExclusive(&_BattleRoom_lock);
	return;
}

BATTLEROOM * CChatServer::FindBattleRoom(int RoomNo)
{
	std::map<int, BATTLEROOM*>::iterator iter;

	AcquireSRWLockExclusive(&_BattleRoom_lock);
	iter = _BattleRoomMap.find(RoomNo);
	ReleaseSRWLockExclusive(&_BattleRoom_lock);

	if (iter == _BattleRoomMap.end())
		return nullptr;
	else
		return (*iter).second;
}

int CChatServer::GetBattleRoomCount()
{
	//-------------------------------------------------------------
	//	현재 생성되어 있는 방 갯수 얻기
	//-------------------------------------------------------------
	return _BattleRoomMap.size();
}

int CChatServer::GetPlayerCount()
{
	//-------------------------------------------------------------
	//	현재 접속 중인 플레이어 수 얻기
	//-------------------------------------------------------------
	return _PlayerMap.size();
}

CPlayer* CChatServer::FindPlayer_ClientID(unsigned __int64 ClientID)
{
	CPlayer *pPlayer = nullptr;
	AcquireSRWLockExclusive(&_PlayerMap_srwlock);
	pPlayer = _PlayerMap.find(ClientID)->second;
	ReleaseSRWLockExclusive(&_PlayerMap_srwlock);
	return pPlayer;
}

unsigned __int64 CChatServer::FindPlayer_AccountNo(INT64 AccountNo)
{
	unsigned __int64 ClientID = NULL;
	AcquireSRWLockExclusive(&_PlayerMap_srwlock);
	for (auto i = _PlayerMap.begin(); i != _PlayerMap.end(); i++)
	{
		if (i->second->_AccountNo == AccountNo)
		{
			ClientID = i->second->_ClientID;
			break;
		}
	}
	ReleaseSRWLockExclusive(&_PlayerMap_srwlock);
	return ClientID;
}

void CChatServer::MonitorThread_Update()
{
	//-------------------------------------------------------------
	//	모니터링 스레드 - NetServer에서 가상함수 상속받아 사용
	//	모니터링 할 항목을 1초 단위로 갱신하여 콘솔 화면에 출력
	//
	//	모니터링 항목들
	//-------------------------------------------------------------
	wprintf(L"\n");
	struct tm *t = new struct tm;
	time_t timer;
	timer = time(NULL);
	localtime_s(t, &timer);

	int year = t->tm_year + 1900;
	int month = t->tm_mon + 1;
	int day = t->tm_mday;
	int hour = t->tm_hour;
	int min = t->tm_min;
	int sec = t->tm_sec;

	while (!m_bShutdown)
	{
		Sleep(1000);
		timer = time(NULL);
		localtime_s(t, &timer);

		if (true == m_bMonitorFlag)
		{
			wprintf(L"	[ServerStart : %d/%d/%d %d:%d:%d]\n\n", year, month, day, hour, min, sec);
			wprintf(L"	[%d/%d/%d %d:%d:%d]\n\n", t->tm_year + 1900, t->tm_mon + 1,
				t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

			wprintf(L"	Connect User			:	%I64d	\n", m_iConnectClient);
			wprintf(L"	PlayerMap User			:	%d		\n", GetPlayerCount());
			wprintf(L"	LoginSuccess User Total		:	%d\n", _JoinSession);
			wprintf(L"	PacketPool Alloc		:	%d	\n", CPacket::GetAllocPool());
			wprintf(L"	PacketPool Use			:	%d	\n", CPacket::_UseCount);
			wprintf(L"	PlayerPool Alloc		:	%d	\n", _PlayerPool->GetAllocCount());
			wprintf(L"	PlayerPool Use			:	%d	\n", _PlayerPool->GetUseCount());
			wprintf(L"	BattleRoomPool Use		:	%d	\n", _BattleRoomPool->GetUseCount());
			wprintf(L"	BattleRoomMap Size		:	%d	\n\n", GetBattleRoomCount());

			wprintf(L"	ChatServer Accept Total		:	%I64d	\n", m_iAcceptTotal);
			wprintf(L"	ChatServer Accept TPS		:	%I64d	\n", m_iAcceptTPS);
			wprintf(L"	ChatServer Send KByte/s		:	%I64d	\n", m_iSendPacketTPS);
			wprintf(L"	ChatServer Recv KByte/s		:	%I64d	\n", m_iRecvPacketTPS);
		}
		m_iAcceptTPS = 0;
		m_iRecvPacketTPS = 0;
		m_iSendPacketTPS = 0;
	}
	delete t;
	return;
}

void CChatServer::ReqChatLogin(CPacket * pPacket, CPlayer * pPlayer)
{
	char ConnectToken[32] = { 0, };

	*pPacket >> pPlayer->_AccountNo;
	pPacket->PopData((char*)&pPlayer->_ID, sizeof(pPlayer->_ID));
	pPacket->PopData((char*)&pPlayer->_Nickname, sizeof(pPlayer->_Nickname));
	pPacket->PopData((char*)&ConnectToken, sizeof(ConnectToken));

	CPacket * ResPacket = CPacket::Alloc();
	WORD Type = en_PACKET_CS_CHAT_RES_LOGIN;
	BYTE Status = LOGIN_SUCCESS;
	//	연결토큰 맞는지 확인
	if (0 != strncmp(ConnectToken, _CurConnectToken, sizeof(_CurConnectToken)))
	{
		if (0 != strncmp(ConnectToken, _OldConnectToken, sizeof(_OldConnectToken)))
		{
			//	다른경우 로그 남기고 ConnectToken 다름 패킷 전송
			Status = LOGIN_FAIL;			
		}
	}
	*ResPacket << Type << Status << pPlayer->_AccountNo;
	SendPacket(pPlayer->_ClientID, ResPacket);
	ResPacket->Free();
	return;
}

void CChatServer::ReqEnterRoom(CPacket * pPacket, CPlayer * pPlayer)
{
	INT64 AccountNo = NULL;
	int RoomNo = NULL;
	char EnterToken[32] = { 0, };
	BATTLEROOM * pRoom = nullptr;

	*pPacket >> AccountNo >> RoomNo;
	pPacket->PopData((char*)&EnterToken, sizeof(EnterToken));

	CPacket * ResPacket = CPacket::Alloc();
	WORD Type = en_PACKET_CS_CHAT_RES_ENTER_ROOM;

	//	AccountNo 체크
	if (pPlayer->_AccountNo != AccountNo)
	{
		
	}

	//	해당 방 검색
	pRoom = FindBattleRoom(RoomNo);
	if (nullptr == pRoom)
	{
		BYTE Status = NOT_FIND;
		*ResPacket << Type << AccountNo << RoomNo << Status;
		SendPacket(pPlayer->_ClientID, ResPacket);
		ResPacket->Free();
		return;
	}

	//	EnterToken 체크
	if (0 != strncmp(EnterToken, pRoom->EnterToken, sizeof(pRoom->EnterToken)))
	{
		//	다른경우 로그 남기고 ConnectToken 다름 패킷 전송
		BYTE Status = TOKEN_ERROR;
		*ResPacket << Type << AccountNo << RoomNo << Status;
		SendPacket(pPlayer->_ClientID, ResPacket);
		ResPacket->Free();
		return;
	}

	RoomPlayerInfo Info;
	Info.AccountNo = pPlayer->_AccountNo;
	Info.ClientID = pPlayer->_ClientID;

	//	해당 방에 유저 넣음
	AcquireSRWLockExclusive(&pRoom->Room_lock);
	pRoom->RoomPlayer.push_back(Info);
	ReleaseSRWLockExclusive(&pRoom->Room_lock);

	//	배틀 서버로 응답
	BYTE Status = SUCCESS;
	*ResPacket << Type << AccountNo << RoomNo << Status;
	SendPacket(pPlayer->_ClientID, ResPacket);
	ResPacket->Free();

	return;
}

void CChatServer::ReqSendMsg(CPacket * pPacket, CPlayer * pPlayer)
{
	INT64 AccountNo = NULL;
	WORD MsgLen = NULL;

	*pPacket >> AccountNo >> MsgLen;

	WCHAR * pMsg = new WCHAR[MsgLen / 2];
	pPacket->PopData(pMsg, MsgLen / 2);

	//	채팅보내기 응답
	CPacket * ResPacket = CPacket::Alloc();
	WORD Type = en_PACKET_CS_CHAT_RES_MESSAGE;
	*ResPacket << Type << AccountNo;
	ResPacket->PushData((char*)&pPlayer->_ID, sizeof(pPlayer->_ID));
	ResPacket->PushData((char*)&pPlayer->_Nickname, sizeof(pPlayer->_Nickname));
	*ResPacket << MsgLen;
	ResPacket->PushData(pMsg, MsgLen / 2);

	ResPacket->AddRef();
	BATTLEROOM * pRoom = FindBattleRoom(pPlayer->_RoomNo);
	if (nullptr != pRoom)
	{
		AcquireSRWLockExclusive(&pRoom->Room_lock);
		for (auto i = pRoom->RoomPlayer.begin(); i != pRoom->RoomPlayer.end(); i++)
		{
			SendPacket((*i).ClientID, ResPacket);
			ResPacket->Free();
		}
		ReleaseSRWLockExclusive(&pRoom->Room_lock);
	}
	ResPacket->Free();
	delete pMsg;
	return;
}