// MQ2XAssist.cpp

#include <mq/Plugin.h>

PreSetup("MQ2XAssist");
PLUGIN_VERSION(1.3);

bool DebugToggle = false;

uint32_t AssistID = 0;
int checkcnt = 0;
int xtcnt = -1;
fEQCommand cmdXTarget;
std::string assistname;
std::string mobname;
PlayerClient* oldtarget = nullptr;

class MQ2XAssistType : public MQ2Type
{
public:
	enum class Members
	{
		XTFullHaterCount,
		XTXAggroCount
	};

	MQ2XAssistType() : MQ2Type("XAssist")
	{
		ScopedTypeMember(Members, XTFullHaterCount);
		ScopedTypeMember(Members, XTXAggroCount);
	}

	virtual bool GetMember(MQVarPtr VarPtr, const char* Member, char* Index, MQTypeVar& Dest) override
	{
		MQTypeMember* pMember = MQ2XAssistType::FindMember(Member);
		if (!pMember)
			return false;

		if (!pLocalPlayer)
			return false;

		switch (static_cast<Members>(pMember->ID))
		{
		case Members::XTFullHaterCount:
			// Return the total number of AutoHater mobs in the XTarget window
			// including the current target. Expansion of ${Me.XTHaterCount}
			Dest.Int = getXTCountByAggro();
			Dest.Type = mq::datatypes::pIntType;
			return true;

		case Members::XTXAggroCount:
			// Return the total number of Autohater mobs less than the passed value
			// -- uses an expanded range over ${Me.XTAggroCount}

			// Default to return 0
			Dest.Int = 0;
			if (IsNumber(Index))
			{
				int param_aggro = atoi(Index);
				if (param_aggro < 1)
					param_aggro = 1;
				Dest.Int = getXTCountByAggro(param_aggro);
			}
			else
			{
				Dest.Int = getXTCountByAggro();
			}

			Dest.Type = mq::datatypes::pIntType;
			return true;

		default:
			break;
		}
		return false;
	}

private:
	/**
	 * getXTCountByAggro
	 * Calculate the total number of XT Auto Haters less than the passed  aggro value
	 * @note The default value passed is the potential max aggro value seen in the aggropct field.
	 * @return Total number of XTarget Auto Haters less than the passed aggro percentage.
	 */
	int getXTCountByAggro(int aggro_check_pct = 65536)
	{
		// Default return
		int aggrocnt = 0;

		if (!pLocalPC) return aggrocnt;
		if (!pAggroInfo) return aggrocnt;

		ExtendedTargetList* xtm = pLocalPC->pXTargetMgr;

		for (int i = 0; i < xtm->GetNumSlots(); i++)
		{
			ExtendedTargetSlot* xts = xtm->GetSlot(i);

			if (xts->SpawnID && xts->xTargetType == XTARGET_AUTO_HATER)
			{
				if (PlayerClient* pSpawn = GetSpawnByID(xts->SpawnID))
				{
					if (pSpawn->Type == SPAWN_NPC)
					{
						int aggropct = pAggroInfo->aggroData[AD_xTarget1 + i].AggroPct;

						// DebugSpewAlways("Checking aggro on %s its %d",pSpawn->DisplayedName, aggropct);
						if (aggropct < aggro_check_pct)
						{
							aggrocnt++;
						}
					}
				}
			}
		}

		return aggrocnt;
	}
};

MQ2XAssistType* pXAssistType = nullptr;

bool XAssistData(const char*, MQTypeVar& Dest)
{
	Dest.DWord = 1;
	Dest.Type = pXAssistType;
	return true;
}

