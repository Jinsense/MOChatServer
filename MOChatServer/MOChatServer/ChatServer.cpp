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
	//	�ʿ� ���� �߰�
	//-------------------------------------------------------------
	CPlayer *pPlayer = _PlayerPool->Alloc();
	pPlayer->Init();
	pPlayer->_ClientID = Info.iClientID;
	pPlayer->_Time = GetTickCount64();
	InsertPlayer(pPlayer->_ClientID, pPlayer);
//	_pLog->Log(const_cast<WCHAR*>(L"Test"), LOG_SYSTEM, const_cast<WCHAR*>(L"Accept [ClientID : %d]"), pPlayer->_ClientID);

	return;
}

void CChatServer::OnClientLeave(unsigned __int64 ClientID)
{
	unsigned __int64 UserID = ClientID;
	CPlayer * pPlayer = FindPlayer_ClientID(UserID);
	if (nullptr == pPlayer)
	{
		_pLog->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM, const_cast<WCHAR*>(L"FindClientID Fail [ClientID : %d]"), ClientID);
		return;
	}
	
	//-------------------------------------------------------------
	//	���� RoomPlayer���� ����
	//-------------------------------------------------------------	
	BATTLEROOM * pRoom = FindBattleRoom(pPlayer->_RoomNo);
	if (nullptr != pRoom)
	{
		std::list<RoomPlayerInfo>::iterator iter;
		AcquireSRWLockExclusive(&pRoom->Room_lock);
		for (iter = pRoom->RoomPlayer.begin(); iter != pRoom->RoomPlayer.end();)
		{
			if ((*iter).AccountNo == pPlayer->_AccountNo)
			{
				iter = pRoom->RoomPlayer.erase(iter);
			}
			else
				iter++;
		}
		ReleaseSRWLockExclusive(&pRoom->Room_lock);
	}
	
	//-------------------------------------------------------------
	//	�ʿ� ���� ����
	//-------------------------------------------------------------
	RemovePlayer(UserID);
	_PlayerPool->Free(pPlayer);
	return;
}

void CChatServer::OnConnectionRequest(WCHAR * pClientID)
{
	//-------------------------------------------------------------
	//	ȭ��Ʈ IP ó��
	//	���� ��� X
	//-------------------------------------------------------------
	return;
}

void CChatServer::OnError(int ErrorCode, WCHAR *pError)
{
	//-------------------------------------------------------------
	//	���� ó�� - �α� ����
	//	������ ���ǵ� �α� �ڵ带 ���� �α׸� txt�� ����
	//	���� ��� X
	//-------------------------------------------------------------
	return;
}

