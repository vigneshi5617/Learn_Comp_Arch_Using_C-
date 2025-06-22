#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "sim.h"
#include <math.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <iomanip>
using namespace std;

//measurement parameters
int L1_Read=0,L1_ReadMiss=0,L1_Write=0,L1_WriteMiss=0,L1_WriteBack=0,L1_Prefetches=0;
double L1_MissRate=0.0000f,L2_MissRate=0.000f;
int L2_Read=0,L2_ReadMiss=0,L2_Write=0,L2_WriteMiss=0,L2_WriteBack=0,L2_Prefetches=0;
int L2_PrefetchRead=0, L2_PrefetchReadMiss=0;
int TolMemTraffic=0;
/*  "argc" holds the number of command-line arguments.
    "argv[]" holds the arguments themselves.

    Example:
    ./sim 32 8192 4 262144 8 3 10 gcc_trace.txt
    argc = 9
    argv[0] = "./sim"
    argv[1] = "32"
    argv[2] = "8192"
    ... and so on
*/
class cache {
   public:
   vector<vector<unsigned long> > cache_x;
   vector<vector<unsigned long> > cache_whole;
   vector<vector<int> > validBit;
   vector<vector<int> > dirtydBit;
   vector<vector<int> > LRUBit;
   vector<vector<unsigned long> > streamBuffer;
   vector<int> LRU_SB;
   int setIndex, tag, blockoffset, sets;
         cache(int BLOCKSIZE,int SIZE,int ASSOC,int N, int M){
            if(SIZE>0 && BLOCKSIZE>0){
               blockoffset = log2(BLOCKSIZE);
               sets = SIZE/(ASSOC*BLOCKSIZE);
               setIndex = log2(sets);
               tag = 32-setIndex-blockoffset;

               this->cache_x = createcache(ASSOC, sets);
               this->validBit = validBitfunc(sets,ASSOC);
               this->dirtydBit = dirtyBitfunc(sets,ASSOC);
               this->LRUBit = LRUBitfunc(sets,ASSOC);
               this->cache_whole = wholeaddress(ASSOC, sets);
               if(N>0 && M>0){
                   this->streamBuffer = createSB(N,M);
                   this->LRU_SB = createLRUSB(N);
               }
            }
         }
   
   vector<vector<int> > validBitfunc(int sets,int ASSOC){
      this->validBit.resize(sets);
         for(int i = 0; i < sets; i++) {
            this->validBit[i].resize(ASSOC);
         }
      return validBit;
   }
   vector<vector<int> > dirtyBitfunc(int sets,int ASSOC){
      this->dirtydBit.resize(sets);
         for(int i = 0; i < sets; i++) {
            this->dirtydBit[i].resize(ASSOC);
         }
      return dirtydBit;
   }
   vector<vector<int> > LRUBitfunc(int sets,int ASSOC){
      this->LRUBit.resize(sets);
         for(int i = 0; i < sets; i++) {
            this->LRUBit[i].resize(ASSOC);
         }
   return LRUBit;
   }
   vector<vector<unsigned long> > createcache(int ASSOC, int sets){
      this->cache_x.resize(sets);
      for(int i=0;i<sets;i++){
         this->cache_x[i].resize(ASSOC);
      }
      return cache_x;
   }
   vector<vector<unsigned long> > createSB(int N, int M){
      this->streamBuffer.resize(N);
      for(int i=0;i<N;i++){
         this->streamBuffer[i].resize(M);
      }
      return streamBuffer;
   }
   vector<int> createLRUSB(int N){
      this->LRU_SB.resize(N);
      return LRU_SB;
   }
   
string bo_func(string str){
   string bo;
   bo=str.substr(32-this->blockoffset,this->blockoffset);
   return bo;
}
string i_func(string str){
   string i;
   i=str.substr(this->tag,this->setIndex);
   return i;
}
string t_func(string str){
   string t;
   t=str.substr(0,this->tag);
   return t;
} 

   vector<vector<unsigned long> > wholeaddress(int ASSOC, int sets){
      this->cache_whole.resize(sets);
      for(int i=0;i<sets;i++){
         this->cache_whole[i].resize(ASSOC);
      }
      return cache_whole;
   }

   bool isSetWaysFull(int indexBits, int ASSOC, vector<vector<int> > validBit) {
      for (int i=0; i<ASSOC; i++){
         if (this->validBit[indexBits][i]==0){
            return false;
         }
      }
   return true;
   }

   bool isSetWaysEmpty(int indexBits, int ASSOC, vector<vector<int> > LRUBit) {
      for (int i=0; i<ASSOC; i++){
         if (this->LRUBit[indexBits][i]!=0){
            return false;
         }
      }
   return true;
   }

   bool isBufferWaysEmpty(int N, vector<int> LRU_SB){
      for(int i=0; i<N;i++){
         if(this->LRU_SB[i]!=0){
            return false;
         }
      }
   return true;
   }


   bool cacheHit(vector<vector<unsigned long> > cache_x, int indexBits, unsigned long tagBits, int ASSOC){
      for(int i=0;i<ASSOC;i++){
         if (this->cache_x[indexBits][i]==tagBits){
            return true;
         }
      }
      return false;
   }
   void incrementLRU(vector<vector<int> > LRUBit, int indexBits, int ASSOC, int temp){
      if(temp==0){
         for(int i=0;i<ASSOC;i++){
            if(this->LRUBit[indexBits][i]!=0){
               this->LRUBit[indexBits][i]+=1;
            }
         }
      }
      else{
         for(int i=0;i<ASSOC;i++){
         if(this->LRUBit[indexBits][i]<temp && this->LRUBit[indexBits][i]!=0){
            this->LRUBit[indexBits][i]+=1;
         }
      }
      }
   }
   vector<int> evictLRU(vector<vector<int> > LRUBit, int indexBits, int ASSOC){
      int max=0;
      int temp=0;
      vector<int> maxLRU;
      maxLRU.resize(2);
      for(int i=0;i<ASSOC;i++){
         if(this->LRUBit[indexBits][i]>max){
            max=this->LRUBit[indexBits][i];
            temp=i;
         }
      }
   maxLRU[0]=temp;
   maxLRU[1]=max;
   return maxLRU;
   }

