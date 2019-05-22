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
#include "ObjectAccessor.h"
#include "ScriptedCreature.h"
#include "SpellAuras.h"
#include "TemporarySummon.h"

enum KelidanSays
{
    SAY_WAKE      = 0,
    SAY_ADD_AGGRO = 1,
    SAY_KILL      = 2,
    SAY_NOVA      = 3,
    SAY_DIE       = 4
};

enum KelidanSpells
{
    SPELL_CORRUPTION            = 30938,
    SPELL_EVOCATION             = 30935,

    SPELL_FIRE_NOVA             = 33132,
    H_SPELL_FIRE_NOVA           = 37371,

    SPELL_SHADOW_BOLT_VOLLEY    = 28599,
    H_SPELL_SHADOW_BOLT_VOLLEY  = 40070,

    SPELL_BURNING_NOVA          = 30940,
    SPELL_VORTEX                = 37370,

    // Channelers
    SPELL_SHADOW_BOLT   = 12739,
    H_SPELL_SHADOW_BOLT = 15472,

    SPELL_MARK_OF_SHADOW = 30937,
    SPELL_CHANNELING = 39123,
    //

    ENTRY_KELIDAN               = 17377,
    ENTRY_CHANNELER             = 17653,

    ACTION_ACTIVATE_ADDS        = 92,
};

enum KelidanEvents
{
    EVENT_SHADOW_VOLLEY = 1,
    EVENT_BURNING_NOVA,
    EVENT_FIRE_NOVA,
    EVENT_CORRUPTION,

    EVENT_SHADOW_BOLT,
    EVENT_MARK_OF_SHADOW
};

Position const ShadowmoonChannelers[5] =
{
    { 301.9876f, -86.74652f, -24.45165f, 0.1570796f },
    { 320.75f,   -63.61208f, -24.63609f, 4.886922f  },
    { 345.8484f, -74.45591f, -24.64024f, 3.595378f  },
    { 343.5839f, -103.6306f, -24.56882f, 2.356194f  },
    { 316.2737f, -108.8766f, -24.60271f, 1.256637f  }
};

struct boss_kelidan_the_breaker : public BossAI
{
    boss_kelidan_the_breaker(Creature* creature) : BossAI(creature, DATA_KELIDAN_THE_BREAKER) { }

    void Reset() override
    {
        _Reset();
        DoCastSelf(SPELL_EVOCATION);
        me->SetReactState(REACT_PASSIVE);

        for (uint8 i = 0; i < 5; ++i)
        {
            if (Creature* channeler = me->SummonCreature(ENTRY_CHANNELER, ShadowmoonChannelers[i], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 300000))
                _channelers[i] = channeler->GetGUID();
        }

        for (uint8 i = 0; i < 5; ++i)
        {
            if (Creature* channelerA = ObjectAccessor::GetCreature(*me, _channelers[i]))
                if (Creature* channelerB = ObjectAccessor::GetCreature(*me, _channelers[(i + 2) % 5]))
                    channelerA->CastSpell(channelerB, SPELL_CHANNELING);
        }
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        _EnterEvadeMode();
        summons.DespawnAll();
        _DespawnAtEvade();
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _JustEngagedWith();
        summons.DoZoneInCombat();
        Talk(SAY_ADD_AGGRO);
    }

    void KilledUnit(Unit* /*victim*/) override
    {
        if (roll_chance_i(50))
            Talk(SAY_KILL);
    }

    void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
    {
        summons.Despawn(summon);

        if (!summons.empty())
            return;

        me->SetReactState(REACT_AGGRESSIVE);
        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        me->SetImmuneToNPC(false);

        Talk(SAY_WAKE);
        if (me->IsNonMeleeSpellCast(false))
            me->InterruptNonMeleeSpells(true);

        DoStartMovement(me->GetVictim());

        events.ScheduleEvent(EVENT_SHADOW_VOLLEY, 1s);
        events.ScheduleEvent(EVENT_BURNING_NOVA, 15s);
        events.ScheduleEvent(EVENT_CORRUPTION, 5s);

    }

    void JustDied(Unit* /*killer*/) override
    {
        _JustDied();
        Talk(SAY_DIE);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_SHADOW_VOLLEY:
                    DoCastAOE(SPELL_SHADOW_BOLT_VOLLEY);
                    events.Repeat(5s, 13s);
                    break;
                case EVENT_CORRUPTION:
                    DoCastAOE(SPELL_CORRUPTION);
                    events.Repeat(30s, 50s);
                    break;
                case EVENT_BURNING_NOVA:
                    if (me->IsNonMeleeSpellCast(false))
                        me->InterruptNonMeleeSpells(true);

                    Talk(SAY_NOVA);

                    me->AddAura(SPELL_BURNING_NOVA, me);

                    if (IsHeroic())
                        DoTeleportAll(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());

                    events.Repeat(20s, 28s);
                    events.ScheduleEvent(EVENT_FIRE_NOVA, 5s);
                    break;
                case EVENT_FIRE_NOVA:
                    DoCastSelf(SPELL_FIRE_NOVA, true);
                    events.RescheduleEvent(EVENT_SHADOW_VOLLEY, 2s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }

        DoMeleeAttackIfReady();
    }

    private:
        ObjectGuid _channelers[5];
};

struct npc_shadowmoon_channeler : public ScriptedAI
{
    npc_shadowmoon_channeler(Creature* creature) : ScriptedAI(creature), _instance(creature->GetInstanceScript()) { }

    void Reset() override
    {
        _events.Reset();
    }

    void JustEngagedWith(Unit* who) override
    {
        _events.ScheduleEvent(EVENT_SHADOW_BOLT, 1s);
        _events.ScheduleEvent(EVENT_MARK_OF_SHADOW, 5s, 7s);

        if (Creature* Kelidan = me->FindNearestCreature(ENTRY_KELIDAN, 100))
            DoZoneInCombat(Kelidan);
        if (me->IsNonMeleeSpellCast(false))
            me->InterruptNonMeleeSpells(true);
        DoStartMovement(who);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_SHADOW_BOLT:
                    DoCastVictim(SPELL_SHADOW_BOLT);
                    _events.Repeat(5s, 6s);
                    break;
                case EVENT_MARK_OF_SHADOW:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        DoCast(target, SPELL_MARK_OF_SHADOW);
                    _events.Repeat(15s, 20s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }

        DoMeleeAttackIfReady();
    }

private:
    EventMap _events;
    InstanceScript* _instance;
};

void AddSC_boss_kelidan_the_breaker()
{
    RegisterBloodFurnaceCreatureAI(boss_kelidan_the_breaker);
    RegisterBloodFurnaceCreatureAI(npc_shadowmoon_channeler);
}
