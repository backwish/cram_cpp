#ifndef GENERAL_DARRAY_H
#define GENERAL_DARRAY_H

#include "block.h"
#include <iostream>
#include <memory>
#include <queue>
#include <stack>
#include <vector>
using std::cout;
using std::cin;
using namespace std;
using namespace std::chrono;

template<typename T,typename Ch,typename index_t,typename block_t>
class Node{
public:
    using node_t = Node<T,Ch,index_t,block_t>;
    explicit Node(bool leaf) : is_leaf(leaf){}
    Node(bool leaf,int size) : is_leaf(leaf),size_vec(1,1),size_psum_vec(1,1){}    
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    Node(node_t&&) noexcept = default;
    Node& operator=(node_t&&) noexcept = default;
    auto get_bytes(){
        size_t ret = 0;
        ret+=size_vec.size()*sizeof(decltype(size_vec.back()));
        ret+=size_psum_vec.size()*sizeof(decltype(size_psum_vec.back()));
        ret+=(is_leaf ? block_ptr_vec.size()*sizeof(decltype(block_ptr_vec.back())) : 
        child_vec.size()*sizeof(decltype(size_vec.back())));
        return ret;
    }
    auto split(index_t pos){
        index_t lsize = pos,rsize = size_vec.size() - pos;
        Node lnode(is_leaf),rnode(is_leaf);
        lnode.size_vec.resize(lsize);lnode.size_psum_vec.resize(lsize);
        rnode.size_vec.resize(rsize);rnode.size_psum_vec.resize(rsize);
        std::copy(size_vec.cbegin(),size_vec.cbegin()+lsize,lnode.size_vec.begin());
        std::copy(size_vec.cbegin()+lsize,size_vec.cend(),rnode.size_vec.begin());
        std::inclusive_scan(lnode.size_vec.cbegin(),lnode.size_vec.cend(),lnode.size_psum_vec.begin());
        std::inclusive_scan(rnode.size_vec.cbegin(),rnode.size_vec.cend(),rnode.size_psum_vec.begin());
        if(is_leaf){
            lnode.block_ptr_vec.insert(lnode.block_ptr_vec.end(),
            std::make_move_iterator(block_ptr_vec.begin()),std::make_move_iterator(block_ptr_vec.begin()+lsize));
            rnode.block_ptr_vec.insert(rnode.block_ptr_vec.end(),
            std::make_move_iterator(block_ptr_vec.begin()+lsize),std::make_move_iterator(block_ptr_vec.end()));
        }else{
            lnode.child_vec.insert(lnode.child_vec.end(),
            std::make_move_iterator(child_vec.begin()),std::make_move_iterator(child_vec.begin()+lsize));
            rnode.child_vec.insert(rnode.child_vec.end(),
            std::make_move_iterator(child_vec.begin()+lsize),std::make_move_iterator(child_vec.end()));
        }
        return std::make_pair(std::move(lnode),std::move(rnode));
    }
    void merge(std::unique_ptr<node_t> rhs){
        size_vec.insert(size_vec.end(),rhs->size_vec.begin(),rhs->size_vec.end());
        size_psum_vec.resize(size_vec.size());
        std::inclusive_scan(size_vec.begin(),size_vec.end(),size_psum_vec.begin());
        if(is_leaf){
            block_ptr_vec.insert(block_ptr_vec.end(),
            std::make_move_iterator(rhs->block_ptr_vec.begin()),
            std::make_move_iterator(rhs->block_ptr_vec.end()));
        }else{
            child_vec.insert(child_vec.end(),
            std::make_move_iterator(rhs->child_vec.begin()),
            std::make_move_iterator(rhs->child_vec.end()));
        }
    }
public:    
    bool is_leaf;
    std::vector<index_t> size_vec;
    std::vector<index_t> size_psum_vec;
    std::vector<std::unique_ptr<node_t>> child_vec;
    std::vector<std::unique_ptr<block_t>> block_ptr_vec;
    //friend class Darray;

};

