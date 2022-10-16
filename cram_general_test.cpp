#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <fstream>
#include <assert.h>
#include "cram_general.h"
using namespace std;
using namespace std::chrono;
using code_t = uint64_t;
using data_t = uint16_t;
using freq_t = int;
const int SIGMA = 65536;

template<typename T,typename Ch,int MODE,int MAX_BLOCK_SIZE = 1024,int MAX_INTERNAL_BLOCK_SIZE = 1024>
void cram_replace_test(const auto& source,const auto& dest,const int rewrite_blocks,const int H){
    assert(source.size()%(MAX_BLOCK_SIZE/2) == 0);
    cout<<"CRAM BUILD START\n";
    CRAM<T,Ch,MODE,MAX_BLOCK_SIZE,MAX_INTERNAL_BLOCK_SIZE> cram(source,rewrite_blocks,H);
    cout<<"CRAM BUILD END\n";
    std::vector<std::vector<data_t>> replace_data;
    for(int i=0;i<dest.size();i+=MAX_BLOCK_SIZE/2){        
        std::vector<data_t> block(MAX_BLOCK_SIZE/2);
        copy(dest.begin()+i,dest.begin()+(i+MAX_BLOCK_SIZE/2),block.begin());
        replace_data.push_back(std::move(block));
    }
    const int block_num = replace_data.size();
    //const int block_num = 1;

    auto start = steady_clock::now();
    cout<<"PROGRESS: "<<0<<" CRAM USE BPC: "<<cram.get_bpc()<<'\n';
    for(int i=0;i<block_num;++i){        
        cram.replace(i,replace_data[i]);
        if((i+1)%(block_num/10)==0){
            cout<<"PROGRESS: "<<(i+1)/(block_num/10)*10<<" CRAM USE BPC: "<<cram.get_bpc()<<'\n';
        }
    }
    auto end = steady_clock::now();
    cout << "CRAM REPLACE Time difference = " << duration_cast<milliseconds> (end - start).count() << "[ms]\n";

    cout<<"CORRECTNESS CHECK\n";
    for(int i=0;i<block_num;++i){        
        auto& dest_block = replace_data[i];
        auto cram_block = cram.get_block(i);                
        for(int j=0;j<MAX_BLOCK_SIZE/2;++j){
            if(dest_block[j]!=cram_block[j]){
                cout<<"incorrect! "<<i<<','<<j<<'\n';
                assert(false);
            }
            //assert(dest_block[j]==cram_block[j]);
        }
    }
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

    cout<<"CRAM REPLACE TEST\n";
    int rewrite_blocks = 4;
    cram_replace_test<code_t,data_t,0,1024,2048>(source,dest,rewrite_blocks,2);
}