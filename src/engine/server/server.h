/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SERVER_SERVER_H
#define ENGINE_SERVER_SERVER_H

#include <engine/server.h>

class CSnapIDPool
{
	enum
	{
		MAX_IDS = 16*1024,
	};

	class CID
	{
	public:
		short m_Next;
		short m_State; // 0 = free, 1 = alloced, 2 = timed
		int m_Timeout;
	};

	CID m_aIDs[MAX_IDS];

	int m_FirstFree;
	int m_FirstTimed;
	int m_LastTimed;
	int m_Usage;
	int m_InUsage;

public:

	CSnapIDPool();

	void Reset();
	void RemoveFirstTimeout();
	int NewID();
	void TimeoutIDs();
	void FreeID(int ID);
};

class CServer : public IServer
{
	class IGameServer *m_pGameServer;
	class IConsole *m_pConsole;
	class IStorage *m_pStorage;
public:
	class IGameServer *GameServer() { return m_pGameServer; }
	class IConsole *Console() { return m_pConsole; }
	class IStorage *Storage() { return m_pStorage; }

	enum
	{
		AUTHED_NO=0,
		AUTHED_MOD,
		AUTHED_ADMIN,

		MAX_RCONCMD_SEND=16,
	};

	class CClient
	{
	public:

		enum
		{
			STATE_EMPTY = 0,
			STATE_AUTH,
			STATE_CONNECTING,
			STATE_SPOOFCHECK,
			STATE_POSTSPOOFCHECK,
			STATE_READY,
			STATE_INGAME,

			SNAPRATE_INIT=0,
			SNAPRATE_FULL,
			SNAPRATE_RECOVER
		};

		class CInput
		{
		public:
			int m_aData[MAX_INPUT_SIZE];
			int m_GameTick; // the tick that was chosen for the input
		};

		// connection state info
		int m_State;
		int m_Latency;
		int m_SnapRate;

		int m_LastAckedSnapshot;
		int m_LastInputTick;
		CSnapshotStorage m_Snapshots;

		CInput m_LatestInput;
		CInput m_aInputs[200]; // TODO: handle input better
		int m_CurrentInput;

		char m_aName[MAX_NAME_LENGTH];
		char m_aClan[MAX_CLAN_LENGTH];
		int m_Country;
		int m_Score;
		int m_Authed;
		int m_AuthTries;

		int m_Nonce; // number to reach
		int m_NonceCount; // current num
		int64 m_LastNonceCount;

		const IConsole::CCommandInfo *m_pRconCmdToSend;

#if defined(CONF_TEERACE)
		int m_UserID;
		char m_aUsername[64];

		int m_PlayTicks;

		int m_SaveDemoTick;
		int m_SaveGhostTick;
#endif
		void Reset();
	};

	CClient m_aClients[MAX_CLIENTS];

	CSnapshotDelta m_SnapshotDelta;
	CSnapshotBuilder m_SnapshotBuilder;
	CSnapIDPool m_IDPool;
	CNetServer m_NetServer;
	CEcon m_Econ;

	IEngineMap *m_pMap;

	int64 m_GameStartTime;
	//int m_CurrentGameTick;
	int m_RunServer;
	int m_MapReload;
	int m_RconClientID;
	int m_RconAuthLevel;
	int m_PrintCBIndex;

	int64 m_Lastheartbeat;
	//static NETADDR4 master_server;

	char m_aCurrentMap[64];
	unsigned m_CurrentMapCrc;
	unsigned char *m_pCurrentMapData;
	int m_CurrentMapSize;

#if defined(CONF_TEERACE)
	char m_aConfigFilename[128];

	CDemoRecorder m_aDemoRecorder[MAX_CLIENTS+1];
	CGhostRecorder m_aGhostRecorder[MAX_CLIENTS];
#else
	CDemoRecorder m_DemoRecorder;
#endif

	CRegister m_Register;
	CMapChecker m_MapChecker;

	CServer();

	int TrySetClientName(int ClientID, const char *pName);

	virtual void SetClientName(int ClientID, const char *pName);
	virtual void SetClientClan(int ClientID, char const *pClan);
	virtual void SetClientCountry(int ClientID, int Country);
	virtual void SetClientScore(int ClientID, int Score);

	void Kick(int ClientID, const char *pReason);

	void DemoRecorder_HandleAutoStart();

