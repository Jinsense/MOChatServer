#include <windows.h>

#include "Config.h"

CConfig::CConfig()
{
	VER_CODE = NULL;

	ZeroMemory(&CHAT_BIND_IP, sizeof(CHAT_BIND_IP));
	CHAT_BIND_IP_SIZE = eNUM_BUF;
	CHAT_BIND_PORT = NULL;

	ZeroMemory(&MY_IP, sizeof(MY_IP));
	MY_IP_SIZE = NULL;

	ZeroMemory(&BATTLE_IP, sizeof(BATTLE_IP));
	BATTLE_IP_SIZE = eNUM_BUF;
	BATTLE_PORT = NULL;

	WORKER_THREAD = NULL;
	SERVER_TIMEOUT = NULL;
	CLIENT_MAX = NULL;
	PACKET_CODE = NULL;
	PACKET_KEY1 = NULL;
	PACKET_KEY2 = NULL;
	LOG_LEVEL = NULL;

	ZeroMemory(&IP, sizeof(IP));
}

CConfig::~CConfig()
{

}

bool CConfig::Set()
{
	bool res = true;
	res = _Parse.LoadFile(L"ChatServer_Config.ini");
	if (false == res)
		return false;
	res = _Parse.ProvideArea("NETWORK");
	if (false == res)
		return false;
	res = _Parse.GetValue("VER_CODE", &VER_CODE);
	if (false == res)
		return false;
	
	_Parse.GetValue("CHAT_BIND_IP", &IP[0], &CHAT_BIND_IP_SIZE);
	_Parse.UTF8toUTF16(IP, CHAT_BIND_IP, sizeof(CHAT_BIND_IP));
	_Parse.GetValue("CHAT_BIND_PORT", &CHAT_BIND_PORT);
	if (false == res)
		return false;

	_Parse.GetValue("MY_IP", &IP[0], &MY_IP_SIZE);
	_Parse.UTF8toUTF16(IP, CHAT_BIND_IP, sizeof(CHAT_BIND_IP));

	_Parse.GetValue("BATTLE_IP", &IP[0], &BATTLE_IP_SIZE);
	_Parse.UTF8toUTF16(IP, BATTLE_IP, sizeof(BATTLE_IP));
	_Parse.GetValue("BATTLE_PORT", &BATTLE_PORT);
	if (false == res)
		return false;

	_Parse.ProvideArea("SYSTEM");
	res = _Parse.GetValue("WORKER_THREAD", &WORKER_THREAD);
	if (false == res)
		return false;
	_Parse.GetValue("USER_TIMEOUT", &SERVER_TIMEOUT);
	_Parse.GetValue("CLIENT_MAX", &CLIENT_MAX);
	_Parse.GetValue("PACKET_CODE", &PACKET_CODE);
	_Parse.GetValue("PACKET_KEY1", &PACKET_KEY1);
	_Parse.GetValue("PACKET_KEY2", &PACKET_KEY2);
	_Parse.GetValue("LOG_LEVEL", &LOG_LEVEL);

	return true;
}