void SetXTarget(uint32_t slot, int id)
{
	if (!pLocalPC)
		return;

	ExtendedTargetSlot* xtSlot = pLocalPC->pExtendedTargetList->GetSlot(slot);
	if (!xtSlot)
		return;

	if (id)
	{
		PlayerClient* pSpawn = GetSpawnByID(id);
		if (pSpawn)
		{
			strcpy_s(xtSlot->Name, pSpawn->Name);
		}
		
		if (DebugToggle)
		{
			WriteChatf("\arMQ2XAssist::\axSetting XTarget. id:[%d] name:%s.",
				id, pSpawn != nullptr ? xtSlot->Name : "N/A");
		}
		xtSlot->SpawnID = id;
		xtSlot->XTargetSlotStatus = eXTSlotCurrentZone;
		xtSlot->xTargetType = XTARGET_AUTO_HATER;
	}
	else
	{
		mobname.clear();
		xtSlot->Name[0] = '\0';
		xtSlot->SpawnID = 0;
		xtSlot->XTargetSlotStatus = eXTSlotEmpty;
		xtSlot->xTargetType = XTARGET_AUTO_HATER;
	}
}

void ClearXTargetWithDebugMessage(const char* szMessage, uint32_t iXTarSlotActual)
{
	if (DebugToggle) {
		WriteChatf("\arMQ2XAssist::\axClearing XTarget %d because %s", iXTarSlotActual + 1, szMessage);
	}
	SetXTarget(iXTarSlotActual, 0);
}

void CleanUpXTarget()
{
	std::set<int> duplicates;

	PlayerClient* pAssistSpawn = GetSpawnByID(AssistID);

	for (int i = 0; i < pLocalPC->pExtendedTargetList->GetNumSlots(); i++)
	{
		ExtendedTargetSlot* pSlot = pLocalPC->pExtendedTargetList->GetSlot(i);

		if (int xid = pSlot->SpawnID)
		{
			if (duplicates.count(xid))
			{
				ClearXTargetWithDebugMessage("it's a duplicate.", i);
			}
			else
			{
				if (PlayerClient* pXTarget = GetSpawnByID(xid))
				{
					if (pXTarget->Type == SPAWN_CORPSE && pXTarget->Deity == 0)
					{
						ClearXTargetWithDebugMessage("it's an NPC Corpse.", i);
					}
					else if (pAssistSpawn && DistanceToSpawn3D(pAssistSpawn, pXTarget) > 1500)
					{
						ClearXTargetWithDebugMessage("it's too far away from our assist.", i);
					}
					else
					{
						// We didn't clear the xtarget, so insert it into the duplicate set.
						duplicates.insert(xid);
					}
				}
				else
				{
					ClearXTargetWithDebugMessage("it doesn't exist.", i);
				}
			}
		}
	}
}

// Get an empty XTarget slot. If none exist, returns -1
int FindEmptyXTargetSlot()
{
	for (int i = 0; i < pLocalPC->pExtendedTargetList->GetNumSlots(); ++i)
	{
		ExtendedTargetSlot* slot = pLocalPC->pExtendedTargetList->GetSlot(i);

		if (slot->XTargetSlotStatus == eXTSlotEmpty && slot->xTargetType == XTARGET_AUTO_HATER)
		{
			return i;
		}
	}

	return -1;
}

// Get the XTarget slot for a specified SpawnID. If not found, returns -1.
int GetXTargetSlotByID(int SpawnID)
{
	for (int i = 0; i < pLocalPC->pExtendedTargetList->GetNumSlots(); ++i)
	{
		if (pLocalPC->pExtendedTargetList->GetSlot(i)->SpawnID == SpawnID)
		{
			return i;
		}
	}

	return -1;
}

