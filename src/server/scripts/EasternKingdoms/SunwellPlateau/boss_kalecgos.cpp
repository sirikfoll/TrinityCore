/*
 * Copyright (C) 2008-2017 TrinityCore <http://www.trinitycore.org/>
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
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "sunwell_plateau.h"
#include "Player.h"

enum Yells
{
    SAY_SATH_AGGRO                  = 0,
    SAY_SATH_SLAY                   = 1,
    SAY_SATH_DEATH                  = 2,
    SAY_SATH_SPELL1                 = 3,
    SAY_SATH_SPELL2                 = 4,

    SAY_EVIL_AGGRO                  = 0,
    SAY_EVIL_SLAY                   = 1,
    SAY_GOOD_PLRWIN                 = 2,
    SAY_EVIL_ENRAGE                 = 3,

    SAY_GOOD_AGGRO                  = 0,
    SAY_GOOD_NEAR_DEATH             = 1,
    SAY_GOOD_NEAR_DEATH2            = 2
};

enum Spells
{
    /*
    //Kalecgos Drake
    44807 Crazed Rage
    44806 Crazed Rage
    //Sathrovar
    44806 Crazed Rage
    */
    AURA_SUNWELL_RADIANCE           = 45769,
    SPELL_SPECTRAL_INVISIBILITY     = 44801,//kalecgos e sathrovar

    //Kalecgos
    SPELL_SPECTRAL_BLAST            = 44869,//
    SPELL_ARCANE_BUFFET             = 45018,//
    SPELL_FROST_BREATH              = 44799,//
    SPELL_TAIL_LASH                 = 45122,//
    SPELL_WILD_MAGIC_1              = 45001,
    SPELL_WILD_MAGIC_2              = 45002,
    SPELL_WILD_MAGIC_3              = 45004,
    SPELL_WILD_MAGIC_4              = 45006,
    SPELL_WILD_MAGIC_5              = 45010,
    SPELL_WILD_MAGIC_6              = 44978,

    SPELL_BANISH                    = 44836,
    SPELL_TRANSFORM_KALEC           = 44670,
    SPELL_ENRAGE                    = 44807,

    //Sathrovar
    SPELL_DEMONIC_VISUAL            = 44800,
    SPELL_CORRUPTION_STRIKE         = 45029,
    SPELL_AGONY_CURSE               = 45032,
    SPELL_SHADOW_BOLT               = 45031,

    //45050
    SPELL_AGONY_CURSE_VISUAL_1      = 45083,
    SPELL_AGONY_CURSE_VISUAL_2      = 45084,
    SPELL_AGONY_CURSE_VISUAL_3      = 45085,
    SPELL_AGONY_CURSE_HOSTILE       = 45032,
    SPELL_AGONY_CURSE_ALLY          = 45034,

    //Kalecgos Friendly
    SPELL_HEROIC_STRIKE             = 45026,//
    SPELL_REVITALIZE                = 45027,//

    //Portal
    SPELL_SPECTRAL_BLAST_EFFECT     = 44866,
    SPELL_SPECTRAL_BLAST_VISUAL     = 46648,
    SPELL_SPECTRAL_REALM_TRIGGER    = 44811,
    SPELL_SPECTRAL_REALM_TELEPORT   = 46019,
    SPELL_SPECTRAL_REALM_AURA       = 46021,
    SPELL_SPECTRAL_REALM_2          = 44845,
    SPELL_SPECTRAL_REALM_REACTION   = 44852,
    SPELL_SPECTRAL_EXHAUSTION       = 44867,
    SPELL_TELEPORT_BACK             = 46020
};

enum KalecgosEvents
{
    //Kalecgos Drake
    EVENT_ARCANE_BUFFET = 1,
    EVENT_FROST_BREATH,
    EVENT_WILD_MAGIC,
    EVENT_TAIL_LASH,
    EVENT_SPECTRAL_BLAST,
    EVENT_CHECK_TIMER,

