#ifndef BLOCK_H_
#define BLOCK_H_
#include <iostream>
#include <vector>
#include <assert.h>
#include <memory>
#include <execution>
#include "encoder.h"

const bool DEBUG = true;

template<typename T,typename Ch>
class NaiveBlock{
private:
    size_t block_size;
    std::vector<T> block_vec;
    std::vector<T>& get(){
        return std::ref(block_vec);
    }
public:    
    NaiveBlock() : block_size(0){}
    explicit NaiveBlock(int size) : block_size(size),block_vec(size){};
    explicit NaiveBlock(std::vector<T>& block) : block_size(block.size()),block_vec(block){}
    explicit NaiveBlock(std::vector<T>&& block) : block_size(block.size()),block_vec(block){}
    NaiveBlock(const NaiveBlock<T,Ch>&) = default;
    NaiveBlock& operator=(const NaiveBlock<T,Ch>&) = default;
    NaiveBlock(NaiveBlock<T,Ch>&&) noexcept = default;
    NaiveBlock& operator=(NaiveBlock<T,Ch>&&) noexcept = default;
    
    
    T at(size_t pos,const auto& encoder) const {
        if constexpr(DEBUG){
            assert(pos>=0 && pos<block_size);
        }
        return block_vec[pos];
    }
    size_t size() const {
        if constexpr(DEBUG) {block_size==block_vec.size();}
        return block_size;
    }

    void replace(size_t pos,const Ch& ch,const auto& encoder){
        if constexpr(DEBUG) assert(pos < block_size);
        block_vec[pos] = ch;
    }
    void insert(size_t pos,const Ch& ch,const auto& encoder){
        /*insert before pos
        block = [2,3,4,5,6]
        block.insert(0,11) => [11,2,3,4,5,6]*/
        if constexpr(DEBUG) assert(pos <= block_size);
        block_vec.insert(block_vec.begin()+pos,ch);
        ++block_size;
    }
    void erase(size_t pos,const auto& encoder){
        /*erase curr pos
        block = [2,3,4,5,6]
        block.erase(2) => [2,4,5,6]*/
        if constexpr(DEBUG) assert(pos < block_size);
        block_vec.erase(block_vec.begin()+pos);
        --block_size;
    }
    /*void merge(NaiveBlock<T,Ch>& rhs,const auto& left_encoder,const auto& right_encoder){
        merge two blocks to curr
        curr = [1,2,3,4] rhs = [11,22,33,44]
        curr.merge(rhs) => [1,2,3,4,11,22,33,44]
        block_vec.insert(block_vec.end(),rhs.get().begin(),rhs.get().end());
        block_size += rhs.block_vec.size();
    }*/
    void merge(auto rhs_unique_ptr,const auto& left_encoder,const auto& right_encoder){                
        block_vec.insert(block_vec.end(),rhs_unique_ptr->get().cbegin(),rhs_unique_ptr->get().cend());
        block_size += rhs_unique_ptr->size();
    }
    auto split(size_t pos,const auto& encoder){
        /*split curr into two blocks
        block = [1,2,3,4,5]
        block.split(2) => [1,2] [3,4,5]*/
        if constexpr(DEBUG) assert(pos>0 && pos<block_size);
        size_t lsize = pos, rsize = block_size - pos;
        std::vector<T> l_vec(lsize);
        std::vector<T> r_vec(rsize);
        std::copy(block_vec.cbegin(),block_vec.cbegin()+lsize,l_vec.begin());
        std::copy(block_vec.cbegin()+lsize,block_vec.cend(),r_vec.begin());
        NaiveBlock<T,Ch> l_block(std::move(l_vec));
        NaiveBlock<T,Ch> r_block(std::move(r_vec));
        return make_pair<NaiveBlock<T,Ch>,NaiveBlock<T,Ch>>(std::move(l_block),std::move(r_block));
    }
};

