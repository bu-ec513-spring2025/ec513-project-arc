// arc_rp.cc
#include "mem/cache/replacement_policies/arc_rp.hh"
#include "params/ARCRP.hh"
#include "sim/cur_tick.hh"
#include "mem/cache/cache_blk.hh"

namespace gem5
{
namespace replacement_policy
{

ARC::ARC(const Params &p) : Base(p) {}

void
ARC::adjustP(uint64_t set, bool hitInB1) const
{
    auto &meta = setMetadata[set];
    size_t b1 = meta.B1.size();
    size_t b2 = meta.B2.size();
    int total = meta.T1.size() + meta.T2.size();

    int delta = 1;
    if (hitInB1) {
        // if both nonzero and B2 < B1, use ratio
        if (b1 > 0 && b2 < b1)
            delta = b2 / b1;
        meta.p = std::min(meta.p + delta, total);
    } else {
        if (b2 > 0 && b1 < b2)
            delta = b1 / b2;
        meta.p = std::max(meta.p - delta, 0);
    }
}

ReplaceableEntry*
ARC::getLRU(const std::list<ReplaceableEntry*>& list) const
{
    return list.empty() ? nullptr : list.back();
}

void
ARC::invalidate(const std::shared_ptr<ReplacementData>& rd)
{
    auto* data = static_cast<ARCReplData*>(rd.get());
    auto &meta = setMetadata[data->set];

    if (data->inT1) {
        meta.T1.remove(data->entry);
        meta.B1.push_front(data->addr);
    } else {
        meta.T2.remove(data->entry);
        meta.B2.push_front(data->addr);
    }
}

void
ARC::touch(const std::shared_ptr<ReplacementData>& rd) const
{
    auto* data = static_cast<ARCReplData*>(rd.get());
    auto &meta = setMetadata[data->set];

    if (data->inT1) {
        // move from T1 → front of T2
        meta.T1.remove(data->entry);
        meta.T2.push_front(data->entry);
        data->inT1 = false;
    } else {
        // refresh position in T2 on a T2 hit
        meta.T2.remove(data->entry);
        meta.T2.push_front(data->entry);
    }
    data->lastTouchTick = curTick();
}

void
ARC::reset(const std::shared_ptr<ReplacementData>& rd) const
{
    auto* data = static_cast<ARCReplData*>(rd.get());
    CacheBlk* blk = static_cast<CacheBlk*>(data->entry);
    data->set  = blk->getSet();
    data->addr = blk->getTag();

    auto &meta = setMetadata[data->set];

    // ghost-list hits?
    auto b1_it = std::find(meta.B1.begin(), meta.B1.end(), data->addr);
    auto b2_it = std::find(meta.B2.begin(), meta.B2.end(), data->addr);

    if (b1_it != meta.B1.end()) {
        adjustP(data->set, true);
        meta.B1.erase(b1_it);
        meta.T2.push_front(data->entry);
        data->inT1 = false;
    } else if (b2_it != meta.B2.end()) {
        adjustP(data->set, false);
        meta.B2.erase(b2_it);
        meta.T2.push_front(data->entry);
        data->inT1 = false;
    } else {
        // cold miss
        meta.T1.push_front(data->entry);
        data->inT1 = true;
    }

    data->lastTouchTick = curTick();
}

ReplaceableEntry*
ARC::getVictim(const ReplacementCandidates& candidates) const
{
    const uint64_t set = candidates[0]->getSet();
    auto &meta = setMetadata[set];

    ReplaceableEntry* victim = nullptr;
    if (meta.T1.size() > static_cast<size_t>(meta.p) || meta.T2.empty()) {
        victim = getLRU(meta.T1);
    } else {
        victim = getLRU(meta.T2);
    }

    if (victim) {
        auto vdata = static_cast<ARCReplData*>(victim->replacementData.get());
        // move to appropriate ghost list
        if (vdata->inT1) {
            meta.T1.remove(victim);
            meta.B1.push_front(vdata->addr);
        } else {
            meta.T2.remove(victim);
            meta.B2.push_front(vdata->addr);
        }
    }

    return victim;
}

std::shared_ptr<ReplacementData>
ARC::instantiateEntry(ReplaceableEntry* e)
{
    // pass the real entry ptr into our ReplData
    return std::make_shared<ARCReplData>(e);
}

} // namespace replacement_policy
} // namespace gem5
