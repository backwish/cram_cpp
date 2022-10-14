#include <iostream>
#include <vector>
#include <random>
#include <assert.h>
#include "block.h"
using namespace std;

void testNaiveReplace(int size){
    vector<int> original_vec(size,rand());
    vector<int> init_block = original_vec;
    NaiveBlock<int,int> nb(std::move(init_block));
    for(int i=0;i<size*10;++i){
        int pos = rand()%size;
        int val = rand();
        original_vec[pos] = val;
        nb.replace(pos,val);
        for(int j=0;j<size;++j){
            assert(original_vec[j]==nb.at(j));
        }        
    }
}
void testNaiveInsert(int size){
    vector<int> original_vec,init_block;    
    NaiveBlock<int,int> nb;
    for(int i=0;i<size;++i){
        int pos = rand()%(original_vec.size()+1);
        int val = rand();
        original_vec.insert(original_vec.begin()+pos,val);
        nb.insert(pos,val);
        for(int j=0;j<original_vec.size();++j){
            assert(original_vec[j]==nb.at(j));
        }        
    }
}
void testNaiveErase(int size){
    vector<int> original_vec(size,rand());
    vector<int> init_block = original_vec;
    NaiveBlock<int,int> nb(std::move(init_block));
    for(int i=0;i<size;++i){
        int pos = rand()%(original_vec.size());
        int val = rand();
        original_vec.erase(original_vec.begin()+pos);
        nb.erase(pos);
        for(int j=0;j<original_vec.size();++j){
            assert(original_vec[j]==nb.at(j));
        }        
    }
}
void testNaiveMerge(int size){    
    for(int i=1;i<=size;++i){
        vector<int> original_vec_left(size*3,rand());
        vector<int> original_vec_right(size,rand());
        NaiveBlock<int,int> nb_left(original_vec_left);
        NaiveBlock<int,int> nb_right(original_vec_right);
        nb_left.merge(nb_right);
        original_vec_left.insert(original_vec_left.end(),original_vec_right.begin(),original_vec_right.end());
        for(int j=0;j<original_vec_left.size();++j){
            assert(original_vec_left[j]==nb_left.at(j));
        }
    }
}
void testNaiveSplit(int size){
    for(int i=2;i<size;++i){
        for(int left_len = 1;left_len<i;++left_len){
            vector<int> original_vec(size,rand());            
            NaiveBlock<int,int> nb(original_vec);
            vector<int> left_vec,right_vec;
            left_vec.insert(left_vec.end(),original_vec.begin(),original_vec.begin()+left_len);
            right_vec.insert(right_vec.end(),original_vec.begin()+left_len,original_vec.end());
            auto splitted = nb.split(left_len);
            for(int j=0;j<left_vec.size();++j){
                assert(left_vec[j]==splitted.first.at(j));
            }
            for(int j=0;j<right_vec.size();++j){
                assert(right_vec[j]==splitted.second.at(j));
            }            
        }
    }
}
void testPartialsumMerge(int size){
    const int MAX_VAL = 50000;
    for(int i=1;i<=size;++i){
        vector<int> original_vec_left(size*3,rand()%MAX_VAL);
        vector<int> original_vec_right(size,rand()%MAX_VAL);
        vector<int> sum_vec(size*4);
        copy(original_vec_left.begin(),original_vec_left.end(),sum_vec.begin());
        copy(original_vec_right.begin(),original_vec_right.end(),sum_vec.begin()+(size*3));
        partial_sum(original_vec_left.begin(),original_vec_left.end(),original_vec_left.begin());
        partial_sum(original_vec_right.begin(),original_vec_right.end(),original_vec_right.begin());
        PartialsumBlock<int> pb_left(original_vec_left);
        PartialsumBlock<int> pb_right(original_vec_right);
        pb_left.merge(pb_right);
        partial_sum(sum_vec.begin(),sum_vec.end(),sum_vec.begin());
        for(int j=0;j<size*4;++j){
            assert(sum_vec[j]==pb_left.at(j));
        }
    }
}
void testPartialsumSplit(int size){
    const int MAX_VAL = 50000;
    for(int i=2;i<size;++i){
        for(int left_len = 1;left_len<i;++left_len){
            vector<int> original_vec(size,rand()%MAX_VAL);            
            vector<int> left_vec,right_vec;
            left_vec.insert(left_vec.end(),original_vec.begin(),original_vec.begin()+left_len);
            right_vec.insert(right_vec.end(),original_vec.begin()+left_len,original_vec.end());
            partial_sum(original_vec.begin(),original_vec.end(),original_vec.begin());
            partial_sum(left_vec.begin(),left_vec.end(),left_vec.begin());
            partial_sum(right_vec.begin(),right_vec.end(),right_vec.begin());
            PartialsumBlock<int> pb_original(original_vec);
            auto splitted = pb_original.split(left_len);
            for(int j=0;j<left_vec.size();++j){
                assert(left_vec[j]==splitted.first.at(j));
            }
            for(int j=0;j<right_vec.size();++j){
                assert(right_vec[j]==splitted.second.at(j));
            }            
        }
    }
}

int main(){
    int test_vec_size = 10000;
    /*cout<<"TEST REPLACE\n";
    testNaiveReplace(test_vec_size);
    cout<<"REPLACE TEST SUCCESS!\n";
    cout<<"TEST INSERT\n";
    testNaiveReplace(test_vec_size);
    cout<<"TEST INSERT SUCCESS\n";
    cout<<"TEST ERASE\n";
    testNaiveErase(test_vec_size);
    cout<<"TEST ERASE SUCCESS\n";
    cout<<"TEST MERGE\n";
    testNaiveMerge(test_vec_size);
    cout<<"TEST MERGE SUCCESS\n";
    cout<<"TEST SPLIT\n";
    testNaiveMerge(100);
    cout<<"TEST SPLIT SUCCESS\n";*/

    cout<<"TEST PARTIAL SUM BLOCK\n";
    cout<<"TEST MERGE\n";
    testPartialsumMerge(test_vec_size);
    cout<<"TEST MERGE SUCCESS\n";
    cout<<"TEST SPLIT\n";
    testPartialsumSplit(100);
    cout<<"TEST SPLIT SUCCESS\n";
}