template<typename T,typename Ch,typename block_t,typename encoder_t,int MAX_BLOCK_SIZE = 1024,int MAX_INTERNAL_BLOCK_SIZE = 2048>
class Darray{
public:
    using index_t = int;
    using node_t = Node<T,Ch,index_t,block_t>;
    Darray(const auto& text,const auto& encoder,const int H) : da_size(0),insert_time(0),erase_time(0){                
        auto input_block_ptr_vec = make_blocks(text,encoder);
        const int n = input_block_ptr_vec.size();        
        const index_t INTERNAL_BLOCK_SIZE = H==1 ? n : MAX_INTERNAL_BLOCK_SIZE/2;
        const index_t BLOCK_SIZE = MAX_BLOCK_SIZE/2;
        da_size = n*BLOCK_SIZE;
        std::queue<std::unique_ptr<node_t>> node_queue;
        for(int i=0;i<n;i+=INTERNAL_BLOCK_SIZE){
            auto internal_block_size = std::min(n-i,INTERNAL_BLOCK_SIZE);  
            bool last = n <= i+INTERNAL_BLOCK_SIZE;
            std::vector<index_t> size_vec(internal_block_size);
            std::vector<index_t> size_psum_vec(internal_block_size);
            std::vector<std::unique_ptr<block_t>> block_ptr_vec;            
            for(int j=0;j<internal_block_size;++j){                
                auto block_ptr = std::move(input_block_ptr_vec[i+j]);                
                size_vec[j] = block_ptr->size();                
                block_ptr_vec.push_back(std::move(block_ptr));
            }
            //if(last) size_vec.back()++;
            std::inclusive_scan(size_vec.begin(),size_vec.end(),size_psum_vec.begin());
            auto node = std::make_unique<node_t>(true);
            node->size_vec = std::move(size_vec);
            node->size_psum_vec = std::move(size_psum_vec);
            node->block_ptr_vec = std::move(block_ptr_vec);
            node_queue.push(std::move(node));
        }
        for(int h=1;h<H;++h){
            const int internal_node_num = node_queue.size();
            const index_t INTERNAL_BLOCK_SIZE = h==H-1 ? internal_node_num : MAX_INTERNAL_BLOCK_SIZE/2;
            std::queue<std::unique_ptr<node_t>> upper_node_queue;
            for(int i=0;i<internal_node_num;i+=INTERNAL_BLOCK_SIZE){
                auto internal_block_size = std::min(internal_node_num-i,INTERNAL_BLOCK_SIZE);
                std::vector<index_t> size_vec(internal_block_size);
                std::vector<index_t> size_psum_vec(internal_block_size);
                std::vector<std::unique_ptr<node_t>> child_vec;
                for(int j=0;j<internal_block_size;++j){
                    auto node_ptr = std::move(node_queue.front());
                    node_queue.pop();
                    size_vec[j] = node_ptr->size_psum_vec.back();
                    child_vec.push_back(std::move(node_ptr));
                }
                std::inclusive_scan(size_vec.begin(),size_vec.end(),size_psum_vec.begin());
                auto node = std::make_unique<node_t>(false);
                node->size_vec = std::move(size_vec);
                node->size_psum_vec = std::move(size_psum_vec);
                node->child_vec = std::move(child_vec);
                upper_node_queue.push(std::move(node));
            }
            node_queue = std::move(upper_node_queue);
        }
        root = std::move(node_queue.front());
    }
    explicit Darray(const int H) : root(std::make_unique<node_t>(H==1,1)), da_size(0),insert_time(0),erase_time(0){
        node_t *curr = root.get();
        for(int depth=2;depth<=H;++depth){
            auto first_child = std::make_unique<node_t>(depth==H,1);
            curr->child_vec.push_back(std::move(first_child));
            curr = curr->child_vec.back().get();
        }
        auto first_block = std::make_unique<block_t>(1);
        curr->block_ptr_vec.push_back(std::move(first_block));
    }
    Darray() = default;
    
