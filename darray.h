#ifndef DARRAY_H
#define DARRAY_H

#include "block.h"
#include "darray_data.h"
#include <iostream>
#include <vector>
using std::cout;
using std::cin;
using namespace std::chrono;

template<typename T,typename Ch,int MAX_BLOCK_SIZE = 1024,int MAX_SB_SIZE = 1024>
class Darray{
public:    
    using index_t = int;
    Darray() : total_size(0),rootblock_size_psum(1,1),rootblock_size(1,1),
    superblock_size_psum_vec(1,std::vector<index_t>(1,1)),superblock_size_vec(1,std::vector<index_t>(1,1)){}
    Darray(const auto& vec,const auto& encoder) : total_size(vec.size()),
    rootblock_size((total_size+MAX_BLOCK_SIZE*MAX_SB_SIZE/4-1)/(MAX_BLOCK_SIZE*MAX_SB_SIZE/4),0),
    rootblock_size_psum((total_size+MAX_BLOCK_SIZE*MAX_SB_SIZE/4-1)/(MAX_BLOCK_SIZE*MAX_SB_SIZE/4),0),
    superblock_size_vec((total_size+MAX_BLOCK_SIZE*MAX_SB_SIZE/4-1)/(MAX_BLOCK_SIZE*MAX_SB_SIZE/4),std::vector<index_t>(MAX_SB_SIZE/2)),
    superblock_size_psum_vec((total_size+MAX_BLOCK_SIZE*MAX_SB_SIZE/4-1)/(MAX_BLOCK_SIZE*MAX_SB_SIZE/4),std::vector<index_t>(MAX_SB_SIZE/2))
    {   
        data = Darray_Data<T,Ch,MAX_BLOCK_SIZE,MAX_SB_SIZE>(vec,encoder);

        std::fill(rootblock_size.begin(),rootblock_size.end(),MAX_BLOCK_SIZE*MAX_SB_SIZE/4);
        if(0 < total_size%(MAX_BLOCK_SIZE*MAX_SB_SIZE/4)){
            rootblock_size.back() = total_size%(MAX_BLOCK_SIZE*MAX_SB_SIZE/4);
        }        
        std::partial_sum(rootblock_size.cbegin(),rootblock_size.cend(),rootblock_size_psum.begin());
        
        for(int i=0;i<superblock_size_vec.size();++i){            
            std::fill(superblock_size_vec[i].begin(),superblock_size_vec[i].end(),MAX_BLOCK_SIZE/2);
            std::partial_sum(superblock_size_vec[i].cbegin(),superblock_size_vec[i].cend(),
            superblock_size_psum_vec[i].begin());
        }        
        
        if(0 < total_size%(MAX_BLOCK_SIZE*MAX_SB_SIZE/4)){
            int remain = total_size%(MAX_BLOCK_SIZE*MAX_SB_SIZE/4);
            auto& size_vec = superblock_size_vec.back();
            auto& psum_vec = superblock_size_psum_vec.back();
            size_vec.clear();
            psum_vec.clear();
            while(MAX_BLOCK_SIZE/2 <= remain){
                size_vec.push_back(MAX_BLOCK_SIZE/2);
                remain-=MAX_BLOCK_SIZE/2;
            }
            size_vec.push_back(remain);
            psum_vec.resize(size_vec.size());
            std::partial_sum(size_vec.cbegin(),size_vec.cend(),psum_vec.begin());
        }
        superblock_size_vec.back().back()+=1;
        superblock_size_psum_vec.back().back()+=1;
        rootblock_size.back()+=1;
        rootblock_size_psum.back()+=1;        
    }
    