    //Kalecgos human
    EVENT_REVITALIZE,
    EVENT_HEROIC_STRIKE,
    EVENT_YELL_TIMER,

    //Sathrovar
    EVENT_SHADOWBOLT,
    EVENT_AGONY_CURSE,
    EVENT_CORRUPTION_STRIKE,
    EVENT_RESET_THREAT,

    EVENT_OUTRO_GOOD_1,
    EVENT_OUTRO_GOOD_2,
    EVENT_OUTRO_GOOD_3,
    EVENT_OUTRO_BAD_1,
    EVENT_OUTRO_BAD_2,
    EVENT_OUTRO_BAD_3,
    EVENT_OUTRO_START
};

enum SWPActions
{
    DO_ENRAGE                       =  1,
    DO_BANISH                       =  2,
    ACTION_END_BAD = 3,
    ACTION_END_GOOD = 4
};

enum Misc
{
    GO_SPECTRAL_RIFT                = 187055,
    MAX_PLAYERS_IN_SPECTRAL_REALM   = 0 // over this, teleport object won't work, 0 disables check
};

#define DRAGON_REALM_Z  53.079f
#define DEMON_REALM_Z   -74.558f

Position const FlyPos = { 1679.0f, 900.0f, 82.0f };
uint32 WildMagicSpells[] =
{
    SPELL_WILD_MAGIC_1,
    SPELL_WILD_MAGIC_2,
    SPELL_WILD_MAGIC_3,
    SPELL_WILD_MAGIC_4,
    SPELL_WILD_MAGIC_5,
    SPELL_WILD_MAGIC_6,
};

class boss_kalecgos : public CreatureScript
{
public:
    boss_kalecgos() : CreatureScript("boss_kalecgos") { }

    struct boss_kalecgosAI : public BossAI
    {
        boss_kalecgosAI(Creature* creature) : BossAI(creature, DATA_KALECGOS)
        {
            Initialize();
        }

        void Initialize()
        {
            isEnraged = false;
            isBanished = false;
        }

        bool isEnraged;
        bool isBanished;

        void Reset() override
        {
            _Reset();
            Initialize();

            events.ScheduleEvent(EVENT_ARCANE_BUFFET, Seconds(8));
            events.ScheduleEvent(EVENT_FROST_BREATH, Seconds(15));
            events.ScheduleEvent(EVENT_WILD_MAGIC, Seconds(10));
            events.ScheduleEvent(EVENT_TAIL_LASH, Seconds(25));
            events.ScheduleEvent(EVENT_SPECTRAL_BLAST, Seconds(20), Seconds(25));
            events.ScheduleEvent(EVENT_CHECK_TIMER, Seconds(1));

            if (Creature* Sath = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_SATHROVARR)))
                Sath->AI()->EnterEvadeMode();

