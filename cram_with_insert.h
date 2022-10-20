#ifndef CRAM_INSERT_H
#define CRAM_INSERT_H

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
    freq(SIGMA,1),curr_tree(0),prev_tree(0),modify_cnt(0),block_size(MAX_BLOCK_SIZE/2),existCodeSpace(true){                
        total_block_num = text.size()/block_size;
        rewrite_blocks = rewrite_blocks_;
        rewrite_start_pos = rewrite_blocks==0 ? 0 : (text.size())*(rewrite_blocks-1)/rewrite_blocks;        
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
    void insert(index_t pos,Ch ch){
        freq[ch]++;
        uint8_t tree_idx = pos < rewrite_start_pos ? curr_tree : prev_tree;
        da.insert(pos,ch,encoders[tree_idx]);
        ++modify_cnt;
        if(pos < rewrite_start_pos) ++rewrite_start_pos;
        if(MAX_BLOCK_SIZE/2 == modify_cnt){
            modify_cnt = 0;
            for(int i=0;i<rewrite_blocks && rewrite_start_pos < da.size();++i){
                const auto curr_block = da.block_at(rewrite_start_pos,encoders[prev_tree]);
                da.block_replace(rewrite_start_pos,curr_block,encoders[curr_tree]);
                rewrite_start_pos+=curr_block.size();
            }
            if(rewrite_start_pos==da.size()){
                //std::cout<<"REWRITE! "<<prev_tree<<" , "<<curr_tree<<'\n';
                rewrite_start_pos = 0;
                prev_tree = curr_tree;
                curr_tree^=1;
                encoders[curr_tree] = encoder_t(freq);                
            }
        }
    }

    /*void replace(index_t block_index,const auto& vec){
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
    }*/
    auto get_block(index_t pos){
        int tree_idx = pos < rewrite_start_pos ? curr_tree : prev_tree;
        return da.block_at(pos,encoders[tree_idx]);
    }
    auto get_bpc()const{
        return da.get_bpc();
    }
    auto size() const{
        return da.size();
    }


private:
    index_t total_block_num;
    index_t rewrite_blocks;    
    index_t block_size;

    int prev_tree;
    int curr_tree;

    index_t rewrite_start_pos;    
    index_t modify_cnt;
    
    bool existCodeSpace;   


    std::vector<int> freq;
    std::vector<int> pivotFreq;    
    encoder_t encoders[2];
    Darray<T,Ch,block_t,encoder_t,MAX_BLOCK_SIZE,MAX_INTERNAL_BLOCK_SIZE> da;
};

#endif