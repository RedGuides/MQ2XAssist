// MQ2XAssist.cpp

#include "../MQ2Plugin.h"

PreSetup("MQ2XAssist");
PLUGIN_VERSION(1.3);

constexpr bool DEBUGXASSIST = false;

int AssistID = 0;
int checkcnt = 0;
int xtcnt = -1;
fEQCommand cmdXTarget;
std::string assistname;
PSPAWNINFO oldtarget = 0;
std::string assname;
class MQ2XAssistType* pXAssistType = nullptr;

class MQ2XAssistType : public MQ2Type
{
public:
	enum Members {
		XTFullHaterCount,
		XTXAggroCount
	};

	MQ2XAssistType() :MQ2Type("XAssist")
	{
		TypeMember(XTFullHaterCount);
		TypeMember(XTXAggroCount);
	}
	~MQ2XAssistType() {}

	bool GetMember(MQ2VARPTR VarPtr, PCHAR Member, PCHAR Index, MQ2TYPEVAR& Dest)
	{
		PMQ2TYPEMEMBER pMember = MQ2XAssistType::FindMember(Member);
		if (!pMember)
			return false;
		if (!pLocalPlayer)
			return false;
		switch ((Members)pMember->ID) {
			// Return the total number of AutoHater mobs in the XTarget window including the current target. Expansion of ${Me.XTHaterCount}
			case XTFullHaterCount:
				Dest.Int = getXTCountByAggro();
				Dest.Type = pIntType;
				return true;
			// Return the total number of Autohater mobs less than the passed value -- uses an expanded range over ${Me.XTAggroCount}
			case XTXAggroCount:
				// Default to return 0
				Dest.Int = 0;
				if (IsNumber(Index)) {
					int param_aggro = atoi(Index);
					if (param_aggro < 1)
						param_aggro = 1;
					if (param_aggro > 1000)
						param_aggro = 1000;
					Dest.Int = getXTCountByAggro(param_aggro);
				}
				else {
					Dest.Int = getXTCountByAggro();
				}

				Dest.Type = pIntType;
				return true;
			default:
				break;
		}
		return false;
	}

	bool FromData(MQ2VARPTR& VarPtr, MQ2TYPEVAR& Source)
	{
		return false;
	}
	bool FromString(MQ2VARPTR& VarPtr, PCHAR Source)
	{
		return false;
	}

private:
	/**
		getXTCountByAggro
		Calculate the total number of XT Auto Haters less than the passed  aggro value.
		@return Total number of XTarget Auto Haters less than the passed aggro percentage.
	*/
	int getXTCountByAggro(int aggro_check_pct = 1000)
	{
		// Default return
		int aggrocnt = 0;

		PCHARINFO pChar = GetCharInfo();
		if (!pChar) return aggrocnt;

		ExtendedTargetList* xtm = pChar->pXTargetMgr;
		if (!xtm) return aggrocnt;

		if (!pAggroInfo) return aggrocnt;

		for (int i = 0; i < xtm->XTargetSlots.Count; i++) {
			XTARGETSLOT xts = xtm->XTargetSlots[i];
			DWORD spID = xts.SpawnID;
			if (spID && xts.xTargetType == XTARGET_AUTO_HATER) {
				if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByID(spID)) {
					if (pSpawn->Type == SPAWN_NPC) {
						int aggropct = pAggroInfo->aggroData[AD_xTarget1 + i].AggroPct;
						// DebugSpewAlways("Checking aggro on %s its %d",pSpawn->DisplayedName, aggropct);
						if (aggropct < aggro_check_pct) {
							aggrocnt++;
						}
					}
				}
			}
		}

		return aggrocnt;
	}
};

BOOL XAssistData(char* szIndex, MQ2TYPEVAR& Dest)
{
	Dest.DWord = 1;
	Dest.Type = pXAssistType;
	return true;
}

