#include <Windows.h>

#include "Player.h"

CPlayer::CPlayer()
{
	Init();
}

CPlayer::~CPlayer()
{

}

void CPlayer::Init()
{
	_ClientID = NULL;
	_Time = NULL;
	_AccountNo = NULL;
	_RoomNo = NULL;
	ZeroMemory(&_ID, sizeof(_ID));
	ZeroMemory(&_Nickname, sizeof(_Nickname));
	return;
}