#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <fstream>
#include <assert.h>
#include "darray.h"
#include "encoder.h"

using namespace std;
using namespace std::chrono;
using code_t = uint64_t;
using data_t = uint16_t;
using freq_t = int;
const int SIGMA = 65536;

void darray_encoder_insert_test(const auto& text){
    vector<freq_t> freq(65536,1);
    for(const auto ch:text){
        freq[ch]++;
    }
    HuffmanEncoder<> encoder(freq);    
    int n = text.size(),iter = 0;        
    Darray<code_t,data_t,64,4096> da;    
    auto start = steady_clock::now();
    for(const auto ch:text){
        ++iter;
        if(iter%(n/10)==0){cout<<"PROGRESS "<<iter/(n/10)<<'\n';}
        da.insert(da.size(),ch,encoder);
    }
    auto end = steady_clock::now();
    cout << "insert Time difference = " << duration_cast<milliseconds> (end - start).count() << "[ms]\n";
    cout << "Block Insert Time: "<<da.getBlockInsertTime()<<'\n';
    cout<<"INVARIANT TEST\n";
    da.checkInvariant();
    cout<<"CORRECTNESS TEST\n";
    for(int i=0;i<n;i+=1000){
        assert(da.at(i,encoder)==text[i]);
    }
}

void darray_bulk_insert_test(const auto& text){    
    vector<freq_t> freq(65536,1);
    for(const auto ch:text){
        freq[ch]++;
    }
    HuffmanEncoder<> encoder(freq);
    auto start = steady_clock::now();
    Darray<code_t,data_t,1024,2048> da(text,encoder);
    auto end = steady_clock::now();
    cout << "Bulk insert Time difference = " << duration_cast<milliseconds> (end - start).count() << "[ms]\n";
    cout<<"INVARIANT TEST\n";
    da.checkInvariant();
    cout<<"CORRECTNESS TEST\n";
    for(int i=0;i<text.size();i+=1000){
        assert(da.at(i,encoder)==text[i]);
    }

    /*da.printDarray();    
    for(int j=0;j<N;++j) cout<<da.at(j)<<' ';
    cout<<'\n';
    for(auto val:raw_vector) cout<<val<<' ';
    cout<<'\n';        
    for(int i=0;i<N;++i){
        assert(da.at(i)==raw_vector[i]);
    }        */
}


int main(int argc,char **argv){
    std::ifstream is(argv[1],std::ios::binary);
    is.seekg(0, std::ios_base::end);
    std::size_t size=is.tellg();
    is.seekg(0, std::ios_base::beg);
    std::vector<data_t> text(size/sizeof(data_t));
    is.read((char*) &text[0], size);
    is.close();


    cout<<"DARRAY FRONT INSERT TEST\n";
    darray_encoder_insert_test(text);
    //darray_bulk_insert_test(text);

    /*cout<<"SMALL SIZE ERASE TEST FRONT\n";
    darray_erase_seq_test<1600,4,4,true>();
    cout<<"SMALL SIZE ERASE TEST BACK\n";
    darray_erase_seq_test<1600,4,4,false>();
    cout<<"SMALL SIZE RAND ERASE TEST\n";
    darray_erase_rand_test<1600,4,4>();
    cout<<"LARGE SIZE ERASE TEST FRONT\n";
    darray_erase_seq_test<40000,100,100,true>();
    cout<<"LARGE SIZE ERASE TEST BACK\n";
    darray_erase_seq_test<40000,100,100,false>();
    cout<<"LARGE SIZE RAND ERASE TEST\n";
    darray_erase_rand_test<40000,100,100>();*/
    /*cout<<"DARRAY ERASE PERFORMANCE TEST\n";
    darray_erase_performance_test<100000000,1024,2048>();*/

    /*cout<<"SMALL SIZE INSERT TEST FRONT\n";
    darray_insert_test<100,2,2,true>();
    cout<<"SMALL SIZE INSERT TEST BACK\n";
    darray_insert_test<100,2,2,false>();
    cout<<"LARGE SIZE INSERT TEST FRONT\n";
    darray_insert_test<10000,100,100,true>();
    cout<<"LARGE SIZE INSERT TEST BACK\n";
    darray_insert_test<10000,100,100,false>();*/
    /*cout<<"DARRAY INSERT PERFORMANCE TEST\n";
    darray_insert_performance_test();*/
}