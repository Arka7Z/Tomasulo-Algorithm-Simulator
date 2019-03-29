#include <bits/stdc++.h>
using namespace std;

#define add_cycles 2
#define sub_cycles 2
#define mul_cycles 10
#define div_cycles 40


struct InstrRecord
{
  int opcode;        // opcode of the instruction; 0-add,1-sub,2-mul,3-div
  int dst;           // destination register
  int src1,src2;     // source registers
};

struct Broadcast{
  int rsTag;
  int val;
};

struct ExecutionUnit{
  int rsTag;
  int val;
  int writebackCycle;
  bool busy=false;
};

struct ReservationStation
{
  bool busy=false;
  int Qj=-1;                    // tag of first RS its waiting for
  int Qk=-1;                    // tag of second RS its waiting for
  int Vj;                       // value of operand 1
  int Vk;                       // value of operand 2
  int latency;
  int op;
  int result;
  bool resultReady;
  int instNum;
  int disp=0;                   // 0: not dispatched, 1: dispatched.
  int writebackCycle;           // cycle it will attempt writeback
};

queue<InstrRecord> instructionQ;
vector<int> registerFile(8);        // Register File having 8 entries 0 to 7
vector<int> rat(8,-1);              // if RAT entry is -1, it means the RAT points to the register in the register file and not any RS
ExecutionUnit addUnit, mulUnit;
vector<ReservationStation> RS(5);   // Reservation statisons