            me->setFaction(14);
            me->SetDisableGravity(false);
            me->SetStandState(UNIT_STAND_STATE_SLEEP);
        }

        void EnterEvadeMode(EvadeReason why) override
        {
            _EnterEvadeMode();
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            _DespawnAtEvade(Seconds(10));
        }

        void DoAction(int32 action) override
        {
            switch (action)
            {
                case DO_ENRAGE:
                    isEnraged = true;
                    DoCastSelf(SPELL_ENRAGE, true);
                    break;
                case DO_BANISH:
                    isBanished = true;
                    DoCastSelf(SPELL_BANISH, true);
                    break;
                case ACTION_END_BAD:
                    events.ScheduleEvent(EVENT_OUTRO_START, Milliseconds(1));
                    events.ScheduleEvent(EVENT_OUTRO_BAD_1, Seconds(1));
                    break;
                case ACTION_END_GOOD:
                    events.ScheduleEvent(EVENT_OUTRO_START, Milliseconds(1));
                    events.ScheduleEvent(EVENT_OUTRO_GOOD_1, Seconds(1));
                    break;
            }
        }

        void DamageTaken(Unit* done_by, uint32 &damage) override
        {
            if (damage >= me->GetHealth() && done_by->GetGUID() != me->GetGUID())
                damage = 0;
        }

        void EnterCombat(Unit* /*who*/) override
        {
            me->SetStandState(UNIT_STAND_STATE_STAND);
            instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me, 1);
            Talk(SAY_EVIL_AGGRO);
            _EnterCombat();

            if (Creature* kalecgosHuman = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_KALECGOS_HUMAN)))
                if (Creature* sathrovar = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_SATHROVARR)))
                {
                    sathrovar->SetReactState(REACT_AGGRESSIVE);
                    kalecgosHuman->SetReactState(REACT_AGGRESSIVE);
                    sathrovar->SetInCombatWith(kalecgosHuman);
                    kalecgosHuman->SetInCombatWith(sathrovar);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, sathrovar, 2);
                }
        }

        void KilledUnit(Unit* /*victim*/) override
        {
            Talk(SAY_EVIL_SLAY);
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            me->SetVisible(false);
            if (id == 0)
            {
                me->GetMap()->ToInstanceMap()->PermBindAllPlayers();
                me->DespawnOrUnsummon();
            }
            else
                me->GetMotionMaster()->MoveTargetedHome();//TalkTimer = 1000;
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
                    case EVENT_ARCANE_BUFFET:
                        DoCastAOE(SPELL_ARCANE_BUFFET);
                        events.Repeat(Seconds(8));
                        break;
                    case EVENT_FROST_BREATH:
                        DoCastAOE(SPELL_FROST_BREATH);
                        events.Repeat(Seconds(15));
                        break;
                    case EVENT_TAIL_LASH:
                        DoCastAOE(SPELL_TAIL_LASH);
                        events.Repeat(Seconds(15));
                        break;
                    case EVENT_WILD_MAGIC:
                        DoCastAOE(WildMagicSpells[rand32() % 6]);
                        events.Repeat(Seconds(20));
                        break;
                    case EVENT_SPECTRAL_BLAST:
                        DoCastAOE(SPELL_SPECTRAL_BLAST, true);
                        events.Repeat(Seconds(20 + rand32() % 5));
                        break;
                    case EVENT_CHECK_TIMER:
                        if (HealthBelowPct(10) && !isEnraged)
                        {
                            if (Creature* Sath = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_SATHROVARR)))
                                Sath->AI()->DoAction(DO_ENRAGE);
                            DoAction(DO_ENRAGE);
                        }
                        if (!isBanished && HealthBelowPct(1))
                        {
                            if (Creature* Sath = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_SATHROVARR)))
                            {
                                if (Sath->HasAura(SPELL_BANISH))
                                {
                                    Sath->DealDamage(Sath, Sath->GetHealth());
                                    return;
                                }
                                else
                                    DoAction(DO_BANISH);
                            }
                        }
                        events.Repeat(Seconds(1));
                        break;
                    case EVENT_OUTRO_START:
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                        me->InterruptNonMeleeSpells(true);
                        me->RemoveAllAuras();
                        me->DeleteThreatList();
                        me->CombatStop();
                    case EVENT_OUTRO_GOOD_1:
                        me->setFaction(35);
                        events.ScheduleEvent(EVENT_OUTRO_GOOD_2, Seconds(1));
                        break;
                    case EVENT_OUTRO_GOOD_2:
                        Talk(SAY_GOOD_PLRWIN);
                        events.ScheduleEvent(EVENT_OUTRO_GOOD_3, Seconds(10));
                        break;
                    case EVENT_OUTRO_GOOD_3:
                        me->SetDisableGravity(true);
                        me->GetMotionMaster()->MovePoint(0, FlyPos);
                        break;
                    case EVENT_OUTRO_BAD_1:
                        Talk(SAY_EVIL_ENRAGE);
                        events.ScheduleEvent(EVENT_OUTRO_BAD_2, Seconds(3));
                        break;
                    case EVENT_OUTRO_BAD_2:
                        me->SetDisableGravity(true);
                        me->GetMotionMaster()->MovePoint(1, FlyPos);
                        events.ScheduleEvent(EVENT_OUTRO_BAD_3, Seconds(15));
                        break;
                    case EVENT_OUTRO_BAD_3:
                        EnterEvadeMode(EVADE_REASON_OTHER);
                        break;
                    default:
                        break;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetSunwellPlateauAI<boss_kalecgosAI>(creature);
    }
};