   void writeBack(string evictwhole, int ASSOC, int N, int M, cache &L2){
         
         char *buffer;
         string temp_t = L2.t_func(evictwhole);
         string temp_i = L2.i_func(evictwhole);
         string mem_add = temp_t + temp_i;
         //int EW = strtol(evictwhole.c_str(),&buffer,2);
         int mem_add_int = strtol(mem_add.c_str(),&buffer,2);
         if(N>0 && M>0){
            if(L2.sbHit(L2.streamBuffer, N, M, mem_add_int)){
               int k = L2.getnumberSB(L2.streamBuffer, N, M, mem_add_int, L2.LRU_SB);
               int hit = L2.getM(L2.streamBuffer, N, M, mem_add_int, L2.LRU_SB);
               L2.prefetchSB(L2.streamBuffer, M, k, mem_add_int);
               L2.incrementLRUSB(L2.LRU_SB, N, k);
               L2.LRU_SB[k]=1;
               L2_Prefetches+=hit;
            }
         }
         int temp_i_int = strtol(temp_i.c_str(),&buffer,2);
         unsigned long temp_t_int = strtol(temp_t.c_str(),&buffer,2);

         for(int j=0;j<ASSOC;j++){
            if(temp_t_int==L2.cache_x[temp_i_int][j]){
               int temp=L2.LRUBit[temp_i_int][j];
               L2.incrementLRU(L2.LRUBit, temp_i_int, ASSOC, temp);
               L2.LRUBit[temp_i_int][j]=1;
               L2.dirtydBit[temp_i_int][j]=1;
               break;
            }
         }
   }

   bool sbHit(vector<vector<unsigned long> > streamBuffer, int N, int M, unsigned long memoryAddress){
      for(int i=0;i<N;i++){
         for(int j=0;j<M;j++){
            if(streamBuffer[i][j] == memoryAddress){
               //cout<<streamBuffer[i][j]<<endl;
               return true;
            }
         }
      }
      return false;
   }

   void prefetchSB(vector<vector<unsigned long> > streamBuffer, int M, int k, int memoryAddress){
      int temp = memoryAddress+1;
      for(int j=0;j<M;j++){
         this->streamBuffer[k][j] = temp;
         temp+=1;
      }
   }

   void incrementLRUSB(vector<int> LRU_SB, int N, int k){
      int temp = this->LRU_SB[k];
      if(temp==0){
         for(int j=0;j<N;j++){
            if(this->LRU_SB[j]!=0){
               this->LRU_SB[j]+=1;
            }
         }
      }
      else{
         for(int i=0;i<N;i++){
            if(this->LRU_SB[i]<temp && this->LRU_SB[i]!=0){
               this->LRU_SB[i]+=1;
            }
         }
      }
   }

   int getM(vector<vector<unsigned long> > streamBuffer, int N, int M, unsigned long memoryAddress, vector<int> LRU_SB){
      int temp=0;
      int min=N;
      for(int i=0;i<N;i++){
         for(int j=0;j<M;j++){
            if(this->streamBuffer[i][j]==memoryAddress && this->LRU_SB[i]<=min){
               min = this->LRU_SB[i];
               temp = j+1;
            }
         }
      }
      return temp;
   }

   int getnumberSB(vector<vector<unsigned long> > streamBuffer, int N, int M, unsigned long memoryAddress, vector<int> LRU_SB){
      int temp=0;
      int flag=0, min=N;
      for(int i=0;i<N;i++){
         for(int j=0;j<M;j++){
            if(this->streamBuffer[i][j]==memoryAddress && this->LRU_SB[i]<=min){
               min = this->LRU_SB[i];
               temp = i;
               flag = 1;
            }
         }
      }
      if(flag==1){
         return temp;
      }

      for(int i=0;i<N;i++){
         if(this->LRU_SB[i]==0){
            return i;
         }
      }

      int max=0;
      int tempA=0;
      for(int i=0;i<N;i++){
         if(this->LRU_SB[i]>max){
            max = this->LRU_SB[i];
            tempA=i;
         }
      }
      return tempA;
   }


   void bubbleSort(vector<vector<int> >& LRUBit,vector<vector<int> >& dirtydBit, vector<vector<unsigned long> >& cache_x, int& sets,uint32_t& ASSOC){
      for(int i=0;i<sets;i++){
         for(uint32_t j=0;j<ASSOC-1;j++){
            for(uint32_t k=0;k<ASSOC-j-1;k++){
               if(LRUBit[i][k] > LRUBit[i][k+1]){
                  int temp_lru = LRUBit[i][k];
                  LRUBit[i][k]=LRUBit[i][k+1];
                  LRUBit[i][k+1]=temp_lru;

                  int temp_tag = cache_x[i][k];
                  cache_x[i][k]=cache_x[i][k+1];
                  cache_x[i][k+1]=temp_tag;

                  int temp_dirty = dirtydBit[i][k];
                  dirtydBit[i][k]=dirtydBit[i][k+1];
                  dirtydBit[i][k+1]=temp_dirty;
               }
            }
         }
      }
   }
   void bubble(vector<vector<unsigned long> > streamBuffer,vector<int> LRU_SB, int N, int M){
      vector<unsigned long> temp_mem;
      temp_mem.resize(M);
      for(int i=0;i<N-1;i++){
         for(int j=0;j<N-i-1;j++){
            if(this->LRU_SB[j] > this->LRU_SB[j+1]){
               int templru_sb = this->LRU_SB[j];
               this->LRU_SB[j]=this->LRU_SB[j+1];
               this->LRU_SB[j+1]=templru_sb;

               for(int k=0;k<M;k++){
                  temp_mem[k]=this->streamBuffer[j][k];
               }
               for(int k=0;k<M;k++){
                  this->streamBuffer[j][k]=this->streamBuffer[j+1][k];
               }
               for(int k=0;k<M;k++){
                  this->streamBuffer[j+1][k]=temp_mem[k];
               }
            }
         }
      }
   }
};

   void inverse(char abc[], int size){
      int initial=0;
      int last = size-1;
      while(initial < last){
         swap(*(abc+initial), *(abc+last));
         initial++;
         last--;
      }
   }

   void baseconvert(int address, char* sample, int base){
      int x=0;
      if(address == 0){
         sample[x++] = '0';
         sample[x] = '\0';
      }
      while(address!=0){
         int reminder = address % base;
         sample[x++] = (reminder > 9)? (reminder-10) + 'a' : reminder + '0';
         address = address/base;
      }
      sample[x] = '\0';
      inverse(sample, x);
   }

