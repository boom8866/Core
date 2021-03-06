/*
 * Copyright (C) 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* ScriptData
Name: instance_commandscript
%Complete: 100
Comment: All instance related commands
Category: commandscripts
EndScriptData */

#include "ScriptMgr.h"
#include "Chat.h"
#include "Group.h"
#include "InstanceSaveMgr.h"
#include "InstanceScript.h"
#include "MapManager.h"
#include "Player.h"
#include "Language.h"

class instance_commandscript : public CommandScript
{
public:
    instance_commandscript() : CommandScript("instance_commandscript") { }

    ChatCommand* GetCommands() const
    {
        static ChatCommand instanceCommandTable[] =
        {
            { "listbinds",      SEC_ADMINISTRATOR,  false,  &HandleInstanceListBindsCommand,    "", NULL },
            { "unbind",         SEC_ADMINISTRATOR,  false,  &HandleInstanceUnbindCommand,       "", NULL },
            { "stats",          SEC_ADMINISTRATOR,  true,   &HandleInstanceStatsCommand,        "", NULL },
            { "setbossstate",   SEC_GAMEMASTER,     true,   &HandleInstanceSetBossStateCommand, "", NULL },
            { "getbossstate",   SEC_GAMEMASTER,     true,   &HandleInstanceGetBossStateCommand, "", NULL },
            { "savedata",       SEC_ADMINISTRATOR,  false,  &HandleInstanceSaveDataCommand,     "", NULL },
            { NULL,             0,                  false,  NULL,                               "", NULL }
        };

        static ChatCommand commandTable[] =
        {
            { "instance",       SEC_ADMINISTRATOR,  true,   NULL,                               "", instanceCommandTable },
            { NULL,             0,                  false,  NULL,                               "", NULL }
        };

        return commandTable;
    }

    static std::string GetTimeString(uint64 time)
    {
        uint64 days = time / DAY, hours = (time % DAY) / HOUR, minute = (time % HOUR) / MINUTE;
        std::ostringstream ss;
        if (days)
            ss << days << "d ";
        if (hours)
            ss << hours << "h ";
        ss << minute << 'm';
        return ss.str();
    }

    static bool HandleInstanceListBindsCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->getSelectedPlayer();
        if (!player)
            player = handler->GetSession()->GetPlayer();

        uint32 counter = 0;
        for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
        {
			BoundInstancesMap const& m_boundInstances = sInstanceSaveMgr->PlayerGetBoundInstances(player->GetGUIDLow(), Difficulty(i));
            for (BoundInstancesMap::const_iterator itr = m_boundInstances.begin(); itr != m_boundInstances.end(); ++itr)
            {
                InstanceSave* save = itr->second.save;
				uint32 resetTime = itr->second.extended ? save->GetExtendedResetTime() : save->GetResetTime();
				uint32 ttr = (resetTime >= time(NULL) ? resetTime - time(NULL) : 0);
                std::string timeleft = GetTimeString(ttr);
				handler->PSendSysMessage("map: %d, inst: %d, perm: %s, diff: %d, canReset: %s, TTR: %s%s", itr->first, save->GetInstanceId(), itr->second.perm ? "yes" : "no",  save->GetDifficulty(), save->CanReset() ? "yes" : "no", timeleft.c_str(), (itr->second.extended ? " (extended)" : ""));
                counter++;
            }
        }
        handler->PSendSysMessage("player binds: %d", counter);

