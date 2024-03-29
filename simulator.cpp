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

void  dispatch_to_add_unit(int cycle,Broadcast& broadCast,bool& instrsDispatchced)
{
  if(cycle>addUnit.writebackCycle || (cycle==addUnit.writebackCycle && cycle!=mulUnit.writebackCycle))                 // Add Unit not currently executing something
  {
    bool rsReady=false;
    int i;
    for(i=0;i<=2;i++)                          // poll the RS in decreasing priority to check readiness
      if(RS[i].busy && RS[i].Qj==-1 && RS[i].Qk==-1 && RS[i].dispatch==0)
      {
        rsReady=true;
        break;
      }
      if(rsReady)                                  // Reservation Station ready to go
      {
        if(addUnit.writebackCycle==cycle)                         // save the broadCast results before issuing the instruction to the unit
        {
          broadCast.willBroadcast=true;
          broadCast.result=addUnit.result;
          broadCast.rsTag=addUnit.rsTag;
        }
        RS[i].dispatch=cycle;
        RS[i].writebackCycle=cycle+add_latency;     // cycle the instruction will attempt a writeback
        addUnit.result=RS[i].result=calculate_result(RS[i].opcode,RS[i].Vj,RS[i].Vk);
        addUnit.busy=true;
        addUnit.writebackCycle=cycle+add_latency;   // cycle the unit will attempt a writeback
        addUnit.rsTag=i;
        broadCast.freeUnit=false;
        broadCast.unitIsAdd=true;
        instrsDispatchced=true;
      }
  }


}

void  dispatch_to_mul_unit(int cycle, Broadcast& broadCast,bool& instrsDispatchced)
{
  if(cycle>=mulUnit.writebackCycle)                 // Add Unit not currently executing something
  {
    bool rsReady=false;
    int i;
    for(i=3;i<=4;i++)                          // poll the RS in decreasing priority to check readiness
      if(RS[i].busy && RS[i].Qj==-1 && RS[i].Qk==-1 && RS[i].dispatch==0)
      {
        rsReady=true;
        break;
      }
      if(rsReady)                                  // Reservation Station ready to go
      {
        if(mulUnit.writebackCycle==cycle)                           // save the broadCast results before issuing the instruction to the unit
        {
          broadCast.willBroadcast=true;
          broadCast.result=mulUnit.result;
          broadCast.rsTag=mulUnit.rsTag;
        }
        RS[i].dispatch=cycle;
        RS[i].writebackCycle=cycle+(RS[i].opcode==2?mul_latency:div_latency);     // cycle the instruction will attempt a writeback
        mulUnit.busy=true;
        mulUnit.writebackCycle=cycle+(RS[i].opcode==2?mul_latency:div_latency);   // cycle the unit will attempt a writeback
        mulUnit.result=RS[i].result=calculate_result(RS[i].opcode,RS[i].Vj,RS[i].Vk);
        mulUnit.rsTag=i;
        broadCast.freeUnit=false;
        broadCast.unitIsAdd=false;
        instrsDispatchced=true;
      }
  }

}

Broadcast dispatch(int cycle,bool& instrsDispatchced)                  // Function to dispatch any available instr. to the mul XOR add unit
{
  if(cycle==addUnit.writebackCycle && cycle==mulUnit.writebackCycle)
    addUnit.writebackCycle++;
  Broadcast broadCastAdd;
  broadCastAdd.willBroadcast=false;
  broadCastAdd.freeUnit=true;
  Broadcast broadCastMul;
  broadCastMul.willBroadcast=false;
  broadCastMul.freeUnit=true;
  dispatch_to_add_unit(cycle,broadCastAdd,instrsDispatchced);
  dispatch_to_mul_unit(cycle,broadCastMul,instrsDispatchced);
  if(broadCastMul.willBroadcast==true)                  // As during dispatch, the execution units are overwritten, broadcast scheduled, if any, are saved and returned for broadcast function
    return broadCastMul;
  else
    return broadCastAdd;
}

SameCycleUpdate issue(int cycle)                                                // issue an instruction(if possible) from the instruction queue
{
  SameCycleUpdate update;
  update.needsUpdate=false;
  if(!instructionQ.empty())
  {
    InstrRecord instr=instructionQ.front();
    bool rsAvailable=false;int i=-1;
    if(instr.opcode<=1)
    {
      for(i=0;i<=2;i++)                                                        // Poll RS for the add unit
        if(RS[i].busy==false)
        {
          rsAvailable=true;
          break;
        }
    }
    else
    {
      for(i=3;i<=4;i++)                                                        // Poll RS for the mul unit
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
        RS[i].opcode=instr.opcode;
        RS[i].dispatch=0;
        int operand1=instr.src1,operand2=instr.src2;
        if(RAT[operand1]==-1)                   // RAT points to registers
        {
          RS[i].Vj=registerFile[operand1];      // get value from register file
          RS[i].Qj=-1;                          // not waiting for anything
        }
        else
        {
          RS[i].Qj=RAT[operand1];     // tag of first RS its waiting for
        }
        if(RAT[operand2]==-1)                   // RAT points to registers
        {
          RS[i].Vk=registerFile[operand2];      // get value from register file
          RS[i].Qk=-1;                         // not waiting for anything
        }
        else
        {
          RS[i].Qk=RAT[operand2];     // tag of second RS its waiting for
        }
        update.rsTag=i;
        update.needsUpdate=true;
        update.opRegister=instr.dst;
    }

  }
  return update;
}

