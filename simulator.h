#include <bits/stdc++.h>
using namespace std;


enum opcode {add,sub,mul,div};
struct InstrRecord
{
  int opcode;        // opcode of the instruction; 0-add,1-sub,2-mul,3-div
  int dst;           // destination register
  int src1,src2;     // source registers
};

struct ReservationStation
{
  bool busy;
  int Qj;
  int Qk;
  int Vj;
  int Vk;
  int lat;
  int op;
  int result;
  bool resultReady;
  int instNum;
  int ISSUE_Lat;
  int WRITEBACK_Lat;
};

queue<InstrRecord> instructionQ;
vector<int> registerFile(8);    // Register File having 8 entries 0 to 7
vector<int> rat(8,-1);          // if RAT entry is -1, it means the RAT points to the register in the register file and not any RS
