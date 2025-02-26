/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
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

#ifndef TRINITYCORE_CREATURE_H
#define TRINITYCORE_CREATURE_H

#include "Unit.h"
#include "Common.h"
#include "CreatureData.h"
#include "DatabaseEnvFwd.h"
#include "Duration.h"
#include "Loot.h"
#include "MapObject.h"

#include <list>

class CreatureAI;
class CreatureGroup;
class Group;
class Quest;
class Player;
class SpellInfo;
class WildBattlePet;
class WorldSession;

enum MovementGeneratorType : uint8;

struct ScriptParam;

struct VendorItemCount
{
    VendorItemCount(uint32 _item, uint32 _count)
        : itemId(_item), count(_count), lastIncrementTime(time(nullptr)) { }

    uint32 itemId;
    uint32 count;
    time_t lastIncrementTime;
};

typedef std::list<Creature*> CreatureList;
typedef std::list<VendorItemCount> VendorItemCounts;

// max different by z coordinate for creature aggro reaction
#define CREATURE_Z_ATTACK_RANGE 3

#define MAX_VENDOR_ITEMS 150                                // Limitation in 4.x.x item count in SMSG_LIST_INVENTORY

//used for handling non-repeatable random texts
typedef std::vector<uint8> CreatureTextRepeatIds;
typedef std::unordered_map<uint8, CreatureTextRepeatIds> CreatureTextRepeatGroup;

class TC_GAME_API Creature : public Unit, public GridObject<Creature>, public MapObject
{
    public:
        explicit Creature(bool isWorldObject = false);
        virtual ~Creature();

        void AddToWorld() override;
        void RemoveFromWorld() override;

        void SetObjectScale(float scale) override;
        void SetDisplayId(uint32 displayId, float displayScale = 1.f) override;
        void SetDisplayFromModel(uint32 modelIdx);

        void DisappearAndDie();

        bool Create(ObjectGuid::LowType guidlow, Map* map, uint32 entry, Position const& pos, CreatureData const* data, uint32 vehId, bool dynamic = false);

        static Creature* CreateCreature(uint32 entry, Map* map, Position const& pos, uint32 vehId = 0);
        static Creature* CreateCreatureFromDB(ObjectGuid::LowType spawnId, Map* map, bool addToMap = true, bool allowDuplicate = false);

        bool LoadCreaturesAddon();
        void SelectLevel();
        void UpdateLevelDependantStats();
        void LoadEquipment(int8 id = 1, bool force = false);
        void SetSpawnHealth();
        void SetBaseHealth(uint64 health);

		void ReLoad(bool skipDB);

        ObjectGuid::LowType GetSpawnId() const { return m_spawnId; }

        void Update(uint32 time) override;                         // overwrited Unit::Update
        void GetRespawnPosition(float &x, float &y, float &z, float* ori = nullptr, float* dist = nullptr) const;
        bool IsSpawnedOnTransport() const { return m_creatureData && m_creatureData->spawnPoint.GetMapId() != GetMapId(); }

        void SetCorpseDelay(uint32 delay) { m_corpseDelay = delay; }
        uint32 GetCorpseDelay() const { return m_corpseDelay; }
        bool IsRacialLeader() const { return GetCreatureTemplate()->RacialLeader; }
        bool IsCivilian() const { return (GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_CIVILIAN) != 0; }
        bool IsTrigger() const { return (GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_TRIGGER) != 0; }
        bool IsGuard() const { return (GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_GUARD) != 0; }
        bool CanWalk() const { return (GetCreatureTemplate()->InhabitType & INHABIT_GROUND) != 0; }
        bool CanSwim() const override { return (GetCreatureTemplate()->InhabitType & INHABIT_WATER) != 0 || IsPet(); }
        bool CanFly()  const override;
        bool IsDungeonBoss() const { return (GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_DUNGEON_BOSS) != 0; }
        bool IsAffectedByDiminishingReturns() const override { return Unit::IsAffectedByDiminishingReturns() || (GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_ALL_DIMINISH) != 0; }

        void SetReactState(ReactStates st) { m_reactState = st; }
        ReactStates GetReactState() const { return m_reactState; }
        bool HasReactState(ReactStates state) const { return (m_reactState == state); }
        void InitializeReactState();

        bool IsStopMovingAndAttacking;
        uint32 stopTimer;
        void StopMovingAndAttacking(uint32 timer = 0);
        void FaceTargetAndStopMoving(Unit* target, uint32 timer);