void SetXTarget(int slot, int id)
{
	if (PCHARINFO pChar = GetCharInfo())
	{
		if (pChar->pXTargetMgr)
		{
			if (slot >= 0 && slot < pChar->pXTargetMgr->XTargetSlots.Count)
			{
				if (id)
				{
					if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByID(id))
					{
						strcpy_s(pChar->pXTargetMgr->XTargetSlots[slot].Name, pSpawn->Name);
					}
					pChar->pXTargetMgr->XTargetSlots[slot].SpawnID = id;
					pChar->pXTargetMgr->XTargetSlots[slot].XTargetSlotStatus = eXTSlotCurrentZone;
					pChar->pXTargetMgr->XTargetSlots[slot].xTargetType = 1;//autohater
				}
				else
				{
					pChar->pXTargetMgr->XTargetSlots[slot].Name[0] = '\0';
					pChar->pXTargetMgr->XTargetSlots[slot].SpawnID = 0;
					pChar->pXTargetMgr->XTargetSlots[slot].XTargetSlotStatus = eXTSlotEmpty;
					pChar->pXTargetMgr->XTargetSlots[slot].xTargetType = 1;
				}
			}
		}
	}
}

void CleanUpXTarget()
{
	PCHARINFO pChar = GetCharInfo();
	std::set<int> duplicates;

	SPAWNINFO* pAssistSpawn = (SPAWNINFO*)GetSpawnByID(AssistID);

	for (int i = 0; i < pChar->pXTargetMgr->XTargetSlots.Count; i++)
	{
		if (int xid = pChar->pXTargetMgr->XTargetSlots[i].SpawnID)
		{
			if (PSPAWNINFO pXTarget = (PSPAWNINFO)GetSpawnByID(xid))
			{
				if (pXTarget->Type == SPAWN_CORPSE && pXTarget->Deity == 0)
				{
					if (DEBUGXASSIST) {
						WriteChatf("Clearing XTarget %d because it's a npc corpse.", i + 1);
					}

					SetXTarget(i, 0);
					continue;
				}

				if (pAssistSpawn && DistanceToSpawn3D(pAssistSpawn, pXTarget) > 1500)
				{
					if (DEBUGXASSIST) {
						WriteChatf("Clearing XTarget %d because it's too far away from our assist.", i + 1);
					}

					SetXTarget(i, 0);
					continue;
				}
			}

			if (duplicates.count(xid))
			{
				if (DEBUGXASSIST) {
					WriteChatf("Clearing XTarget %d because it's a duplicate.", i);
				}

				SetXTarget(i, 0);
				continue;
			}

			duplicates.insert(xid);
		}
	}
}

// Get an empty XTarget slot. If none exist, returns -1
int FindEmptyXTargetSlot()
{
	CHARINFO* pChar = GetCharInfo();

	for (int i = 0; i < pChar->pXTargetMgr->XTargetSlots.Count; i++)
	{
		XTARGETSLOT& slot = pChar->pXTargetMgr->XTargetSlots[i];

		if (slot.XTargetSlotStatus == eXTSlotEmpty && slot.xTargetType == 1)
		{
			return i;
		}
	}

	return -1;
}

// Get the XTarget slot for a specified SpawnID. If not found, returns -1.
int GetXTargetSlotByID(int SpawnID)
{
	CHARINFO* pChar = GetCharInfo();

	for (int i = 0; i < pChar->pXTargetMgr->XTargetSlots.Count; i++)
	{
		if (pChar->pXTargetMgr->XTargetSlots[i].SpawnID == SpawnID)
		{
			return i;
		}
	}

	return -1;
}

void XTargetCmd(PSPAWNINFO pChar, PCHAR szLine)
{
	CHAR szCmd[MAX_STRING] = { 0 };
	GetArg(szCmd,szLine, 1);
	if (!szCmd[0])
	{
		WriteChatColor("Usage: /xtarget id #id #slot",CONCOLOR_YELLOW);
		WriteChatColor("Usage: /xtarget assist #id",CONCOLOR_YELLOW);
		cmdXTarget(pChar, szLine);
	}
	else if (!_stricmp(szCmd, "assist"))
	{
		CHAR szID[MAX_STRING] = { 0 };
		GetArg(szID, szLine, 2);
		AssistID = atoi(szID);
		PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByID(AssistID);
		if (pSpawn)
		{
			assistname = pSpawn->Name;
			WriteChatf("\agMQ2XAssist\ax::\ayNow Placing mobs \am%s\ay fights on XTarget\ax.", pSpawn->Name);
		}
		else
		{
			assistname.clear();
			WriteChatf("\agMQ2XAssist\ax::\ayNot XAssisting anyone anymore.\ax.", pSpawn->Name);
		}
	}
	else if (!_stricmp(szCmd, "id"))
	{
		CHAR szSlot[MAX_STRING] = { 0 };
		CHAR szID[MAX_STRING] = { 0 };
		GetArg(szID, szLine, 2);
		GetArg(szSlot, szLine, 3);
		int id = atoi(szID);
		int slot = atoi(szSlot);
		if (slot > 0)
			slot--;
		SetXTarget(slot, id);
	}
	else
	{
		cmdXTarget(pChar, szLine);
	}
}

