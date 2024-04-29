#include "llvm/Pass.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include <iterator>
#include <iostream>

using namespace llvm;

namespace {
class ILPEstimator : public BasicBlockPass {
public:
  static char ID;
  int WCET;
  int maxLatency;
  std::map<const Instruction *, int> ASAPschedule;
  std::map<const Instruction *, int> ALAPschedule;
  //an extra map for cleaner printing
  std::map<const Instruction *, int, int> ASAPALAPschedule;
  ILPEstimator() : BasicBlockPass(ID) {}
  
  bool runOnBasicBlock(BasicBlock &BB) override {
    errs() << "Hello: ";
    errs().write_escaped(BB.getName()) << "\n";

    WCET = estimateWCET(BB);

    ASAPschedule.clear();
    ALAPschedule.clear();
    
    maxLatency = scheduleASAP(BB,ASAPschedule);
    scheduleALAP(BB,ALAPschedule,maxLatency);
    
    return false;
  }

  void print(raw_ostream &OS, const Module* = 0) const override {
	  OS << "WCET estimate: " << WCET << "\n";

    //Printing the ASAP and the ALAP schedule
    // for (auto iter = ASAPschedule.begin(); iter != ASAPschedule.end(); iter++){
    //   iter->first->dump(); 
    //   OS << "ASAP:  " <<  iter->second << "\n";
    //   OS << "ALAP: " << ALAPschedule.at(iter->first) << "\n"; 
    // }

    //Printing the ALAP schedule
    for (auto it = ALAPschedule.begin(); it != ALAPschedule.end(); ++it){
      it->first->dump(); 
      OS << "ALAP:  " <<  it->second << "\n"; 
      OS << "ASAP: " << ASAPschedule.at(it->first) << "\n";
    }

    OS << "Maximum Latency for this basic block is: " << maxLatency << "\n";
  }


  int getInstructionLatency(const Instruction *I)
  {
    if(!I) return 0;
    if(isa<LoadInst>(I)) return 2;
    switch(I->getOpcode()) 
    {
      case Instruction::Mul:
         return 2;
      default:
        return 1;
    }
  }

  int estimateWCET(BasicBlock &BB) {
    int wcet = 0;
    for(Instruction &inst : BB) 
    { 
      wcet += getInstructionLatency(&inst); 
    }
    return wcet;
  }

  int scheduleASAP(BasicBlock &BB, std::map<const Instruction *, int> &schedule) {
    int maxLatency = 0;

    for(Instruction &inst : BB) {
      for(User *user : inst.users()) {
        Instruction *I = dyn_cast<Instruction>(user); 
        if(I && I->getParent() == &BB && !isa<PHINode>(I)) {
        // longest-path algorithm
          schedule[&(*I)] = std::max(schedule[&(*I)], schedule[&inst] + getInstructionLatency(&inst));
        }
      }
      maxLatency = std::max(maxLatency, schedule[&inst] + getInstructionLatency(&inst));
    }
    return maxLatency;
}

  void scheduleALAP(BasicBlock &BB, std::map<const Instruction *, int> &schedule, int ASAPFinishTime) {
    // Iterate over BB’s instructions in reverse
    for(auto it=BB.rbegin(), end=BB.rend(); it != end; it++) {
      int EarliestUserALAP = ASAPFinishTime;
      for(User *user : it->users()) {
        Instruction *I = dyn_cast<Instruction>(user);
        //1)Find earliest ALAP of all of the users of an instruction
        if(I && I->getParent() == &BB && !isa<PHINode>(I)) {
          if (schedule[&*I] > 0)
            EarliestUserALAP = std::min(schedule[&*I], EarliestUserALAP);
        }
        //it->dump();
        // if(I && I->getParent() == &BB && !isa<PHINode>(I)) {
        //   schedule[&*it] = std::min(schedule[&*I] - getInstructionLatency(&*it), schedule[&*it]); 
        //   it->dump();
        // }
        // //i.e., instruction has no users 
        // else if (!I)
        // {
        //   schedule[&*it] = std::min(ASAPFinishTime - getInstructionLatency(&*it), schedule[&*it])
        // }
      }
      //2)Compute instr ALAP
      if(!isa<PHINode>(&*it)) {
        schedule[&*it] = EarliestUserALAP  - getInstructionLatency(&*it);
      }
      //std::cout << "ALAP Schedule: " << schedule.at(&*it) << "\n";
    }
  }
}; // end of class ILPEstimator
}  // end of anonymous namespace


char ILPEstimator::ID = 0;

static RegisterPass<ILPEstimator> X("ilp-estimate", "ILP Estimator Pass",
                                    false /* Only looks at CFG */,
                                    false /* Analysis Pass */);
