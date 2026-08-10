// Durable MT4 unit identity → ordinal K map (AD-6). Owned by DeviceSessionManager layer.
// Prefer USB serial; topology/instance path is the fallback key. MidiBackend never assigns K.

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

enum class UnitIdentityKind : std::uint8_t
{
    Serial = 1,
    Topology = 2
};

struct UnitIdentityBinding
{
    UnitIdentityKind kind = UnitIdentityKind::Topology;
    std::string key;
    unsigned unitOrdinalK = 0;
};

// Primary key (serial when present) plus optional topology alias for AD-6 migration.
struct UnitIdentityResolveRequest
{
    UnitIdentityKind kind = UnitIdentityKind::Topology;
    const std::string* key = nullptr;
    // When non-null/non-empty, also bind or migrate this topology key to the same K.
    const std::string* topologyKey = nullptr;
};

class UnitIdentityRegistry
{
public:
    UnitIdentityRegistry() = default;

    // Resolve or allocate K. Known keys keep K; new keys get next free K.
    // Does not renumber existing bindings when peers disappear.
    // Multiple identity keys may share one K (serial + topology for the same unit).
    // newlyAssignedOut is set when this call allocated a fresh K (not a lookup/migrate).
    bool resolveOrAssign(
        const UnitIdentityResolveRequest& request,
        unsigned& unitOrdinalKOut,
        std::string& errorOut,
        bool* newlyAssignedOut = nullptr);

    // Lookup without allocating. Returns false when the identity is unknown.
    bool tryLookup(
        UnitIdentityKind kind,
        const std::string& key,
        unsigned& unitOrdinalKOut) const;

    // Remove one map entry (used to roll back a failed Start after a fresh assign).
    void unbindKey(UnitIdentityKind kind, const std::string& key) noexcept;

    void clear() noexcept;

    // Replace in-memory map (tests / load). Keys must be non-empty; K must be >= 1.
    // Duplicate map keys fail. Distinct serial primaries must not share one K;
    // serial+topology aliases for the same unit may share K.
    bool replaceAll(
        const std::vector<UnitIdentityBinding>& bindings,
        std::string& errorOut);

    std::vector<UnitIdentityBinding> snapshot() const;

    // Text persistence (user-scoped path). Fail closed with English diagnostics.
    bool loadFromFile(const std::string& path, std::string& errorOut);
    bool saveToFile(const std::string& path, std::string& errorOut) const;

    // Default user-scoped path for the Bridge process identity (Windows LOCALAPPDATA).
    static std::string defaultPersistencePath(std::string& errorOut);

private:
    static std::string makeMapKey(UnitIdentityKind kind, const std::string& key);
    static bool parseMapKey(
        const std::string& mapKey,
        UnitIdentityKind& kindOut,
        std::string& keyOut);
    unsigned nextFreeOrdinal() const;
    void bindKey(UnitIdentityKind kind, const std::string& key, unsigned unitOrdinalK);
    void bindPrimaryAndTopology(
        UnitIdentityKind kind,
        const std::string& key,
        const std::string* topologyKey,
        unsigned unitOrdinalK);
    bool hasSerialCollisionOnK(
        const std::string& serialKey,
        unsigned ordinal,
        std::string& errorOut) const;

    std::unordered_map<std::string, unsigned> identityToK_;
};
