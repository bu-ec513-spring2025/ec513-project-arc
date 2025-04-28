#include "mem/cache/replacement_policies/arc_rp.hh"
#include "params/ARCRP.hh"
#include "mem/cache/cache_blk.hh"
#include <algorithm>
#include <cassert>

namespace gem5 {
namespace replacement_policy {

/** Constructor just calls Base; we delay sizing until first use. */
ARC::ARC(const Params &p) : Base(p) {}

/** Fresh ReplacementData for each block. */
std::shared_ptr<ReplacementData>
ARC::instantiateEntry()
{
    return std::make_shared<ARCReplData>();
}

/**  
 * If we encounter set index “set” for first time, grow all our per-set arrays.  
 * Ensures T1/T2/B1/B2/p exist for that set.  
 */
void
ARC::ensureSetExists(int set) const
{
    if (set >= (int)targetP.size()) {
        int newSz = set + 1;
        targetP .resize(newSz,    0);
        ghostB1 .resize(newSz, {});
        ghostB2 .resize(newSz, {});
        lruT1   .resize(newSz, {});
        lruT2   .resize(newSz, {});
    }
}

/**  
 * invalidate(): called when a block is evicted or invalidated  
 *  – remove from T1/T2 if present, mark valid=false  
 */
void
ARC::invalidate(const std::shared_ptr<ReplacementData>& rd)
{
    auto data = std::static_pointer_cast<ARCReplData>(rd);
    if (!data->valid) return;

    ensureSetExists(data->set);

    // pick the right list (T1 or T2)
    auto &lst = (data->listId == 1 ? lruT1[data->set]
                                  : lruT2[data->set]);

    // erase by tag (safe even if our stored iterator got stale)
    auto it = std::find(lst.begin(), lst.end(), data->tag);
    if (it != lst.end())
        lst.erase(it);

    data->valid = false;
}

/**  
 * touch(): called on a cache hit  
 *  – for ARC we do **not** reorder on touch (instead promotion happens in reset())  
 */
void
ARC::touch(const std::shared_ptr<ReplacementData>& rd) const
{
    // Intentionally empty; Ruby invokes reset() on each access.
}

/**  
 * reset(): called on a new fill or on any hit  
 * Implements ARC’s “on access” logic:  
 *  – ghost hits in B1/B2 adjust p and move block into T2  
 *  – cold miss → insert into T1  
 *  – always enforce |T1|+|T2| ≤ associativity  
 *  – cap B1/B2 to associativity  
 */
void
ARC::reset(const std::shared_ptr<ReplacementData>& rd) const
{
    auto data = std::static_pointer_cast<ARCReplData>(rd);

    // read set & tag from the CacheBlk owner
    const CacheBlk *blk = static_cast<const CacheBlk*>(data->entry);
    data->set   = blk->getSet();
    data->tag   = blk->getTag();
    data->valid = true;

    ensureSetExists(data->set);

    // Ghost hit in B1?
    if (removeFromGhost(data->set, true, data->tag)) {
        adjustP(data->set, true);       // increase p
        data->listId = 2;               // bring into T2
        lruT2[data->set].push_back(data->tag);
        data->lruIter = std::prev(lruT2[data->set].end());

    // Ghost hit in B2?
    } else if (removeFromGhost(data->set, false, data->tag)) {
        adjustP(data->set, false);      // decrease p
        data->listId = 2;
        lruT2[data->set].push_back(data->tag);
        data->lruIter = std::prev(lruT2[data->set].end());

    // Cold miss → T1
    } else {
        data->listId = 1;
        lruT1[data->set].push_back(data->tag);
        data->lruIter = std::prev(lruT1[data->set].end());
    }

    // Enforce |T1| + |T2| <= k (associativity)
    if (lruT1[data->set].size() + lruT2[data->set].size()
        > (unsigned)params().assoc) {
        // evict whichever the ARC rules say
        bool evictT1 = (lruT1[data->set].size() > (unsigned)targetP[data->set])
                       || lruT2[data->set].empty();
        auto &victimList = evictT1 ? lruT1[data->set]
                                   : lruT2[data->set];
        Addr victimTag  = evictT1 ? victimList.front()
                                  : victimList.front();
        // remove from T1 or T2
        victimList.pop_front();
        // add to B1 or B2
        addToGhost(data->set, evictT1, victimTag, params().assoc);
    }

    // Cap size of B1/B2
    if (ghostB1[data->set].size() > (unsigned)params().assoc)
        ghostB1[data->set].pop_front();
    if (ghostB2[data->set].size() > (unsigned)params().assoc)
        ghostB2[data->set].pop_front();
}

/**  
 * getVictim(): choose which block to evict  
 *   – same rule: evict LRU in T1 if |T1|>p or T2 empty, else LRU in T2  
 */
ReplaceableEntry*
ARC::getVictim(const ReplacementCandidates& cands) const
{
    // all candidates share same set
    auto data0 = std::static_pointer_cast<ARCReplData>(
                      cands[0]->replacementData);
    int set = data0->set;

    ensureSetExists(set);

    bool evictT1 = (lruT1[set].size() > (unsigned)targetP[set])
                   || lruT2[set].empty();
    auto &victimList = evictT1 ? lruT1[set]
                               : lruT2[set];

    if (victimList.empty())
        return nullptr;

    Addr victimTag = victimList.front();
    ReplaceableEntry *victim = nullptr;
    for (auto *e : cands) {
        auto *d = static_cast<ARCReplData*>(e->replacementData.get());
        if (d->valid && d->tag == victimTag) {
            victim = e;
            break;
        }
    }
    assert(victim);

    // remove from T1/T2
    victimList.pop_front();
    // record in ghost
    addToGhost(set, evictT1, victimTag, params().assoc);
    return victim;
}

/** adjustP(): increase or decrease targetP using ghost sizes */
void
ARC::adjustP(int set, bool hitInB1) const
{
    size_t b1 = ghostB1[set].size();
    size_t b2 = ghostB2[set].size();
    int delta = 1;

    if (hitInB1 && b1 < b2 && b1 > 0)
        delta = (b2 + b1 - 1) / b1;

    if (!hitInB1 && b2 < b1 && b2 > 0)
        delta = (b1 + b2 - 1) / b2;

    targetP[set] += hitInB1 ?  delta
                            : -delta;
    if (targetP[set] < 0)            targetP[set] = 0;
    if ((unsigned)targetP[set] > params().assoc)
        targetP[set] = params().assoc;
}

/** addToGhost(): append to B1 or B2, capping at assoc */
void
ARC::addToGhost(int set, bool isB1, Addr tag,
                int assoc) const
{
    auto &G = isB1 ? ghostB1[set]
                   : ghostB2[set];
    G.push_back(tag);
    if (G.size() > (unsigned)assoc)
        G.pop_front();
}

/** removeFromGhost() finds & erases tag from B1/B2 */
bool
ARC::removeFromGhost(int set, bool isB1, Addr tag) const
{
    auto &G = isB1 ? ghostB1[set]
                   : ghostB2[set];
    auto it = std::find(G.begin(), G.end(), tag);
    if (it == G.end()) return false;
    G.erase(it);
    return true;
}

} // namespace replacement_policy
} // namespace gem5