class SpectralBlastSelector : NonTankTargetSelector
{
    public:
        SpectralBlastSelector(Unit* source) : NonTankTargetSelector(source, true) { }

        bool operator()(WorldObject* target) const
        {
            if (Unit* unitTarget = target->ToUnit())
                return !NonTankTargetSelector::operator()(unitTarget) ||
                unitTarget->HasAura(SPELL_SPECTRAL_EXHAUSTION) || unitTarget->HasAura(SPELL_SPECTRAL_REALM_AURA);
            return false;
        }
};

class boss_kalec : public CreatureScript
{
public:
    boss_kalec() : CreatureScript("boss_kalec") { }

    struct boss_kalecAI : public ScriptedAI
    {
        boss_kalecAI(Creature* creature) : ScriptedAI(creature), _instance(creature->GetInstanceScript())
        {
            Initialize();
        }

        void Initialize()
        {
            me->SetReactState(REACT_PASSIVE);
            YellSequence = 0;
            isEnraged = false;
        }

        void Reset() override
        {
            SathGUID = _instance->GetGuidData(DATA_SATHROVARR);
            Initialize();

            _events.ScheduleEvent(EVENT_REVITALIZE, Seconds(5));
            _events.ScheduleEvent(EVENT_HEROIC_STRIKE, Seconds(3));
            _events.ScheduleEvent(EVENT_YELL_TIMER, Seconds(5));
        }

        void DamageTaken(Unit* done_by, uint32 &damage) override
        {
            if (done_by->GetGUID() != SathGUID)
                damage = 0;
            else if (isEnraged)
                damage *= 3;
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
                    case EVENT_REVITALIZE:
                        DoCast(me, SPELL_REVITALIZE);
                        _events.Repeat(Seconds(5));
                        break;
                    case EVENT_HEROIC_STRIKE:
                        DoCastVictim(SPELL_HEROIC_STRIKE);
                        _events.Repeat(Seconds(2));
                        break;
                    case EVENT_YELL_TIMER:
                        switch (YellSequence)
                        {
                            case 0:
                                Talk(SAY_GOOD_AGGRO);
                                ++YellSequence;
                                break;
                            case 1:
                                if (HealthBelowPct(50))
                                {
                                    Talk(SAY_GOOD_NEAR_DEATH);
                                    ++YellSequence;
                                }
                                break;
                            case 2:
                                if (HealthBelowPct(10))
                                {
                                    Talk(SAY_GOOD_NEAR_DEATH2);
                                    ++YellSequence;
                                }
                                break;
                            default:
                                break;
                        }
                        _events.Repeat(Seconds(5));
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
        InstanceScript* _instance;
        EventMap _events;
        uint32 YellSequence;
        ObjectGuid SathGUID;
        bool isEnraged; // if demon is enraged
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetSunwellPlateauAI<boss_kalecAI>(creature);
    }
};

class kalecgos_teleporter : public GameObjectScript
{
public:
    kalecgos_teleporter() : GameObjectScript("kalecgos_teleporter") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
#if MAX_PLAYERS_IN_SPECTRAL_REALM > 0
        uint8 SpectralPlayers = 0;
        Map::PlayerList const &PlayerList = go->GetMap()->GetPlayers();
        for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
        {
            if (i->GetSource() && i->GetSource()->GetPositionZ() < DEMON_REALM_Z + 5)
                ++SpectralPlayers;
        }

        if (player->HasAura(AURA_SPECTRAL_EXHAUSTION) || SpectralPlayers >= MAX_PLAYERS_IN_SPECTRAL_REALM)
        {
            return true;
        }
#else
        (void)go;
#endif

