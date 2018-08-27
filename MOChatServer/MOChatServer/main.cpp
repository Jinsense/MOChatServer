#include <conio.h>

#include "Config.h"
#include "ChatServer.h"

int main()
{
	SYSTEM_INFO SysInfo;
	CChatServer Server;
	CPacket::MemoryPoolInit();
	GetSystemInfo(&SysInfo);

	//-----------------------------------------------------------
	// Config 파일 불러오기
	//-----------------------------------------------------------
	if (false == Server._Config.Set())
	{
		wprintf(L"[MatchServer :: Main]	Config Error\n");
		return false;
	}
	//-----------------------------------------------------------
	// Packet Code 설정
	//-----------------------------------------------------------
	CPacket::Init(Server._Config.PACKET_CODE, Server._Config.PACKET_KEY1, Server._Config.PACKET_KEY2);
	
	
	//-----------------------------------------------------------
	// 배틀 서버 연결
	//-----------------------------------------------------------
	if (false == Server._pGameServer->Connect(Server._Config.BATTLE_IP, Server._Config.BATTLE_PORT, true, LANCLIENT_WORKERTHREAD))
	{
		wprintf(L"[MatchServer :: Main] Master Connect Error\n");
		return false;
	}
	//-----------------------------------------------------------
	// 채팅 서버 가동
	//-----------------------------------------------------------
	if (false == Server.ServerStart(Server._Config.CHAT_BIND_IP, Server._Config.CHAT_BIND_PORT, Server._Config.WORKER_THREAD, true, Server._Config.CLIENT_MAX))
	{
		wprintf(L"[MatchServer :: Main] MatchingServer Start Error\n]");
		return false;
	}
	
	Server.ThreadInit();
	//-----------------------------------------------------------
	// 콘솔 창 제어
	//-----------------------------------------------------------
	while (!Server.GetShutDownMode())
	{
		char ch = _getch();
		switch (ch)
		{
		case 'q': case 'Q':
		{
			Server.SetShutDownMode(true);
			wprintf(L"[MatchServer :: Main] Server Shutdown\n");
			break;
		}
		case 'm': case 'M':
		{
			if (false == Server.GetMonitorMode())
			{
				Server.SetMonitorMode(true);
				wprintf(L"[MatchServer :: Main] MonitorMode Start\n");
			}
			else
			{
				Server.SetMonitorMode(false);
				wprintf(L"[MatchServer :: Main] MonitorMode Stop\n");
			}
		}
		}
	}
	return 0;
}