#ifndef CRAM_H_
#define CRAM_H_

#include "darray.h"
using std::cout;
using std::cin;

template<typename T,typename Ch,int MAX_BLOCK_SIZE = 1024,int MAX_SB_SIZE = 1024,int SIGMA = 65536>
class CRAM{
public:
    using index_t = int;
    CRAM() = delete;
    std::vector<Ch> text_copied;
    explicit CRAM(const auto& text,const int rewrite_blocks_) : 
    freq(SIGMA,1),tree_num_vec(text.size()/(MAX_BLOCK_SIZE/2)),curr_tree(0){        
        text_copied = text;
        total_block_num = text.size()/(MAX_BLOCK_SIZE/2);
        rewrite_blocks = rewrite_blocks_;
        rewrite_pos = rewrite_blocks==0 ? 0 : (total_block_num)*(rewrite_blocks-1)/rewrite_blocks;
        
        for(const auto ch:text){
            freq[ch]++;
        }
        encoders[0] = HuffmanEncoder<>(freq);
        da = Darray<T,Ch,MAX_BLOCK_SIZE,MAX_SB_SIZE>(text,encoders[0]);        
    }
    void replace(index_t block_index,const auto& vec){
        int prev_tree_index = tree_num_vec[block_index];
        const auto curr_block = da.block_at(block_index,encoders[tree_num_vec[block_index]]);        
        for(const auto ch:vec){
            freq[ch]++;
        }
        for(const auto ch:curr_block){
            freq[ch]--;                        
        }
        
        da.block_replace(block_index,vec,encoders[curr_tree]);        
        tree_num_vec[block_index] = curr_tree;
        

        for(int i=0;i<rewrite_blocks && rewrite_pos < total_block_num;++i,++rewrite_pos){
            const auto curr_block = da.block_at(rewrite_pos,encoders[tree_num_vec[rewrite_pos]]);
            da.block_replace(rewrite_pos,curr_block,encoders[curr_tree]);
            const auto rewrited_block = da.block_at(rewrite_pos,encoders[curr_tree]);            
            tree_num_vec[rewrite_pos] = curr_tree;
        }
        if(rewrite_pos == total_block_num){                        
            rewrite_pos = 0;
            curr_tree^=1;            
            encoders[curr_tree] = HuffmanEncoder<>(freq);                        
        }
    }
    auto get_block(index_t block_index){
        return da.block_at(block_index,encoders[tree_num_vec[block_index]]);
    }
    size_t use_bytes()const{
        return da.use_bytes();
    }


private:
    index_t total_block_num;
    index_t rewrite_blocks;
    index_t rewrite_pos;    
    uint8_t curr_tree;

    std::vector<int> freq;
    std::vector<uint8_t> tree_num_vec;
    HuffmanEncoder<> encoders[2];
    Darray<T,Ch,MAX_BLOCK_SIZE,MAX_SB_SIZE> da;
};

#endif