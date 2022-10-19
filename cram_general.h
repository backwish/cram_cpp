#ifndef CRAM_H_
#define CRAM_H_

#include "general_darray.h"
#include "block.h"
using std::cout;
using std::cin;

template<typename T,typename Ch,int MODE,int MAX_BLOCK_SIZE = 1024,int MAX_INTERNAL_BLOCK_SIZE = 1024,int SIGMA = 65536>
class CRAM{
public:
    using index_t = int;
    using block_t = HuffmanBlock<T,Ch>;
    using encoder_t = HuffmanEncoder<T,Ch,MODE>;
    CRAM() = delete;    
    explicit CRAM(const auto& text,const int rewrite_blocks_,const int H) : 
    freq(SIGMA,1),tree_num_vec(text.size()/(MAX_BLOCK_SIZE/2)),curr_tree(0),block_size(MAX_BLOCK_SIZE/2),existCodeSpace(true){                
        total_block_num = text.size()/block_size;
        rewrite_blocks = rewrite_blocks_;
        rewrite_pos = rewrite_blocks==0 ? 0 : (total_block_num)*(rewrite_blocks-1)/rewrite_blocks;        
        for(const auto ch:text){
            freq[ch]++;
        }
        encoders[0] = encoder_t(freq);
        if constexpr(0 < MODE){
            pivotFreq.resize(SIGMA);
            std::copy(freq.begin(),freq.end(),pivotFreq.begin());
        }        
        da = Darray<T,Ch,block_t,encoder_t,MAX_BLOCK_SIZE,MAX_INTERNAL_BLOCK_SIZE>(text,encoders[0],H);        
    }
    auto make_huffman_blocks(const auto& text,const auto& encoder){
        int n = text.size();
        std::vector<std::unique_ptr<block_t>> ret;
        for(int i=0;i<n;i+=block_size){
            int curr_block_size = std::min(n-i,block_size);
            std::vector<Ch> curr_block(curr_block_size);
            std::copy(text.begin()+i,text.begin()+(i+curr_block_size),curr_block.begin());
            auto block_ptr = std::make_unique<block_t>(std::move(curr_block),encoder);
            ret.push_back(std::move(block_ptr));
        }
        return std::move(ret);
    }
    void makeNewCode(const auto& vec){
        if(!existCodeSpace) return;
        auto inserted_ch_vec = vec;
        std::sort(inserted_ch_vec.begin(),inserted_ch_vec.end());
        inserted_ch_vec.erase(std::unique(inserted_ch_vec.begin(),inserted_ch_vec.end()),inserted_ch_vec.end());
        for(const auto ch:inserted_ch_vec){
            int candidate_len = std::log2(total_block_num*block_size/freq[ch]);
            if(pivotFreq[ch]*2 < freq[ch] && candidate_len < 16){
                existCodeSpace = encoders[curr_tree].insertCode(ch,candidate_len);
                pivotFreq[ch] = freq[ch];
            }            
            if(!existCodeSpace) break;
        }
    }
    void replace(index_t block_index,const auto& vec){
        int prev_tree_index = tree_num_vec[block_index];
        const auto curr_block = da.block_at(block_index*block_size,encoders[tree_num_vec[block_index]]);           
        for(const auto ch:vec){
            freq[ch]++;
        }
        for(const auto ch:curr_block){
            freq[ch]--;                        
        }
        if constexpr(0 < MODE){
            makeNewCode(vec);
        }
        
        da.block_replace(block_index*block_size,vec,encoders[curr_tree]);        
        tree_num_vec[block_index] = curr_tree;        

        for(int i=0;i<rewrite_blocks && rewrite_pos < total_block_num;++i,++rewrite_pos){
            const auto curr_block = da.block_at(rewrite_pos*block_size,encoders[tree_num_vec[rewrite_pos]]);
            da.block_replace(rewrite_pos*block_size,curr_block,encoders[curr_tree]);
            const auto rewrited_block = da.block_at(rewrite_pos*block_size,encoders[curr_tree]);            
            tree_num_vec[rewrite_pos] = curr_tree;
        }
        if(rewrite_pos == total_block_num){                        
            rewrite_pos = 0;
            curr_tree^=1;            
            encoders[curr_tree] = encoder_t(freq);          
            if constexpr(0 < MODE){
                pivotFreq = freq;
                existCodeSpace = true;
            }        
        }
    }
    auto get_block(index_t block_index){
        return da.block_at(block_index*block_size,encoders[tree_num_vec[block_index]]);
    }
    auto get_bpc()const{
        return da.get_bpc();
    }


private:
    index_t total_block_num;
    index_t rewrite_blocks;
    index_t rewrite_pos;    
    index_t block_size;
    uint8_t curr_tree;
    bool existCodeSpace;

    std::vector<int> freq;
    std::vector<int> pivotFreq;
    std::vector<uint8_t> tree_num_vec;
    encoder_t encoders[2];
    Darray<T,Ch,block_t,encoder_t,MAX_BLOCK_SIZE,MAX_INTERNAL_BLOCK_SIZE> da;
};

#endif