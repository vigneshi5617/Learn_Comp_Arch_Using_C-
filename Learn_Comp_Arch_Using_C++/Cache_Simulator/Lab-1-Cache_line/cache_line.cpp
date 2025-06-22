#include <iostream>
#include <stdlib.h>
using namespace std;


struct cache_line {
    unsigned int tag;
    unsigned int valid;
    int dirty;
    //int data[16];
};

int main() {
    cout << "Hello, World!" << endl;
    return 0;
}