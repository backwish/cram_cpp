#ifndef DARRAY_DATA_H
#define DARRAY_DATA_H 

#include <vector>
#include <chrono>
#include "block.h"
#include "encoder.h"
using namespace std::chrono;

template<typename T,typename Ch,int MAX_BLOCK_SIZE = 1024,int MAX_SB_SIZE = 1024>
class Darray_Data{
public:
    using index_t = int;
    using block_t = HuffmanBlock<T,Ch>;
    using encoder_t = HuffmanBlock<T,Ch>;
    Darray_Data() : time_cnt(0),data(1,std::vector<block_t>(1,block_t(1))){}
    Darray_Data(const auto& vec,const auto& encoder) : time_cnt(0){
        int start_index = 0;        
        while(start_index+MAX_BLOCK_SIZE/2 < vec.size()){
            int block_num = 0;
            std::vector<block_t> sb;
            while(start_index+MAX_BLOCK_SIZE/2 < vec.size() && block_num < MAX_SB_SIZE/2){
                std::vector<Ch> part(MAX_BLOCK_SIZE/2);
                std::copy(vec.begin()+start_index,vec.begin()+(start_index+MAX_BLOCK_SIZE/2),part.begin());
                block_t hb(part,encoder);
                sb.push_back(std::move(hb));
                start_index+=MAX_BLOCK_SIZE/2;
                ++block_num;
            }
            data.push_back(std::move(sb));
        }
        int remain = vec.size() - start_index;
        std::vector<Ch> part(remain);
        std::copy(vec.begin()+start_index,vec.end(),part.begin());
        block_t hb(part,encoder);
        if(data.back().size() < MAX_SB_SIZE/2){
            data.back().push_back(std::move(hb));
        }else{
            std::vector<block_t> sb;
            sb.push_back(std::move(hb));
            data.push_back(std::move(sb));
        }
    }
    Darray_Data(const Darray_Data&) = default;
    Darray_Data& operator=(const Darray_Data&) = default;
    Darray_Data(Darray_Data&&) noexcept = default;
    Darray_Data& operator=(Darray_Data&&) noexcept = default;
    size_t use_bytes() const{
        size_t ret = 0;
        for(const auto& each:data){
            for(const auto& block:each){
                ret+=block.use_bytes();
            }
        }
        return ret;
    }
    auto block_at(size_t rb_pos,size_t sb_pos,const auto& encoder) const {
        return data[rb_pos][sb_pos].get(encoder);
    }
    void block_replace(size_t rb_pos,size_t sb_pos,const auto& vec,const auto& encoder){
        data[rb_pos][sb_pos].replace_block(vec,encoder);        
    }
    auto at(size_t rb_pos,size_t sb_pos,size_t b_pos,const auto& encoder) const {
        return data[rb_pos][sb_pos].at(b_pos,encoder);
    }

    void splitBlock(size_t rb_pos,size_t sb_pos,const auto& encoder){
        //int left_size = MAX_BLOCK_SIZE/2+1,right_size = MAX_BLOCK_SIZE/2;
        int left_size = MAX_BLOCK_SIZE/2+1,right_size = data[rb_pos][sb_pos].size()-left_size;
        auto [left_block,right_block] = data[rb_pos][sb_pos].split(left_size,encoder);
        data[rb_pos].erase(data[rb_pos].begin()+sb_pos);
        data[rb_pos].insert(data[rb_pos].begin()+sb_pos,right_block);
        data[rb_pos].insert(data[rb_pos].begin()+sb_pos,left_block);
    }
    void splitSuperBlock(size_t rb_pos){
        //int left_block_size = MAX_SB_SIZE/2+1,right_block_size = MAX_SB_SIZE/2;
        int left_block_size = MAX_SB_SIZE/2+1,right_block_size = data[rb_pos].size()-left_block_size;
        std::vector<block_t> left_superblock,right_superblock;
        auto curr_superblock = std::move(data[rb_pos]);
        data.erase(data.begin()+rb_pos);
        left_superblock.insert(left_superblock.end(),
        std::make_move_iterator(curr_superblock.begin()),
        std::make_move_iterator(curr_superblock.begin()+left_block_size));
        right_superblock.insert(right_superblock.end(),
        std::make_move_iterator(curr_superblock.begin()+left_block_size),
        std::make_move_iterator(curr_superblock.end()));
        data.insert(data.begin()+rb_pos,right_superblock);
        data.insert(data.begin()+rb_pos,left_superblock);
    }
    auto getBlockSizeVec(const auto& vec) const {
        std::vector<index_t> block_size_vec;
        block_size_vec.reserve(vec.size());
        std::transform(vec.cbegin(),vec.cend(),std::back_inserter(block_size_vec),
        [](const auto& each){return each.size();});
        return block_size_vec;
    }

