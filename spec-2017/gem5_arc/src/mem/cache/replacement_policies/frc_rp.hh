// #ifndef __MEM_CACHE_REPLACEMENT_POLICIES_FRC_RP_HH__
// #define __MEM_CACHE_REPLACEMENT_POLICIES_FRC_RP_HH__

// #include "mem/cache/replacement_policies/base.hh"
// #include <list>
// #include <vector>

// namespace gem5 {
// namespace replacement_policy {

// class FRC : public Base {
//   protected:
//     struct FRCReplData : public ReplacementData {
//         bool inT2;      // True if in T2 (frequent), False if in T1 (recent)
//         bool valid;
//         Addr tag;
//         int set;
//         FRCReplData() : inT2(false), valid(false), tag(0), set(-1) {}
//     };

//   public:
//     using Params = Base::Params;
//     FRC(const Params &p);
//     ~FRC() override = default;

//     void invalidate(const std::shared_ptr<ReplacementData>& rd) override;
//     void touch(const std::shared_ptr<ReplacementData>& rd) const override;
//     void reset(const std::shared_ptr<ReplacementData>& rd) const override;
//     ReplaceableEntry* getVictim(const ReplacementCandidates& candidates) const override;
//     std::shared_ptr<ReplacementData> instantiateEntry() override;

//   private:
//     mutable std::vector<std::list<Addr>> lruT1, lruT2; // T1 (recent), T2 (frequent)
//     mutable std::mutex mtx; // Thread safety
//     const double pFraction; // Fixed fraction of cache for T1 (e.g., 0.5)

//     void ensureSetExists(int set) const;
// };

// } // namespace replacement_policy
// } // namespace gem5

// #endif // __MEM_CACHE_REPLACEMENT_POLICIES_FRC_RP_HH__