        /// @todo Rename these properly
        bool isCanInteractWithBattleMaster(Player* player, bool msg) const;
        bool CanResetTalents(Player* player) const;
        bool CanCreatureAttack(Unit const* victim, bool force = true) const;
        void LoadTemplateImmunities();
        bool IsImmunedToSpell(SpellInfo const* spellInfo, Unit* caster) const override;
        bool IsImmunedToSpellEffect(SpellInfo const* spellInfo, uint32 index, Unit* caster) const override;
        bool isElite() const;
        bool isWorldBoss() const;

        bool HasScalableLevels() const;
        uint8 GetLevelForTarget(WorldObject const* target) const override;

        uint64 GetMaxHealthByLevel(uint8 level) const;
        float GetHealthMultiplierForTarget(WorldObject const* target) const override;

        float GetBaseDamageForLevel(uint8 level) const;
        float GetDamageMultiplierForTarget(WorldObject const* target) const override;

        float GetBaseArmorForLevel(uint8 level) const;
        float GetArmorMultiplierForTarget(WorldObject const* target) const override;

        bool IsInEvadeMode() const { return HasUnitState(UNIT_STATE_EVADE); }
        bool IsEvadingAttacks() const { return IsInEvadeMode() || CanNotReachTarget(); }

        bool AIM_Destroy();
        bool AIM_Create(CreatureAI* ai = nullptr);
        void AI_InitializeAndEnable();
        bool AIM_Initialize(CreatureAI* ai = nullptr);
        void Motion_Initialize();

        CreatureAI* AI() const { return reinterpret_cast<CreatureAI*>(i_AI); }

        SpellSchoolMask GetMeleeDamageSchoolMask() const override { return m_meleeDamageSchoolMask; }
        void SetMeleeDamageSchool(SpellSchools school) { m_meleeDamageSchoolMask = SpellSchoolMask(1 << school); }

        bool HasSpell(uint32 spellID) const override;

        bool UpdateEntry(uint32 entry, CreatureData const* data = nullptr, bool updateLevel = true, bool updateScript = false);

        void UpdateMovementFlags();

        bool UpdateStats(Stats stat) override;
        bool UpdateAllStats() override;
        void UpdateArmor() override;
        void UpdateMaxHealth() override;
        void UpdateMaxPower(Powers power) override;
        uint32 GetPowerIndex(Powers power) const override;
        void UpdateAttackPowerAndDamage(bool ranged = false) override;
        void CalculateMinMaxDamage(WeaponAttackType attType, bool normalized, bool addTotalPct, float& minDamage, float& maxDamage) const override;

        void SetCanDualWield(bool value) override;
        int8 GetOriginalEquipmentId() const { return m_originalEquipmentId; }
        uint8 GetCurrentEquipmentId() const { return m_equipmentId; }
        void SetCurrentEquipmentId(uint8 id) { m_equipmentId = id; }

        float GetSpellDamageMod(int32 Rank) const;

        VendorItemData const* GetVendorItems() const;
        uint32 GetVendorItemCurrentCount(VendorItem const* vItem);
        uint32 UpdateVendorItemCurrentCount(VendorItem const* vItem, uint32 used_count);

        TrainerSpellData const* GetTrainerSpells() const;

        CreatureTemplate const* GetCreatureTemplate() const { return m_creatureInfo; }
        CreatureData const* GetCreatureData() const { return m_creatureData; }
        CreatureAddon const* GetCreatureAddon() const;

        std::string const& GetAIName() const;
        std::string GetScriptName() const;
        uint32 GetScriptId() const;
        ScriptParam GetScriptParam(uint8 index) const;

        // override WorldObject function for proper name localization
        std::string GetNameForLocaleIdx(LocaleConstant locale) const override;

        void setDeathState(DeathState s) override;                   // override virtual Unit::setDeathState

        bool LoadFromDB(ObjectGuid::LowType spawnId, Map* map, bool addToMap, bool allowDuplicate);
        void SaveToDB();
                                                            // overriden in Pet
        virtual void SaveToDB(uint32 mapid, std::vector<Difficulty> const& spawnDifficulties);
        virtual void DeleteFromDB();                        // overriden in Pet

        Loot loot;
        void StartPickPocketRefillTimer();
        void ResetPickPocketRefillTimer() { _pickpocketLootRestore = 0; }
        bool CanGeneratePickPocketLoot() const { return _pickpocketLootRestore <= time(nullptr); }
        void SetSkinner(ObjectGuid guid) { _skinner = guid; }
        ObjectGuid GetSkinner() const { return _skinner; } // Returns the player who skinned this creature
        std::vector<Player*> GetLootRecipients() const;
        std::vector<Group*> GetLootRecipientGroups() const;
        bool HasLootRecipients() const { return !m_lootRecipients.empty(); }

