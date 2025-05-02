// #include "mem/cache/replacement_policies/frc_rp.hh"
// #include "params/FRCRP.hh"
// #include "mem/cache/cache_blk.hh"
// #include <algorithm>

// namespace gem5 {
// namespace replacement_policy {

// FRC::FRC(const Params &p)
//   : Base(p), pFraction(p.p_fraction) {}

// std::shared_ptr<ReplacementData> FRC::instantiateEntry() {
//     return std::make_shared<FRCReplData>();
// }

// void FRC::ensureSetExists(int set) const {
//     std::lock_guard<std::mutex> lock(mtx);
//     if (set < 0) panic("Invalid set index: %d", set);
//     if (set >= (int)lruT1.size()) {
//         lruT1.resize(set + 1);
//         lruT2.resize(set + 1);
//     }
// }

// void FRC::invalidate(const std::shared_ptr<ReplacementData>& rd) {
//     std::lock_guard<std::mutex> lock(mtx);
//     auto data = std::static_pointer_cast<FRCReplData>(rd);
//     if (!data->valid) return;

//     ensureSetExists(data->set);
//     std::list<Addr>& list = data->inT2 ? lruT2[data->set] : lruT1[data->set];
//     list.remove(data->tag);
//     data->valid = false;
// }

// void FRC::touch(const std::shared_ptr<ReplacementData>& rd) const {
//     std::lock_guard<std::mutex> lock(mtx);
//     auto data = std::static_pointer_cast<FRCReplData>(rd);
//     if (!data->valid) return;

//     ensureSetExists(data->set);
//     if (!data->inT2) {
//         // Move from T1 to T2
//         lruT1[data->set].remove(data->tag);
//         lruT2[data->set].push_back(data->tag);
//         data->inT2 = true;
//     } else {
//         // Reorder in T2
//         lruT2[data->set].remove(data->tag);
//         lruT2[data->set].push_back(data->tag);
//     }
// }

// void FRC::reset(const std::shared_ptr<ReplacementData>& rd) const {
//     std::lock_guard<std::mutex> lock(mtx);
//     auto data = std::static_pointer_cast<FRCReplData>(rd);
//     const CacheBlk* blk = static_cast<const CacheBlk*>(data->entry);

//     data->set = blk->getSet();
//     data->tag = blk->getTag();
//     data->valid = true;
//     data->inT2 = false; // Start in T1

//     ensureSetExists(data->set);
//     lruT1[data->set].push_back(data->tag);
// }

// ReplaceableEntry* FRC::getVictim(const ReplacementCandidates& cands) const {
//     std::lock_guard<std::mutex> lock(mtx);
//     if (cands.empty()) return nullptr;

//     auto data0 = std::static_pointer_cast<FRCReplData>(cands[0]->replacementData);
//     int set = data0->set;
//     ensureSetExists(set);

//     // Calculate target size for T1
//     int assoc = cands.size();
//     int p = static_cast<int>(pFraction * assoc);

//     std::list<Addr>& t1 = lruT1[set];
//     std::list<Addr>& t2 = lruT2[set];

//     // Evict from T1 if it exceeds target size, else from T2
//     bool evictFromT1 = (t1.size() > (size_t)p) || t2.empty();
//     std::list<Addr>& victimList = evictFromT1 ? t1 : t2;

//     if (victimList.empty()) return nullptr;

//     Addr victimTag = victimList.front();
//     ReplaceableEntry* victim = nullptr;
//     for (auto* entry : cands) {
//         auto* d = static_cast<FRCReplData*>(entry->replacementData.get());
//         if (d->valid && d->tag == victimTag) {
//             victim = entry;
//             break;
//         }
//     }

//     if (victim) victimList.pop_front();
//     return victim;
// }

// } // namespace replacement_policy
// } // namespace gem5
