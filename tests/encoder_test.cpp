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
    std::vector<ch_t> text(size/sizeof(ch_t));
    is.read((char*) &text[0], size);
    is.close();
        
    vector<freq_t> freq(65536,1);
    int n = text.size();    
    for(const auto ch:text){
        freq[ch]++;
    }

    HuffmanEncoder<> huf(freq);    
    
    cout<<"huf built finished\n";
    for(const auto ch:text){
        assert(ch == huf.decode(huf.encode(ch).first).first);
    }    
    uint64_t max_size = UINT32_MAX;
    uint64_t stride = 1<<6;
    auto start = steady_clock::now();
    for(uint64_t i=0;i<max_size-stride;i+=stride){        
        uint64_t long_code = (i<<32);        
        auto ch = huf.decode(long_code).first;
        assert(ch == huf.decode(huf.encode(ch).first).first);
    }
    auto end = steady_clock::now();
    cout << "Time difference = " << duration_cast<milliseconds> (end - start).count() << "[ms]\n";
    
}