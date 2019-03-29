#include "simulator.h"


void  dispatch_to_add_unit(int cycle)
{
  if(cycle>=addUnit.writebackCycle)                 // Add Unit not currently executing something
  {
    bool rsReady=false;
    for(int i=0;i<=2;i++)                          // poll the RS in decreasing priority to check readiness
      if(RS[i].busy && RS[i].Qj!=-1 && RS[i].Qk!=-1)
      {
        rsReady=true;
        break;
      }
      if(rsReady)                                  // Reservation Station ready to go
      {
        RS[i].dispatch=1;
        RS[i].writebackCycle=cycle+add_latency;     // cycle the instruction will attempt a writeback
        addUnit.busy=true;
        addUnit.writebackCycle=cycle+add_latency;   // cycle the unit will attempt a writeback
      }
  }

}

void  dispatch_to_mul_unit(int cycle)
{
  if(cycle>=mulUnit.writebackCycle)                 // Add Unit not currently executing something
  {
    bool rsReady=false;
    for(int i=3;i<=4;i++)                          // poll the RS in decreasing priority to check readiness
      if(RS[i].busy && RS[i].Qj!=-1 && RS[i].Qk!=-1)
      {
        rsReady=true;
        break;
      }
      if(rsReady)                                  // Reservation Station ready to go
      {
        RS[i].dispatch=1;
        RS[i].writebackCycle=cycle+(RS[i].opcode==2?mul_latency:div_latency);     // cycle the instruction will attempt a writeback
        mulUnit.busy=true;
        mulUnit.writebackCycle=cycle+(RS[i].opcode==2?mul_latency:div_latency);   // cycle the unit will attempt a writeback
      }
  }

}

void dispatch(int cycle)
{
  dispatch_to_add_unit(cycle);
  dispatch_to_mul_unit(cycle);
}

int main()
{

}