bool CChatServer::OnRecv(unsigned __int64 ClientID, CPacket *pPacket)
{
	//-------------------------------------------------------------
	//	����͸� ���� ����
	//-------------------------------------------------------------
	InterlockedIncrement(&m_iRecvPacketTPS);
	//-------------------------------------------------------------
	//	���� ó�� - ���� �����ϴ� �������� �˻�
	//-------------------------------------------------------------
	CPlayer *pPlayer = FindPlayer_ClientID(ClientID);
	if (nullptr == pPlayer)
	{
		Disconnect(ClientID);
		return false;
	}
	//-------------------------------------------------------------
	//	��Ŷ ó�� - ������ ó��
	//-------------------------------------------------------------
	WORD Type;
	*pPacket >> Type;
	//-------------------------------------------------------------
	//	��Ŷ ó�� - ä�� ������ �α��� ��û
	//-------------------------------------------------------------
	if (Type == en_PACKET_CS_CHAT_REQ_LOGIN)
	{
		ReqChatLogin(pPacket, pPlayer);
		pPlayer->_Time = GetTickCount64();
	}
	//-------------------------------------------------------------
	//	��Ŷ ó�� - ä�� ���� �� ����
	//-------------------------------------------------------------
	else if (Type == en_PACKET_CS_CHAT_REQ_ENTER_ROOM)
	{
		ReqEnterRoom(pPacket, pPlayer);
		pPlayer->_Time = GetTickCount64();
	}
	//-------------------------------------------------------------
	//	��Ŷ ó�� - ä�� ������ ��û
	//-------------------------------------------------------------
	else if (Type == en_PACKET_CS_CHAT_REQ_MESSAGE)
	{
		ReqSendMsg(pPacket, pPlayer);
		pPlayer->_Time = GetTickCount64();
	}
	//-------------------------------------------------------------
	//	��Ŷ ó�� - ��Ʈ ��Ʈ
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
	//	StatusDB�� �����ð����� ���� ��ġ���� Timestamp �� ����
	//	��ü ���� ������� Heartbeat �������� TimeOut üũ 
	//	���� ������ ���� �̻� ��ȭ�� �߻����� �� DB�� connectuser �� ����
	//-------------------------------------------------------------
	UINT64 start = GetTickCount64();
	int count = GetPlayerCount();
	int usercount = NULL;
	std::stack<unsigned __int64> temp;

	while (1)
	{
		Sleep(1000);
		UINT64 now = GetTickCount64();

		
		AcquireSRWLockExclusive(&_PlayerMap_srwlock);
		for (auto i = _PlayerMap.begin(); i != _PlayerMap.end(); i++)
		{
		if (now - i->second->_Time > _Config.USER_TIMEOUT)
		{
		_pLog->Log(const_cast<WCHAR*>(L"HeartBeat"), LOG_SYSTEM, const_cast<WCHAR*>(L"USER TIMEOUT : %d [AccountNo : %d]"), now - i->second->_Time, i->second->_AccountNo);
		//	�ٷ� Disconnect �ϸ� ����� ���輺
		//	�ӽ� stack�� �ְ� �������� Disconnect ȣ���� ��
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
		
	}
}

void CChatServer::LanGameCheckThread_Update()
{
	//-------------------------------------------------------------
	//	���� ���� ���� �ֱ������� Ȯ��
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
	//	Config ���� �ҷ�����
	//-------------------------------------------------------------
	_Config.Set();
	return;
}

void CChatServer::UTF8toUTF16(const char *szText, WCHAR *szBuf, int iBufLen)
{
	//-------------------------------------------------------------
	//	UTF8 Ÿ���� UTF16���� ��ȯ
	//-------------------------------------------------------------
	int iRe = MultiByteToWideChar(CP_UTF8, 0, szText, strlen(szText), szBuf, iBufLen);
	if (iRe < iBufLen)
		szBuf[iRe] = L'\0';
	return;
}

void CChatServer::UTF16toUTF8(WCHAR *szText, char *szBuf, int iBufLen)
{
	//-------------------------------------------------------------
	//	UTF16 Ÿ���� UTF8�� ��ȯ
	//-------------------------------------------------------------
	int iRe = WideCharToMultiByte(CP_UTF8, 0, szText, lstrlenW(szText), szBuf, iBufLen, NULL, NULL);
	return;
}

bool CChatServer::InsertPlayer(unsigned __int64 ClientID, CPlayer* pPlayer)
{
	//-------------------------------------------------------------
	//	Player Map�� Ŭ���̾�Ʈ �߰�
	//-------------------------------------------------------------
	AcquireSRWLockExclusive(&_PlayerMap_srwlock);
	_PlayerMap.insert(make_pair(ClientID, pPlayer));
	ReleaseSRWLockExclusive(&_PlayerMap_srwlock);
	return true;
}

bool CChatServer::RemovePlayer(unsigned __int64 ClientID)
{
	//-------------------------------------------------------------
	//	Player Map�� Ŭ���̾�Ʈ ����
	//-------------------------------------------------------------
	AcquireSRWLockExclusive(&_PlayerMap_srwlock);
	_PlayerMap.erase(ClientID);
	ReleaseSRWLockExclusive(&_PlayerMap_srwlock);
	return true;
}

bool CChatServer::DisconnectPlayer(unsigned __int64 ClientID, INT64 AccountNo)
{
	//-------------------------------------------------------------
	//	TimeOut ���� ����
	//	Log ����� Disconnect ȣ��
	//-------------------------------------------------------------
	_pLog->Log(const_cast<WCHAR*>(L"TimeOut"), LOG_SYSTEM,
		const_cast<WCHAR*>(L"TimeOut [AccountNo : %d]"), AccountNo);
	Disconnect(ClientID);
	return true;
}

void CChatServer::BattleDisconnect()
{
	std::map<int, BATTLEROOM*>::iterator Map;
	//	��� ���� ������ ���� �ش� �� �ı�
	AcquireSRWLockExclusive(&_BattleRoom_lock);
	for (Map = _BattleRoomMap.begin(); Map != _BattleRoomMap.end();)
	{
		std::list<RoomPlayerInfo>::iterator RoomPlayer;
		AcquireSRWLockExclusive(&(*Map).second->Room_lock);
		for (RoomPlayer = (*Map).second->RoomPlayer.begin(); RoomPlayer != (*Map).second->RoomPlayer.end();)
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
	bool Find = false;
	BATTLEROOM * pRoom = nullptr;
	std::map<int, BATTLEROOM*>::iterator iter;

	AcquireSRWLockExclusive(&_BattleRoom_lock);
	iter = _BattleRoomMap.find(RoomNo);
	if (iter == _BattleRoomMap.end())
		Find = false;
	else
	{
		Find = true;
		pRoom = (*iter).second;
	}
	ReleaseSRWLockExclusive(&_BattleRoom_lock);

	if (false == Find)
		return nullptr;
	else
		return pRoom;
}

int CChatServer::GetBattleRoomCount()
{
	//-------------------------------------------------------------
	//	���� �����Ǿ� �ִ� �� ���� ���
	//-------------------------------------------------------------
	return _BattleRoomMap.size();
}

int CChatServer::GetPlayerCount()
{
	//-------------------------------------------------------------
	//	���� ���� ���� �÷��̾� �� ���
	//-------------------------------------------------------------
	return _PlayerMap.size();
}

CPlayer* CChatServer::FindPlayer_ClientID(unsigned __int64 ClientID)
{
	bool Find = false;
	std::map<unsigned __int64, CPlayer*>::iterator iter;
	
	AcquireSRWLockExclusive(&_PlayerMap_srwlock);
	iter = _PlayerMap.find(ClientID);
	if (iter == _PlayerMap.end())
		Find = false;
	else
		Find = true;
	ReleaseSRWLockExclusive(&_PlayerMap_srwlock);

	if (false == Find)
		return nullptr;
	else
		return (*iter).second;
}

unsigned __int64 CChatServer::FindPlayer_AccountNo(INT64 AccountNo)
{
	std::map<unsigned __int64, CPlayer*>::iterator iter;
	unsigned __int64 ClientID = NULL;
	AcquireSRWLockExclusive(&_PlayerMap_srwlock);
	for (iter = _PlayerMap.begin(); iter != _PlayerMap.end(); iter++)
	{
		if ((*iter).second->_AccountNo == AccountNo)
		{
			ClientID = (*iter).second->_ClientID;
			break;
		}
	}
	ReleaseSRWLockExclusive(&_PlayerMap_srwlock);
	return ClientID;
}

void CChatServer::MonitorThread_Update()
{
	//-------------------------------------------------------------
	//	����͸� ������ - NetServer���� �����Լ� ��ӹ޾� ���
	//	����͸� �� �׸��� 1�� ������ �����Ͽ� �ܼ� ȭ�鿡 ���
	//
	//	����͸� �׸��
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
			wprintf(L"	ChatServer Send TPS		:	%I64d	\n", m_iSendPacketTPS);
			wprintf(L"	ChatServer Recv TPS		:	%I64d	\n", m_iRecvPacketTPS);
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
	//	������ū �´��� Ȯ��
	if (0 != strncmp(ConnectToken, _CurConnectToken, sizeof(_CurConnectToken)))
	{
		if (0 != strncmp(ConnectToken, _OldConnectToken, sizeof(_OldConnectToken)))
		{
			//	�ٸ���� �α� ����� ConnectToken �ٸ� ��Ŷ ����
			_pLog->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM, const_cast<WCHAR*>(L"ConnectToken Not Same [AccountNo : %d]"), pPlayer->_AccountNo);
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

	//	AccountNo üũ
	if (pPlayer->_AccountNo != AccountNo && NULL != AccountNo)
	{
		//	�ٸ���� �α� ����� �� ���� ���� ����
		_pLog->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM, const_cast<WCHAR*>(L"ReqEnterRoom - AccountNo Not Same [My AccountNo : %d, Recv AccountNo]"), pPlayer->_AccountNo, AccountNo);
		BYTE Status = TOKEN_ERROR;
		*ResPacket << Type << AccountNo << RoomNo << Status;
		SendPacket(pPlayer->_ClientID, ResPacket);
		ResPacket->Free();
		return;
	}

	//	�ش� �� �˻�
	pRoom = FindBattleRoom(RoomNo);
	if (nullptr == pRoom)
	{
		BYTE Status = NOT_FIND;
		*ResPacket << Type << pPlayer->_AccountNo << RoomNo << Status;
		SendPacket(pPlayer->_ClientID, ResPacket);
		ResPacket->Free();
		return;
	}

	//	EnterToken üũ
	if (0 != strncmp(EnterToken, pRoom->EnterToken, sizeof(pRoom->EnterToken)))
	{
		//	�ٸ���� �α� ����� ConnectToken �ٸ� ��Ŷ ����
		BYTE Status = TOKEN_ERROR;
		*ResPacket << Type << pPlayer->_AccountNo << RoomNo << Status;
		SendPacket(pPlayer->_ClientID, ResPacket);
		ResPacket->Free();
		return;
	}

	RoomPlayerInfo Info;
	Info.AccountNo = pPlayer->_AccountNo;
	Info.ClientID = pPlayer->_ClientID;
	Info.pPlayer = pPlayer;

	pPlayer->_RoomNo = RoomNo;

	//	�� ���� ����
	BYTE Status = SUCCESS;
	*ResPacket << Type << AccountNo << RoomNo << Status;
	SendPacket(pPlayer->_ClientID, ResPacket);
	ResPacket->Free();
	
	//	�ش� �濡 ���� ����
	AcquireSRWLockExclusive(&pRoom->Room_lock);
	pRoom->RoomPlayer.push_back(Info);
	ReleaseSRWLockExclusive(&pRoom->Room_lock);

	return;
}

void CChatServer::ReqSendMsg(CPacket * pPacket, CPlayer * pPlayer)
{
	INT64 AccountNo = NULL;
	WORD MsgLen = NULL;

	*pPacket >> AccountNo >> MsgLen;

	//	AccountNo üũ
	if (pPlayer->_AccountNo != AccountNo && NULL != AccountNo)
	{
		//	�ٸ���� �α�
		_pLog->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM, const_cast<WCHAR*>(L"ReqSendMsg - AccountNo Not Same [My AccountNo : %d, Recv AccountNo]"), pPlayer->_AccountNo, AccountNo);
		Disconnect(pPlayer->_ClientID);
		return;
	}

	WCHAR * pMsg = new WCHAR[MsgLen / 2];
	pPacket->PopData(pMsg, MsgLen / 2);
	
	BATTLEROOM * pRoom = FindBattleRoom(pPlayer->_RoomNo);
	if (nullptr != pRoom)
	{
		//	Ŭ���̾�ƮID�� ��´�
		unsigned __int64 ClientID[6] = { 0, };
		int num = 0;
		AcquireSRWLockExclusive(&pRoom->Room_lock);
		for (auto i = pRoom->RoomPlayer.begin(); i != pRoom->RoomPlayer.end(); i++)
		{
			ClientID[num] = (*i).ClientID;
			num++;
			if (num > 6)
				g_CrashDump->Crash();
		}
		ReleaseSRWLockExclusive(&pRoom->Room_lock);

		//	�ش� Ŭ���̾�ƮID�� ä�� �޼��� ����
		for (int k = 0; k < 6; k++)
		{
			if (NULL != ClientID[k] && pPlayer->_AccountNo == AccountNo)
			{
				CPacket * ResPacket = CPacket::Alloc();
				WORD Type = en_PACKET_CS_CHAT_RES_MESSAGE;
				*ResPacket << Type << AccountNo;
				ResPacket->PushData((char*)&pPlayer->_ID, sizeof(pPlayer->_ID));
				ResPacket->PushData((char*)&pPlayer->_Nickname, sizeof(pPlayer->_Nickname));
				*ResPacket << MsgLen;
				ResPacket->PushData(pMsg, MsgLen / 2);
				
				SendPacket(ClientID[k], ResPacket);
				ResPacket->Free();
			}
		}		
	}
	delete[] pMsg;
	return;
}