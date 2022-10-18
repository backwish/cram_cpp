#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <fstream>
#include <assert.h>
#include "darray_huffman.h"
using namespace std;
using namespace std::chrono;
using code_t = uint64_t;
using data_t = uint16_t;
using freq_t = int;
const int N = 100000;
template<int MAX_BLOCK_SIZE,int MAX_INTERNAL_BLOCK_SIZE,int H>
void darray_huffman_insert_erase_test_without_correctness_check(const auto& source,const auto& dest){
    Darray_Huffman<MAX_BLOCK_SIZE,MAX_INTERNAL_BLOCK_SIZE> da(source,H,true);    

    long long insert_time = 0,erase_time = 0;
    std::cout<<"INSERT TEST START\n";
    for(int i=0;i<N;++i){
        const int insert_pos = rand()%(da.size()+1);
        const int insert_ch_pos = rand()%dest.size();
        //const data_t insert_ch = dest[insert_ch_pos];        
        const data_t insert_ch = rand()%SIGMA;
        auto insert_start = steady_clock::now();
        da.insert(insert_pos,insert_ch);
        auto insert_end = steady_clock::now();
        insert_time+=duration_cast<nanoseconds> (insert_end - insert_start).count();        

        const int erase_pos = rand()%(da.size());        
        auto erase_start = steady_clock::now();        
        da.erase(erase_pos);
        auto erase_end = steady_clock::now();
        erase_time+=duration_cast<nanoseconds> (erase_end - erase_start).count();        
        //std::cout<<da.size()<<','<<erase_pos<<'\n';
    }
    std::cout<<"TOTAL INSERT TIME: "<<insert_time/1000000<<'\n';
    std::cout<<"TOTAL ERASE TIME: "<<erase_time/1000000<<'\n';    
    auto [block_insert_time,block_erase_time] = da.getBlockInsertEraseTime();
    std::cout<<"BLOCK INSERT TIME: "<<block_insert_time/1000000<<'\n';
    std::cout<<"BLOCK ERASE TIME: "<<block_erase_time/1000000<<'\n';    
    std::cout<<"TREE INSERT TIME: "<<(insert_time-block_insert_time)/1000000<<'\n';
    std::cout<<"TREE ERASE TIME: "<<(erase_time-block_erase_time)/1000000<<'\n';    
}