void XTargetCmd(PlayerClient* pChar, const char* szLine)
{
	char szCmd[MAX_STRING] = { 0 };
	GetArg(szCmd, szLine, 1);

	if (!szCmd[0])
	{
		WriteChatf("Usage: \ay/xtarget\ax \agid #id #slot\ax");
		WriteChatf("Usage: \ay/xtarget\ax \agassist #id\ax");
		WriteChatf("Usage: \ay/xtarget\ax \agdebug on/off\ax");
		cmdXTarget(pChar, szLine);
	}
	else if (ci_equals(szCmd, "assist"))
	{
		char szID[MAX_STRING] = { 0 };
		GetArg(szID, szLine, 2);

		AssistID = atoi(szID);
		if (PlayerClient* pSpawn = GetSpawnByID(AssistID))
		{
			assistname = pSpawn->Name;
			WriteChatf("\agMQ2XAssist\ax::\ayNow Placing mobs \am%s\ay fights on XTarget\ax.", pSpawn->Name);
		}
		else
		{
			assistname.clear();
			WriteChatf("\agMQ2XAssist\ax::\ayNot XAssisting anyone anymore.\ax.");
		}
	}
	else if (ci_equals(szCmd, "id"))
	{
		char szSlot[MAX_STRING] = { 0 };
		char szID[MAX_STRING] = { 0 };
		GetArg(szID, szLine, 2);
		GetArg(szSlot, szLine, 3);

		int id = atoi(szID);
		int slot = atoi(szSlot);
		if (slot > 0)
			slot--;

		SetXTarget(slot, id);
	}
	else if (ci_equals(szCmd, "debug"))
	{
		char szToggle[MAX_STRING] = { 0 };
		GetArg(szToggle, szLine, 2);

		if (ci_equals(szToggle, "on"))
		{
			WriteChatf("\agMQ2XAssist\ax::Turning Debug Messages \agON\ax.");
			DebugToggle = true;
		}
		else if (ci_equals(szToggle, "off"))
		{
			WriteChatf("\agMQ2XAssist\ax::Turning Debug Messages \arOFF\ax.");
			DebugToggle = false;
		}
	}
	else
	{
		cmdXTarget(pChar, szLine);
	}
}

void AddXAssistCmd()
{
	// Import EQ commands
	cmdXTarget = FindEQCommand("/xtarget");

	AddCommand("/xtarget", XTargetCmd);
}

// Called once, when the plugin is to initialize
PLUGIN_API void InitializePlugin()
{
	AddXAssistCmd();
	AddMQ2Data("XAssist", XAssistData);
	pXAssistType = new MQ2XAssistType;
}

// Called once, when the plugin is to shut down
PLUGIN_API void ShutdownPlugin()
{
	RemoveCommand("/xtarget");
	RemoveMQ2Data("XAssist");
	delete pXAssistType;
}

PLUGIN_API void OnPulse()
{
	if (GetGameState() != GAMESTATE_INGAME || !AssistID)
		return;

	if (!pLocalPC)
		return;

	// Check every 20 frames
	if (checkcnt++ > 20)
	{
		checkcnt = 0;

		// if it's myself we should not mess with adding it manually to xtarget
		if (AssistID == pLocalPlayer->SpawnID)
			return;

		if (PlayerClient* pSpawn = GetSpawnByID(AssistID))
		{
			if (pSpawn->AssistName[0] && !string_equals(pSpawn->AssistName, mobname))
			{
				if (PlayerClient* pXTarget = GetSpawnByName(pSpawn->AssistName))
				{
					if (pXTarget->SpawnID != AssistID
						&& pXTarget->Type == SPAWN_NPC
						&& DistanceToSpawn3D(pSpawn, pXTarget) <= 1500)
					{
						if (GetXTargetSlotByID(pXTarget->SpawnID) == -1) // not on there already so lets add it
						{
							int slot = FindEmptyXTargetSlot();
							if (slot == -1)
							{
								if (DebugToggle)
								{
									WriteChatf("\arMQ2XAssist::\axFailed to set XTarget to %d (%s) - no more slots", pXTarget->SpawnID, pSpawn->AssistName);
								}

								return;
							}

							SetXTarget(slot, pXTarget->SpawnID);
							if (DebugToggle)
							{
								WriteChatf("\arMQ2XAssist::\axSetting XTarget %d to %d (%s)", slot + 1, pXTarget->SpawnID, pSpawn->AssistName);
							}

							mobname = pSpawn->AssistName;
							return;
						}
					}
				}
			}

			CleanUpXTarget();
		}
		else
		{
			// id is gone lets pick it up again if we can
			if (!assistname.empty())
			{
				pSpawn = GetSpawnByName(assistname.c_str());
				if (pSpawn != nullptr)
				{
					AssistID = pSpawn->SpawnID;
					WriteChatf("\agMQ2XAssist\ax::\aySetting new AssistID for \am%s\ay to %d because it changed.\ax.", pSpawn->Name, AssistID);
				}
			}
		}
	}
}
