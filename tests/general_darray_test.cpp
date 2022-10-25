#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <array>
#include <fstream>
#include <assert.h>
#include "general_darray.h"
#include "block.h"
using namespace std;
using code_t = int;
using data_t = int;
using huff_code_t = uint64_t;
using huff_ch_t = uint16_t;
using naiveblock_t = NaiveBlock<code_t,data_t>;
using huffmanblock_t = HuffmanBlock<huff_code_t,huff_ch_t>;
using huffman_encoder_t = HuffmanEncoder<>;
using encoder_t = NaiveEncoder;
NaiveEncoder ne;
void da_init_test(){
    Darray<code_t,data_t,naiveblock_t,encoder_t,2,2> da(4);
    da.traversal();
}
template<int N,int B,int SB>
void da_insert_test(const int H,const bool back){
    Darray<code_t,data_t,naiveblock_t,encoder_t,B,SB> da(H);
    std::vector<int> raw_data;
    for(int i=0;i<N;++i){
        int val = rand()%10000;
        int pos = back ? i : 0;
        raw_data.insert(raw_data.begin()+pos,val);
        da.insert(pos,val,ne);
        for(int j=0;j<raw_data.size();++j){
            if(raw_data[j]!=da.at(j,ne)){
                std::cout<<"incorrect at front or back insert H: "<<H<<" B: "<<B<<" SB: "<<SB<<" back? "<<(back ? "yes\n" : "no\n");
                assert(false);
            }
        }
        /*da.traversal();
        cout<<'\n';*/
    }
    for(int i=0;i<N;++i){
        int val = rand()%10000;
        int pos = rand()%(raw_data.size()+1);
        raw_data.insert(raw_data.begin()+pos,val);
        da.insert(pos,val,ne);
        for(int j=0;j<raw_data.size();++j){
            if(raw_data[j]!=da.at(j,ne)){
                std::cout<<"incorrect at random insert H: "<<H<<" B: "<<B<<" SB: "<<SB<<" back? "<<(back ? "yes\n" : "no\n");
                assert(false);
            }
        }
    }
}
template<int B,int SB>
void da_erase_test(const int N,const int H){
    Darray<code_t,data_t,naiveblock_t,encoder_t,B,SB> da(H);
    std::vector<int> raw_data;
    for(int i=0;i<N;++i){
        int val = rand();
        int pos = 0;
        raw_data.insert(raw_data.begin()+pos,val);
        da.insert(pos,val,ne);        
    }   
    /*da.traversal();        
    cout<<'\n'; */
    for(int i=0;i<N;++i){
        int val = rand()%10000;
        int pos = rand()%(raw_data.size());        
        raw_data.erase(raw_data.begin()+pos);        
        da.erase(pos,ne);        
        /*da.traversal();        
        cout<<'\n';*/
        for(int j=0;j<raw_data.size();++j){            
            if(raw_data[j]!=da.at(j,ne)){
                std::cout<<"incorrect at random erase H: "<<H<<" B: "<<B<<" SB: "<<SB<<'\n';
                assert(false);
            }
        }                
    }
}
void da_insert_small_test(){    
    const int N = 100,B = 2,SB = 2,MAX_H = 4;
    for(int h=1;h<=MAX_H;++h){
        for(int b=0;b<=1;++b){
            da_insert_test<N,B,SB>(h,static_cast<bool>(b));
        }
    }    
}
void da_insert_large_test(){
    const int N = 10000,B = 10,SB = 10,MAX_H = 4;
    for(int h=1;h<=MAX_H;++h){
        for(int b=0;b<=1;++b){
            da_insert_test<N,B,SB>(h,static_cast<bool>(b));
        }
    }
}
void da_erase_small_test(){
    const int N = 100,B = 4,SB = 4,MAX_H = 4;
    for(int h=1;h<=MAX_H;++h){
        da_erase_test<B,SB>(N,h);
    }    
}
void da_erase_large_test(){
    const int N = 10000,B = 20,SB = 20,MAX_H = 4;
    for(int h=1;h<=MAX_H;++h){
        da_erase_test<B,SB>(N,h);
    }
}
auto make_naive_blocks(const auto& text,const int B){
    int n = text.size();
    std::vector<std::unique_ptr<naiveblock_t>> ret;
    for(int i=0;i<n;i+=B){
        int curr_block_size = std::min(n-i,B);
        std::vector<int> curr_block(curr_block_size);
        std::copy(text.begin()+i,text.begin()+(i+curr_block_size),curr_block.begin());
        auto block_ptr = std::make_unique<naiveblock_t>(std::move(curr_block));
        ret.push_back(std::move(block_ptr));
    }
    return std::move(ret);
}
void da_naiveblock_bulk_insert_test(){
    const int N = 10000,B = 20,SB = 20,H = 4;
    std::vector<int> text(N);
    std::generate(text.begin(),text.end(),std::rand);
    auto naive_blocks_ptr_vec = make_naive_blocks(text,B);
    Darray<int,int,naiveblock_t,encoder_t,B,SB> da(std::move(naive_blocks_ptr_vec),H);
    for(int i=0;i<N;++i){
        assert(text[i]==da.at(i,ne));
    }
}
void da_naiveblock_bulk_erase_test(){
    const int N = 1000,B = 16,SB = 16,H = 4;
    std::vector<int> text(N);
    std::generate(text.begin(),text.end(),std::rand);
    auto naive_blocks_ptr_vec = make_naive_blocks(text,B);
    Darray<int,int,naiveblock_t,encoder_t,B,SB> da(std::move(naive_blocks_ptr_vec),H);
    for(int i=0;i<N;++i){
        int val = rand()%10000;
        int pos = rand()%(text.size());        
        text.erase(text.begin()+pos);        
        da.erase(pos,ne);                
        for(int j=0;j<text.size();++j){            
            if(text[j]!=da.at(j,ne)){
                std::cout<<"incorrect at random erase H: "<<H<<" B: "<<B<<" SB: "<<SB<<'\n';
                assert(false);
            }
        }                
    }
}


int main(){
    /*cout<<"DA NAIVEBLOCK BULK INSERT TEST\n";
    da_naiveblock_bulk_insert_test();
    cout<<"DA NAIVEBLOCK BULKD ERASE TEST\n";
    da_naiveblock_bulk_erase_test();*/
    cout<<"DA ERASE SMALL SIZE TEST\n";
    da_erase_small_test();    
    cout<<"DA INSERT SMALL SIZE TEST\n";
    da_insert_small_test();
    cout<<"DA ERASE LARGE SIZE TEST\n";
    da_erase_large_test();
    cout<<"DA INSERT LARGE SIZE TEST\n";
    da_insert_large_test();
}