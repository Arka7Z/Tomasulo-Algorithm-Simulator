#include "simulator.h"

int calculate_result(int opcode, int operand1, int operand2)          // calculate the result of an instruction with operands already obtained;
{
  switch (opcode) {
    case 0: return (operand1+operand2);
    case 1: return (operand1-operand2);
    case 2: return (operand1*operand2);
    case 3: return (operand1/operand2);
    }
    return INT_MIN;
}

void  dispatch_to_add_unit(int cycle)
{
  if(cycle>=addUnit.writebackCycle)                 // Add Unit not currently executing something
  {
    bool rsReady=false;
    int i;
    for(i=0;i<=2;i++)                          // poll the RS in decreasing priority to check readiness
      if(RS[i].busy && RS[i].Qj!=-1 && RS[i].Qk!=-1)
      {
        rsReady=true;
        break;
      }
      if(rsReady)                                  // Reservation Station ready to go
      {
        RS[i].dispatch=1;
        RS[i].writebackCycle=cycle+add_latency;     // cycle the instruction will attempt a writeback
        addUnit.result=RS[i].result=calculate_result(RS[i].opcode,RS[i].Vj,RS[i].Vk);
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
    int i;
    for(i=3;i<=4;i++)                          // poll the RS in decreasing priority to check readiness
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
        mulUnit.result=RS[i].result=calculate_result(RS[i].opcode,RS[i].Vj,RS[i].Vk);
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
  int cycle=1,targetCycle=100;
  while(cycle<=targetCycle)
  {
    dispatch(cycle);
    //issue();
  //  broadcast();
    cycle++;
  }
}