        return true;
    }

    static bool HandleInstanceUnbindCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        Player* player = handler->getSelectedPlayer();
        if (!player)
            player = handler->GetSession()->GetPlayer();

        char* map = strtok((char*)args, " ");
        char* pDiff = strtok(NULL, " ");
        int8 diff = -1;
        if (pDiff)
            diff = atoi(pDiff);
        uint16 counter = 0;
        uint16 MapId = 0;

        if (strcmp(map, "all"))
        {
            MapId = uint16(atoi(map));
            if (!MapId)
                return false;
        }

        for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
        {
            BoundInstancesMap const& m_boundInstances = sInstanceSaveMgr->PlayerGetBoundInstances(player->GetGUIDLow(), Difficulty(i));
            for (BoundInstancesMap::const_iterator itr = m_boundInstances.begin(); itr != m_boundInstances.end();)
            {
                InstanceSave* save = itr->second.save;
                if (itr->first != player->GetMapId() && (!MapId || MapId == itr->first) && (diff == -1 || diff == save->GetDifficulty()))
                {
					uint32 resetTime = itr->second.extended ? save->GetExtendedResetTime() : save->GetResetTime();
					uint32 ttr = (resetTime >= time(NULL) ? resetTime - time(NULL) : 0);
                    std::string timeleft = GetTimeString(ttr);
                    handler->PSendSysMessage("unbinding map: %d, inst: %d, perm: %s, diff: %d, canReset: %s, TTR: %s%s", itr->first, save->GetInstanceId(), itr->second.perm ? "yes" : "no", save->GetDifficulty(), save->CanReset() ? "yes" : "no", timeleft.c_str(), (itr->second.extended ? " (extended)" : ""));
					sInstanceSaveMgr->PlayerUnbindInstance(player->GetGUIDLow(), itr->first, Difficulty(i), true, player);
					itr = m_boundInstances.begin();
                    counter++;
                }
                else
                    ++itr;
            }
        }
        handler->PSendSysMessage("instances unbound: %d", counter);

        return true;
    }

    static bool HandleInstanceStatsCommand(ChatHandler* handler, char const* /*args*/)
    {
		uint32 dungeon = 0, battleground = 0, arena = 0, spectators = 0;
		sMapMgr->GetNumInstances(dungeon, battleground, arena);
        handler->PSendSysMessage("instances loaded: dungeons (%d), battlegrounds (%d), arenas (%d)", dungeon, battleground, arena);
		dungeon = 0; battleground = 0; arena = 0; spectators = 0;
		sMapMgr->GetNumPlayersInInstances(dungeon, battleground, arena, spectators);
        handler->PSendSysMessage("players in instances: dungeons (%d), battlegrounds (%d), arenas (%d + %d spect)", dungeon, battleground, arena, spectators);

		handler->SetSentErrorMessage(true);
        return false;
    }

    static bool HandleInstanceSetBossStateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* param1 = strtok((char*)args, " ");
        char* param2 = strtok(NULL, " ");
        char* param3 = strtok(NULL, " ");
        uint32 encounterId = 0;
        int32 state = 0;
        Player* player = NULL;
        std::string playerName;

        // Character name must be provided when using this from console.
        if (!param2 || (!param3 && !handler->GetSession()))
        {
            handler->PSendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!param3)
            player = handler->GetSession()->GetPlayer();
        else
        {
            playerName = param3;
            if (normalizePlayerName(playerName))
                player = ObjectAccessor::FindPlayerByName(playerName);
        }

        if (!player)
        {
            handler->PSendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        InstanceMap* map = player->GetMap()->ToInstanceMap();
        if (!map)
        {
            handler->PSendSysMessage(LANG_NOT_DUNGEON);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!map->GetInstanceScript())
        {
            handler->PSendSysMessage(LANG_NO_INSTANCE_DATA);
            handler->SetSentErrorMessage(true);
            return false;
        }

        encounterId = atoi(param1);
        state = atoi(param2);

        // Reject improper values.
        if (state > TO_BE_DECIDED || encounterId > map->GetInstanceScript()->GetEncounterCount())
        {
            handler->PSendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        map->GetInstanceScript()->SetBossState(encounterId, EncounterState(state));
        std::string stateName = InstanceScript::GetBossStateName(state);
        handler->PSendSysMessage(LANG_COMMAND_INST_SET_BOSS_STATE, encounterId, state, stateName.c_str());
        return true;
    }

    static bool HandleInstanceGetBossStateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* param1 = strtok((char*)args, " ");
        char* param2 = strtok(NULL, " ");
        uint32 encounterId = 0;
        Player* player = NULL;
        std::string playerName;

        // Character name must be provided when using this from console.
        if (!param1 || (!param2 && !handler->GetSession()))
        {
            handler->PSendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!param2)
            player = handler->GetSession()->GetPlayer();
        else
        {
            playerName = param2;
            if (normalizePlayerName(playerName))
                player = ObjectAccessor::FindPlayerByName(playerName);
        }

        if (!player)
        {
            handler->PSendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        InstanceMap* map = player->GetMap()->ToInstanceMap();
        if (!map)
        {
            handler->PSendSysMessage(LANG_NOT_DUNGEON);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!map->GetInstanceScript())
        {
            handler->PSendSysMessage(LANG_NO_INSTANCE_DATA);
            handler->SetSentErrorMessage(true);
            return false;
        }

        encounterId = atoi(param1);

        if (encounterId > map->GetInstanceScript()->GetEncounterCount())
        {
            handler->PSendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 state = map->GetInstanceScript()->GetBossState(encounterId);
        std::string stateName = InstanceScript::GetBossStateName(state);
        handler->PSendSysMessage(LANG_COMMAND_INST_GET_BOSS_STATE, encounterId, state, stateName.c_str());
        return true;
    }

    static bool HandleInstanceSaveDataCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        Map* map = player->GetMap();
        if (!map->IsDungeon())
        {
            handler->PSendSysMessage("Map is not a dungeon.");
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!map->ToInstanceMap()->GetInstanceScript())
        {
            handler->PSendSysMessage("Map has no instance data.");
            handler->SetSentErrorMessage(true);
            return false;
        }

        map->ToInstanceMap()->GetInstanceScript()->SaveToDB();

        return true;
    }
};

void AddSC_instance_commandscript()
{
    new instance_commandscript();
}
