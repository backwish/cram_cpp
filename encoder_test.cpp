#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <chrono>
#include <assert.h>
#include "encoder.h"
using namespace std;
using namespace std::chrono;
using freq_t = uint64_t;
using ch_t = uint16_t;


int main(int argc,char **argv){
    std::ifstream is(argv[1],std::ios::binary);
    is.seekg(0, std::ios_base::end);
    std::size_t size=is.tellg();
    is.seekg(0, std::ios_base::beg);
    std::vector<uint64_t> text(size/sizeof(uint64_t));
    is.read((char*) &text[0], size);
    is.close();
        
    vector<freq_t> freq(65536,1);
    int n = text.size();    

    for(int i=0;i<n;++i){
        uint64_t fourCh = text[i];
        ++freq[static_cast<ch_t>(fourCh)];
        ++freq[static_cast<ch_t>(fourCh>>16)];
        ++freq[static_cast<ch_t>(fourCh>>32)];
        ++freq[static_cast<ch_t>(fourCh>>48)];
    }    

    HuffmanEncoder<> huf(freq);    
    
    cout<<"huf built finished\n";
    for(int i=0;i<n;++i){
        for(int j=0;j<4;++j){
            ch_t ch = static_cast<ch_t>(text[i]>>(16*(3-j)));
            assert(ch == huf.decode(huf.encode(ch).first).first);
        }
    }
    /*uint64_t max_size = UINT32_MAX;
    uint64_t stride = 1<<6;
    start = steady_clock::now();
    for(uint64_t i=0;i<max_size-stride;i+=stride){        
        uint64_t long_code = (i<<32);        
        auto ch = huf.decode(long_code).first;
        assert(ch == huf.decode(huf.encode(ch).first).first);
    }
    end = steady_clock::now();
    cout << "Time difference = " << duration_cast<milliseconds> (end - start).count() << "[ms]\n";*/
    
}