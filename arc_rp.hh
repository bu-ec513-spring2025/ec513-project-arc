// arc_rp.hh
#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_ARC_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_ARC_RP_HH__

#include "mem/cache/replacement_policies/base.hh"
#include <list>
#include <unordered_map>

namespace gem5
{
struct ARCRPParams;

namespace replacement_policy
{

class ARC : public Base
{
  private:
    struct SetMetadata {
        std::list<ReplaceableEntry*> T1; // MRU at front, LRU at back
        std::list<ReplaceableEntry*> T2;
        std::list<Addr> B1;  // Ghost list for T1
        std::list<Addr> B2;  // Ghost list for T2
        int p = 0;           // Target size for T1
    };

    mutable std::unordered_map<uint64_t, SetMetadata> setMetadata;

  protected:
    struct ARCReplData : public ReplacementData {
        ReplaceableEntry* entry;
        Tick lastTouchTick;
        bool inT1;
        Addr addr;
        uint64_t set;

        ARCReplData(ReplaceableEntry* e)
          : ReplacementData(), entry(e),
            lastTouchTick(0), inT1(false),
            addr(0), set(0) {}
    };

  public:
    using Params = ARCRPParams;
    ARC(const Params &p);
    ~ARC() override = default;

    void invalidate(const std::shared_ptr<ReplacementData>& rd) override;
    void touch(const std::shared_ptr<ReplacementData>& rd) const override;
    void reset(const std::shared_ptr<ReplacementData>& rd) const override;
    ReplaceableEntry* getVictim(const ReplacementCandidates& candidates) const override;

    // NOTE: signature takes the real entry pointer
    std::shared_ptr<ReplacementData>
    instantiateEntry(ReplaceableEntry* e) override;

  private:
    void adjustP(uint64_t set, bool hitInB1) const;
    ReplaceableEntry* getLRU(const std::list<ReplaceableEntry*>& list) const;
};

} // namespace replacement_policy
} // namespace gem5

#endif // __MEM_CACHE_REPLACEMENT_POLICIES_ARC_RP_HH__
