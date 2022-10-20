#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <fstream>
#include <assert.h>
#include "cram_with_insert.h"
using namespace std;
using namespace std::chrono;
using code_t = uint64_t;
using data_t = uint16_t;
using freq_t = int;
const int SIGMA = 65536;

template<typename T,typename Ch,int MODE,int MAX_BLOCK_SIZE = 1024,int MAX_INTERNAL_BLOCK_SIZE = 1024>
auto cram_insert_test(const auto& source,const auto& dest,const int rewrite_blocks,const int H){
    assert(source.size()%(MAX_BLOCK_SIZE/2) == 0);
    cout<<"CRAM BUILD START\n";
    CRAM<T,Ch,MODE,MAX_BLOCK_SIZE,MAX_INTERNAL_BLOCK_SIZE> cram(source,rewrite_blocks,H);
    cout<<"CRAM BUILD END\n";
    std::vector<data_t> raw_vector{source.begin(),source.end()};        
    std::vector<double> ret;

    
    auto [node_bpc,block_bpc] = cram.get_bpc();
    const int N = dest.size();    
    cout<<"PROGRESS: "<<0<<" CRAM NODE BPC: "<<node_bpc<<"CRAM BLOCK BPC: "<<block_bpc<<'\n';
    ret.push_back(node_bpc+block_bpc);
    
    long long insert_time = 0;

    for(int i=0;i<N;++i){        
        auto ch = dest[rand()%N];
        auto pos = rand()%(cram.size()+1);
        
        auto insert_start = steady_clock::now();
        cram.insert(pos,ch);        
        auto insert_end = steady_clock::now();
        //raw_vector.insert(raw_vector.begin()+pos,ch);
        insert_time += duration_cast<nanoseconds>(insert_end - insert_start).count();
        if((i+1)%(N/10)==0){
            auto [node_bpc,block_bpc] = cram.get_bpc();
            cout<<"PROGRESS: "<<(i+1)/(N/10)*10<<" CRAM NODE BPC: "<<node_bpc<<"CRAM BLOCK BPC: "<<block_bpc<<'\n';            
            ret.push_back(node_bpc+block_bpc);
        }
    }    
    cout << "CRAM INSERT Time difference = " << insert_time/1000000 << "[ms]\n";


    /*cout<<"CORRECTNESS CHECK\n";
    for(int i=0;i<2*N;){        
        auto cram_block = cram.get_block(i);
        const int block_size = cram_block.size();
        std::vector<data_t> raw_block{raw_vector.begin()+i,raw_vector.begin()+(i+block_size)};
        for(int j=0;j<block_size;++j){
            if(raw_block[j]!=cram_block[j]){
                std::cout<<"incorrect! "<<i<<','<<j<<std::endl;
                assert(false);
            }
        }
        i+=block_size;
    }*/
    return ret;
}

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

    std::ofstream out("test.csv");
    cout<<"CRAM REPLACE TEST\n";

    const int N = source.size();
    const int DIVIDE = 100;
    const std::vector<data_t> mini_source{source.begin(),source.begin()+N/DIVIDE};
    const std::vector<data_t> mini_dest{dest.begin(),dest.begin()+N/DIVIDE};
    int rewrite_blocks = 4;        
    //auto ret = cram_insert_test<code_t,data_t,0,1024,2048>(mini_source,mini_dest,rewrite_blocks,2);
    cout<<"CH SIZE: "<<mini_source.size()<<'\n';
    for(int H=1;H<=4;H*=2){
        for(int rewrite_blocks=4;rewrite_blocks>=1;rewrite_blocks/=2){
            cout<<"H: "<<H<<" BU: "<<rewrite_blocks<<'\n';
            std::vector<double> ret;
            if(H==4) ret = cram_insert_test<code_t,data_t,0,1024,32>(mini_source,mini_dest,rewrite_blocks,H);
            else ret = cram_insert_test<code_t,data_t,0,1024,2048>(mini_source,mini_dest,rewrite_blocks,H);            
            for(auto col:ret) out<<col<<',';
            out<<'\n';
        }   
    }

    /*for(int rewrite_blocks=4;rewrite_blocks>=1;rewrite_blocks/=2){
        auto ret = cram_replace_test<code_t,data_t,0,1024,2048>(source,dest,rewrite_blocks,2);
        for(auto col:ret) out<<col<<',';
        out<<'\n';
    }*/
    /*for(int rewrite_blocks=4;rewrite_blocks>=1;rewrite_blocks/=2){
        auto ret = cram_replace_test<code_t,data_t,1,1024,2048>(source,dest,rewrite_blocks,2);
        for(auto col:ret) out<<col<<',';
        out<<'\n';
    }
    for(int rewrite_blocks=4;rewrite_blocks>=1;rewrite_blocks/=2){
        auto ret = cram_replace_test<code_t,data_t,2,1024,2048>(source,dest,rewrite_blocks,2);
        for(auto col:ret) out<<col<<',';
        out<<'\n';
    }*/
    
}