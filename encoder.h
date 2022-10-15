#ifndef ENCODER_H_
#define ENCODER_H_

#include <map>
#include <vector>
#include <queue>
#include <algorithm>
#include <iostream>

class NaiveEncoder{    
    void encode(){}
    void decode(){}
};

template<typename code_t = uint64_t,typename ch_t = uint16_t,int alphabet_size = 65536>
class HuffmanEncoder{
public:       
    using clen_t = uint8_t;    
    using tbl_t = uint64_t;    
    using freq_t = uint64_t;
    
public:
    int NILNODE = -1;
    int TBL_WIDTH = 16;
    struct pq_block{
        freq_t freq;
        int ch;
        bool operator<(const pq_block& rhs) const{
            return freq > rhs.freq;
        }
    };
    struct tbl_extended_t{
        code_t code;
        ch_t ch;        
        bool operator<(const tbl_extended_t& rhs)const{
            return code < rhs.code;
        }
    };
    std::vector<clen_t> clen_v;
    std::vector<code_t> code_v;
    std::vector<tbl_t> tbl;
    std::vector<tbl_extended_t> tbl_extended;
    void make_code(std::vector<int>& left_child_v,std::vector<int>& right_child_v,int curr_node,int code_len,code_t code){
        if(left_child_v[curr_node]!=NILNODE){
            make_code(left_child_v,right_child_v,
            left_child_v[curr_node],code_len+1,code);
        }
        if(right_child_v[curr_node]!=NILNODE){
            make_code(left_child_v,right_child_v,
            right_child_v[curr_node],code_len+1,code+(static_cast<code_t>(1)<<(sizeof(code_t)*8-1-code_len)));
        }
        if(left_child_v[curr_node]==NILNODE && right_child_v[curr_node]==NILNODE){                        
            clen_v[curr_node] = code_len;
            code_v[curr_node] = code;
        }
    }
    inline code_t get_tbl_code(code_t code) const {
        return code >> (sizeof(code_t)*8 - TBL_WIDTH);
    }
    void make_tbl(){
        std::map<code_t,ch_t> dic;
        
        for(int ch=0;ch<alphabet_size;++ch){            
            const auto code = code_v[ch];            
            auto len = clen_v[ch];
            auto tbl_start = get_tbl_code(code);            
            if(len <= TBL_WIDTH){                
                auto block_size = (1<<(TBL_WIDTH - len));
                for(int i=tbl_start;i<tbl_start+block_size;++i){                    
                    tbl[i] = ch;
                }
            }else{
                tbl[tbl_start] = alphabet_size;                
                dic[code] = ch;
            }
        }        
        if(dic.empty()) return;
        tbl_t index_start = 1,index_end = 1;
        auto curr_tbl_start = get_tbl_code(dic.begin()->first);
        for(auto& kvp:dic){                        
            tbl_extended.push_back({kvp.first,kvp.second});
            ++index_end;            
            if(curr_tbl_start!=get_tbl_code(kvp.first)){
                tbl[curr_tbl_start] = (index_start<<TBL_WIDTH)+index_end;
                curr_tbl_start = get_tbl_code(kvp.first);
                index_start = index_end;
            }
        }
        tbl[curr_tbl_start] = (index_start<<TBL_WIDTH)+index_end;
    }
    

public:
    HuffmanEncoder(){}
    explicit HuffmanEncoder(const auto& freq) : clen_v(alphabet_size),code_v(alphabet_size),tbl(alphabet_size){
        std::vector<int> left_child_v(alphabet_size*2+3,NILNODE);
        std::vector<int> right_child_v(alphabet_size*2+3,NILNODE);
        std::priority_queue<pq_block> pq;
        for(int i=0;i<alphabet_size;++i){
            pq_block blk;blk.freq = freq[i];blk.ch = i;
            pq.push(blk);
        }
        int curr_node = alphabet_size;
        while(pq.size()>1){
            auto first_block = pq.top();pq.pop();
            auto second_block = pq.top();pq.pop();            
            left_child_v[curr_node] = first_block.ch;
            right_child_v[curr_node] = second_block.ch;
            pq.push({first_block.freq+second_block.freq,curr_node});
            ++curr_node;
        }
        make_code(left_child_v,right_child_v,curr_node-1,0,static_cast<code_t>(0));                
        make_tbl();        
        
    }
    HuffmanEncoder(const HuffmanEncoder&) = default;
    HuffmanEncoder& operator=(const HuffmanEncoder&) = default;
    HuffmanEncoder(HuffmanEncoder&&) noexcept = default;
    HuffmanEncoder& operator=(HuffmanEncoder&&) noexcept = default;
    auto encode(const ch_t ch) const {        
        return std::make_pair(code_v[ch],clen_v[ch]);
    }
    auto decode(const code_t code) const {        
        auto tbl_idx = get_tbl_code(code);
        if(tbl[tbl_idx]<alphabet_size){
            auto ch = tbl[tbl_idx];
            return std::make_pair(static_cast<ch_t>(tbl[tbl_idx]),clen_v[tbl[tbl_idx]]);
        }               
        tbl_extended_t code_for_tbl_extended{code,0};
        auto tbl_code = std::prev(std::upper_bound(tbl_extended.begin(),tbl_extended.end(),code_for_tbl_extended));
        auto ch = tbl_code->ch;
        return std::make_pair(ch,clen_v[ch]);
    }
};

#endif