void updateRAT(SameCycleUpdate update)                                          // update RAT of the o/p register of the instruction dispatched in the current cycle
{
  RAT[update.opRegister]=update.rsTag;
}
void broadcast(int cycle, Broadcast& broadCast, bool& instrsDispatchced)        // broadcast RS tag and result if scheduled for the current cycle
{
  bool needToBroadcast=false;
  int rsTag,result;
  if(instrsDispatchced)     // check if any broadcast scheduled using broadCast as unit overwritten
  {
    if(broadCast.willBroadcast)
    {
      needToBroadcast=true;
      rsTag=broadCast.rsTag;
      result=broadCast.result;
      // no neeed to free execution unit as an instruction is dispatched there
    }
  }
  else                                       // Units not overwritten, check writeback using writebackCycles of units, add & mul wb conflict handled in dispatch stage
  {
      if(mulUnit.writebackCycle==cycle)
      {
        needToBroadcast=true;
        rsTag=mulUnit.rsTag;
        result=mulUnit.result;
        mulUnit.busy=false;
        mulUnit.writebackCycle=0;
      }
      else if(addUnit.writebackCycle==cycle)
      {
        needToBroadcast=true;
        rsTag=addUnit.rsTag;
        result=addUnit.result;
        addUnit.busy=false;
        addUnit.writebackCycle=0;
      }
  }
  if(needToBroadcast)
  {
    for(int i=0;i<8;i++)         // Update RAT
      if(RAT[i]==rsTag)
      {
        registerFile[i]=result;
        RAT[i]=-1;
      }
    for(int i=0;i<5;i++)                                       // Poll the RS to check if any instruction was waiting on this broadcast, by matching the RS tag of broadcast
    {
      if(RS[i].busy && RS[i].Qj==rsTag)
      {
        RS[i].Vj=result;
        RS[i].Qj=-1;
      }
      if(RS[i].busy && RS[i].Qk==rsTag)
      {
        RS[i].Vk=result;
        RS[i].Qk=-1;
      }
    }
    RS[rsTag].busy=false;
    RS[rsTag].dispatch=0;
  }
}

void init(int& targetCycle)                                       // read from the input file to populate the instructionQ and register file etc.
{
  int no_of_instructions;
  cin >> no_of_instructions;
	
	cin >> targetCycle;
	for(int i=0;i<no_of_instructions;++i){
		InstrRecord tmp;
		cin >> tmp.opcode >> tmp.dst >> tmp.src1>>tmp.src2;
		instructionQ.push(tmp);
	}
	for(int i=0;i<8;++i)
	{
		cin >>registerFile[i];
	}
}



string getOpcode(int opcode)                                        // convert opcode to string
{
  switch (opcode) {
    case 0: return "ADD";
    case 1: return "SUB";
    case 2: return "MUL";
    case 3: return "DIV";
    }
  return "";
}
void prettyPrint()                                                     // print the status of the sytem including Reservation Stations, RAT and register contents
{
  cout<<setw(8)<<"Entry"<<setw(8)<<"Busy"<<setw(8)<<"op"<<setw(8)<<"Vj"<<setw(8)<<"Vk" << setw(8)<< "Qj" << setw(8)<< "Qk" <<setw(8)<<"Disp" << endl;
  for(int i=0;i<RS.size();++i)
  {
    cout << setw(8) << "RS"+to_string(i) <<setw(8) << RS[i].busy << setw(8)<< (RS[i].busy?getOpcode(RS[i].opcode):"")<<setw(8)<<(RS[i].busy ?to_string(RS[i].Vj):"") <<setw(8)<<(RS[i].busy ?to_string(RS[i].Vk):"")<< setw(8)<< (RS[i].Qj==-1?"":"RS"+to_string(RS[i].Qj)) <<
      setw(8)<< (RS[i].Qk==-1?"":"RS"+to_string(RS[i].Qk)) <<setw(8)<< (RS[i].busy && RS[i].dispatch!=0 ?to_string(RS[i].dispatch):"") <<endl;
  }

  cout << "------------------------------------"<<endl;
  cout << setw(8) << " " << setw(8) << "RF" << setw(8) << "RAT" << endl;
  for(int i=0;i<8;++i)
  {
    cout << setw(8) << i << setw(8) << registerFile[i] << setw(8)
    << (RAT[i]==-1?"":"RS"+to_string(RAT[i])) << endl;
  }
  cout << "-----------------------------------"<<endl;
  cout<<"Instruction queue(printing from the front of the queue first):"<<endl;
  int count=instructionQ.size();                                        // print the instruction Queue
  while(count>0)
  {
    InstrRecord instr=instructionQ.front();
    instructionQ.pop();
    cout << setw(8) << getOpcode(instr.opcode) << setw(8) << "R" + to_string(instr.dst)+","
    << setw(8) << "R" + to_string(instr.src1)+"," << setw(8) << "R" + to_string(instr.src2)+","
    << endl;
    instructionQ.push(instr);
    count--;
  }
}

int main()
{
  int cycle=1,targetCycle;
  init(targetCycle);
  while(cycle<=targetCycle)
  {
   
    bool instrDispatchced=false;
    Broadcast broadCast=dispatch(cycle,instrDispatchced);///////////
    SameCycleUpdate update=issue(cycle);
    broadcast(cycle, broadCast,instrDispatchced);
    if(update.needsUpdate)
      updateRAT(update);    
    cycle++;
  }
  prettyPrint();

}
