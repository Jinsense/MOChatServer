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
	// Config ���� �ҷ�����
	//-----------------------------------------------------------
	if (false == Server._Config.Set())
	{
		wprintf(L"[MatchServer :: Main]	Config Error\n");
		return false;
	}
	//-----------------------------------------------------------
	// Packet Code ����
	//-----------------------------------------------------------
	CPacket::Init(Server._Config.PACKET_CODE, Server._Config.PACKET_KEY1, Server._Config.PACKET_KEY2);
	
	
	//-----------------------------------------------------------
	// ��Ʋ ���� ����
	//-----------------------------------------------------------
	if (false == Server._pGameServer->Connect(Server._Config.BATTLE_IP, Server._Config.BATTLE_PORT, true, LANCLIENT_WORKERTHREAD))
	{
		wprintf(L"[MatchServer :: Main] Master Connect Error\n");
		return false;
	}
	//-----------------------------------------------------------
	// ä�� ���� ����
	//-----------------------------------------------------------
	if (false == Server.ServerStart(Server._Config.CHAT_BIND_IP, Server._Config.CHAT_BIND_PORT, Server._Config.WORKER_THREAD, true, Server._Config.CLIENT_MAX))
	{
		wprintf(L"[MatchServer :: Main] MatchingServer Start Error\n]");
		return false;
	}
	
	Server.ThreadInit();
	//-----------------------------------------------------------
	// �ܼ� â ����
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