int main (int argc, char *argv[]) {
   FILE *fp;			// File pointer.
   char *trace_file;		// This variable holds the trace file name.
   cache_params_t params;	// Look at the sim.h header file for the definition of struct cache_params_t.
   char rw;			// This variable holds the request's type (read or write) obtained from the trace.
   uint32_t addr;		// This variable holds the request's address obtained from the trace.
				// The header file <inttypes.h> above defines signed and unsigned integers of various sizes in a machine-agnostic way.  "uint32_t" is an unsigned integer of 32 bits.        
   // Exit with an error if the number of command-line arguments is incorrect.
   //ofstream out;
   //out.open("out.txt");
   if (argc != 9) {
      printf("Error: Expected 8 command-line arguments but was provided %d.\n", (argc - 1));
      exit(EXIT_FAILURE);
   }
    
   // "atoi()" (included by <stdlib.h>) converts a string (char *) to an integer (int).
   params.BLOCKSIZE = (uint32_t) atoi(argv[1]);
   params.L1_SIZE   = (uint32_t) atoi(argv[2]);
   params.L1_ASSOC  = (uint32_t) atoi(argv[3]);
   params.L2_SIZE   = (uint32_t) atoi(argv[4]);
   params.L2_ASSOC  = (uint32_t) atoi(argv[5]);
   params.PREF_N    = (uint32_t) atoi(argv[6]);
   params.PREF_M    = (uint32_t) atoi(argv[7]);
   trace_file       = argv[8];

   // Open the trace file for reading.
   fp = fopen(trace_file, "r");
   if (fp == (FILE *) NULL) {
      // Exit with an error if file open failed.
      printf("Error: Unable to open file %s\n", trace_file);
      exit(EXIT_FAILURE);
   }
    
   // Print simulator configuration.
   //printf("===== Simulator configuration =====\n");
   //printf("BLOCKSIZE:  %u\n", params.BLOCKSIZE);
   //printf("L1_SIZE:    %u\n", params.L1_SIZE);
   //printf("L1_ASSOC:   %u\n", params.L1_ASSOC);
   //printf("L2_SIZE:    %u\n", params.L2_SIZE);
   //printf("L2_ASSOC:   %u\n", params.L2_ASSOC);
   //printf("PREF_N:     %u\n", params.PREF_N);
   //printf("PREF_M:     %u\n", params.PREF_M);
   //printf("trace_file: %s\n", trace_file);
   //printf("\n");
   //printf("===================================\n");
   cout<<"===== Simulator configuration ====="<<"\n";
   cout<<"BLOCKSIZE:   "<<params.BLOCKSIZE<<"\n";
   cout<<"L1_SIZE:     "<<params.L1_SIZE<<"\n";
   cout<<"L1_ASSOC:    "<<params.L1_ASSOC<<"\n";
   cout<<"L2_SIZE:     "<<params.L2_SIZE<<"\n";
   cout<<"L2_ASSOC:    "<<params.L2_ASSOC<<"\n";
   cout<<"PREF_N:      "<<params.PREF_N<<"\n";
   cout<<"PREF_M:      "<<params.PREF_M<<"\n";
   cout<<"trace_file:  "<<trace_file<<"\n";
   cout<<"\n";

   cache L1(params.BLOCKSIZE,params.L1_SIZE,params.L1_ASSOC,params.PREF_N,params.PREF_M);
   
   cache L2(params.BLOCKSIZE,params.L2_SIZE,params.L2_ASSOC,params.PREF_N,params.PREF_M);

   while (fscanf(fp, "%c %x\n", &rw, &addr) == 2) {	// Stay in the loop if fscanf() successfully parsed two tokens as specified.
      char sample[32];
      baseconvert(addr, sample, 2);
      //itoa(addr,sample,2);
      string str(sample);
      int len=str.length();
      int x = 32 - min(32,len);
      str.insert(0,x,'0');
   
      char *buffer;
      //index bits of L1 cache in string and integer
      string L1_indexBits_bin = L1.i_func(str);
      int L1_indexBits = strtol(L1_indexBits_bin.c_str(),&buffer,2);
      //index bits of L1 cache in string and integer
      string L1_tagBits_bin = L1.t_func(str);
      unsigned long L1_tagBits = strtol(L1_tagBits_bin.c_str(),&buffer,2);
      //blockOfsset bits of L1 cache in string and integer
      string L1_blockOffset_bin = L1.bo_func(str);
      //int L1_blockOffset = strtol(L1_blockOffset_bin.c_str(),&buffer,2);
      //memory address of L1 cache in string and integer
      string L1_memoryAddress_bin = L1_tagBits_bin + L1_indexBits_bin;
      unsigned long L1_memoryAddress =strtol(L1_memoryAddress_bin.c_str(),&buffer,2);
      //physical address of L1 cache in string and integer
      string L1_whole_bin = L1_tagBits_bin + L1_indexBits_bin + L1_blockOffset_bin;
      int L1_whole = strtol(L1_whole_bin.c_str(),&buffer,2);

      string L2_indexBits_bin;
      int L2_indexBits;
      string L2_tagBits_bin;
      unsigned long L2_tagBits;
      string L2_blockOffset_bin;
      //int L2_blockOffset;
      string L2_memoryAddress_bin;
      unsigned long L2_memoryAddress;
      string L2_whole_bin;
      int L2_whole;
      if(params.L2_SIZE>0){
         //index bits of L2 cache in string and integer
         L2_indexBits_bin = L2.i_func(str);
         L2_indexBits = strtol(L2_indexBits_bin.c_str(),&buffer,2);
         //tag bits of L2 cache in string and integer
         L2_tagBits_bin = L2.t_func(str);
         L2_tagBits = strtol(L2_tagBits_bin.c_str(),&buffer,2);
         //blockoffset bits of L2 cache in string and integer
         L2_blockOffset_bin = L2.bo_func(str);
         //L2_blockOffset = strtol(L2_blockOffset_bin.c_str(),&buffer,2);
         //memoryAddress bits of L2 cache in string and integer
         L2_memoryAddress_bin = L2_tagBits_bin + L2_indexBits_bin;
         L2_memoryAddress = strtol(L2_memoryAddress_bin.c_str(),&buffer,2);
         //physical address of L2 cache in string and integer
         L2_whole_bin = L2_tagBits_bin + L2_indexBits_bin + L2_blockOffset_bin;
         L2_whole = strtol(L2_whole_bin.c_str(),&buffer,2);

      }
      //check if L2 size is 0 or not
      if(params.L2_SIZE==0){
         //if it gets hit in L1 cache
         if(L1.cacheHit(L1.cache_x, L1_indexBits, L1_tagBits, params.L1_ASSOC)){
            if(params.PREF_N>0 && params.PREF_M>0){
               if(L1.sbHit(L1.streamBuffer, params.PREF_N, params.PREF_M, L1_memoryAddress)){
                  //cout<<L1_memoryAddress<<endl;
                  int hit = L1.getM(L1.streamBuffer, params.PREF_N, params.PREF_M, L1_memoryAddress, L1.LRU_SB);
                  int k = L1.getnumberSB(L1.streamBuffer, params.PREF_N, params.PREF_M, L1_memoryAddress, L1.LRU_SB);
                  L1.prefetchSB(L1.streamBuffer, params.PREF_M, k, L1_memoryAddress);
                  L1.incrementLRUSB(L1.LRU_SB, params.PREF_N, k);
                  L1.LRU_SB[k]=1;
                  L1_Prefetches+=hit;
                  //TolMemTraffic+=hit;
               }
            }
            int temp;
            for(uint32_t i=0;i<params.L1_ASSOC;i++){
               if (L1.cache_x[L1_indexBits][i]==L1_tagBits){
                  temp=L1.LRUBit[L1_indexBits][i];
                  L1.incrementLRU(L1.LRUBit, L1_indexBits, params.L1_ASSOC, temp);
                  L1.LRUBit[L1_indexBits][i]=1;
                  
                  if (rw == 'w'){
                     L1.dirtydBit[L1_indexBits][i]=1;
                     L1_Write+=1;
                  }
                  else if (rw == 'r' && L1.dirtydBit[L1_indexBits][i]==1){
                     L1.dirtydBit[L1_indexBits][i]=1;
                     L1_Read+=1;
                  }
                  else if(rw == 'r' && L1.dirtydBit[L1_indexBits][i]==0){
                     L1_Read+=1;
                  }
                  break;
               }
            }
         }
         //if it's not in L1 cache
         else if(!L1.cacheHit(L1.cache_x, L1_indexBits, L1_tagBits, params.L1_ASSOC)){
            if(params.PREF_N>0 && params.PREF_M>0){
               if(L1.sbHit(L1.streamBuffer, params.PREF_N, params.PREF_M, L1_memoryAddress)){
                  int hit = L1.getM(L1.streamBuffer, params.PREF_N, params.PREF_M, L1_memoryAddress, L1.LRU_SB);
                  L1_Prefetches+=hit;
                  //TolMemTraffic+=hit;
               }
               else{
                  L1_Prefetches+=params.PREF_M;
                  //TolMemTraffic+=params.PREF_M;
                  if(rw == 'r'){
                     L1_ReadMiss+=1;
                  }
                  else if(rw == 'w'){
                     L1_WriteMiss+=1;
                  }
               }
               int k = L1.getnumberSB(L1.streamBuffer, params.PREF_N, params.PREF_M, L1_memoryAddress, L1.LRU_SB);
               L1.prefetchSB(L1.streamBuffer, params.PREF_M, k, L1_memoryAddress);
               L1.incrementLRUSB(L1.LRU_SB, params.PREF_N, k);
               L1.LRU_SB[k]=1;
            }
            // if L1 ways are full
            if(L1.isSetWaysFull(L1_indexBits, params.L1_ASSOC, L1.validBit)){
               vector<int> maxLRU;
               maxLRU = L1.evictLRU(L1.LRUBit, L1_indexBits, params.L1_ASSOC);
               L1.incrementLRU(L1.LRUBit, L1_indexBits, params.L1_ASSOC, maxLRU[1]);
               L1.LRUBit[L1_indexBits][maxLRU[0]]=1;
               if(L1.dirtydBit[L1_indexBits][maxLRU[0]]==1){
                  if(params.PREF_N==0 && params.PREF_M==0){
                     TolMemTraffic+=1;
                  }
                  L1_WriteBack+=1;
               }
               L1.cache_x[L1_indexBits][maxLRU[0]]=L1_tagBits;
               if (rw == 'w'){
                  L1.dirtydBit[L1_indexBits][maxLRU[0]]=1;
                  if(params.PREF_M==0 && params.PREF_N==0){
                     L1_WriteMiss+=1;
                  }
                  L1_Write+=1;
               }
               else if (rw == 'r'){
                  L1.dirtydBit[L1_indexBits][maxLRU[0]]=0;
                  if(params.PREF_M==0 && params.PREF_N==0){
                     L1_ReadMiss+=1;
                  }
                  L1_Read+=1;
               }
               if(params.PREF_M==0 && params.PREF_N==0){
                  TolMemTraffic+=1;
               }
            }
            else{
               //if L1 ways are not full
               for(uint32_t i=0;i<params.L1_ASSOC;i++){
                  if(L1.validBit[L1_indexBits][i]==0){
                      int temp;
                      temp=L1.LRUBit[L1_indexBits][i];
                      L1.incrementLRU(L1.LRUBit, L1_indexBits, params.L1_ASSOC, temp);
                      L1.LRUBit[L1_indexBits][i]=1;
                      L1.cache_x[L1_indexBits][i]=L1_tagBits;
                      L1.validBit[L1_indexBits][i]=1;

                     if (rw == 'w'){
                        L1.dirtydBit[L1_indexBits][i]=1;
                        if(params.PREF_M==0 && params.PREF_N==0){
                           L1_WriteMiss+=1;
                        }
                        //L1_WriteMiss+=1;
                        L1_Write+=1;
                     }
                     else if (rw == 'r'){
                        L1.dirtydBit[L1_indexBits][i]=0;
                        if(params.PREF_M==0 && params.PREF_N==0){
                           L1_ReadMiss+=1;
                        }
                        //L1_ReadMiss+=1;
                        L1_Read+=1;
                     }
                     if(params.PREF_M==0 && params.PREF_N==0){
                        TolMemTraffic+=1;
                     }
                     break;
                  }
               }
            }
         }
      }

      else{
         //if L2 size is not zero
         //if it's a hit in L1 cache
         if(L1.cacheHit(L1.cache_x, L1_indexBits, L1_tagBits, params.L1_ASSOC)){
            /*if(params.PREF_N>0 && params.PREF_M>0){
               if(L2.sbHit(L2.streamBuffer, params.PREF_N, params.PREF_M, L2_memoryAddress)){
                  int k = L2.getnumberSB(L2.streamBuffer, params.PREF_N, params.PREF_M, L2_memoryAddress, L2.LRU_SB);
                  L2.prefetchSB(L2.streamBuffer, params.PREF_M, k, L2_memoryAddress);
                  L2.incrementLRUSB(L2.LRU_SB, params.PREF_N, k);
                  L2.LRU_SB[k]=1;
               }
            }*/
            int temp;
            for(uint32_t i=0;i<params.L1_ASSOC;i++){
               if (L1.cache_x[L1_indexBits][i]==L1_tagBits){
                  temp=L1.LRUBit[L1_indexBits][i];
                  L1.incrementLRU(L1.LRUBit, L1_indexBits, params.L1_ASSOC, temp);
                  L1.LRUBit[L1_indexBits][i]=1;
                  
                  if (rw == 'w'){
                     L1.dirtydBit[L1_indexBits][i]=1;
                     L1_Write+=1;
                  }
                  else if (rw == 'r' && L1.dirtydBit[L1_indexBits][i]==1){
                     L1.dirtydBit[L1_indexBits][i]=1;
                     L1_Read+=1;
                  }
                  else if (rw == 'r' && L1.dirtydBit[L1_indexBits][i]==0){
                     L1_Read+=1;
                  }
                  break;
               }
            }
         }
         //if it's not present in L1 but present in L2 cache
         else if(!L1.cacheHit(L1.cache_x, L1_indexBits, L1_tagBits, params.L1_ASSOC) && L2.cacheHit(L2.cache_x, L2_indexBits, L2_tagBits, params.L2_ASSOC)){
            int temp;
            //if L1 ways are full
            if(L1.isSetWaysFull(L1_indexBits, params.L1_ASSOC, L1.validBit)){
               vector<int> maxLRU;
               maxLRU.resize(2);
               maxLRU = L1.evictLRU(L1.LRUBit, L1_indexBits, params.L1_ASSOC);
               //int x = L1.cache_x[L1_indexBits][maxLRU[0]];
               int y = L1.cache_whole[L1_indexBits][maxLRU[0]];
               //char q[33];
               char z[33];
               //baseconvert(x, q, 2);
               baseconvert(y, z, 2);
               //itoa(x,q,2);
               //itoa(y,z,2);
               //string evictTag(q);
               string evictWhole(z);
               //int lex=L1.tag-evictTag.length();
               int lx=evictWhole.length();
               int xk = 32 - min(32,lx);
               evictWhole.insert(0,xk,'0');
               //evictTag.insert(0,lex,'0');
               if (rw == 'w' && L1.dirtydBit[L1_indexBits][maxLRU[0]]==1){
                  L1_WriteMiss+=1;
                  L1.writeBack(evictWhole, params.L2_ASSOC, params.PREF_N, params.PREF_M, L2);
                  L1_WriteBack+=1;
                  L2_Write+=1;
                  L2_Read+=1;
                  L1_Write+=1;
                  L1.dirtydBit[L1_indexBits][maxLRU[0]]=1;
               }
               else if(rw == 'r' && L1.dirtydBit[L1_indexBits][maxLRU[0]]==1){
                  L1_ReadMiss+=1;
                  L1.writeBack(evictWhole, params.L2_ASSOC, params.PREF_N, params.PREF_M, L2);
                  L1_WriteBack+=1;
                  L2_Write+=1;
                  L2_Read+=1;
                  L1_Read+=1;
                  L1.dirtydBit[L1_indexBits][maxLRU[0]]=0;
               }
               else if(rw == 'w' && L1.dirtydBit[L1_indexBits][maxLRU[0]]==0){
                  L1_WriteMiss+=1;
                  L2_Read+=1;
                  L1_Write+=1;
                  L1.dirtydBit[L1_indexBits][maxLRU[0]]=1;
               }
               else if(rw == 'r' && L1.dirtydBit[L1_indexBits][maxLRU[0]]==0){
                  L1_ReadMiss+=1;
                  L2_Read+=1;
                  L1_Read+=1;
                  L1.dirtydBit[L1_indexBits][maxLRU[0]]=0;
               }
               L1.incrementLRU(L1.LRUBit, L1_indexBits, params.L1_ASSOC, maxLRU[1]);
               L1.LRUBit[L1_indexBits][maxLRU[0]]=1;
               L1.cache_x[L1_indexBits][maxLRU[0]]=L1_tagBits;
               L1.cache_whole[L1_indexBits][maxLRU[0]]=L1_whole;
            }
            //if L1 ways are not full
            else{
               for(uint32_t i=0;i<params.L1_ASSOC;i++){
                  if(L1.validBit[L1_indexBits][i]==0){
                     int temp;
                     temp=L1.LRUBit[L1_indexBits][i];
                     L1.incrementLRU(L1.LRUBit, L1_indexBits, params.L1_ASSOC, temp);
                     L1.LRUBit[L1_indexBits][i]=1;
                     L1.cache_x[L1_indexBits][i]=L1_tagBits;
                     L1.cache_whole[L1_indexBits][i]=L1_whole;
                     L1.validBit[L1_indexBits][i]=1;

                     if (rw == 'w'){
                        L1_WriteMiss+=1;
                        L2_Read+=1;
                        L1_Write+=1;
                        L1.dirtydBit[L1_indexBits][i]=1;
                     }
                     else{
                        L1_ReadMiss+=1;
                        L2_Read+=1;
                        L1_Read+=1;
                        L1.dirtydBit[L1_indexBits][i]=0;
                     }
                     break;
                  }
               }
            }
            
            if(params.PREF_N>0 && params.PREF_M>0){
               if(L2.sbHit(L2.streamBuffer, params.PREF_N, params.PREF_M, L2_memoryAddress)){
                  int hit = L2.getM(L2.streamBuffer, params.PREF_N, params.PREF_M, L2_memoryAddress, L2.LRU_SB);
                  int k = L2.getnumberSB(L2.streamBuffer, params.PREF_N, params.PREF_M, L2_memoryAddress, L2.LRU_SB);
                  L2.prefetchSB(L2.streamBuffer, params.PREF_M, k, L2_memoryAddress);
                  L2.incrementLRUSB(L2.LRU_SB, params.PREF_N, k);
                  L2.LRU_SB[k]=1;
                  L2_Prefetches+=hit;
               }
            }
            //reading it from L2 cache
            for(uint32_t i=0;i<params.L2_ASSOC;i++){
               if (L2.cache_x[L2_indexBits][i]==L2_tagBits){
                  temp=L2.LRUBit[L2_indexBits][i];
                  L2.incrementLRU(L2.LRUBit, L2_indexBits, params.L2_ASSOC, temp);
                  L2.LRUBit[L2_indexBits][i]=1;
                  break;
               }
            }
         }
         //if it's not present in L1 and not present in L2 cache
         else if(!L1.cacheHit(L1.cache_x, L1_indexBits, L1_tagBits, params.L1_ASSOC) && !L2.cacheHit(L2.cache_x, L2_indexBits, L2_tagBits, params.L2_ASSOC)){
            //if L1 ways are full
            if(L1.isSetWaysFull(L1_indexBits, params.L1_ASSOC, L1.validBit)){
               vector<int> maxLRU;
               maxLRU.resize(2);
               maxLRU = L1.evictLRU(L1.LRUBit, L1_indexBits, params.L1_ASSOC);
               //int x = L1.cache_x[L1_indexBits][maxLRU[0]];
               int y = L1.cache_whole[L1_indexBits][maxLRU[0]];
               char z[33];
               baseconvert(y, z, 2);
               //itoa(y,z,2);
               string evictWhole(z);
               int lx=evictWhole.length();
               int xk = 32 - min(32,lx);
               evictWhole.insert(0,xk,'0');
               //char q[32];
               //baseconvert(x, q, 2);
               //itoa(x,q,2);
               //string evictTag(q);
               //int lex=L1.tag-evictTag.length();
               //evictTag.insert(0,lex,'0');
               
               if (rw == 'w' && L1.dirtydBit[L1_indexBits][maxLRU[0]]==1){
                  L1_WriteMiss+=1;
                  L1.writeBack(evictWhole, params.L2_ASSOC, params.PREF_N, params.PREF_M, L2);
                  L1_WriteBack+=1;
                  L2_Write+=1;
                  if(params.PREF_N==0 && params.PREF_M==0){
                     L2_ReadMiss+=1;
                     TolMemTraffic+=1;
                  }
                  L2_Read+=1;
                  L1_Write+=1;
                  
                  //cout<<"def"<<endl;
                  L1.dirtydBit[L1_indexBits][maxLRU[0]]=1;
               }
               else if(rw == 'r' && L1.dirtydBit[L1_indexBits][maxLRU[0]]==1){
                  L1_ReadMiss+=1;
                  L1.writeBack(evictWhole, params.L2_ASSOC, params.PREF_N, params.PREF_M, L2);
                  L1_WriteBack+=1;
                  L2_Write+=1;
                  if(params.PREF_N==0 && params.PREF_M==0){
                     L2_ReadMiss+=1;
                     TolMemTraffic+=1;
                  }
                  L2_Read+=1;
                  L1_Read+=1;
                  L1.dirtydBit[L1_indexBits][maxLRU[0]]=0;
               }
               else if(rw == 'w' && L1.dirtydBit[L1_indexBits][maxLRU[0]]==0){
                  L1_WriteMiss+=1;
                  if(params.PREF_N==0 && params.PREF_M==0){
                     L2_ReadMiss+=1;
                     TolMemTraffic+=1;
                  }
                  L2_Read+=1;
                  L1_Write+=1;
                  L1.dirtydBit[L1_indexBits][maxLRU[0]]=1;
               }
               else if(rw == 'r' && L1.dirtydBit[L1_indexBits][maxLRU[0]]==0){
                   L1_ReadMiss+=1;
                   if(params.PREF_N==0 && params.PREF_M==0){
                     L2_ReadMiss+=1;
                     TolMemTraffic+=1;
                  }
                   L2_Read+=1;
                   L1_Read+=1;
               }
               L1.incrementLRU(L1.LRUBit, L1_indexBits, params.L1_ASSOC, maxLRU[1]);
               L1.LRUBit[L1_indexBits][maxLRU[0]]=1;
               L1.cache_x[L1_indexBits][maxLRU[0]]=L1_tagBits;
               L1.cache_whole[L1_indexBits][maxLRU[0]]=L1_whole; 
            }
            //if L1 ways are not full
            else{
               for(uint32_t i=0;i<params.L1_ASSOC;i++){
                  if(L1.validBit[L1_indexBits][i]==0){
                      int temp;
                      temp=L1.LRUBit[L1_indexBits][i];
                      L1.incrementLRU(L1.LRUBit, L1_indexBits, params.L1_ASSOC, temp);
                      L1.LRUBit[L1_indexBits][i]=1;
                      L1.cache_x[L1_indexBits][i]=L1_tagBits;
                      L1.cache_whole[L1_indexBits][i]=L1_whole;
                      L1.validBit[L1_indexBits][i]=1;
                      if (rw == 'w'){
                        L1_WriteMiss+=1;
                        if(params.PREF_N==0 && params.PREF_M==0){
                           L2_ReadMiss+=1;
                           TolMemTraffic+=1;
                        }
                        L2_Read+=1;
                        L1_Write+=1;
                        L1.dirtydBit[L1_indexBits][i]=1;
                      }
                      else if(rw == 'r'){
                        L1_ReadMiss+=1;
                        if(params.PREF_N==0 && params.PREF_M==0){
                           L2_ReadMiss+=1;
                           TolMemTraffic+=1;
                        }
                        L2_Read+=1;
                        L1_Read+=1;
                        L1.dirtydBit[L1_indexBits][i]=0;
                      }
                      //cout<<"def"<<endl;
                      //out<<L1_indexBits<<" "<<L1.validBit[L1_indexBits][i]<<" "<<L1_tagBits<<" "<<L1.LRUBit[L1_indexBits][i]<<" "<<L1.dirtydBit[L1_indexBits][i]<<endl;
                      break;
                  }
               }
            }

            if(params.PREF_N>0 && params.PREF_M>0){
               if(L2.sbHit(L2.streamBuffer, params.PREF_N, params.PREF_M, L2_memoryAddress)){
                  int hit = L2.getM(L2.streamBuffer, params.PREF_N, params.PREF_M, L2_memoryAddress, L2.LRU_SB);
                  L2_Prefetches+=hit;
                  //TolMemTraffic+=hit;
               }
               else{
                  L2_Prefetches+=params.PREF_M;
                  //TolMemTraffic+=params.PREF_M;
                  if(rw == 'r' || rw == 'w'){
                     L2_ReadMiss+=1;
                  }
                  //else if(rw == 'w'){
                    // L1_WriteMiss+=1;
                  //}
               }
               int k = L2.getnumberSB(L2.streamBuffer, params.PREF_N, params.PREF_M, L2_memoryAddress, L2.LRU_SB);
               L2.prefetchSB(L2.streamBuffer, params.PREF_M, k, L2_memoryAddress);
               L2.incrementLRUSB(L2.LRU_SB, params.PREF_N, k);
               L2.LRU_SB[k]=1;
            }
            //if L2 ways are full
            if(L2.isSetWaysFull(L2_indexBits, params.L2_ASSOC, L2.validBit)){
               vector<int> maxLRU1;
               maxLRU1.resize(2);
               maxLRU1 = L2.evictLRU(L2.LRUBit, L2_indexBits, params.L2_ASSOC);
               L2.incrementLRU(L2.LRUBit, L2_indexBits, params.L2_ASSOC, maxLRU1[1]);
               L2.LRUBit[L2_indexBits][maxLRU1[0]]=1;
               L2.cache_x[L2_indexBits][maxLRU1[0]]=L2_tagBits;
               L2.cache_whole[L2_indexBits][maxLRU1[0]]= L2_whole;

               if(L2.dirtydBit[L2_indexBits][maxLRU1[0]]==1){
                  L2_WriteBack+=1;
                  if(params.PREF_N==0 && params.PREF_M==0){
                     TolMemTraffic+=1;
                  } 
               }
               if (rw == 'w'){
                  L2.dirtydBit[L2_indexBits][maxLRU1[0]]=0;
               }
               else if (rw == 'r'){
                  L2.dirtydBit[L2_indexBits][maxLRU1[0]]=0;
               }
            }
            //if L2 ways are not full
            else{
               for(uint32_t i=0;i<params.L2_ASSOC;i++){
                  if(L2.validBit[L2_indexBits][i]==0){
                      int temp;
                      temp=L2.LRUBit[L2_indexBits][i];
                      L2.incrementLRU(L2.LRUBit, L2_indexBits, params.L2_ASSOC, temp);
                      L2.LRUBit[L2_indexBits][i]=1;
                      L2.cache_x[L2_indexBits][i]=L2_tagBits;
                      L2.cache_whole[L2_indexBits][i]=L2_whole;
                      L2.validBit[L2_indexBits][i]=1;
                      break;
                  }
               }
            }
            
         }
      }
   }

      L1_MissRate = (double)(L1_ReadMiss + L1_WriteMiss)/(double)(L1_Read + L1_Write);
      if(params.L2_SIZE>0){
         L2_MissRate = (double)(L2_ReadMiss)/(double)(L2_Read);
      }

      L1.bubbleSort(L1.LRUBit, L1.dirtydBit, L1.cache_x, L1.sets, params.L1_ASSOC);
      if(params.L2_SIZE>0){
         L2.bubbleSort(L2.LRUBit, L2.dirtydBit, L2.cache_x, L2.sets, params.L2_ASSOC);
      }
      if(params.L2_SIZE==0 && params.PREF_N>0 && params.PREF_M>0){
         L1.bubble(L1.streamBuffer, L1.LRU_SB, params.PREF_N, params.PREF_M);
      }
      else if(params.L2_SIZE>0 && params.PREF_N>0 && params.PREF_M>0){
         L2.bubble(L2.streamBuffer, L2.LRU_SB, params.PREF_N, params.PREF_M);
      }

      if(params.L2_SIZE==0 && params.PREF_N>0 && params.PREF_M>0){
         TolMemTraffic = L1_ReadMiss + L1_WriteMiss + L1_WriteBack + L1_Prefetches;
      }
      else if(params.L2_SIZE>0 && params.PREF_N>0 && params.PREF_M>0){
         TolMemTraffic = L2_ReadMiss + L2_WriteMiss + L2_WriteBack + L2_Prefetches;
      }

      char ab[32];
      cout<<"===== L1 contents ====="<<"\n";
      for(int i=0;i<L1.sets;i++)
      {
         if(!L1.isSetWaysEmpty(i, params.L1_ASSOC, L1.LRUBit)){
            cout<<"set"<<"      "<<i<<":"<<"    ";
            for(uint32_t j=0;j<params.L1_ASSOC;j++){
               if(L1.LRUBit[i][j]!=0){
                  baseconvert(L1.cache_x[i][j], ab, 16);
                  //itoa(L1.cache_x[i][j],ab,16);
                  cout<<ab<<" ";
                  if(L1.dirtydBit[i][j]==1){
                     cout<<"D"<<"   ";
                  }
                  else{
                     cout<<"    ";
                  }
               }
               else{
                  cout<<" "<<" ";
                  cout<<"   ";
               }
            }
            cout<<"\n";
         }
      }
      if(params.L2_ASSOC>0){
         cout<<"\n";
         //cout<<"\n";
         char abc[32];
         cout<<"===== L2 contents ====="<<"\n";
      for(int i=0;i<L2.sets;i++)
      {
         if(!L2.isSetWaysEmpty(i, params.L2_ASSOC, L2.LRUBit)){
            cout<<"set"<<"      "<<i<<":"<<"    ";
            for(uint32_t j=0;j<params.L2_ASSOC;j++){
               if(L2.LRUBit[i][j]!=0){
                  baseconvert(L2.cache_x[i][j], abc, 16);
                  //itoa(L2.cache_x[i][j],abc,16);
                  cout<<abc<<" ";
                  if(L2.dirtydBit[i][j]==1){
                  cout<<"D"<<"   ";
                  }
                  else{
                  cout<<"    ";
                  }
               }
               else{
                  cout<<" "<<" ";
                  cout<<"   ";
               }
            }
            cout<<"\n";
         }
      }
      }

      //out<<"\n";
      if(params.L2_SIZE>0 && params.PREF_N>0 && params.PREF_M>0){
         cout<<"\n";
         cout<<"===== Stream Buffer(s) contents ====="<<"\n";
         char abc[32];
         if(!L2.isBufferWaysEmpty(params.PREF_N, L2.LRU_SB)){
            for(uint32_t i=0;i<params.PREF_N;i++){
            //out<<"set"<<"      "<<i<<":"<<"    ";
            cout<<" ";
            for(uint32_t j=0;j<params.PREF_M;j++){
               baseconvert(L2.streamBuffer[i][j], abc, 16);
               //itoa(L2.cache_x[i][j],abc,16);
               cout<<abc<<" ";
            }  
         cout<<"\n";
         }
         }
      }
      else if(params.L2_SIZE==0 && params.PREF_N>0 && params.PREF_M>0){
         cout<<"\n";
         cout<<"===== Stream Buffer(s) contents ====="<<"\n";
         char abc[32];
         if(!L1.isBufferWaysEmpty(params.PREF_N, L1.LRU_SB)){
            for(uint32_t i=0;i<params.PREF_N;i++){
            //out<<"set"<<"      "<<i<<":"<<"    ";
            cout<<" ";
            for(uint32_t j=0;j<params.PREF_M;j++){
               baseconvert(L1.streamBuffer[i][j], abc, 16);
               //itoa(L2.cache_x[i][j],abc,16);
               cout<<abc<<" ";
            }  
         cout<<"\n";
         }
         }
      }
      
      cout<<"\n";
      cout<<"===== Measurements ====="<<"\n";
      cout<<"a. L1 reads:                       "<<L1_Read<<endl;
      cout<<"b. L1 read misses:                 "<<L1_ReadMiss<<endl;
      cout<<"c. L1 writes:                      "<<L1_Write<<endl;
      cout<<"d. L1 write misses:                "<<L1_WriteMiss<<endl;
      cout<<"e. L1 miss rate:                   "<<fixed<<setprecision(4)<<L1_MissRate<<endl;
      cout<<"f. L1 writebacks:                  "<<L1_WriteBack<<endl;
      cout<<"g. L1 prefetches:                  "<<L1_Prefetches<<endl;
      cout<<"h. L2 reads (demand):              "<<L2_Read<<endl;
      cout<<"i. L2 read misses (demand):        "<<L2_ReadMiss<<endl;
      cout<<"j. L2 reads (prefetch):            "<<L2_PrefetchRead<<endl;
      cout<<"k. L2 read misses (prefetch):      "<<L2_PrefetchReadMiss<<endl;
      cout<<"l. L2 writes:                      "<<L2_Write<<endl;
      cout<<"m. L2 write misses:                "<<L2_WriteMiss<<endl;
      cout<<"n. L2 miss rate:                   "<<fixed<<setprecision(4)<<L2_MissRate<<endl;
      cout<<"o. L2 writebacks:                  "<<L2_WriteBack<<endl;
      cout<<"p. L2 prefetches:                  "<<L2_Prefetches<<endl;
      cout<<"q. memory traffic:                 "<<TolMemTraffic<<endl;      
 
      //out.close();
   return(0);
   }