    auto insert(size_t rb_pos,size_t sb_pos,size_t block_pos,Ch val,const auto& encoder){
        steady_clock::time_point start = steady_clock::now();
        data[rb_pos][sb_pos].insert(block_pos,val,encoder);
        steady_clock::time_point end = steady_clock::now(); 
        time_cnt+=duration_cast<nanoseconds> (end - start).count();

        std::vector<index_t> block_size_vec_curr;
        std::vector<index_t> block_size_vec_next;        
        bool split_block = false;
        bool split_superblock = false;
        if(MAX_BLOCK_SIZE < data[rb_pos][sb_pos].size()){
            split_block = true;
            splitBlock(rb_pos,sb_pos,encoder);            
        }
        if(MAX_SB_SIZE < data[rb_pos].size()){
            split_superblock = true;
            splitSuperBlock(rb_pos);
            block_size_vec_curr = getBlockSizeVec(data[rb_pos]);
            block_size_vec_next = getBlockSizeVec(data[rb_pos+1]);            
        }
        return std::make_tuple(split_block,split_superblock,std::move(block_size_vec_curr),std::move(block_size_vec_next));          
    }

    void mergeBlock(size_t rb_pos,size_t left_pos,size_t right_pos,const auto& encoder){
        //int left_size = MAX_BLOCK_SIZE/2+1,right_size = MAX_BLOCK_SIZE/2;
        data[rb_pos][left_pos].merge(data[rb_pos][right_pos],encoder);
        data[rb_pos].erase(data[rb_pos].begin()+right_pos);
    }
    void mergeSuperBlock(size_t left_pos,size_t right_pos){
        data[left_pos].insert(data[left_pos].end(),
        std::make_move_iterator(data[right_pos].begin()),
        std::make_move_iterator(data[right_pos].end()));
        data.erase(data.begin()+right_pos);
    }

    auto erase(size_t rb_pos,size_t sb_pos,size_t block_pos,const auto& encoder){
        data[rb_pos][sb_pos].erase(block_pos,encoder);
        bool block_merged = false;
        bool sb_merged = false;
        bool block_split = false;
        bool sb_split = false;
        std::vector<index_t> block_size_vec_curr;
        std::vector<index_t> block_size_vec_next;        
        if(data[rb_pos][sb_pos].size() <= MAX_BLOCK_SIZE/4 && 1 < data[rb_pos].size()){          
            block_merged = true;  
            auto left_pos = (sb_pos==0 ? sb_pos : sb_pos-1);
            auto right_pos = (sb_pos==0 ? sb_pos+1 : sb_pos);
            mergeBlock(rb_pos,left_pos,right_pos,encoder);
            if(MAX_BLOCK_SIZE < data[rb_pos][left_pos].size()){
                block_split = true;
                splitBlock(rb_pos,left_pos,encoder);
            }
        }
        if(data[rb_pos].size() <= MAX_SB_SIZE/4 && 1 < data.size() && !block_split){
            sb_merged = true;
            auto left_pos = (rb_pos==0 ? rb_pos : rb_pos-1);
            auto right_pos = (rb_pos==0 ? rb_pos+1 : rb_pos);
            mergeSuperBlock(left_pos,right_pos);
            if(MAX_SB_SIZE < data[left_pos].size()){
                sb_split = true;
                splitSuperBlock(left_pos);
                block_size_vec_curr = getBlockSizeVec(data[left_pos]);
                block_size_vec_next = getBlockSizeVec(data[left_pos+1]);
            }
        }
        return std::make_tuple(block_merged,sb_merged,block_split,sb_split,block_size_vec_curr,block_size_vec_next);        
    }
    long long time_cnt;
private:    
    std::vector<std::vector<HuffmanBlock<T,Ch,4>>> data;
};

#endif