void AddXAssistCmd()
{
	int i = 0;
	// Import EQ commands
	PCMDLIST pCmdListOrig = (PCMDLIST)EQADDR_CMDLIST;
	for (i=0;pCmdListOrig[i].fAddress != 0;i++) {
		if (!strcmp(pCmdListOrig[i].szName,"/xtarget")) {
			cmdXTarget = (fEQCommand)pCmdListOrig[i].fAddress;
			break;
		}
	}
	RemoveCommand("/xtarget");
	AddCommand("/xtarget",XTargetCmd);
}

// Called once, when the plugin is to initialize
PLUGIN_API VOID InitializePlugin()
{
	DebugSpewAlways("Initializing MQ2XAssist");
	AddXAssistCmd();
	AddMQ2Data("XAssist", XAssistData);
	pXAssistType = new MQ2XAssistType;
}

// Called once, when the plugin is to shutdown
PLUGIN_API VOID ShutdownPlugin()
{
	DebugSpewAlways("Shutting down MQ2XAssist");
	RemoveCommand("/xtarget");
	AddCommand("/xtarget",cmdXTarget);
	RemoveMQ2Data("XAssist");
	delete pXAssistType;
}

PLUGIN_API VOID OnPulse()
{
	if (GetGameState() == GAMESTATE_INGAME || !AssistID)
		return;
	PCHARINFO pChar = GetCharInfo();
	if (!pChar || !pChar->pXTargetMgr)
		return;

	// Check every 20 frames
	if (checkcnt++ > 20)
	{
		checkcnt = 0;

		// if it's myself we should not mess with adding it manually to xtarget
		if (AssistID == ((PSPAWNINFO)pLocalPlayer)->SpawnID)
			return;

		if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByID(AssistID))
		{
			if (pSpawn->AssistName[0])
			{
				if (pSpawn->AssistName != assname)
				{
					if (PSPAWNINFO pXTarget = (PSPAWNINFO)GetSpawnByName(pSpawn->AssistName))
					{
						if (pXTarget->SpawnID != AssistID)
						{
							if (pXTarget->Type == SPAWN_NPC)
							{
								int slot = GetXTargetSlotByID(AssistID);
								if (slot == -1) //not on there already so lets add it
								{
									slot = FindEmptyXTargetSlot();
									if (slot == -1)
									{
										if (DEBUGXASSIST) {
											WriteChatf("Failed to set XTarget to %d (%s) - no more slots", pXTarget->SpawnID, pSpawn->AssistName);
										}

										return;
									}

									SetXTarget(slot, pXTarget->SpawnID);
									if (DEBUGXASSIST) {
										WriteChatf("Setting XTarget %d to %d (%s)", slot+1, pXTarget->SpawnID, pSpawn->AssistName);
									}

									assname = pSpawn->AssistName;
									return;
								}
							}
						}
					}
				}
			}

			CleanUpXTarget();
		}
		else
		{
			// guys id is gone lets pick it up again if we can
			if (!assistname.empty())
			{
				// FIX ME:  Don't cast a constant to a mutable type, but GetSpawnByName should take const
				pSpawn = (PSPAWNINFO)GetSpawnByName((char*)assistname.c_str());
				if (pSpawn != nullptr)
				{
					AssistID = pSpawn->SpawnID;
					WriteChatf("\agMQ2XAssist\ax::\aySetting new AssistID for \am%s\ay to %d because it changed.\ax.", pSpawn->Name, AssistID);
				}
			}
		}
	}
}
