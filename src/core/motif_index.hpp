#pragma once
#include "core/row_selection.hpp"
#include <unordered_map>
namespace kleiotide {
// Immutable after finish(). Arithmetic hit runs are varint encoded; checkpoints
// provide ordinal lookup and prefix-max-end queries without expanding every hit.
class MotifIndex {
public:
    explicit MotifIndex(std::size_t limit=512*1024*1024):limit_(limit){}
    void add(MotifHit); // current row order, then (start,end) order; no duplicates
    void finish();
    MotifHit at(std::uint64_t ordinal)const;
    // Per visible column: shaded=1, hit starts=2, hit ends=4.
    std::vector<unsigned char> coverage(SequenceId,std::size_t begin,std::size_t end)const;
    std::uint64_t total=0,forward=0,reverse=0,both=0;
    std::size_t encoded_bytes()const{return bytes_;}
private:
    struct Run {std::uint64_t start=0,end=0,step=0,count=0;std::int64_t endStep=0;unsigned flags=0;};
    struct Block {std::size_t offset=0;std::uint64_t ordinal=0,firstStart=0,maxEnd=0;};
    struct Row {SequenceId id=0;std::uint64_t first=0,count=0,maxEnd=0;std::size_t runs=0;
        std::vector<unsigned char> data;std::vector<Block> blocks;Run pending;};
    void flush(Row&);
    void encode(Row&,std::uint64_t);
    static Run decode(const Row&,std::size_t&);
    static std::uint64_t end_at(const Run&,std::uint64_t);
    std::vector<Row> rows_;
    std::unordered_map<SequenceId,std::size_t> lookup_;
    std::size_t limit_,bytes_=0;
    bool finished_=false;
};
}
