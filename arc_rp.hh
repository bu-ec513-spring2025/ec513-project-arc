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
  protected:
    // Replacement data for ARC
    struct ARCReplData : ReplacementData
    {
      ARCReplData(ReplaceableEntry* entry)
        : entry(entry), lastTouchTick(0), inT1(false), ghost(false), addr(0), set(0) {}

      ReplaceableEntry* entry;
      Tick lastTouchTick;
      bool inT1;
      bool ghost;
      Addr addr;
      uint64_t set;
    };

  public:
    typedef ARCRPParams Params;
    ARC(const Params &p);
    ~ARC() = default;

    void invalidate(const std::shared_ptr<ReplacementData>& replacement_data) override;
    void touch(const std::shared_ptr<ReplacementData>& replacement_data) const override;
    void reset(const std::shared_ptr<ReplacementData>& replacement_data) const override;
    ReplaceableEntry* getVictim(const ReplacementCandidates& candidates) const override;
    std::shared_ptr<ReplacementData> instantiateEntry() override;

  private:
    struct SetMetadata
    {
        SetMetadata() : p(0) {}

        std::list<ReplaceableEntry*> T1;
        std::list<ReplaceableEntry*> T2;
        std::unordered_map<Addr, ReplaceableEntry*> B1;
        std::unordered_map<Addr, ReplaceableEntry*> B2;
        int p;
    };

    mutable std::unordered_map<uint64_t, SetMetadata> setMetadataMap;
    mutable uint64_t currentSet;

    void adjustP(uint64_t set, bool hitInB1) const;
    ReplaceableEntry* getLRU(std::list<ReplaceableEntry*>& list) const;
};

} // namespace replacement_policy
} // namespace gem5

#endif // __MEM_CACHE_REPLACEMENT_POLICIES_ARC_RP_HH__