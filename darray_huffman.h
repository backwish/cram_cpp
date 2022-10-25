#ifndef DARRAY_HUFFMAN_H_
#define DARRAY_HUFFMAN_H_
#include "general_darray.h"
#include "block.h"
#include "encoder.h"
const int SIGMA = 65536;

template<int H,int MAX_BLOCK_SIZE = 1024,int MAX_INTERNAL_BLOCK_SIZE = 1024>
class Darray_Huffman{
public:
    using code_t = uint64_t;
    using ch_t = uint16_t;
    using block_t = HuffmanBlock<code_t,ch_t>;
    using encoder_t = HuffmanEncoder<>;
    using freq_t = int;
    using index_t = int;    
    explicit Darray_Huffman(const auto& text,bool init) : freq(SIGMA,1){
        int n = text.size();
        for(const auto ch:text){ freq[ch]++; }
        huffman_encoder = encoder_t(freq);
        if(init){            
            da = Darray<code_t,ch_t,block_t,encoder_t,H,MAX_BLOCK_SIZE,MAX_INTERNAL_BLOCK_SIZE>(text,huffman_encoder);
        }else{
            da = Darray<code_t,ch_t,block_t,encoder_t,H,MAX_BLOCK_SIZE,MAX_INTERNAL_BLOCK_SIZE>{};
        }
    }
    void insert(index_t pos,auto val){
        da.insert(pos,val,huffman_encoder);
    }
    void erase(index_t pos){
        da.erase(pos,huffman_encoder);
    }
    auto at(index_t pos) const{
        return da.at(pos,huffman_encoder);
    }
    auto block_at(index_t pos) const{
        return da.block_at(pos,huffman_encoder);
    }
    auto size() const{
        return da.size();
    }
    auto getBlockInsertEraseTime() const{
        return da.getBlockInsertEraseTime();
    }


private:
    
    std::vector<freq_t> freq;
    encoder_t huffman_encoder;
    Darray<code_t,ch_t,block_t,encoder_t,H,MAX_BLOCK_SIZE,MAX_INTERNAL_BLOCK_SIZE> da;
};


#endif