template<typename T,typename Ch,int U = 4>//U: superchar num == 4
class HuffmanBlock{
private:
    size_t block_size;    
    std::vector<T> block_vec;
public:    
    HuffmanBlock() = default;    
    auto encode_block(const auto& block,const auto& encoder) const {
        std::vector<T> ret;        
        int total_size = std::transform_reduce(block.begin(),block.end(),0,std::plus{},
        [&encoder](const auto& ch){return encoder.encode(ch).second;});        
        int comp_pos = 0,encoded_blocks = 0;      
        const int block_bits = sizeof(T)*8;          
        ret.resize((total_size+block_bits-1)/block_bits,static_cast<T>(0));
        const int n = ret.size();
        for(auto ch:block){
            auto [code,len] = encoder.encode(ch);                   
            auto block_index = comp_pos/block_bits;
            auto block_offset = comp_pos%block_bits;
            auto remain = block_offset+static_cast<int>(len)-block_bits;
            ret[block_index] |= (code>>block_offset);
            if(block_index+1 < n)
                ret[block_index+1] |= remain > 0 ? (code<<(static_cast<int>(len)-remain)) : static_cast<T>(0);
            comp_pos+=len;
        }
        return ret;
    }
    auto decode_block(const auto& comp_block,const int block_size,const auto& encoder) const {
        std::vector<Ch> ret;        
        if(block_size==0) return ret;
        ret.resize(block_size);
        auto comp_block_iter = comp_block.begin();int comp_pos = 0;
        const int block_bits = sizeof(T)*8;
        T curr_code = *comp_block_iter,next_code = (next(comp_block_iter) == comp_block.end() ? 0 : *std::next(comp_block_iter));
        for(int i=0;i<block_size;++i){
            auto block_index = comp_pos/block_bits;
            auto block_offset = comp_pos%block_bits;            
            auto code = curr_code<<block_offset;        
            code |= (next(comp_block_iter) == comp_block.end() || block_offset==0 ? 0 : next_code>>(block_bits - block_offset));
            auto [ch,len] = encoder.decode(code);
            comp_pos += len;
            std::advance(comp_block_iter,comp_pos/block_bits==block_index ? 0 : 1);
            curr_code = comp_pos/block_bits==block_index ? curr_code : next_code;
            next_code = comp_pos/block_bits==block_index ? next_code : (next(comp_block_iter) == comp_block.end() ? 0 : *next(comp_block_iter));
            ret[i] = ch;            
        }
        return ret;
    }
    explicit HuffmanBlock(int size) : block_size(size),block_vec(size){}
    explicit HuffmanBlock(const auto& block,const auto& encoder){
        block_size = block.size();
        block_vec = encode_block(block,encoder);
    }    
    HuffmanBlock(const HuffmanBlock<T,Ch,U>&) = default;
    HuffmanBlock& operator=(const HuffmanBlock<T,Ch,U>&) = default;
    HuffmanBlock(HuffmanBlock<T,Ch,U>&&) noexcept = default;
    HuffmanBlock& operator=(HuffmanBlock<T,Ch,U>&&) noexcept = default;
    
    std::vector<Ch> get(const auto& encoder) const {
        return decode_block(block_vec,block_size,encoder);
    }
    Ch at(size_t pos,const auto& encoder) const {
        if constexpr(DEBUG){
            assert(pos>=0 && pos<block_size);
        }
        auto decoded = decode_block(block_vec,pos+1,encoder);
        return decoded.back();
    }
    size_t use_bytes() const{
        return block_vec.size()*sizeof(T);
    }
    size_t size() const {        
        return block_size;
    }
    void replace_block(const auto& block,const auto& encoder){
        block_size = block.size();
        block_vec = encode_block(block,encoder);
    }

    void replace(size_t pos,const Ch& ch,const auto& encoder){
        auto decoded = decode_block(block_vec,block_size,encoder);
        decoded[pos] = ch;        
        block_vec = encode_block(decoded,encoder);        
    }
    void insert(size_t pos,const Ch& ch,const auto& encoder){        
        if constexpr(DEBUG) assert(pos <= block_size);        
        auto decoded = decode_block(block_vec,block_size,encoder);
        decoded.insert(decoded.begin()+pos,ch);
        block_vec = encode_block(decoded,encoder);        
        ++block_size;
    }
    void erase(size_t pos,const auto& encoder){        
        if constexpr(DEBUG) assert(pos < block_size);        
        auto decoded = decode_block(block_vec,block_size,encoder);
        decoded.erase(decoded.begin()+pos);
        block_vec = encode_block(decoded,encoder);   
        --block_size;             
    }
    void merge(auto rhs_unique_ptr,const auto& left_encoder,const auto& right_encoder){
        auto decoded_left = decode_block(block_vec,block_size,left_encoder);
        auto decoded_right = rhs_unique_ptr->get(right_encoder);        
        block_size+=decoded_right.size();        
        decoded_left.insert(decoded_left.end(),decoded_right.begin(),decoded_right.end());
        block_vec = encode_block(decoded_left,left_encoder);
    }
    /*void merge(HuffmanBlock<T,Ch,U>& rhs,const auto& left_encoder,const auto& right_encoder){
        auto decoded_left = decode_block(block_vec,block_size,left_encoder);
        auto decoded_right = rhs.get(right_encoder);
        block_size+=decoded_right.size();        
        decoded_left.insert(decoded_left.end(),decoded_right.begin(),decoded_right.end());
        block_vec = encode_block(decoded_left,left_encoder);
    }*/
    auto split(size_t pos,const auto& encoder){        
        if constexpr(DEBUG) assert(pos>0 && pos<block_size);
        size_t lsize = pos, rsize = block_size - pos;
        auto decoded = decode_block(block_vec,block_size,encoder);
        std::vector<Ch> l_vec(lsize);
        std::vector<Ch> r_vec(rsize);
        std::copy(decoded.begin(),decoded.begin()+lsize,l_vec.begin());
        std::copy(decoded.begin()+lsize,decoded.end(),r_vec.begin());
        HuffmanBlock<T,Ch> l_block(l_vec,encoder);
        HuffmanBlock<T,Ch> r_block(r_vec,encoder);
        return std::make_pair(std::move(l_block),std::move(r_block));
    }
};


#endif