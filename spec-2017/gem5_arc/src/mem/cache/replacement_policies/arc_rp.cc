#include "mem/cache/replacement_policies/arc_rp.hh"
#include "params/ARCRP.hh"
#include "mem/cache/cache_blk.hh"
#include "sim/cur_tick.hh"
#include <algorithm>
#include <cmath>
namespace gem5
{
namespace replacement_policy
{
ARC::ARC(const Params &p)
    : Base(p)
{
    // Ghost lists and targetP will be sized lazily when sets are first used
}
std::shared_ptr<ReplacementData>
ARC::instantiateEntry()
{
    // Create a new ARC-specific replacement metadata object
    return std::make_shared<ARCReplData>();
}
void
ARC::invalidate(const std::shared_ptr<ReplacementData>& rd)
{
    auto data = std::static_pointer_cast<ARCReplData>(rd);
    data->valid = false;
}
void
ARC::touch(const std::shared_ptr<ReplacementData>& rd) const
{
    auto data = std::static_pointer_cast<ARCReplData>(rd);
    data->lastTouchTick = curTick();
    if (data->listId == 1)
        data->listId = 2; // Promote from T1 to T2
}
void
ARC::reset(const std::shared_ptr<ReplacementData>& rd) const
{
    auto data = std::static_pointer_cast<ARCReplData>(rd);
    data->lastTouchTick = curTick();
    data->valid = true;
    ReplaceableEntry* entry = data->entry;
    if (!entry) {
        panic("ARC: ReplacementData not linked to ReplaceableEntry!");
    }
    int set = entry->getSet();
    // Ensure per-set structures are large enough
    if (set >= ghostB1.size()) {
        ghostB1.resize(set + 1);
        ghostB2.resize(set + 1);
        targetP.resize(set + 1, 0);
    }
    // Try promoting from ghost lists
    if (removeFromGhost(set, true, data->tag)) {
        adjustP(set, true); // Hit in B1  Increase targetP
        data->listId = 2;
    } else if (removeFromGhost(set, false, data->tag)) {
        adjustP(set, false); // Hit in B2  Decrease targetP
        data->listId = 2;
    } else {
        data->listId = 1; // New entry, added to T1
    }
}
void
ARC::adjustP(int set, bool hitInB1) const
{
    // Adjust adaptive parameter P based on which ghost list had a hit
    size_t b1Size = ghostB1[set].size();
    size_t b2Size = ghostB2[set].size();
    if (hitInB1) {
        int delta = (b1Size >= b2Size) ? 1 : (b2Size + b1Size - 1) / b1Size;
        targetP[set] = std::min(targetP[set] + delta, static_cast<int>(b1Size + b2Size));
    } else {
        int delta = (b2Size >= b1Size) ? 1 : (b1Size + b2Size - 1) / b2Size;
        targetP[set] = std::max(targetP[set] - delta, 0);
    }
}
void
ARC::addToGhost(int set, bool isB1, Addr tag) const
{
    // Add to the appropriate ghost list
    std::list<Addr>& ghostList = isB1 ? ghostB1[set] : ghostB2[set];
    ghostList.push_back(tag);
    // Optionally cap ghost size to associativity (not enforced strictly here)
    if (ghostList.size() > ghostB1.size()) // Should ideally use associativity
        ghostList.pop_front();
}
bool
ARC::removeFromGhost(int set, bool isB1, Addr tag) const
{
    // Search and remove tag from ghost list if found
    std::list<Addr>& ghostList = isB1 ? ghostB1[set] : ghostB2[set];
    auto it = std::find(ghostList.begin(), ghostList.end(), tag);
    if (it != ghostList.end()) {
        ghostList.erase(it);
        return true;
    }
    return false;
}
ReplaceableEntry*
ARC::getVictim(const ReplacementCandidates& candidates) const
{
    if (candidates.empty())
        return nullptr;
    const CacheBlk* first_blk = static_cast<const CacheBlk*>(candidates[0]);
    int set = first_blk->getSet();
    // Resize per-set data if needed
    if (set >= ghostB1.size()) {
        ghostB1.resize(set + 1);
        ghostB2.resize(set + 1);
        targetP.resize(set + 1, 0);
    }
    // Count blocks in T1 and T2
    int t1Count = 0, t2Count = 0;
    for (const auto& entry : candidates) {
        auto data = std::static_pointer_cast<ARCReplData>(entry->replacementData);
        if (data->valid) {
            if (data->listId == 1) t1Count++;
            else t2Count++;
        }
    }
    // Decide from which list to evict
    bool evictFromT1 = (t1Count > targetP[set]) || (t2Count == 0);
    ReplaceableEntry* victim = nullptr;
    Tick oldest = curTick();
    // Choose the least recently used (LRU) in the selected list
    for (const auto& entry : candidates) {
        auto data = std::static_pointer_cast<ARCReplData>(entry->replacementData);
        if (data->valid && ((evictFromT1 && data->listId == 1) || (!evictFromT1 && data->listId == 2))) {
            if (data->lastTouchTick < oldest) {
                oldest = data->lastTouchTick;
                victim = entry;
            }
        }
    }
    // Fallback: pick any valid block
    if (!victim) {
        for (const auto& entry : candidates) {
            auto data = std::static_pointer_cast<ARCReplData>(entry->replacementData);
            if (data->valid) {
                victim = entry;
                break;
            }
        }
    }
    // Add the victim's tag to the appropriate ghost list
    if (victim) {
        auto victimData = std::static_pointer_cast<ARCReplData>(victim->replacementData);
        addToGhost(set, victimData->listId == 1, victimData->tag);
    }
    return victim;
}
} // namespace replacement_policy
} // namespace gem5
