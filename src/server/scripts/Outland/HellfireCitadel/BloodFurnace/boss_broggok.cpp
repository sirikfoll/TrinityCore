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
#include "GameObject.h"
#include "GameObjectAI.h"
#include "InstanceScript.h"
#include "ObjectAccessor.h"
#include "ScriptedCreature.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"

enum Yells
{
    SAY_AGGRO                   = 0
};

enum Spells
{
    SPELL_SLIME_SPRAY           = 30913,
    SPELL_POISON_CLOUD          = 30916,
    SPELL_POISON_BOLT           = 30917,

    SPELL_POISON_CLOUD_PASSIVE  = 30914
};

enum Events
{
    EVENT_SLIME_SPRAY = 1,
    EVENT_POISON_BOLT,
    EVENT_POISON_CLOUD,
};

enum SummonGroups
{
    CREATURE_SUMMON_GRUP_PRISONERS = 0
};

enum ActionIds
{
    ACTION_ACTIVATE_BROGGOK = 1,
    ACTION_PREPARE_BROGGOK  = 2
};

struct boss_broggok : public BossAI
{
    boss_broggok(Creature* creature) : BossAI(creature, DATA_BROGGOK) { }

    void Reset() override
    {
        _Reset();
        PrisonersCell5.clear();
        PrisonersCell6.clear();
        PrisonersCell7.clear();
        PrisonersCell8.clear();

        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        me->SetImmuneToAll(true);
        me->SummonCreatureGroup(CREATURE_SUMMON_GRUP_PRISONERS);
        for (uint8 i = DATA_PRISON_CELL1; i <= DATA_PRISON_CELL8; ++i)
            instance->HandleGameObject(instance->GetObjectGuid(i), false);

        if (GameObject* lever = instance->GetGameObject(DATA_BROGGOK_LEVER))
        {
            lever->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE | GO_FLAG_IN_USE);
            lever->SetGoState(GO_STATE_READY);
        }
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _JustEngagedWith();
        Talk(SAY_AGGRO);
        ActivateCell(DATA_PRISON_CELL5);
    }

    void JustSummoned(Creature* summoned) override
    {
        if (summoned->GetEntry() == NPC_BROGGOK_POISON_CLOUD)
        {
            summoned->SetReactState(REACT_PASSIVE);
            summoned->CastSpell(summoned, SPELL_POISON_CLOUD_PASSIVE, true);
        }
        else if (summoned->GetEntry() == NPC_PRISONER_1 || summoned->GetEntry() == NPC_PRISONER_2)
            StorePrisoner(summoned);
        summons.Summon(summoned);
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_SLIME_SPRAY:
                DoCastVictim(SPELL_SLIME_SPRAY);
                events.Repeat(4s, 12s);
                break;
            case EVENT_POISON_BOLT:
                DoCastVictim(SPELL_POISON_BOLT);
                events.Repeat(4s, 12s);
                break;
            case EVENT_POISON_CLOUD:
                DoCastSelf(SPELL_POISON_CLOUD);
                events.Repeat(20s);
                break;
            default:
                break;
        }
    }

    void DoAction(int32 action) override
    {
        switch (action)
        {
            case ACTION_PREPARE_BROGGOK:
                DoZoneInCombat();
                break;
            case ACTION_ACTIVATE_BROGGOK:
                me->SetReactState(REACT_AGGRESSIVE);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetImmuneToAll(false);
                events.ScheduleEvent(EVENT_SLIME_SPRAY, 10s);
                events.ScheduleEvent(EVENT_POISON_BOLT, 7s);
                events.ScheduleEvent(EVENT_POISON_CLOUD, 5s);
                break;
        }
    }

    void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
    {
        if (summon->GetEntry() == NPC_PRISONER_1 || summon->GetEntry() == NPC_PRISONER_2)
        {
            if (PrisonersCell5.erase(summon->GetGUID()) && PrisonersCell5.empty())
                ActivateCell(DATA_PRISON_CELL6);
            else if (PrisonersCell6.erase(summon->GetGUID()) && PrisonersCell6.empty())
                ActivateCell(DATA_PRISON_CELL7);
            else if (PrisonersCell7.erase(summon->GetGUID()) && PrisonersCell7.empty())
                ActivateCell(DATA_PRISON_CELL8);
            else if (PrisonersCell8.erase(summon->GetGUID()) && PrisonersCell8.empty())
                ActivateCell(DATA_DOOR_4);
        }
    }

    void StorePrisoner(Creature* creature)
    {
        float posX = creature->GetPositionX();
        float posY = creature->GetPositionY();

        if (posX >= 405.0f && posX <= 423.0f)
        {
            if (posY >= 106.0f && posY <= 123.0f)
                PrisonersCell5.insert(creature->GetGUID());
            else if (posY >= 76.0f && posY <= 91.0f)
                PrisonersCell6.insert(creature->GetGUID());
        }
        else if (posX >= 490.0f && posX <= 506.0f)
        {
            if (posY >= 106.0f && posY <= 123.0f)
                PrisonersCell7.insert(creature->GetGUID());
            else if (posY >= 76.0f && posY <= 91.0f)
                PrisonersCell8.insert(creature->GetGUID());
        }
    }

    void ActivateCell(uint8 id)
    {
        switch (id)
        {
            case DATA_PRISON_CELL5:
                ActivatePrisoners(PrisonersCell5);
                break;
            case DATA_PRISON_CELL6:
                ActivatePrisoners(PrisonersCell6);
                break;
            case DATA_PRISON_CELL7:
                ActivatePrisoners(PrisonersCell7);
                break;
            case DATA_PRISON_CELL8:
                ActivatePrisoners(PrisonersCell8);
                break;
            case DATA_DOOR_4:
                DoAction(ACTION_ACTIVATE_BROGGOK);
                break;
            default:
                return;
        }

        instance->HandleGameObject(instance->GetObjectGuid(id), true);
    }

    void ActivatePrisoners(GuidSet const& prisoners)
    {
        for (ObjectGuid const guid : prisoners)
            if (Creature* prisoner = ObjectAccessor::GetCreature(*me, guid))
            {
                prisoner->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                DoZoneInCombat(prisoner);
            }
    }

    private:
        GuidSet PrisonersCell5;
        GuidSet PrisonersCell6;
        GuidSet PrisonersCell7;
        GuidSet PrisonersCell8;
};

