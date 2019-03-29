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
      if(RS[i].busy && RS[i].Qj==-1 && RS[i].Qk==-1)
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
      if(RS[i].busy && RS[i].Qj==-1 && RS[i].Qk==-1)
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

SameCycleUpdate issue(int cycle)
{
  SameCycleUpdate update;
  update.needsUpdate=false;
  if(!instructionQ.empty())
  {
    InstrRecord instr=instructionQ.front();
    bool rsAvailable=false;int i=-1;
    if(instr.opcode<=1)
    {
      for(i=0;i<=2;i++)
        if(RS[i].busy==false)
        {
          rsAvailable=true;
          break;
        }
    }
    else
    {
      for(i=3;i<=4;i++)
        if(RS[i].busy==false)
        {
          rsAvailable=true;
          break;
        }
    }
    if(rsAvailable)
    {
        // issue the instruction from the instr Q to the RS i
        instructionQ.pop();
        RS[i].busy=true;
        RS[i].opcode=isntr.opcode;
        RS[i].dispatch=0;
        int operand1=instr.src1,operand2=instr.src2;
        if(RAT[operand1]==-1)                   // RAT points to registers
        {
          RS[i].Vj=registerFile[operand1]      // get value from register file
          RS[i].Qj=-1;                          // not waiting for anything
        }
        else
        {
          RS[i].Qj=registerFile[operand1]      // tag of first RS its waiting for
        }
        if(RAT[operand2]==-1)                   // RAT points to registers
        {
          RS[i].Vk=registerFile[operand2]      // get value from register file
          RS[i].Qk=-1;                         // not waiting for anything
        }
        else
        {
          RS[i].Qk=registerFile[operand2]      // tag of second RS its waiting for
        }
        update.rsTag=i;
        update.needsUpdate=true;
        update.opRegister=instr.dst;
    }

  }
  return update;
}

int main()
{
  int cycle=1,targetCycle=100;
  while(cycle<=targetCycle)
  {
    dispatch(cycle);
    SameCycleUpdate update=issue();
  //  broadcast();
    cycle++;
  }
}
