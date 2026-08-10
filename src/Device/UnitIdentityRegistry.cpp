#include "Device/UnitIdentityRegistry.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
const char* kindToken(UnitIdentityKind kind)
{
    return kind == UnitIdentityKind::Serial ? "serial" : "topology";
}

bool parseKindToken(const std::string& token, UnitIdentityKind& kindOut)
{
    if (token == "serial")
    {
        kindOut = UnitIdentityKind::Serial;
        return true;
    }
    if (token == "topology")
    {
        kindOut = UnitIdentityKind::Topology;
        return true;
    }
    return false;
}
} // namespace

std::string UnitIdentityRegistry::makeMapKey(
    UnitIdentityKind kind,
    const std::string& key)
{
    return std::string(kindToken(kind)) + '|' + key;
}

bool UnitIdentityRegistry::parseMapKey(
    const std::string& mapKey,
    UnitIdentityKind& kindOut,
    std::string& keyOut)
{
    const auto bar = mapKey.find('|');
    if (bar == std::string::npos || bar == 0 || bar + 1 >= mapKey.size())
    {
        return false;
    }
    if (!parseKindToken(mapKey.substr(0, bar), kindOut))
    {
        return false;
    }
    keyOut = mapKey.substr(bar + 1);
    return !keyOut.empty();
}

unsigned UnitIdentityRegistry::nextFreeOrdinal() const
{
    std::unordered_set<unsigned> used;
    for (const auto& entry : identityToK_)
    {
        used.insert(entry.second);
    }
    unsigned candidate = 1;
    while (used.find(candidate) != used.end())
    {
        ++candidate;
    }
    return candidate;
}

bool UnitIdentityRegistry::tryLookup(
    UnitIdentityKind kind,
    const std::string& key,
    unsigned& unitOrdinalKOut) const
{
    if (key.empty())
    {
        return false;
    }
    const auto found = identityToK_.find(makeMapKey(kind, key));
    if (found == identityToK_.end())
    {
        return false;
    }
    unitOrdinalKOut = found->second;
    return true;
}

void UnitIdentityRegistry::clear() noexcept
{
    identityToK_.clear();
}

namespace
{
bool lookupExistingOrdinal(
    const UnitIdentityRegistry& registry,
    const UnitIdentityResolveRequest& request,
    const std::string* topologyKey,
    unsigned& unitOrdinalKOut)
{
    if (registry.tryLookup(request.kind, *request.key, unitOrdinalKOut))
    {
        return true;
    }
    return topologyKey != nullptr
        && registry.tryLookup(UnitIdentityKind::Topology, *topologyKey, unitOrdinalKOut);
}
} // namespace

void UnitIdentityRegistry::bindKey(
    UnitIdentityKind kind,
    const std::string& key,
    unsigned unitOrdinalK)
{
    identityToK_[makeMapKey(kind, key)] = unitOrdinalK;
}

bool UnitIdentityRegistry::resolveOrAssign(
    const UnitIdentityResolveRequest& request,
    unsigned& unitOrdinalKOut,
    std::string& errorOut)
{
    if (request.key == nullptr || request.key->empty())
    {
        errorOut = "UnitIdentityRegistry: identity key is empty";
        return false;
    }
    const std::string* topologyKey = nullptr;
    if (request.topologyKey != nullptr && !request.topologyKey->empty()
        && *request.topologyKey != *request.key)
    {
        topologyKey = request.topologyKey;
    }

    unsigned existing = 0;
    if (lookupExistingOrdinal(*this, request, topologyKey, existing))
    {
        bindKey(request.kind, *request.key, existing);
        if (topologyKey != nullptr)
        {
            bindKey(UnitIdentityKind::Topology, *topologyKey, existing);
        }
        unitOrdinalKOut = existing;
        errorOut.clear();
        return true;
    }

    const unsigned assigned = nextFreeOrdinal();
    bindKey(request.kind, *request.key, assigned);
    if (topologyKey != nullptr)
    {
        bindKey(UnitIdentityKind::Topology, *topologyKey, assigned);
    }
    unitOrdinalKOut = assigned;
    errorOut.clear();
    return true;
}

bool UnitIdentityRegistry::replaceAll(
    const std::vector<UnitIdentityBinding>& bindings,
    std::string& errorOut)
{
    std::unordered_map<std::string, unsigned> next;
    for (const UnitIdentityBinding& binding : bindings)
    {
        if (binding.key.empty() || binding.unitOrdinalK < 1)
        {
            errorOut = "UnitIdentityRegistry: binding key/K invalid";
            return false;
        }
        const std::string mapKey = makeMapKey(binding.kind, binding.key);
        const auto inserted = next.emplace(mapKey, binding.unitOrdinalK);
        if (!inserted.second)
        {
            if (inserted.first->second != binding.unitOrdinalK)
            {
                errorOut = "UnitIdentityRegistry: duplicate identity key in bindings";
                return false;
            }
            continue;
        }
    }
    identityToK_ = std::move(next);
    errorOut.clear();
    return true;
}

std::vector<UnitIdentityBinding> UnitIdentityRegistry::snapshot() const
{
    std::vector<UnitIdentityBinding> out;
    out.reserve(identityToK_.size());
    for (const auto& entry : identityToK_)
    {
        UnitIdentityBinding binding;
        if (!parseMapKey(entry.first, binding.kind, binding.key))
        {
            continue;
        }
        binding.unitOrdinalK = entry.second;
        out.push_back(std::move(binding));
    }
    std::sort(out.begin(), out.end(), [](const UnitIdentityBinding& a,
                                         const UnitIdentityBinding& b) {
        if (a.unitOrdinalK != b.unitOrdinalK)
        {
            return a.unitOrdinalK < b.unitOrdinalK;
        }
        if (a.kind != b.kind)
        {
            return static_cast<unsigned>(a.kind) < static_cast<unsigned>(b.kind);
        }
        return a.key < b.key;
    });
    return out;
}