class go_broggok_lever : public GameObjectScript
{
    public:
        go_broggok_lever() : GameObjectScript("go_broggok_lever") { }

        struct go_broggok_leverAI : public GameObjectAI
        {
            go_broggok_leverAI(GameObject* go) : GameObjectAI(go), instance(go->GetInstanceScript()) { }

            InstanceScript* instance;

            bool GossipHello(Player* /*player*/) override
            {
                if (instance->GetBossState(DATA_BROGGOK) != DONE && instance->GetBossState(DATA_BROGGOK) != IN_PROGRESS)
                {
                    instance->SetBossState(DATA_BROGGOK, IN_PROGRESS);
                    if (Creature* broggok = instance->GetCreature(DATA_BROGGOK))
                        broggok->AI()->DoAction(ACTION_PREPARE_BROGGOK);
                }

                me->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE | GO_FLAG_IN_USE);
                me->SetGoState(GO_STATE_ACTIVE);
                return true;
            }
        };

        GameObjectAI* GetAI(GameObject* go) const override
        {
            return GetBloodFurnaceAI<go_broggok_leverAI>(go);
        }
};

// 30914, 38462 - Poison (Broggok)
class spell_broggok_poison_cloud : public AuraScript
{
    PrepareAuraScript(spell_broggok_poison_cloud);

    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellInfo({ spellInfo->Effects[EFFECT_0].TriggerSpell });
    }

    void PeriodicTick(AuraEffect const* aurEff)
    {
        PreventDefaultAction();
        if (!aurEff->GetTotalTicks())
            return;

        uint32 triggerSpell = GetSpellInfo()->Effects[aurEff->GetEffIndex()].TriggerSpell;
        int32 mod = int32(((float(aurEff->GetTickNumber()) / aurEff->GetTotalTicks()) * 0.9f + 0.1f) * 10000 * 2 / 3);
        GetTarget()->CastSpell(nullptr, triggerSpell, CastSpellExtraArgs(aurEff).AddSpellMod(SPELLVALUE_RADIUS_MOD, mod));
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_broggok_poison_cloud::PeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

void AddSC_boss_broggok()
{
    RegisterBloodFurnaceCreatureAI(boss_broggok);
    new go_broggok_lever();
    RegisterAuraScript(spell_broggok_poison_cloud);
}