    Darray(const Darray&) = delete;
    Darray& operator=(const Darray&) = delete;
    Darray(Darray&&) noexcept = default;
    Darray& operator=(Darray&&) noexcept = default;
    void increasePartialSum(auto& vec,int pos){
        std::transform(std::execution::unseq,vec.begin()+pos,vec.end(),
        vec.begin()+pos,[](auto i){return ++i;});
    }
    void decreasePartialSum(auto& vec,int pos){
        std::transform(std::execution::unseq,vec.begin()+pos,vec.end(),
        vec.begin()+pos,[](auto i){return --i;});
    }
    void splitSuperBlock(size_t rb_pos,auto& block_size_vec_curr,auto& block_size_vec_next){
        auto right_size_sum = std::accumulate(block_size_vec_next.cbegin(),block_size_vec_next.cend(),static_cast<index_t>(0));
        auto left_size_sum = std::accumulate(block_size_vec_curr.cbegin(),block_size_vec_curr.cend(),static_cast<index_t>(0));
        rootblock_size[rb_pos] = right_size_sum;
        rootblock_size.insert(rootblock_size.begin()+rb_pos,left_size_sum);
        auto offset = rb_pos == 0 ? static_cast<index_t>(0) : rootblock_size_psum[rb_pos-1];
        rootblock_size_psum.insert(rootblock_size_psum.begin()+rb_pos,offset+left_size_sum);
                        
        superblock_size_vec[rb_pos] = std::move(block_size_vec_curr);
        superblock_size_vec.insert(superblock_size_vec.begin()+(rb_pos+1),std::move(block_size_vec_next));            
        superblock_size_psum_vec[rb_pos].resize(superblock_size_vec[rb_pos].size());
        superblock_size_psum_vec.insert(superblock_size_psum_vec.begin()+(rb_pos+1),std::vector<int>(superblock_size_vec[rb_pos+1].size()));
                        
        std::partial_sum(superblock_size_vec[rb_pos].begin(),superblock_size_vec[rb_pos].end(),
        superblock_size_psum_vec[rb_pos].begin());
        std::partial_sum(superblock_size_vec[rb_pos+1].begin(),superblock_size_vec[rb_pos+1].end(),
        superblock_size_psum_vec[rb_pos+1].begin());            
    }
    void splitBlock(size_t rb_pos,size_t sb_pos){
        auto& superblock_size_psum = superblock_size_psum_vec[rb_pos];
        auto& superblock_size = superblock_size_vec[rb_pos];
        index_t left_size = MAX_BLOCK_SIZE/2+1,right_size = superblock_size[sb_pos]-left_size;
        superblock_size[sb_pos] = right_size;
        superblock_size.insert(superblock_size.begin()+sb_pos,left_size);
        auto offset = sb_pos == 0 ? static_cast<index_t>(0) : superblock_size_psum[sb_pos-1];
        superblock_size_psum.insert(superblock_size_psum.begin()+sb_pos,offset+left_size);            
    }

    void insert(size_t pos,auto val,const auto& encoder){
        auto [rb_pos,sb_pos,block_pos] = getPos(pos);        
        auto& superblock_size_psum = superblock_size_psum_vec[rb_pos];
        auto& superblock_size = superblock_size_vec[rb_pos];

        increasePartialSum(superblock_size_psum,sb_pos);
        superblock_size[sb_pos]++;
        increasePartialSum(rootblock_size_psum,rb_pos);
        rootblock_size[rb_pos]++;        
        
        auto [block_split,superblock_split,block_size_vec_curr,block_size_vec_next] = data.insert(rb_pos,sb_pos,block_pos,val,encoder);
        
        if(superblock_split){                        
            splitSuperBlock(rb_pos,block_size_vec_curr,block_size_vec_next);
        }else if(block_split){
            splitBlock(rb_pos,sb_pos);
        }
        ++total_size;
    }
    void erase(size_t pos,const auto& encoder){
        auto [rb_pos,sb_pos,block_pos] = getPos(pos);        
        auto& superblock_size_psum = superblock_size_psum_vec[rb_pos];
        auto& superblock_size = superblock_size_vec[rb_pos];
        decreasePartialSum(superblock_size_psum,sb_pos);
        superblock_size[sb_pos]--;
        decreasePartialSum(rootblock_size_psum,rb_pos);
        rootblock_size[rb_pos]--;
        auto [block_merged,sb_merged,block_split,sb_split,block_size_vec_curr,block_size_vec_next] = data.erase(rb_pos,sb_pos,block_pos,encoder);
        if(block_merged){
            auto left_pos = (sb_pos==0 ? sb_pos : sb_pos-1);
            auto right_pos = (sb_pos==0 ? sb_pos+1 : sb_pos);
            superblock_size[right_pos] += superblock_size[left_pos];
            superblock_size.erase(superblock_size.begin()+left_pos);
            superblock_size_psum.erase(superblock_size_psum.begin()+left_pos);
        }
        if(block_split){
            auto left_pos = (sb_pos==0 ? sb_pos : sb_pos-1);
            splitBlock(rb_pos,left_pos);
        }
        if(sb_merged){
            auto left_pos = (rb_pos==0 ? rb_pos : rb_pos-1);
            auto right_pos = (rb_pos==0 ? rb_pos+1 : rb_pos);            
            superblock_size_vec[left_pos].insert(superblock_size_vec[left_pos].end(),
            superblock_size_vec[right_pos].begin(),superblock_size_vec[right_pos].end());

            superblock_size_vec.erase(superblock_size_vec.begin()+right_pos);
            superblock_size_psum_vec[left_pos].resize(superblock_size_vec[left_pos].size());
            std::partial_sum(superblock_size_vec[left_pos].begin(),superblock_size_vec[left_pos].end(),
            superblock_size_psum_vec[left_pos].begin());
            superblock_size_psum_vec.erase(superblock_size_psum_vec.begin()+right_pos);

            rootblock_size[right_pos] = superblock_size_psum_vec[left_pos].back();
            rootblock_size.erase(rootblock_size.begin()+left_pos);            
            auto offset = left_pos==0 ? static_cast<index_t>(0) : rootblock_size_psum[left_pos-1];
            rootblock_size_psum[right_pos] = offset + superblock_size_psum_vec[left_pos].back();
            rootblock_size_psum.erase(rootblock_size_psum.begin()+left_pos);
        }
        if(sb_split){
            auto left_pos = (rb_pos==0 ? rb_pos : rb_pos-1);
            splitSuperBlock(left_pos,block_size_vec_curr,block_size_vec_next);
        }
        --total_size;
    }

