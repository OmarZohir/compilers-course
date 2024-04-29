#include "llvm/Pass.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include <iterator>
#include <iostream>
#include <set>

using namespace llvm;

namespace {

class ResourceManager{
  //Assumptions:
  //1) All resources can provide any functionality needed by any instruction
  //2) Every instruction uses only one resource at a time
public:
  int totalRes, UsedRes;
  
  ResourceManager(int n_resources){
    totalRes = n_resources;
  }

  void reset(){
    UsedRes = 0;
  }

  bool canSchedule(Instruction *I){
    return (totalRes - UsedRes);  
  }

  void schedule(Instruction *I){
    UsedRes += 1; totalRes -= 1;
  }
};

class ILPEstimator : public BasicBlockPass {
public:
  static char ID;
  int WCET;
  int maxLatency;
  int NumResources = 10;
  
  std::map<const Instruction *, int> ASAPschedule;
  std::map<const Instruction *, int> ALAPschedule;
  std::set<Instruction *> ReadyList;
  
  ILPEstimator() : BasicBlockPass(ID) {}

  ResourceManager rm = ResourceManager(NumResources);

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

    //Printing the ASAP and ALAP schedule
    for (auto it = ALAPschedule.begin(); it != ALAPschedule.end(); it++){
      it->first->dump(); 
      OS << "ALAP:  " <<  it->second << "\n"; 
      OS << "ASAP: " << ASAPschedule.at(it->first) << "\n";
      OS << "Slack: " << it->second -  ASAPschedule.at(it->first) << "\n";
    }

    OS << "Maximum Latency is: " << maxLatency << "\n";
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
      }
      //2)Compute instr ALAP
      if(!isa<PHINode>(&*it)) {
        schedule[&*it] = EarliestUserALAP  - getInstructionLatency(&*it);
      }
    }
  }


private: 
  int instructionScore(Instruction *I){
    //Get the slack of this instuction
    int criticalDistance = ALAPschedule[&*I] - ASAPschedule[&*I];
    //Assuming maximum slack that can actually happen is 10,000 
    int BasicScore = 10000 - criticalDistance;

    if(rm.canSchedule(I))
      return std::max(BasicScore,0); //just in case there is a slack that is larger than 10,000
    else
      return 0;

  }


  Instruction *selectNextOperation(std::set<Instruction *> &ready_list, ResourceManager &rm){
    Instruction *nextOperation = NULL;
    int maxScore = 0;
    int currentScore = 0;

    for (auto it : ready_list){
      //if an instruction exists in the ready list
      if (&*it) {          
        currentScore = instructionScore(&*it); 
        if(currentScore > maxScore){
          nextOperation = &*it; maxScore = currentScore;
        }
      }
    }
  }
}; // end of class ILPEstimator

}  // end of anonymous namespace


char ILPEstimator::ID = 0;

static RegisterPass<ILPEstimator> X("ilp-estimate", "ILP Estimator Pass",
                                    false /* Only looks at CFG */,
                                    false /* Analysis Pass */);

