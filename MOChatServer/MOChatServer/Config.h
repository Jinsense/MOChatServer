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

	WCHAR BATTLE_BIND_IP[20];
	int BATTLE_BIND_IP_SIZE;
	int BATTLE_BIND_PORT;

	WCHAR MONITOR_IP[20];
	int MONITOR_IP_SIZE;
	int MONITOR_PORT;

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
	char IP[60];
};


#endif // !_CHATSERVER_SERVER_CONFIG_H_