        void SetLootRecipientGUIDs(GuidVector lootRecipients) { m_lootRecipients = lootRecipients; }
        GuidVector const& GetLootRecipientGUIDs() const { return m_lootRecipients; }

        bool IsTappedBy(Player const* player) const;                          // return true if the creature is tapped by the player or a member of his party.

        void AddLootRecipient(Unit* unit);
        void ResetLootRecipients();
        void AllLootRemovedFromCorpse();

        uint16 GetLootMode() const { return m_LootMode; }
        bool HasLootMode(uint16 lootMode) { return (m_LootMode & lootMode) != 0; }
        void SetLootMode(uint16 lootMode) { m_LootMode = lootMode; }
        void AddLootMode(uint16 lootMode) { m_LootMode |= lootMode; }
        void RemoveLootMode(uint16 lootMode) { m_LootMode &= ~lootMode; }
        void ResetLootMode() { m_LootMode = LOOT_MODE_DEFAULT; }

        uint32 m_spells[MAX_CREATURE_SPELLS];

        bool CanStartAttack(Unit const* u, bool force) const;
        float GetAttackDistance(Unit const* player) const;
        float GetAggroRange(Unit const* target) const;

        void SendAIReaction(AiReaction reactionType);

        Unit* SelectNearestTarget(float dist = 0, bool playerOnly = false) const;
        Unit* SelectNearestTargetInAttackDistance(float dist = 0) const;
        std::vector<Unit*> SelectNearestTargetsInAttackDistance(float dist = 0) const;
        Unit* SelectNearestHostileUnitInAggroRange(bool useLOS = false) const;

        void DoFleeToGetAssistance();
        void CallForHelp(float fRadius);
        void CallAssistance();
        void SetNoCallAssistance(bool val) { m_AlreadyCallAssistance = val; }
        void SetNoSearchAssistance(bool val) { m_AlreadySearchedAssistance = val; }
        bool HasSearchedAssistance() const { return m_AlreadySearchedAssistance; }
        bool CanAssistTo(Unit const* u, Unit const* enemy, bool checkfaction = true) const;
        bool _IsTargetAcceptable(Unit const* target) const;

        MovementGeneratorType GetDefaultMovementType() const { return m_defaultMovementType; }
        void SetDefaultMovementType(MovementGeneratorType mgt) { m_defaultMovementType = mgt; }

        void RemoveCorpse(bool setSpawnTime = true, bool destroyForNearbyPlayers = true);

        void DespawnOrUnsummon(uint32 msTimeToDespawn = 0, Seconds const& forceRespawnTime = Seconds(0));
        void DespawnOrUnsummon(Milliseconds const& time, Seconds const& forceRespawnTime = Seconds(0)) { DespawnOrUnsummon(uint32(time.count()), forceRespawnTime); }
        void DespawnCreaturesInArea(uint32 entry, float range = 125.0f);

        time_t const& GetRespawnTime() const { return m_respawnTime; }
        time_t GetRespawnTimeEx() const;
        void SetRespawnTime(uint32 respawn) { m_respawnTime = respawn ? time(nullptr) + respawn : 0; }
        void Respawn(bool force = false);
        void SaveRespawnTime(uint32 forceDelay = 0, bool savetodb = true) override;

        uint32 GetRespawnDelay() const { return m_respawnDelay; }
        void SetRespawnDelay(uint32 delay) { m_respawnDelay = delay; }

        float GetRespawnRadius() const { return m_respawnradius; }
        void SetRespawnRadius(float dist) { m_respawnradius = dist; }

        void DoImmediateBoundaryCheck() { m_boundaryCheckTime = 0; }
        uint32 GetCombatPulseDelay() const { return m_combatPulseDelay; }
        void SetCombatPulseDelay(uint32 delay) // (secs) interval at which the creature pulses the entire zone into combat (only works in dungeons)
        {
            m_combatPulseDelay = delay;
            if (m_combatPulseTime == 0 || m_combatPulseTime > delay)
                m_combatPulseTime = delay;
        }

        void SendZoneUnderAttackMessage(Player* attacker);

        void SetInCombatWithZone();

        bool hasQuest(uint32 quest_id) const override;
        bool hasInvolvedQuest(uint32 quest_id)  const override;

        bool CanRegenerateHealth() { return !_regenerateHealthLock && _regenerateHealth; }
        void SetRegenerateHealth(bool value) { _regenerateHealthLock = !value; }
        virtual uint8 GetPetAutoSpellSize() const { return MAX_SPELL_CHARM; }
        virtual uint32 GetPetAutoSpellOnPos(uint8 pos) const;
        float GetPetChaseDistance() const;

