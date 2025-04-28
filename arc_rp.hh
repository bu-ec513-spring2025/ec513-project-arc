#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_ARC_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_ARC_RP_HH__

#include "mem/cache/replacement_policies/base.hh"
#include <list>
#include <vector>

namespace gem5 {
namespace replacement_policy {

/** The ARC replacement policy applied per cache set. */
class ARC : public Base
{
  protected:
    /**  
     * Per-block metadata we keep in ReplacementData.
     * Mirrors the “entry” concept in the paper.
     */
    struct ARCReplData : public ReplacementData {
        int     listId;   // 1=T1, 2=T2
        bool    valid;    // in cache?
        Addr    tag;      // block’s tag
        int     set;      // set index
        mutable
        std::list<Addr>::iterator lruIter;  // position inside T1/T2

        ARCReplData()
          : listId(1), valid(false), tag(0), set(-1) {}
    };

  public:
    using Params = Base::Params;
    ARC(const Params &p);
    ~ARC() override = default;

    // Standard ReplacementPolicy interface:
    void invalidate(const std::shared_ptr<ReplacementData>& rd) override;
    void touch     (const std::shared_ptr<ReplacementData>& rd) const override;
    void reset     (const std::shared_ptr<ReplacementData>& rd) const override;
    ReplaceableEntry*
         getVictim(const ReplacementCandidates& cands) const override;
    std::shared_ptr<ReplacementData> instantiateEntry() override;

  private:
    /**  
     * Per-set ARC state:  
     *   targetP[set]  — the adaptive “p” parameter for set  
     *   ghostB1/B2    — lists of evicted tags (B1,B2)  
     *   lruT1/T2      — the real cache lists (T1,T2)  
     */
    mutable std::vector<int>                  targetP;
    mutable std::vector<std::list<Addr>>      ghostB1, ghostB2;
    mutable std::vector<std::list<Addr>>      lruT1,   lruT2;

    /** Grow all vectors if we see a set index for the first time */
    void ensureSetExists(int set) const;

    /** Adjust the “p” parameter:  
        if hit in B1, p += ⌈|B2|/|B1|⌉  
        if hit in B2, p -= ⌈|B1|/|B2|⌉ */  
    void adjustP(int set, bool hitInB1) const;

    /** When evicting from T1/T2, append tag to B1/B2 and cap size to associativity */
    void addToGhost(int set, bool isB1, Addr tag, int assoc) const;

    /** Remove from B1 or B2 if present (for ghost hits) */
    bool removeFromGhost(int set, bool isB1, Addr tag) const;
};

} // namespace replacement_policy
} // namespace gem5

#endif // __MEM_CACHE_REPLACEMENT_POLICIES_ARC_RP_HH__
