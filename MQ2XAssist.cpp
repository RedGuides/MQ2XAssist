// MQ2XAssist.cpp : Defines the entry point for the DLL application.
//

// PLUGIN_API is only to be used for callbacks.  All existing callbacks at this time
// are shown below. Remove the ones your plugin does not use.  Always use Initialize
// and Shutdown for setup and cleanup, do NOT do it in DllMain.



#include "../MQ2Plugin.h"

PLUGIN_VERSION(1.0);

PreSetup("MQ2XAssist");
#define DEBUGTHIS 1
int AssistID = 0;

fEQCommand			cmdXTarget;
std::string assistname;

void SetXTarget(int slot,int id)
{
	if (PCHARINFO pChar = GetCharInfo())
	{
		if (pChar->pXTargetMgr)
		{
			if (slot < pChar->pXTargetMgr->XTargetSlots.Count)
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
				else {
					pChar->pXTargetMgr->XTargetSlots[slot].Name[0] = '\0';
					pChar->pXTargetMgr->XTargetSlots[slot].SpawnID = 0;
					pChar->pXTargetMgr->XTargetSlots[slot].XTargetSlotStatus = eXTSlotEmpty;
					pChar->pXTargetMgr->XTargetSlots[slot].xTargetType = 1;
				}
			}
		}
	}
}
VOID XTargetCmd(PSPAWNINFO pChar, PCHAR szLine)
{
	CHAR szCmd[MAX_STRING] = { 0 };
	GetArg(szCmd,szLine, 1);
	if (!szCmd[0])
	{
		WriteChatColor("Usage: /xtarget id #id #slot",CONCOLOR_YELLOW);
		WriteChatColor("Usage: /xtarget assist #id",CONCOLOR_YELLOW);
		cmdXTarget(pChar, szLine);
		RETURN(0);
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
			WriteChatf("\agMQ2XAssist\ax::\ayNow Placing mobs \am%s\ay fights on XTarget 1\ax.", pSpawn->Name);
		}
		else
		{
			assistname.clear();
			WriteChatf("\agMQ2XAssist\ax::\ayNot XAssisting anyone anymore.\ax.", pSpawn->Name);
		}
		RETURN(0);
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
		RETURN(0);
	}
	cmdXTarget(pChar, szLine);
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
PLUGIN_API VOID InitializePlugin(VOID)
{
    DebugSpewAlways("Initializing MQ2XAssist");
	AddXAssistCmd();
    //Add commands, MQ2Data items, hooks, etc.
    //AddCommand("/mycommand",MyCommand);
    //AddXMLFile("MQUI_MyXMLFile.xml");
    //bmMyBenchmark=AddMQ2Benchmark("My Benchmark Name");
}

// Called once, when the plugin is to shutdown
PLUGIN_API VOID ShutdownPlugin(VOID)
{
    DebugSpewAlways("Shutting down MQ2XAssist");
	RemoveCommand("/xtarget");
    AddCommand("/xtarget",cmdXTarget);
    //Remove commands, MQ2Data items, hooks, etc.
    //RemoveMQ2Benchmark(bmMyBenchmark);
    //RemoveCommand("/mycommand");
    //RemoveXMLFile("MQUI_MyXMLFile.xml");
}
int checkcnt = 0;
PSPAWNINFO oldtarget = 0;
std::string assname;
int OnXTarget(ExtendedTargetList*lst, int spawnid,int *out)
{
	*out = -1;
	int cnt = 0;
	for (int i = 0; i < lst->XTargetSlots.Count; i++)
	{
		if (lst->XTargetSlots[i].SpawnID == spawnid)
		{
			*out = i;
			cnt++;
		}
	}
	return cnt;
}
int xtcnt = -1;
PLUGIN_API VOID OnPulse(VOID)
{
	if (GetGameState() == GAMESTATE_INGAME && AssistID) {
		if (checkcnt > 20)
		{
			checkcnt = 0;
			if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByID(AssistID))
			{
				if (PCHARINFO pChar = GetCharInfo())
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
										int out = 0;
										xtcnt = OnXTarget(pChar->pXTargetMgr, pXTarget->SpawnID,&out);
										if (!xtcnt)
										{
											#ifdef DEBUGTHIS
											WriteChatf("Setting XTarget 1 to %d (%s)", pXTarget->SpawnID, pSpawn->AssistName);
											#endif
											SetXTarget(0, pXTarget->SpawnID);
											assname = pSpawn->AssistName;
											return;
										}
									}
								}
							}
						}
					}
					if (pChar->pXTargetMgr)
					{
						if (pChar->pXTargetMgr->XTargetSlots.Count)
						{
							if (pChar->pXTargetMgr->XTargetSlots[0].SpawnID > 0)
							{
								int out = 0;
								xtcnt = OnXTarget(pChar->pXTargetMgr, pChar->pXTargetMgr->XTargetSlots[0].SpawnID,&out);
								if (xtcnt > 1)
								{
									#ifdef DEBUGTHIS
									WriteChatf("Clearing XTarget %d because it's a duplicate.",out);
									#endif
									SetXTarget(out, 0);
								}
								if (PSPAWNINFO pXTarget = (PSPAWNINFO)GetSpawnByID(pChar->pXTargetMgr->XTargetSlots[0].SpawnID))
								{
									if (pXTarget->Type == SPAWN_CORPSE)
									{
										#ifdef DEBUGTHIS
										WriteChatf("Clearing XTarget 1 because it's a corpse.");
										#endif
										SetXTarget(0, 0);
									}
								}
							}
						}
					}
				}
			}
			else {
				//guys id is gone lets pick it up again if we can
				if (assistname.size())
				{
					if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByName((char*)assistname.c_str()))
					{
						AssistID = pSpawn->SpawnID;
						WriteChatf("\agMQ2XAssist\ax::\aySetting new AssistID for \am%s\ay to %d because it changed.\ax.", pSpawn->Name,AssistID);
					}
				}
			}
		}
		checkcnt++;
	}
}