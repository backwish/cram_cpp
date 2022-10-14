#include <iostream>
#include <vector>
#include <random>
#include <assert.h>
#include <fstream>
#include "block.h"
#include "encoder.h"
using namespace std;
const int SIGMA = 65536;
using freq_t = uint64_t;
using ch_t = uint16_t;
using data_t = uint16_t;
using code_t = uint64_t;

void testHuffmanAt(std::vector<data_t>& original_vector,HuffmanEncoder<>& encoder){    
    HuffmanBlock<code_t,ch_t> huffmanBlock(original_vector,original_vector.size(),encoder);    
    for(int i=0;i<original_vector.size();++i){
        assert(original_vector[i]==huffmanBlock.at(i,encoder));        
    }
}
void testHuffmanReplace(std::vector<data_t>& original_vector,HuffmanEncoder<>& encoder){    
    HuffmanBlock<code_t,ch_t> huffmanBlock(original_vector,original_vector.size(),encoder);    
    for(int i=0;i<original_vector.size();++i){
        data_t to_ch = rand()%SIGMA;
        huffmanBlock.replace(i,to_ch,encoder);
        assert(to_ch==huffmanBlock.at(i,encoder));        
    }
}
auto buildFreq(std::vector<data_t>& text){
    std::vector<freq_t> freq(SIGMA,1);
    int n = text.size();
    std::for_each(text.begin(),text.end(),[&freq](const auto& ch){++freq[ch];});    
    return freq;
}

int main(int argc,char **argv){
    std::ifstream is(argv[1],std::ios::binary);
    is.seekg(0, std::ios_base::end);
    std::size_t size=is.tellg();
    is.seekg(0, std::ios_base::beg);
    std::vector<data_t> text(size/sizeof(data_t));
    is.read((char*) &text[0], size);
    is.close();
    auto freq = buildFreq(text);
    HuffmanEncoder<> encoder(freq);

    int n = text.size();
    const int BLOCK_SIZE = 512;
    std::cout<<"TEST HUFFMAN AT\n";
    for(int i=0;i<100;++i){
        std::vector<data_t> original_block(BLOCK_SIZE);        
        std::copy(text.begin()+i*BLOCK_SIZE,text.begin()+(i+1)*BLOCK_SIZE,original_block.begin());
        testHuffmanAt(original_block,encoder);
    }
    std::cout<<"TEST HUFFMAN REPLACE\n";
    for(int i=0;i<100;++i){
        std::vector<data_t> original_block(BLOCK_SIZE);        
        std::copy(text.begin()+i*BLOCK_SIZE,text.begin()+(i+1)*BLOCK_SIZE,original_block.begin());
        testHuffmanReplace(original_block,encoder);
    }

}