        void SetCannotReachTarget(bool cannotReach) { if (cannotReach == m_cannotReachTarget) return; m_cannotReachTarget = cannotReach; m_cannotReachTimer = 0; }
        bool CanNotReachTarget() const { return m_cannotReachTarget; }

        void SetHomePosition(float x, float y, float z, float o) { m_homePosition.Relocate(x, y, z, o); }
        void SetHomePosition(Position const& pos) { m_homePosition.Relocate(pos); }
        void GetHomePosition(float& x, float& y, float& z, float& ori) const { m_homePosition.GetPosition(x, y, z, ori); }
        Position const& GetHomePosition() const { return m_homePosition; }

        void SetTransportHomePosition(float x, float y, float z, float o) { m_transportHomePosition.Relocate(x, y, z, o); }
        void SetTransportHomePosition(Position const& pos) { m_transportHomePosition.Relocate(pos); }
        void GetTransportHomePosition(float& x, float& y, float& z, float& ori) const { m_transportHomePosition.GetPosition(x, y, z, ori); }
        Position const& GetTransportHomePosition() const { return m_transportHomePosition; }

        uint32 GetWaypointPath() const { return _waypointPathId; }
        void LoadPath(uint32 pathid) { _waypointPathId = pathid; }

        // nodeId, pathId
        std::pair<uint32, uint32> GetCurrentWaypointInfo() const { return _currentWaypointNodeInfo; }
        void UpdateCurrentWaypointInfo(uint32 nodeId, uint32 pathId) { _currentWaypointNodeInfo = { nodeId, pathId }; }

        bool IsReturningHome() const;

        void SearchFormation();
        CreatureGroup* GetFormation() { return m_formation; }
        void SetFormation(CreatureGroup* formation) { m_formation = formation; }
        bool IsFormationLeader() const;
        void SignalFormationMovement(Position const& destination, uint32 id = 0, uint32 moveType = 0, bool orientation = false);
        bool IsFormationLeaderMoveAllowed() const;

        Unit* SelectVictim(bool evadeIfNoVictim = true);

        void SetDisableReputationGain(bool disable) { DisableReputationGain = disable; }
        bool IsReputationGainDisabled() const { return DisableReputationGain; }
        bool IsDamageEnoughForLootingAndReward() const { return (m_creatureInfo->flags_extra & CREATURE_FLAG_EXTRA_NO_PLAYER_DAMAGE_REQ) || (m_PlayerDamageReq == 0); }
        void LowerPlayerDamageReq(uint64 unDamage);
        void ResetPlayerDamageReq() { m_PlayerDamageReq = GetHealth() / 2; }
        uint64 m_PlayerDamageReq;

        uint32 GetOriginalEntry() const { return m_originalEntry; }
        void SetOriginalEntry(uint32 entry) { m_originalEntry = entry; }

        // There's many places not ready for dynamic spawns. This allows them to live on for now.
        void SetRespawnCompatibilityMode(bool mode = true) { m_respawnCompatibilityMode = mode; }
        bool GetRespawnCompatibilityMode() { return m_respawnCompatibilityMode; }

        static float _GetDamageMod(int32 Rank);

        float m_SightDistance, m_CombatDistance;

        bool m_isTempWorldObject; //true when possessed

        // Handling caster facing during spellcast
        void SetTarget(ObjectGuid const& guid) override;
        void MustReacquireTarget() { m_shouldReacquireTarget = true; } // flags the Creature for forced (client displayed) target reacquisition in the next ::Update call
        void DoNotReacquireTarget() { m_shouldReacquireTarget = false; m_suppressedTarget = ObjectGuid::Empty; m_suppressedOrientation = 0.0f; }
        void FocusTarget(Spell const* focusSpell, WorldObject const* target);
        bool IsFocusing(Spell const* focusSpell = nullptr, bool withDelay = false) override;
        void ReleaseFocus(Spell const* focusSpell = nullptr, bool withDelay = true);

        bool IsMovementPreventedByCasting() const override;

        // Part of Evade mechanics
        time_t GetLastDamagedTime() const { return _lastDamagedTime; }
        void SetLastDamagedTime(time_t val) { _lastDamagedTime = val; }

        CreatureTextRepeatIds GetTextRepeatGroup(uint8 textGroup);
        void SetTextRepeatId(uint8 textGroup, uint8 id);
        void ClearTextRepeatGroup(uint8 textGroup);
        bool IsEscortNPC(bool onlyIfActive = true);

        bool CanGiveExperience() const;

        void ForcedDespawn(uint32 timeMSToDespawn = 0, Seconds const& forceRespawnTimer = Seconds(0));

