#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_ARC_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_ARC_RP_HH__

#include "mem/cache/replacement_policies/base.hh"
#include "sim/cur_tick.hh"
#include <list>
#include <vector>

namespace gem5
{

struct ARCRPParams;

namespace replacement_policy
{

class ARC : public Base
{
  protected:
    struct ARCReplData : public ReplacementData {
        Tick lastTouchTick;
        int listId;  // 1 = T1 (new), 2 = T2 (frequent)
        bool valid;
        Addr tag;

        ARCReplData() : lastTouchTick(0), listId(1), valid(false), tag(0) {}
    };

  public:
    using Params = ARCRPParams;
    ARC(const Params &p);
    ~ARC() override = default;

    void invalidate(const std::shared_ptr<ReplacementData>& rd) override;
    void touch(const std::shared_ptr<ReplacementData>& rd) const override;
    void reset(const std::shared_ptr<ReplacementData>& rd) const override;
    ReplaceableEntry* getVictim(const ReplacementCandidates& candidates) const override;
    std::shared_ptr<ReplacementData> instantiateEntry() override;

  private:
    mutable std::vector<int> targetP;                 // Per-set targetP
    mutable std::vector<std::list<Addr>> ghostB1;     // Per-set B1 (LRU)
    mutable std::vector<std::list<Addr>> ghostB2;     // Per-set B2 (LRU)

    void adjustP(int set, bool hitInB1) const;
    void addToGhost(int set, bool isB1, Addr tag) const;
    bool removeFromGhost(int set, bool isB1, Addr tag) const;
};

} // namespace replacement_policy
} // namespace gem5

#endif // __MEM_CACHE_REPLACEMENT_POLICIES_ARC_RP_HH__