        player->CastSpell(player, SPELL_SPECTRAL_REALM_TRIGGER, true);
        return true;
    }
};

class boss_sathrovarr : public CreatureScript
{
public:
    boss_sathrovarr() : CreatureScript("boss_sathrovarr") { }

    struct boss_sathrovarrAI : public BossAI
    {
        boss_sathrovarrAI(Creature* creature) : BossAI(creature, DATA_KALECGOS)
        {
            Initialize();
        }

        void Initialize()
        {
            me->SetReactState(REACT_PASSIVE);
            isEnraged = false;
            isBanished = false;
        }

        bool isEnraged;
        bool isBanished;

        void Reset() override
        {
            _Reset();
            if (Creature* kalecgos = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_KALECGOS_DRAGON)))
                kalecgos->setDeathState(JUST_DIED);

            Initialize();

            DoCastSelf(SPELL_DEMONIC_VISUAL, true);
            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SPECTRAL_REALM_AURA);

            events.ScheduleEvent(EVENT_SHADOWBOLT, Seconds(7), Seconds(10));
            events.ScheduleEvent(EVENT_AGONY_CURSE, Seconds(20));
            events.ScheduleEvent(EVENT_CORRUPTION_STRIKE, Seconds(13));
            events.ScheduleEvent(EVENT_CHECK_TIMER, Seconds(1));
            events.ScheduleEvent(EVENT_RESET_THREAT, Seconds(1));
        }

        void EnterCombat(Unit* /*who*/) override
        {
            _EnterCombat();
            Talk(SAY_SATH_AGGRO);
        }

        void DamageTaken(Unit* done_by, uint32 &damage) override
        {
            if (damage >= me->GetHealth() && done_by->GetGUID() != me->GetGUID())
                damage = 0;
        }

        void KilledUnit(Unit* target) override
        {
            if (Creature* kalecgosHuman = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_KALECGOS_HUMAN)))
                if (kalecgosHuman->GetGUID() ==  target->GetGUID())
                {
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SPECTRAL_REALM_AURA);
                    kalecgosHuman->AI()->DoAction(ACTION_END_BAD);
                    EnterEvadeMode();
                    return;
                }
            Talk(SAY_SATH_SLAY);
        }

        void JustDied(Unit* /*killer*/) override
        {
            _JustDied();
            Talk(SAY_SATH_DEATH);
            me->SetPosition(me->GetPositionX(), me->GetPositionY(), DRAGON_REALM_Z, me->GetOrientation());
            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SPECTRAL_REALM_AURA);
            if (Creature* kalecgosHuman = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_KALECGOS_HUMAN)))
                kalecgosHuman->AI()->DoAction(ACTION_END_GOOD);

            instance->SetBossState(DATA_KALECGOS, DONE);
        }

        void DoAction(int32 param) override
        {
            switch (param)
            {
                case DO_ENRAGE:
                    isEnraged = true;
                    DoCastSelf(SPELL_ENRAGE, true);
                    break;
                case DO_BANISH:
                    isBanished = true;
                    DoCastSelf(SPELL_BANISH, true);
                    break;
            }
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
                    case EVENT_SHADOWBOLT:
                        if (!(rand32() % 5))
                            Talk(SAY_SATH_SPELL1);
                        DoCastAOE(SPELL_SHADOW_BOLT);
                        events.Repeat(Seconds(7 + (rand32() % 3)));
                        break;
                    case EVENT_AGONY_CURSE:
                    {
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1))
                            DoCast(target, SPELL_AGONY_CURSE);
                        else
                            DoCastVictim(SPELL_AGONY_CURSE);
                        events.Repeat(Seconds(20));
                        break;
                    }
                    case EVENT_CORRUPTION_STRIKE:
                        if (!(rand32() % 5))
                            Talk(SAY_SATH_SPELL2);
                        DoCastVictim(SPELL_CORRUPTION_STRIKE);
                        events.Repeat(Seconds(13));
                        break;
                    case EVENT_RESET_THREAT:
                    {
                        ThreatContainer::StorageType threatlist = me->getThreatManager().getThreatList();
                        for (ThreatContainer::StorageType::const_iterator itr = threatlist.begin(); itr != threatlist.end(); ++itr)
                        {
                            if (Unit* unit = ObjectAccessor::GetUnit(*me, (*itr)->getUnitGuid()))
                                if (unit->GetPositionZ() > me->GetPositionZ() + 5)
                                    me->getThreatManager().modifyThreatPercent(unit, -100);
                        }
                        events.Repeat(Seconds(1));
                        break;
                    }
                    case EVENT_CHECK_TIMER:
                    {
                        Creature* Kalec = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_KALECGOS_HUMAN));
                        if (!Kalec || !Kalec->IsAlive())
                        {
                            if (Creature* Kalecgos = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_KALECGOS_DRAGON)))
                                Kalecgos->AI()->EnterEvadeMode();
                            return;
                        }

                        if (HealthBelowPct(10) && !isEnraged)
                        {
                            if (Creature* Kalecgos = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_KALECGOS_DRAGON)))
                                Kalecgos->AI()->DoAction(DO_ENRAGE);
                            DoAction(DO_ENRAGE);
                        }

                        Creature* Kalecgos = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_KALECGOS_DRAGON));
                        if (Kalecgos && !Kalecgos->IsInCombat())
                        {
                            EnterEvadeMode();
                            return;
                        }

                        if (!isBanished && HealthBelowPct(1))
                        {
                            if (Kalecgos)
                            {
                                if (Kalecgos->HasAura(SPELL_BANISH))
                                {
                                    me->DealDamage(me, me->GetHealth());
                                    return;
                                }
                                DoAction(DO_BANISH);
                            }
                            else
                            {
                                EnterEvadeMode();
                                return;
                            }
                        }
                        events.Repeat(Seconds(1));
                        break;
                    }
                    default:
                        break;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetSunwellPlateauAI<boss_sathrovarrAI>(creature);
    }
};

