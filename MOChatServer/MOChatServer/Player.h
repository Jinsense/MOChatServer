#ifndef _CHATSERVER_SERVER_PLAYER_H_
#define _CHATSERVER_SERVER_PLAYER_H_

class CPlayer
{
public:
	CPlayer();
	~CPlayer();

	//-------------------------------------------------------------
	//	
	//-------------------------------------------------------------
	void Init();


private:


public:
	unsigned __int64 _ClientID;
	UINT64	_Time;
	INT64	_AccountNo;
	int		_RoomNo;
	WCHAR	_ID[20];
	WCHAR	_Nickname[20];
	

private:


};


#endif