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
/**
* @class ARC
* @brief Implements the Adaptive Replacement Cache (ARC) replacement policy.
*
* ARC balances between recency (T1) and frequency (T2) by maintaining two
* history lists (B1, B2) and adjusting the target size `p` dynamically.
*/
class ARC : public Base
{
  protected:
    // Stores metadata for each cache block
    struct ARCReplData : public ReplacementData {
        Tick lastTouchTick; // Last time the block was accessed
        int listId; // 1 = T1 (recent), 2 = T2 (frequent)
        bool valid; // Indicates if the block is currently valid
        Addr tag; // Tag of the block
        ARCReplData() : lastTouchTick(0), listId(1), valid(false), tag(0) {}
    };
  public:
    using Params = ARCRPParams;
    ARC(const Params &p);
    ~ARC() override = default;
    // Invalidate a block's metadata
    void invalidate(const std::shared_ptr<ReplacementData>& rd) override;
    // Called on access (read/write) to update recency/frequency
    void touch(const std::shared_ptr<ReplacementData>& rd) const override;
    // Called on block insertion or promotion
    void reset(const std::shared_ptr<ReplacementData>& rd) const override;
    // Select a block to evict
    ReplaceableEntry* getVictim(const ReplacementCandidates& candidates) const override;
    // Allocate new replacement metadata
    std::shared_ptr<ReplacementData> instantiateEntry() override;
  private:
    // Per-set adaptive target size for T1
    mutable std::vector<int> targetP;
    // Per-set ghost lists (recently evicted from T1 and T2)
    mutable std::vector<std::list<Addr>> ghostB1;
    mutable std::vector<std::list<Addr>> ghostB2;
    // Adjust targetP based on which ghost list had a hit
    void adjustP(int set, bool hitInB1) const;
    // Add a tag to a ghost list
    void addToGhost(int set, bool isB1, Addr tag) const;
    // Remove a tag from a ghost list if present
    bool removeFromGhost(int set, bool isB1, Addr tag) const;
};
} // namespace replacement_policy
} // namespace gem5
#endif // __MEM_CACHE_REPLACEMENT_POLICIES_ARC_RP_HH__
