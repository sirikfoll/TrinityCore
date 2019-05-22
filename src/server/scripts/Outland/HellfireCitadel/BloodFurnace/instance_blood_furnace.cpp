/*
 * Copyright (C) 2008-2019 TrinityCore <https://www.trinitycore.org/>
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

#include "ScriptMgr.h"
#include "blood_furnace.h"
#include "InstanceScript.h"
#include "Map.h"

DoorData const doorData[] =
{
    { GO_PRISON_DOOR_01, DATA_KELIDAN_THE_BREAKER, DOOR_TYPE_PASSAGE },
    { GO_PRISON_DOOR_02, DATA_THE_MAKER,           DOOR_TYPE_ROOM },
    { GO_PRISON_DOOR_03, DATA_THE_MAKER,           DOOR_TYPE_PASSAGE },
    { GO_PRISON_DOOR_04, DATA_BROGGOK,             DOOR_TYPE_PASSAGE },
    { GO_PRISON_DOOR_05, DATA_BROGGOK,             DOOR_TYPE_ROOM },
    { GO_SUMMON_DOOR,    DATA_KELIDAN_THE_BREAKER, DOOR_TYPE_PASSAGE },
    { 0,                 0,                        DOOR_TYPE_ROOM } // END
};


ObjectData const creatureData[] =
{
    { NPC_THE_MAKER,           DATA_THE_MAKER           },
    { NPC_BROGGOK,             DATA_BROGGOK             },
    { NPC_KELIDAN_THE_BREAKER, DATA_KELIDAN_THE_BREAKER },
    { 0,                       0                        } // END
};

ObjectData const gameObjectData[] =
{
    { GO_PRISON_DOOR_04,     DATA_DOOR_4        },
    { GO_PRISON_CELL_DOOR_1, DATA_PRISON_CELL1  },
    { GO_PRISON_CELL_DOOR_2, DATA_PRISON_CELL2  },
    { GO_PRISON_CELL_DOOR_3, DATA_PRISON_CELL3  },
    { GO_PRISON_CELL_DOOR_4, DATA_PRISON_CELL4  },
    { GO_PRISON_CELL_DOOR_5, DATA_PRISON_CELL5  },
    { GO_PRISON_CELL_DOOR_6, DATA_PRISON_CELL6  },
    { GO_PRISON_CELL_DOOR_7, DATA_PRISON_CELL7  },
    { GO_PRISON_CELL_DOOR_8, DATA_PRISON_CELL8  },
    { GO_BROGGOK_LEVER,      DATA_BROGGOK_LEVER },
    { 0,                     0                  } //END
};

class instance_blood_furnace : public InstanceMapScript
{
    public:
        instance_blood_furnace() : InstanceMapScript(BFScriptName, 542) { }

        struct instance_blood_furnace_InstanceMapScript : public InstanceScript
        {
            instance_blood_furnace_InstanceMapScript(Map* map) : InstanceScript(map)
            {
                SetHeaders(DataHeader);
                SetBossNumber(EncounterCount);
                LoadDoorData(doorData);
                LoadObjectData(creatureData, gameObjectData);
            }
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_blood_furnace_InstanceMapScript(map);
        }
};

void AddSC_instance_blood_furnace()
{
    new instance_blood_furnace();
}
