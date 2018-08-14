#ifndef _CHATSERVER_SERVER_CONFIG_H_
#define _CHATSERVER_SERVER_CONFIG_H_

#include "Parse.h"

class CConfig
{
	enum eNumConfig
	{
		eNUM_BUF = 20,
	};
public:
	CConfig();
	~CConfig();

	bool Set();

public:
	//	NETWORK
	UINT VER_CODE;

	WCHAR CHAT_BIND_IP[16];
	int CHAT_BIND_IP_SIZE;
	int CHAT_BIND_PORT;

	WCHAR MY_IP[16];
	int MY_IP_SIZE;

	WCHAR BATTLE_IP[16];
	int BATTLE_IP_SIZE;
	int BATTLE_PORT;

	//	SYSTEM
	int WORKER_THREAD;
	int SERVER_TIMEOUT;
	int CLIENT_MAX;
	int PACKET_CODE;
	int PACKET_KEY1;
	int PACKET_KEY2;
	int LOG_LEVEL;

	CINIParse _Parse;

private:
	char IP[40];
};


#endif // !_CHATSERVER_SERVER_CONFIG_H_