template<int MAX_BLOCK_SIZE,int MAX_INTERNAL_BLOCK_SIZE,int H>
void darray_huffman_insert_erase_test(const auto& source,const auto& dest){
    Darray_Huffman<MAX_BLOCK_SIZE,MAX_INTERNAL_BLOCK_SIZE> da(source,H,true);
    std::vector<data_t> raw_vector(source.size());
    std::copy(source.begin(),source.end(),raw_vector.begin());

    long long insert_time = 0,erase_time = 0;
    std::cout<<"INSERT TEST START\n";
    for(int i=0;i<N;++i){
        const int insert_pos = rand()%(da.size()+1);
        const data_t insert_ch = rand()%SIGMA;
        raw_vector.insert(raw_vector.begin()+insert_pos,insert_ch);
        auto insert_start = steady_clock::now();
        da.insert(insert_pos,insert_ch);
        auto insert_end = steady_clock::now();
        insert_time+=duration_cast<nanoseconds> (insert_end - insert_start).count();

        //std::cout<<da.size()<<','<<insert_pos<<'\n';

        const int erase_pos = rand()%(da.size());
        raw_vector.erase(raw_vector.begin()+erase_pos);
        auto erase_start = steady_clock::now();        
        da.erase(erase_pos);
        auto erase_end = steady_clock::now();
        erase_time+=duration_cast<nanoseconds> (erase_end - erase_start).count();        
        //std::cout<<da.size()<<','<<erase_pos<<'\n';
    }
    std::cout<<"INSERT TIME: "<<insert_time/1000000<<'\n';
    std::cout<<"ERASE TIME: "<<erase_time/1000000<<'\n';
    std::cout<<"CORRECTNESS CHECK\n";
    int pos = 0;
    while(pos < da.size()){
        auto da_block = da.block_at(pos);
        int block_size = da_block.size();
        assert(block_size < MAX_BLOCK_SIZE);
        std::vector<data_t> raw_block(block_size);
        std::copy(raw_vector.begin()+pos,raw_vector.begin()+(pos+block_size),raw_block.begin());
        for(int j=0;j<block_size;++j){
            if(da_block[j]!=raw_block[j]){
                std::cout<<pos<<','<<j<<'\n';
                assert(false);
            }            
        }
        pos+=block_size;
    }
}
template<int MAX_BLOCK_SIZE,int MAX_INTERNAL_BLOCK_SIZE,int H>
void darray_huffman_insert_test(const auto& source){
    Darray_Huffman<MAX_BLOCK_SIZE,MAX_INTERNAL_BLOCK_SIZE> da(source,H,false);
    int N = source.size()/2048;
    std::cout<<"TEST SIZE: "<<N<<'\n';
    //int N = 1000;
    std::vector<data_t> raw_vector;    
    std::cout<<"INSERT FROM EMPTY START\n";
    long long insert_time = 0,erase_time = 0;
    for(int i=0;i<N;++i){
        //int pos = rand()%(raw_vector.size()+1);
        int pos = raw_vector.size();
        data_t ch = source[rand()%N];
        auto insert_start = steady_clock::now();
        da.insert(pos,ch);
        auto insert_end = steady_clock::now();
        insert_time+=duration_cast<nanoseconds> (insert_end - insert_start).count();
        raw_vector.insert(raw_vector.begin()+pos,ch);        
    }
    std::cout<<"INSERT TIME: "<<insert_time/1000000<<'\n';
    std::cout<<"CORRECTNESS CHECK\n";
    int pos = 0;    
    while(pos < da.size()){
        auto da_block = da.block_at(pos);
        int block_size = da_block.size();
        assert(block_size < MAX_BLOCK_SIZE);
        std::vector<data_t> raw_block(block_size);
        std::copy(raw_vector.begin()+pos,raw_vector.end()+(pos+block_size),raw_block.begin());
        if(!std::equal(da_block.begin(),da_block.end(),raw_block.begin(),raw_block.end())){
            std::cout<<"not equal at: "<<pos<<" ,block size: "<<block_size<<'\n';
            assert(false);
        }
        pos+=block_size;
    }
    std::cout<<"CORRECTNESS CHECK END\n";
}
/*void darray_huffman_erase_test(const auto& source){
    Darray_Huffman<MAX_BLOCK_SIZE,MAX_INTERNAL_BLOCK_SIZE> da(source,H,true);
    std::vector<data_t> raw_vector(source.size());
    std::copy(source.begin(),source.end(),raw_vector.begin());    

    for(int i=0;i<N;++i){
        const int insert_pos = rand()%(da.size()+1);
        const data_t insert_ch = rand()%SIGMA;
        raw_vector.insert(raw_vector.begin()+insert_pos,insert_ch);
        auto insert_start = steady_clock::now();
        da.insert(insert_pos,insert_ch);
        auto insert_end = steady_clock::now();
        insert_time+=duration_cast<nanoseconds> (insert_end - insert_start).count();

        //std::cout<<da.size()<<','<<insert_pos<<'\n';

        const int erase_pos = rand()%(da.size());
        raw_vector.erase(raw_vector.begin()+erase_pos);
        auto erase_start = steady_clock::now();        
        da.erase(erase_pos);
        auto erase_end = steady_clock::now();
        erase_time+=duration_cast<nanoseconds> (erase_end - erase_start).count();        
        //std::cout<<da.size()<<','<<erase_pos<<'\n';
    }
    std::cout<<"INSERT TIME: "<<insert_time<<'\n';
    std::cout<<"ERASE TIME: "<<erase_time<<'\n';
    std::cout<<"CORRECTNESS CHECK\n";
    int pos = 0;
    while(pos < da.size()){
        auto da_block = da.block_at(pos);
        int block_size = da_block.size();
        assert(block_size < MAX_BLOCK_SIZE);
        std::vector<data_t> raw_block(block_size);
        std::copy(raw_vector.begin()+pos,raw_vector.begin()+(pos+block_size),raw_block.begin());
        for(int j=0;j<block_size;++j){
            if(da_block[j]!=raw_block[j]){
                std::cout<<pos<<','<<j<<'\n';
                assert(false);
            }            
        }
        pos+=block_size;
    }
}*/


int main(int argc,char **argv){
    std::ifstream is_source(argv[1],std::ios::binary);
    is_source.seekg(0, std::ios_base::end);
    std::size_t size=is_source.tellg();
    is_source.seekg(0, std::ios_base::beg);
    std::vector<data_t> source(size/sizeof(data_t));
    is_source.read((char*) &source[0], size);
    is_source.close();

    std::ifstream is_dest(argv[2],std::ios::binary);
    is_dest.seekg(0, std::ios_base::end);
    size=is_dest.tellg();
    is_dest.seekg(0, std::ios_base::beg);
    std::vector<data_t> dest(size/sizeof(data_t));
    is_dest.read((char*) &dest[0], size);
    is_dest.close();

    //darray_huffman_insert_test<64,2048,2>(source);
    std::cout<<"insert and erase test\n";
    darray_huffman_insert_erase_test_without_correctness_check<1024,2048,2>(source,dest);
    darray_huffman_insert_erase_test_without_correctness_check<1024,64,4>(source,dest);    
    //darray_huffman_insert_erase_test_without_correctness_check<64,2048,2>(source,dest);
    //darray_huffman_insert_erase_test_without_correctness_check<64,64,4>(source,dest);        
    //darray_huffman_insert_erase_test<1024,64,4>(source,dest);    
    //darray_huffman_insert_erase_test<1024,16,6>(source,dest);
    //darray_huffman_insert_erase_test<1024,2048,1>(source,dest);
}