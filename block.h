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
    size_t total_block_bits;
    std::vector<T> block_vec;

    
    inline auto get_requried_blocks(int total_bits) const {        
        return (total_bits+sizeof(T)*8-1)/(sizeof(T)*8);
    }
public:    
    HuffmanBlock() = default;    
    auto encode_block(const auto& block,const auto& encoder) const {
        std::vector<T> ret;        
        int total_size = std::transform_reduce(block.begin(),block.end(),0,std::plus{},
        [&encoder](const auto& ch){return encoder.encode(ch).second;});        
        int comp_pos = 0,encoded_blocks = 0;      
        const int block_bits = sizeof(T)*8;          
        ret.resize(get_requried_blocks(total_size),static_cast<T>(0));
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
        return std::make_pair(ret,total_size);
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
        assert(comp_pos == total_block_bits);
        return ret;
    }
    explicit HuffmanBlock(int size) : block_size(size),block_vec(size),total_block_bits(size){}
    explicit HuffmanBlock(const auto& block,const auto& encoder){
        block_size = block.size();
        auto [vec,len] = encode_block(block,encoder);
        block_vec = std::move(vec);
        total_block_bits = len;

    }    
    HuffmanBlock(const HuffmanBlock<T,Ch,U>&) = default;
    HuffmanBlock& operator=(const HuffmanBlock<T,Ch,U>&) = default;
    HuffmanBlock(HuffmanBlock<T,Ch,U>&&) noexcept = default;
    HuffmanBlock& operator=(HuffmanBlock<T,Ch,U>&&) noexcept = default;

    auto get_start_shift_pos(const int start_shift_idx,const auto& encoder) const{
        if(start_shift_idx==0) return 0;
        assert(!block_vec.empty());
        auto block_vec_iter = block_vec.begin();int comp_pos = 0;
        const int block_bits = sizeof(T)*8;        
        T curr_code = *block_vec_iter,next_code = (std::next(block_vec_iter) == block_vec.end() ? 0 : *std::next(block_vec_iter));
        for(int i=0;i<start_shift_idx;++i){
            const auto block_index = comp_pos/block_bits;
            const auto block_offset = comp_pos%block_bits;            
            auto code = curr_code<<block_offset;        
            code |= (next(block_vec_iter) == block_vec.end() || block_offset==0 ? 0 : next_code>>(block_bits - block_offset));
            auto [ch,len] = encoder.decode(code);
            comp_pos+=len;
            std::advance(block_vec_iter,comp_pos/block_bits==block_index ? 0 : 1);
            curr_code = comp_pos/block_bits==block_index ? curr_code : next_code;
            next_code = comp_pos/block_bits==block_index ? next_code : (std::next(block_vec_iter) == block_vec.end() ? 0 : *std::next(block_vec_iter));
        }
        return comp_pos;
    }
    void shift_left(const int start_shift_pos,const int shift_len){
        const bool pop_block = get_requried_blocks(total_block_bits) != get_requried_blocks(total_block_bits-shift_len);
        const int code_size = sizeof(T)*8;     
        const int start_idx = (start_shift_pos - shift_len)/code_size;
        const int start_shift_pos_minus_one_inblock = (start_shift_pos - shift_len)%code_size;
        const int curr_block_size = block_vec.size();
        //whitening and fill first block
        if(start_shift_pos_minus_one_inblock + shift_len <= code_size){
            const int right_remain_size = code_size - start_shift_pos_minus_one_inblock - shift_len;
            //cout<<"right remain size: "<<right_remain_size<<'\n';            
            const T white_mask = start_shift_pos_minus_one_inblock==0 ? static_cast<T>(0) : (static_cast<T>(-1)<<(code_size - start_shift_pos_minus_one_inblock));
            const T right_remain = (block_vec[start_idx]&((static_cast<T>(1)<<right_remain_size)-1));
            const T shifted_from_right = (start_idx==curr_block_size-1 ? static_cast<T>(0) : (block_vec[start_idx+1]>>(code_size - shift_len)));            
            //cout<<"shifted from right: "<<shifted_from_right<<'\n';
            const T new_right_remain = ((right_remain<<shift_len)|shifted_from_right);
            block_vec[start_idx] &= white_mask;
            block_vec[start_idx] |= new_right_remain;
        }else{
            const int right_white_size = start_shift_pos%code_size;            
            const int left_white_size = code_size - start_shift_pos_minus_one_inblock;
            assert(0 < left_white_size && 0 < right_white_size);
            const T left_white_mask = (static_cast<T>(1)<<left_white_size)-1;                        
            const T new_right_remain = ((block_vec[start_idx+1]>>(code_size - shift_len))&left_white_mask);
            block_vec[start_idx] &= (~left_white_mask);            
            block_vec[start_idx] |= new_right_remain;
        }
        int curr_idx = start_idx+1;
        while(curr_idx < curr_block_size){
            const T left_part = block_vec[curr_idx]<<shift_len;
            const T right_part = (curr_idx == curr_block_size-1 ? static_cast<T>(0) : block_vec[curr_idx+1]>>(code_size - shift_len));
            block_vec[curr_idx] = (left_part | right_part);
            ++curr_idx;
        }
        if(pop_block) block_vec.pop_back();
        total_block_bits -= shift_len;
    }
    void shift_right(const int start_shift_pos,const int shift_len){
        const bool add_block = get_requried_blocks(total_block_bits) != get_requried_blocks(total_block_bits+shift_len);
        const int code_size = sizeof(T)*8;     
        const int start_idx = start_shift_pos/code_size;   
        int curr_idx = block_vec.size()-1;        
        if(add_block) block_vec.push_back(static_cast<T>(0));
        if(add_block && 1 < block_vec.size()){            
            const int n = block_vec.size();
            const int empty_bits = (code_size - total_block_bits%code_size)%code_size;
            const int last_block_bits = shift_len - empty_bits;            
            assert(last_block_bits > 0);            
            block_vec[n-1] = (block_vec[n-2]>>empty_bits)<<(code_size - last_block_bits);            
        }
        while(start_idx < curr_idx){
            block_vec[curr_idx] = ((block_vec[curr_idx-1]<<(code_size - shift_len)) | (block_vec[curr_idx]>>shift_len));
            --curr_idx;
        }
        const int start_shift_pos_inblock = start_shift_pos%code_size;
        if(start_shift_pos_inblock+shift_len <= code_size){            
            const int valid_bit_size = code_size - start_shift_pos_inblock - shift_len;
            const int white_size = code_size - start_shift_pos_inblock;
            T white_mask = (static_cast<T>(-1))>>start_shift_pos_inblock;
            //T white_mask = (static_cast<T>(1)<<white_size)-1;            
            T shift_mask = (static_cast<T>(1)<<valid_bit_size)-1;            
            T to_shift = ((block_vec[start_idx]>>shift_len)&shift_mask);
            block_vec[start_idx] &= (~white_mask);
            block_vec[start_idx] |= to_shift;
        }else{
            assert(start_idx+1 < block_vec.size());
            const int left_white_size = code_size - start_shift_pos_inblock;
            const int right_white_size = start_shift_pos_inblock+shift_len-code_size;
            T left_white_mask = (static_cast<T>(1)<<left_white_size)-1;
            T right_white_mask = static_cast<T>(-1)<<(code_size - right_white_size);            
            block_vec[start_idx] &= (~left_white_mask);
            block_vec[start_idx+1] &= (~right_white_mask);            
        }
        total_block_bits += shift_len;
    }   
    void insert(size_t pos,const Ch& ch,const auto& encoder){        
        if constexpr(DEBUG) assert(pos <= block_size);        
        const int code_size = sizeof(T)*8; 
        const int start_shift_pos = get_start_shift_pos(pos,encoder);
        auto [code,len] = encoder.encode(ch);              
        shift_right(start_shift_pos,len);        
        const int start_shift_pos_inblock = start_shift_pos%code_size;
        const int start_idx = start_shift_pos/code_size;        
        if(start_shift_pos_inblock+len <= code_size){            
            code>>=start_shift_pos_inblock;
            block_vec[start_idx] |= code;
        }else{
            auto left_code = code>>start_shift_pos_inblock;
            auto right_code = code<<(code_size - start_shift_pos_inblock);
            block_vec[start_idx] |= left_code;
            block_vec[start_idx+1] |= right_code;
        }
        ++block_size;        
        const int lastblock_valid_bits = (total_block_bits%code_size == 0 ? code_size : total_block_bits%code_size);
        T white = static_cast<T>(-1)<<(code_size - lastblock_valid_bits);
        block_vec.back() &= white;
    }

    void erase(size_t pos,const auto& encoder){
        if constexpr(DEBUG) assert(pos < block_size);        
        const int code_size = sizeof(T)*8; 
        const int start_shift_pos_minus_one = get_start_shift_pos(pos,encoder);
        const int start_idx = start_shift_pos_minus_one/code_size;
        const int left_bits = start_shift_pos_minus_one%code_size;
        const int curr_block_size = block_vec.size();        
        const T code = ((block_vec[start_idx]<<left_bits) | ((left_bits==0 || start_idx==curr_block_size-1) ? static_cast<T>(0) : block_vec[start_idx+1]>>(code_size - left_bits)));
        const int erasecode_len = encoder.decode(code).second;
        //cout<<"erasecode len: "<<erasecode_len<<" ,left bits: "<<left_bits<<'\n';
        shift_left(start_shift_pos_minus_one+erasecode_len,erasecode_len);
        --block_size;
        const int lastblock_valid_bits = (total_block_bits%code_size == 0 ? code_size : total_block_bits%code_size);
        T white = static_cast<T>(-1)<<(code_size - lastblock_valid_bits);
        block_vec.back() &= white;
    }

    
    
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
        auto [vec,len] = encode_block(block,encoder);
        block_vec = std::move(vec);
        total_block_bits = len;
    }

    void replace(size_t pos,const Ch& ch,const auto& encoder){
        auto decoded = decode_block(block_vec,block_size,encoder);
        decoded[pos] = ch;        
        auto [vec,len] = encode_block(decoded,encoder);
        block_vec = std::move(vec);
        total_block_bits = len;
    }
    /*void insert(size_t pos,const Ch& ch,const auto& encoder){        
        if constexpr(DEBUG) assert(pos <= block_size);        
        auto decoded = decode_block(block_vec,block_size,encoder);
        decoded.insert(decoded.begin()+pos,ch);
        auto [vec,len] = encode_block(decoded,encoder);
        block_vec = std::move(vec);
        total_block_bits = len; 
        ++block_size;
    }
    void erase(size_t pos,const auto& encoder){        
        if constexpr(DEBUG) assert(pos < block_size);        
        auto decoded = decode_block(block_vec,block_size,encoder);
        decoded.erase(decoded.begin()+pos);
        auto [vec,len] = encode_block(decoded,encoder);
        block_vec = std::move(vec);
        total_block_bits = len;
        --block_size;             
    }*/
    void merge(auto rhs_unique_ptr,const auto& left_encoder,const auto& right_encoder){
        auto decoded_left = decode_block(block_vec,block_size,left_encoder);
        auto decoded_right = rhs_unique_ptr->get(right_encoder);        
        block_size+=decoded_right.size();        
        decoded_left.insert(decoded_left.end(),decoded_right.begin(),decoded_right.end());
        auto [vec,len] = encode_block(decoded_left,left_encoder);
        block_vec = std::move(vec);
        total_block_bits = len;
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