    //method for fixed-size CRAM
    auto block_at(index_t pos,const auto& encoder) const {
        auto [node_stack,child_pos_stack,block_pos,last_block] = getPos(pos);
        auto leaf = node_stack.top();
        auto leaf_pos = child_pos_stack.top();
        auto ret = leaf->block_ptr_vec[leaf_pos]->get(encoder);
        if(last_block) ret.pop_back();
        return ret;
    }
    auto block_replace(index_t pos,const auto& vec,const auto& encoder){
        auto [node_stack,child_pos_stack,block_pos,last_block] = getPos(pos);
        auto leaf = node_stack.top();
        auto leaf_pos = child_pos_stack.top();
        auto modify_vec = vec;
        if(last_block) modify_vec.push_back(0);
        const auto input_vec = modify_vec;
        leaf->block_ptr_vec[leaf_pos]->replace_block(input_vec,encoder);
    }
    //end methods
    auto get_bpc() const {
        size_t block_num = 0;
        size_t block_bytes = 0;        
        size_t node_bytes = 0;
        std::queue<node_t*> q;
        q.push(root.get());
        while(!q.empty()){
            auto here = q.front();            
            q.pop();
            node_bytes+=here->get_bytes();
            if(here->is_leaf){
                for(int i=0;i<here->block_ptr_vec.size();++i){
                    auto block_ptr = here->block_ptr_vec[i].get();
                    block_num+=block_ptr->size();
                    block_bytes+=block_ptr->use_bytes();
                }
            }
            for(int i=0;i<here->child_vec.size();++i){
                auto child = here->child_vec[i].get();
                q.push(child);
            }
        }
        auto node_bpc = static_cast<double>(node_bytes*4)/block_num;
        auto block_bpc = static_cast<double>(block_bytes*4)/block_num;
        return std::make_pair(node_bpc,block_bpc);
    }
    auto at(index_t pos,const auto& encoder) const{
        auto [node_stack,child_pos_stack,block_pos,last_block] = getPos(pos);
        auto leaf = node_stack.top();
        auto leaf_pos = child_pos_stack.top();
        return leaf->block_ptr_vec[leaf_pos]->at(block_pos,encoder);        
    }
    void insert(index_t pos,auto val,const auto& encoder){
        auto [node_stack,child_pos_stack,block_pos,last_block] = getPos(pos);
        auto leaf = node_stack.top();
        auto leaf_pos = child_pos_stack.top();
        auto insert_start = steady_clock::now();
        leaf->block_ptr_vec[leaf_pos]->insert(block_pos,val,encoder);
        auto insert_end = steady_clock::now();
        insert_time+=duration_cast<nanoseconds> (insert_end - insert_start).count();
        auto node_stack_temp = node_stack;
        auto child_pos_stack_temp = child_pos_stack;
        while(!node_stack_temp.empty()){
            auto node = node_stack_temp.top();node_stack_temp.pop();
            auto pos = child_pos_stack_temp.top();child_pos_stack_temp.pop();
            modifyPartialSum(node->size_psum_vec,pos,1);
            node->size_vec[pos]++;            
        }
        
        if(MAX_BLOCK_SIZE < leaf->block_ptr_vec[leaf_pos]->size()){
            splitDataBlock(leaf,leaf_pos,encoder);            
        }
        while(1 < node_stack.size()){
            auto node = node_stack.top();node_stack.pop();
            auto pos = child_pos_stack.top();child_pos_stack.pop();
            if(node->size_vec.size() <= MAX_INTERNAL_BLOCK_SIZE){
                break;
            }
            auto parent_node = node_stack.top();
            auto parent_pos = child_pos_stack.top();
            splitNode(parent_node,parent_pos);            
        }
        ++da_size;
    }    
    void erase(index_t pos,const auto& encoder){
        auto [node_stack,child_pos_stack,block_pos,last_block] = getPos(pos);
        auto leaf = node_stack.top();
        auto leaf_pos = child_pos_stack.top();
        auto erase_start = steady_clock::now();        
        leaf->block_ptr_vec[leaf_pos]->erase(block_pos,encoder);
        auto erase_end = steady_clock::now();
        erase_time+=duration_cast<nanoseconds> (erase_end - erase_start).count();        
        auto node_stack_temp = node_stack;
        auto child_pos_stack_temp = child_pos_stack;
        //std::cout<<pos<<','<<block_pos<<'\n';
        while(!node_stack_temp.empty()){
            auto node = node_stack_temp.top();node_stack_temp.pop();
            auto pos = child_pos_stack_temp.top();child_pos_stack_temp.pop();
            modifyPartialSum(node->size_psum_vec,pos,-1);
            node->size_vec[pos]--;
        }
        if(leaf->block_ptr_vec[leaf_pos]->size() <= MAX_BLOCK_SIZE/4 && 1 < leaf->block_ptr_vec.size()){
            auto left_pos = (leaf_pos==0 ? leaf_pos : leaf_pos-1);
            auto right_pos = (leaf_pos==0 ? leaf_pos+1 : leaf_pos);
            mergeDataBlock(leaf,left_pos,right_pos,encoder);
            if(MAX_BLOCK_SIZE < leaf->block_ptr_vec[left_pos]->size()){
                splitDataBlock(leaf,left_pos,encoder);
            }
        }
        while(1 < node_stack.size()){
            auto node = node_stack.top();node_stack.pop();
            auto pos = child_pos_stack.top();child_pos_stack.pop();
            if(MAX_INTERNAL_BLOCK_SIZE/4 < node->size_vec.size()){
                break;
            }
            auto parent_node = node_stack.top();
            auto parent_pos = child_pos_stack.top();
            if(parent_node->size_vec.size() <= 1){
                break;
            }
            auto left_pos = (parent_pos==0 ? parent_pos : parent_pos-1);
            auto right_pos = (parent_pos==0 ? parent_pos+1 : parent_pos);
            mergeNode(parent_node,left_pos,right_pos);
            if(MAX_INTERNAL_BLOCK_SIZE < parent_node->child_vec[left_pos]->size_vec.size()){
                splitNode(parent_node,left_pos);
            }
        }
        --da_size;
    }
    auto size() const {return da_size;}
    auto getBlockInsertEraseTime() const{return std::make_pair(insert_time,erase_time);}
    void traversal() const {
        std::queue<std::tuple<node_t*,int,int>> q;
        q.push(std::make_tuple(root.get(),0,0));
        while(!q.empty()){
            auto [here,depth,kth] = q.front();            
            q.pop();
            cout<<"depth: "<<depth<<" ,kth node: "<<kth<<'\n';
            for(auto val:here->size_vec) cout<<val<<' ';
            cout<<'\n';
            for(auto val:here->size_psum_vec) cout<<val<<' ';
            cout<<'\n';
            std::vector<size_t> size_value_vec{here->size_vec.size(),here->size_psum_vec.size()};
            size_value_vec.push_back(here->is_leaf ? here->block_ptr_vec.size() : here->child_vec.size());
            if(!std::all_of(size_value_vec.begin(),size_value_vec.end(),[&](int i){return i==size_value_vec[0];})){
                cout<<"not equal vector sizes\n";
                for(auto val:size_value_vec) cout<<val<<' ';
                cout<<'\n';
                assert(false);
            }
            for(int i=0;i<here->child_vec.size();++i){
                auto child = here->child_vec[i].get();
                q.push(std::make_tuple(child,depth+1,i));
            }
        }
    }
    
private:
    auto make_blocks(const auto& text,const auto& encoder) const{
        const int n = text.size();
        const int block_size = MAX_BLOCK_SIZE/2;
        std::vector<std::unique_ptr<block_t>> ret;
        for(int i=0;i<n;i+=block_size){
            int curr_block_size = std::min(n-i,block_size);
            std::vector<Ch> curr_block(curr_block_size);
            std::copy(text.begin()+i,text.begin()+(i+curr_block_size),curr_block.begin());
            if(i+block_size>=n) curr_block.push_back(0);
            auto block_ptr = std::make_unique<block_t>(std::move(curr_block),encoder);
            ret.push_back(std::move(block_ptr));
        }
        return std::move(ret);
    }
    auto getPos(index_t total_pos) const {
        node_t *curr = root.get();
        int block_pos = -1;
        bool last_block = true;
        std::stack<node_t*> node_stack;
        std::stack<index_t> child_pos_stack;
        while(true){
            auto psum_block_it = std::upper_bound(curr->size_psum_vec.cbegin(),curr->size_psum_vec.cend(),total_pos);
            auto pos = std::distance(curr->size_psum_vec.cbegin(),psum_block_it);
            auto offset = pos==0 ? 0 : curr->size_psum_vec[pos-1];  
            last_block &= std::next(psum_block_it) == curr->size_psum_vec.cend();
            total_pos-=offset;
            node_stack.push(curr);
            child_pos_stack.push(pos);            
            if(curr->is_leaf){
                block_pos = total_pos;                
                break;
            }else{
                curr = curr->child_vec[pos].get();
            }
        }        
        assert(block_pos>=0);
        return std::make_tuple(std::move(node_stack),std::move(child_pos_stack),block_pos,last_block);
    }
    void splitDataBlock(auto leaf,index_t leaf_pos,const auto& encoder){
        index_t left_size = MAX_BLOCK_SIZE/2+1,right_size = leaf->block_ptr_vec[leaf_pos]->size() - left_size;
        auto [left_block,right_block] = leaf->block_ptr_vec[leaf_pos]->split(left_size,encoder);
        leaf->block_ptr_vec.erase(leaf->block_ptr_vec.begin()+leaf_pos);
        leaf->block_ptr_vec.insert(leaf->block_ptr_vec.begin()+leaf_pos,std::make_unique<block_t>(right_block));
        leaf->block_ptr_vec.insert(leaf->block_ptr_vec.begin()+leaf_pos,std::make_unique<block_t>(left_block));
        leaf->size_vec[leaf_pos] = right_size;
        leaf->size_vec.insert(leaf->size_vec.begin()+leaf_pos,left_size);
        auto offset = leaf_pos == 0 ? static_cast<index_t>(0) : leaf->size_psum_vec[leaf_pos-1];
        leaf->size_psum_vec.insert(leaf->size_psum_vec.begin()+leaf_pos,offset+left_size);
    }
    void splitNode(auto parent_node,auto parent_pos){
        index_t lsize = MAX_INTERNAL_BLOCK_SIZE/2+1,rsize = MAX_INTERNAL_BLOCK_SIZE;
        auto node = parent_node->child_vec[parent_pos].get();
        auto [lnode,rnode] = node->split(lsize);
        index_t lnode_total_size = lnode.size_psum_vec.back();
        index_t rnode_total_size = rnode.size_psum_vec.back();        
        parent_node->size_vec[parent_pos] = rnode_total_size;
        parent_node->size_vec.insert(parent_node->size_vec.begin()+parent_pos,lnode_total_size);
        auto offset = parent_pos == 0 ? static_cast<index_t>(0) : parent_node->size_psum_vec[parent_pos-1];
        parent_node->size_psum_vec.insert(parent_node->size_psum_vec.begin()+parent_pos,offset+lnode_total_size);
        parent_node->child_vec.erase(parent_node->child_vec.begin()+parent_pos);
        parent_node->child_vec.insert(parent_node->child_vec.begin()+parent_pos,std::make_unique<node_t>(std::move(rnode)));
        parent_node->child_vec.insert(parent_node->child_vec.begin()+parent_pos,std::make_unique<node_t>(std::move(lnode)));
    }    
    void mergeDataBlock(auto leaf,index_t left_pos,index_t right_pos,const auto& encoder){
        auto right_block = std::move(leaf->block_ptr_vec[right_pos]);        
        leaf->block_ptr_vec[left_pos]->merge(std::move(right_block),encoder,encoder);
        leaf->block_ptr_vec.erase(leaf->block_ptr_vec.begin()+right_pos);
        leaf->size_vec[right_pos] += leaf->size_vec[left_pos];
        leaf->size_vec.erase(leaf->size_vec.begin()+left_pos);
        leaf->size_psum_vec.erase(leaf->size_psum_vec.begin()+left_pos);
    }
    void mergeNode(auto parent_node,auto left_pos,auto right_pos){        
        //cout<<"mergenode start "<<left_pos<<','<<right_pos<<'\n';
        auto left_node = std::move(parent_node->child_vec[left_pos]);
        auto right_node = std::move(parent_node->child_vec[right_pos]);        
        left_node->merge(std::move(right_node));
        //cout<<"merged to left node\n";
        index_t left_cumulative = left_node->size_psum_vec.back();
        
        parent_node->child_vec.erase(parent_node->child_vec.begin()+right_pos);
        parent_node->child_vec[left_pos] = std::move(left_node);
        
        parent_node->size_vec[right_pos] = left_cumulative;
        parent_node->size_vec.erase(parent_node->size_vec.begin()+left_pos);
        auto offset = left_pos == 0 ? static_cast<index_t>(0) : parent_node->size_psum_vec[left_pos-1];
        parent_node->size_psum_vec[right_pos] = offset+left_cumulative;
        parent_node->size_psum_vec.erase(parent_node->size_psum_vec.begin()+left_pos);
        //cout<<"mergenode end\n";
    }
    void modifyPartialSum(auto& vec,int pos,int val){
        std::transform(vec.begin()+pos,vec.end(),
        vec.begin()+pos,[val](auto i){return i+val;});
    }
    std::unique_ptr<node_t> root;
    index_t da_size;
    long long insert_time = 0,erase_time = 0;
};

#endif 