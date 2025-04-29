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
    // Initialize ghost lists and targetP dynamically
}

std::shared_ptr<ReplacementData>
ARC::instantiateEntry()
{
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
        data->listId = 2;
}

void
ARC::reset(const std::shared_ptr<ReplacementData>& rd) const
{
    auto data = std::static_pointer_cast<ARCReplData>(rd);
    data->lastTouchTick = curTick();
    data->valid = true;

    // Ensure entry is valid
    ReplaceableEntry* entry = data->entry;
    if (!entry) {
        panic("ARC: ReplacementData not linked to ReplaceableEntry!");
    }

    int set = entry->getSet();

    // Resize per-set data structures
    if (set >= ghostB1.size()) {
        ghostB1.resize(set + 1);
        ghostB2.resize(set + 1);
        targetP.resize(set + 1, 0);
    }

    // Check ghost lists for promotion
    if (removeFromGhost(set, true, data->tag)) {
        adjustP(set, true);
        data->listId = 2;
    } else if (removeFromGhost(set, false, data->tag)) {
        adjustP(set, false);
        data->listId = 2;
    } else {
        data->listId = 1;
    }
}

void
ARC::adjustP(int set, bool hitInB1) const
{
    size_t b1Size = ghostB1[set].size();
    size_t b2Size = ghostB2[set].size();

    if (hitInB1) {
        int delta = (b1Size >= b2Size) ? 1 : (b2Size + b1Size - 1) / b1Size; // Ceiling division
        targetP[set] = std::min(targetP[set] + delta, static_cast<int>(ghostB1[set].size() + ghostB2[set].size()));
    } else {
        int delta = (b2Size >= b1Size) ? 1 : (b1Size + b2Size - 1) / b2Size;
        targetP[set] = std::max(targetP[set] - delta, 0);
    }
}

void
ARC::addToGhost(int set, bool isB1, Addr tag) const
{
    std::list<Addr>& ghostList = isB1 ? ghostB1[set] : ghostB2[set];
    ghostList.push_back(tag);

    // Cap ghost list size to cache associativity (from candidates.size())
    if (ghostList.size() > ghostB1.size()) // Adjust based on your cache's associativity
        ghostList.pop_front();
}

bool
ARC::removeFromGhost(int set, bool isB1, Addr tag) const
{
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

    // Get set from the first candidate
    const CacheBlk* first_blk = static_cast<const CacheBlk*>(candidates[0]);
    int set = first_blk->getSet();

    // Resize if needed
    if (set >= ghostB1.size()) {
        ghostB1.resize(set + 1);
        ghostB2.resize(set + 1);
        targetP.resize(set + 1, 0);
    }

    // Calculate T1 and T2 counts
    int t1Count = 0, t2Count = 0;
    for (const auto& entry : candidates) {
        auto data = std::static_pointer_cast<ARCReplData>(entry->replacementData);
        if (data->valid) {
            if (data->listId == 1) t1Count++;
            else t2Count++;
        }
    }

    // Decide eviction from T1 or T2
    bool evictFromT1 = (t1Count > targetP[set]) || (t2Count == 0);
    ReplaceableEntry* victim = nullptr;
    Tick oldest = curTick();

    // Find LRU in the selected list
    for (const auto& entry : candidates) {
        auto data = std::static_pointer_cast<ARCReplData>(entry->replacementData);
        if (data->valid && ((evictFromT1 && data->listId == 1) || (!evictFromT1 && data->listId == 2))) {
            if (data->lastTouchTick < oldest) {
                oldest = data->lastTouchTick;
                victim = entry;
            }
        }
    }

    // Fallback to any valid entry
    if (!victim) {
        for (const auto& entry : candidates) {
            auto data = std::static_pointer_cast<ARCReplData>(entry->replacementData);
            if (data->valid) {
                victim = entry;
                break;
            }
        }
    }

    // Update ghost lists
    if (victim) {
        auto victimData = std::static_pointer_cast<ARCReplData>(victim->replacementData);
        addToGhost(set, victimData->listId == 1, victimData->tag);
    }

    return victim;
}

} // namespace replacement_policy
} // namespace gem5