// 44869 - Spectral Blast
class spell_kalecgos_spectral_blast : public SpellScriptLoader
{
    public:
        spell_kalecgos_spectral_blast() : SpellScriptLoader("spell_kalecgos_spectral_blast") { }

        class spell_kalecgos_spectral_blast_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_kalecgos_spectral_blast_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                targets.remove_if(SpectralBlastSelector(GetCaster()));
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Player* target = GetHitPlayer();
                Unit* caster = GetCaster();
                target->CastSpell(target, SPELL_SPECTRAL_BLAST_EFFECT, true);

                if (GameObject* portal = caster->FindNearestGameObject(GO_SPECTRAL_RIFT, 100.0f))
                    caster->CastSpell(target, SPELL_SPECTRAL_BLAST_VISUAL, true);

                caster->CastSpell(target, SPELL_SPECTRAL_REALM_TRIGGER, true);
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_kalecgos_spectral_blast_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnEffectHitTarget += SpellEffectFn(spell_kalecgos_spectral_blast_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_kalecgos_spectral_blast_SpellScript();
        }
};

// 44811 - Spectral Realm
class spell_kalecgos_spectral_realm_trigger : public SpellScriptLoader
{
    public:
        spell_kalecgos_spectral_realm_trigger() : SpellScriptLoader("spell_kalecgos_spectral_realm_trigger") { }

        class spell_kalecgos_spectral_realm_trigger_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_kalecgos_spectral_realm_trigger_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* target = GetHitUnit();
                target->CastSpell(target, SPELL_SPECTRAL_REALM_TELEPORT, true);
                target->CastSpell(target, SPELL_SPECTRAL_REALM_AURA, true);
                target->CastSpell(target, SPELL_SPECTRAL_REALM_2, true);
                target->CastSpell(target, SPELL_SPECTRAL_REALM_REACTION, true);
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_kalecgos_spectral_realm_trigger_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_kalecgos_spectral_realm_trigger_SpellScript();
        }
};