    //method for fixed-size CRAM
    auto block_at(size_t block_pos,const auto& encoder) const{
        auto [rb_pos,sb_pos,b_pos] = getPos(block_pos*MAX_BLOCK_SIZE/2);
        return data.block_at(rb_pos,sb_pos,encoder);
    }
    void block_replace(size_t block_pos,const auto& vec,const auto& encoder){
        auto [rb_pos,sb_pos,b_pos] = getPos(block_pos*MAX_BLOCK_SIZE/2);        
        data.block_replace(rb_pos,sb_pos,vec,encoder);        
    }
    
    auto at(size_t total_pos,const auto& encoder) const{
        auto [rb_pos,sb_pos,b_pos] = getPos(total_pos);
        return data.at(rb_pos,sb_pos,b_pos,encoder);
    }
    auto getPos(size_t total_pos) const {        
        auto rootblock_it = std::upper_bound(rootblock_size_psum.cbegin(),rootblock_size_psum.cend(),total_pos);
        auto rootblock_pos = std::distance(rootblock_size_psum.cbegin(),rootblock_it);        
        auto offset = (rootblock_pos==0 ? 0 : rootblock_size_psum[rootblock_pos-1]);        
        total_pos -= offset;        
        auto superblock_it = std::upper_bound(superblock_size_psum_vec[rootblock_pos].cbegin(),
        superblock_size_psum_vec[rootblock_pos].cend(),total_pos);        
        auto superblock_pos = std::distance(superblock_size_psum_vec[rootblock_pos].cbegin(),superblock_it);
        offset = (superblock_pos==0 ? 0 : superblock_size_psum_vec[rootblock_pos][superblock_pos-1]);        
        total_pos -= offset;
        return std::make_tuple(rootblock_pos,superblock_pos,total_pos);
    }

    auto size()const{return total_size;}

    size_t use_bytes() const{
        return data.use_bytes();
    }

    void printDarray() const {
        cout<<"rootblock index:\n";
        for(auto each:rootblock_size_psum){
            cout<<each<<' ';            
        }
        cout<<"\nrootblock size:\n";
        for(auto each:rootblock_size){
            cout<<each<<' ';            
        }
        cout<<"\nsuperblock index\n";
        for(auto& each:superblock_size_psum_vec){
            for(auto val:each){
                cout<<val<<' ';            
            }
        }
        cout<<"\nsuperblock size\n";
        for(auto& each:superblock_size_vec){
            for(auto val:each){
                cout<<val<<' ';            
            }
        }   
        cout<<'\n';     
    }

    void checkInvariant() const {
        auto root_psum = rootblock_size;
        std::partial_sum(root_psum.begin(),root_psum.end(),root_psum.begin());
        for(int i=0;i<rootblock_size.size();++i){
            assert(root_psum[i]==rootblock_size_psum[i]);
        }
        assert(rootblock_size.size()==superblock_size_vec.size());
        for(int i=0;i<superblock_size_vec.size();++i){
            assert(superblock_size_vec[i].size() <= MAX_SB_SIZE);
            auto to_psum = superblock_size_vec[i];
            std::partial_sum(to_psum.begin(),to_psum.end(),to_psum.begin());
            for(int j=0;j<to_psum.size();++j){
                assert(superblock_size_vec[i][j] <= MAX_BLOCK_SIZE);
                assert(to_psum[j]==superblock_size_psum_vec[i][j]);
            }
        }
    }
    auto getBlockInsertTime(){
        return data.time_cnt/1000000;
    }

    

private:    

    size_t total_size;    
    std::vector<index_t> rootblock_size_psum;
    std::vector<index_t> rootblock_size;
    std::vector<std::vector<index_t>> superblock_size_psum_vec;
    std::vector<std::vector<index_t>> superblock_size_vec;        
    Darray_Data<T,Ch,MAX_BLOCK_SIZE,MAX_SB_SIZE> data;

};

#endif