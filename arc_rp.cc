#include "mem/cache/replacement_policies/arc_rp.hh"
#include "base/random.hh"
#include "params/ARCRP.hh"
#include "sim/cur_tick.hh"
#include "mem/cache/cache_blk.hh"

namespace gem5
{
namespace replacement_policy
{

ARC::ARC(const Params &p) : Base(p) {}

void ARC::adjustP(uint64_t set, bool hitInB1) const
{
    SetMetadata& metadata = setMetadataMap[set];
    if (hitInB1) {
        metadata.p = std::min(metadata.p + 1,
                              static_cast<int>(metadata.T1.size() + metadata.T2.size()));
    } else {
        metadata.p = std::max(metadata.p - 1, 0);
    }
}

ReplaceableEntry* ARC::getLRU(std::list<ReplaceableEntry*>& list) const
{
    ReplaceableEntry* lru = nullptr;
    Tick oldest = Tick(0);
    for (auto entry : list) {
        Tick entryTick = std::static_pointer_cast<ARCReplData>(entry->replacementData)->lastTouchTick;
        if (!lru || entryTick < oldest) {
            lru = entry;
            oldest = entryTick;
        }
    }
    return lru;
}

void ARC::invalidate(const std::shared_ptr<ReplacementData>& replacement_data)
{
    ARCReplData* data = std::static_pointer_cast<ARCReplData>(replacement_data).get();
    SetMetadata& metadata = setMetadataMap[data->set];
    if (data->ghost) {
        if (data->inT1)
            metadata.B1.erase(data->addr);
        else
            metadata.B2.erase(data->addr);
    }
    data->lastTouchTick = Tick(0);
    data->ghost = false;
}

void ARC::touch(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    auto data = std::static_pointer_cast<ARCReplData>(replacement_data);
    if (data->ghost) return;

    SetMetadata& metadata = setMetadataMap[data->set];
    if (data->inT1) {
        metadata.T1.remove(data->entry);
        metadata.T2.push_front(data->entry);
        data->inT1 = false;
    }
    data->lastTouchTick = curTick();
}

void ARC::reset(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    auto data = std::static_pointer_cast<ARCReplData>(replacement_data);
    data->set = currentSet;

    // Cast ReplaceableEntry to CacheBlk to access getTag()
    CacheBlk* blk = static_cast<CacheBlk*>(data->entry);
    data->addr = blk->getTag();  // Use blk->getTag()

    SetMetadata& metadata = setMetadataMap[data->set];
    if (metadata.B1.erase(data->addr)) {
        adjustP(data->set, true);
        data->inT1 = false;
        metadata.T2.push_front(data->entry);
    } else if (metadata.B2.erase(data->addr)) {
        adjustP(data->set, false);
        data->inT1 = false;
        metadata.T2.push_front(data->entry);
    } else {
        data->inT1 = true;
        metadata.T1.push_front(data->entry);
    }
    data->lastTouchTick = curTick();
    data->ghost = false;
}
  
ReplaceableEntry* ARC::getVictim(const ReplacementCandidates& candidates) const
{
    assert(!candidates.empty());
    currentSet = candidates[0]->getSet();

    SetMetadata& metadata = setMetadataMap[currentSet];
    ReplaceableEntry* victim = (metadata.T1.size() > metadata.p || metadata.T2.empty())
        ? getLRU(metadata.T1) : getLRU(metadata.T2);

    if (victim) {
        auto data = std::static_pointer_cast<ARCReplData>(victim->replacementData);
        (data->inT1 ? metadata.B1 : metadata.B2)[data->addr] = victim;
        (data->inT1 ? metadata.T1 : metadata.T2).remove(victim);
        data->ghost = true;
    }

    return victim;
}

std::shared_ptr<ReplacementData> ARC::instantiateEntry()
{
    return std::shared_ptr<ReplacementData>(new ARCReplData(nullptr)); 
}

} // namespace replacement_policy
} // namespace gem5