// 46021 - Spectral Realm
class spell_kalecgos_spectral_realm_aura : public SpellScriptLoader
{
    public:
        spell_kalecgos_spectral_realm_aura() : SpellScriptLoader("spell_kalecgos_spectral_realm_aura") { }

        class spell_kalecgos_spectral_realm_aura_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_kalecgos_spectral_realm_aura_AuraScript);

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                target->RemoveAurasDueToSpell(SPELL_SPECTRAL_REALM_REACTION);
                target->CastSpell(target, SPELL_TELEPORT_BACK, true);
                target->CastSpell(target, SPELL_SPECTRAL_EXHAUSTION, true);
            }

            void Register() override
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_kalecgos_spectral_realm_aura_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_MOD_INVISIBILITY_DETECT, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_kalecgos_spectral_realm_aura_AuraScript();
        }
};

// 45032, 45032 - Curse of Boundless Agony
class spell_kalecgos_curse_of_boundless_agony : public SpellScriptLoader
{
    public:
        spell_kalecgos_curse_of_boundless_agony() : SpellScriptLoader("spell_kalecgos_curse_of_boundless_agony") { }

        class spell_kalecgos_curse_of_boundless_agony_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_kalecgos_curse_of_boundless_agony_AuraScript);

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (InstanceScript* instance = GetTarget()->GetInstanceScript())
                    if (instance->GetBossState(DATA_KALECGOS) == IN_PROGRESS)
                        return;
                std::cout << "\n Removing by cancel";
                Remove(AURA_REMOVE_BY_CANCEL);
            }

            void OnPeriodic(AuraEffect const* aurEff)
            {
                std::cout << "\nPeriodic - Tick: " << std::to_string(aurEff->GetTickNumber());
                if (aurEff->GetTickNumber() <= 5)
                    GetTarget()->CastSpell(GetTarget(), SPELL_AGONY_CURSE_VISUAL_1, true);
                else if (aurEff->GetTickNumber() <= 10)
                    GetTarget()->CastSpell(GetTarget(), SPELL_AGONY_CURSE_VISUAL_2, true);
                else
                    GetTarget()->CastSpell(GetTarget(), SPELL_AGONY_CURSE_VISUAL_3, true);
            }

            void HandleEffectPeriodicUpdate(AuraEffect* aurEff)
            {
                if (aurEff->GetTickNumber() > 1 && aurEff->GetTickNumber() % 5 == 1)
                    aurEff->SetAmount(aurEff->GetAmount() * 2);
                std::cout << "\n Tick: " << std::to_string(aurEff->GetTickNumber()) << " Amount: " << std::to_string(aurEff->GetAmount());
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                std::cout << "\n On RemoveMode: " << std::to_string(GetTargetApplication()->GetRemoveMode());
                if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_CANCEL)
                    GetTarget()->CastSpell(GetTarget(), SPELL_AGONY_CURSE_ALLY, true);
            }

            void Register() override
            {
                AfterEffectApply += AuraEffectApplyFn(spell_kalecgos_curse_of_boundless_agony_AuraScript::OnApply, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_kalecgos_curse_of_boundless_agony_AuraScript::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
                OnEffectUpdatePeriodic += AuraEffectUpdatePeriodicFn(spell_kalecgos_curse_of_boundless_agony_AuraScript::HandleEffectPeriodicUpdate, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
                AfterEffectRemove += AuraEffectRemoveFn(spell_kalecgos_curse_of_boundless_agony_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_kalecgos_curse_of_boundless_agony_AuraScript();
        }
};

void AddSC_boss_kalecgos()
{
    new boss_kalecgos();
    new boss_sathrovarr();
    new boss_kalec();
    new kalecgos_teleporter();
    new spell_kalecgos_spectral_blast();
    new spell_kalecgos_spectral_realm_trigger();
    new spell_kalecgos_spectral_realm_aura();
    new spell_kalecgos_curse_of_boundless_agony();
}