	//int Tick()
	int64 TickStartTime(int Tick);
	//int TickSpeed()

	int Init();

	bool IsAuthed(int ClientID);
	int GetClientInfo(int ClientID, CClientInfo *pInfo);
	void GetClientAddr(int ClientID, char *pAddrStr, int Size);
	const char *ClientName(int ClientID);
	const char *ClientClan(int ClientID);
	int ClientCountry(int ClientID);
	bool ClientIngame(int ClientID);

	virtual int SendMsg(CMsgPacker *pMsg, int Flags, int ClientID);
	int SendMsgEx(CMsgPacker *pMsg, int Flags, int ClientID, bool System);

	void DoSnapshot();

	static int NewClientCallback(int ClientID, void *pUser);
	static int NewClientNoAuthCallback(int ClientID, void *pUser);
	static int DelClientCallback(int ClientID, const char *pReason, void *pUser);

	void SendMap(int ClientID);
	void SendConnectionReady(int ClientID);
	void SendRconLine(int ClientID, const char *pLine);
	static void SendRconLineAuthed(const char *pLine, void *pUser);

	void SendRconCmdAdd(const IConsole::CCommandInfo *pCommandInfo, int ClientID);
	void SendRconCmdRem(const IConsole::CCommandInfo *pCommandInfo, int ClientID);
	void UpdateClientRconCommands();

	void ProcessClientPacket(CNetChunk *pPacket);

	void SendServerInfo(NETADDR *pAddr, int Token);
	void UpdateServerInfo();

	int BanAdd(NETADDR Addr, int Seconds, const char *pReason);
	int BanRemove(NETADDR Addr);

	void PumpNetwork();

#if defined(CONF_TEERACE)
	void SaveConfigFilename(int NumArgs, const char **ppArguments);
	const char* GetConfigFilename() { return m_aConfigFilename; }
	
	void ReloadMap();

	void SetUserID(int ClientID, int UserID) { m_aClients[ClientID].m_UserID = UserID; }
	int GetUserID(int ClientID) { return m_aClients[ClientID].m_UserID; }
	int GetPlayTicks(int ClientID) { return m_aClients[ClientID].m_PlayTicks; }
	void HigherPlayTicks(int ClientID) { m_aClients[ClientID].m_PlayTicks++; }

	void SetUserName(int ClientID, const char* pUsername) { str_copy(m_aClients[ClientID].m_aUsername, pUsername, sizeof(m_aClients[ClientID].m_aUsername)); }
	const char* GetUserName(int ClientID) { return m_aClients[ClientID].m_aUsername; }

	void SaveGhostDemo(int ClientID);

	void StartRecord(int ClientID);
	void StopRecord(int ClientID);
	bool IsRecording(int ClientID);

	void StaffAuth(int ClientID, int SendRconCmds);

	void StartGhostRecord(int ClientID, const char* pSkinName, int UseCustomColor, int ColorBody, int ColorFeet);
	void StopGhostRecord(int ClientID, float Time=0.0f);
	bool IsGhostRecording(int ClientID);
	void GhostAddInfo(int ClientID, IGhostRecorder::CGhostCharacter *pPlayer);
#endif

	char *GetMapName();
	int LoadMap(const char *pMapName);

	void InitRegister(CNetServer *pNetServer, IEngineMasterServer *pMasterServer, IConsole *pConsole);
	int Run();

	static void ConKick(IConsole::IResult *pResult, void *pUser);
	static void ConBan(IConsole::IResult *pResult, void *pUser);
	static void ConUnban(IConsole::IResult *pResult, void *pUser);
	static void ConBans(IConsole::IResult *pResult, void *pUser);
 	static void ConStatus(IConsole::IResult *pResult, void *pUser);
	static void ConShutdown(IConsole::IResult *pResult, void *pUser);
	static void ConRecord(IConsole::IResult *pResult, void *pUser);
	static void ConStopRecord(IConsole::IResult *pResult, void *pUser);
	static void ConMapReload(IConsole::IResult *pResult, void *pUser);
	static void ConchainSpecialInfoupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainMaxclientsperipUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainModCommandUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainConsoleOutputLevelUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

	void RegisterCommands();


	virtual int SnapNewID();
	virtual void SnapFreeID(int ID);
	virtual void *SnapNewItem(int Type, int ID, int Size);
	void SnapSetStaticsize(int ItemType, int Size);
};

#endif