        WildBattlePet* GetWildBattlePet() { return m_wildBattlePet; }

    protected:
        bool CreateFromProto(ObjectGuid::LowType guidlow, uint32 entry, CreatureData const* data = nullptr, uint32 vehId = 0);
        bool InitEntry(uint32 entry, CreatureData const* data = nullptr);

        // vendor items
        VendorItemCounts m_vendorItemCounts;

        static float _GetHealthMod(int32 Rank);

        std::vector<ObjectGuid> m_lootRecipients;
        ObjectGuid _skinner;

        /// Timers
        time_t _pickpocketLootRestore;
        time_t m_corpseRemoveTime;                          // (msecs)timer for death or corpse disappearance
        time_t m_respawnTime;                               // (secs) time of next respawn
        uint32 m_respawnDelay;                              // (secs) delay between corpse disappearance and respawning
        uint32 m_corpseDelay;                               // (secs) delay between death and corpse disappearance
        float m_respawnradius;
        uint32 m_boundaryCheckTime;                         // (msecs) remaining time for next evade boundary check
        uint32 m_combatPulseTime;                           // (msecs) remaining time for next zone-in-combat pulse
        uint32 m_combatPulseDelay;                          // (secs) how often the creature puts the entire zone in combat (only works in dungeons)

        ReactStates m_reactState;                           // for AI, not charmInfo
        void RegenerateHealth();
        void Regenerate(Powers power);
        MovementGeneratorType m_defaultMovementType;
        ObjectGuid::LowType m_spawnId;                               ///< For new or temporary creatures is 0 for saved it is lowguid
        uint8 m_equipmentId;
        int8 m_originalEquipmentId; // can be -1

        bool m_AlreadyCallAssistance;
        bool m_AlreadySearchedAssistance;
        bool m_cannotReachTarget;
        uint32 m_cannotReachTimer;
        bool m_AI_locked;

        SpellSchoolMask m_meleeDamageSchoolMask;
        uint32 m_originalEntry;

        Position m_homePosition;
        Position m_transportHomePosition;

        bool DisableReputationGain;

        CreatureTemplate const* m_creatureInfo;                 // Can differ from sObjectMgr->GetCreatureTemplate(GetEntry()) in difficulty mode > 0
        CreatureData const* m_creatureData;

        uint16 m_LootMode;                                  // Bitmask (default: LOOT_MODE_DEFAULT) that determines what loot will be lootable

        bool IsInvisibleDueToDespawn() const override;
        bool CanAlwaysSee(WorldObject const* obj) const override;

    private:
        bool CheckNoGrayAggroConfig(uint32 playerLevel, uint32 creatureLevel) const; // No aggro from gray creatures

        // Waypoint path
        uint32 _waypointPathId;
        std::pair<uint32/*nodeId*/, uint32/*pathId*/> _currentWaypointNodeInfo;

        //Formation var
        CreatureGroup* m_formation;
        bool m_triggerJustAppeared;
        bool m_respawnCompatibilityMode;

        /* Spell focus system */
        Spell const* m_focusSpell;   // Locks the target during spell cast for proper facing
        uint32 m_focusDelay;
        bool m_shouldReacquireTarget;
        ObjectGuid m_suppressedTarget; // Stores the creature's "real" target while casting
        float m_suppressedOrientation; // Stores the creature's "real" orientation while casting

        time_t _lastDamagedTime; // Part of Evade mechanics
        CreatureTextRepeatGroup m_textRepeat;

        WildBattlePet* m_wildBattlePet;

        // Regenerate health
        bool _regenerateHealth; // Set on creation
        bool _regenerateHealthLock; // Dynamically set
};

class TC_GAME_API AssistDelayEvent : public BasicEvent
{
    public:
        AssistDelayEvent(ObjectGuid victim, Unit& owner) : BasicEvent(), m_victim(victim), m_owner(owner) { }

        bool Execute(uint64 e_time, uint32 p_time) override;
        void AddAssistant(ObjectGuid guid) { m_assistants.push_back(guid); }
    private:
        AssistDelayEvent();

        ObjectGuid        m_victim;
        GuidList          m_assistants;
        Unit&             m_owner;
};

class TC_GAME_API ForcedDespawnDelayEvent : public BasicEvent
{
    public:
        ForcedDespawnDelayEvent(Creature& owner, Seconds const& respawnTimer) : BasicEvent(), m_owner(owner), m_respawnTimer(respawnTimer) { }
        bool Execute(uint64 e_time, uint32 p_time) override;

    private:
        Creature& m_owner;
        Seconds const m_respawnTimer;
};

#endif
