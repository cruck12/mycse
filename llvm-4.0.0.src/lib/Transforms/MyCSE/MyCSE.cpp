#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include <set>
using namespace llvm;

namespace {
  struct expr {
    Instruction *inst;
    unsigned opcode;
    Value *op0, *op1;
  };
}

static std::vector<expr>::iterator find_inst(std::vector<expr> &vec, Instruction *inst)
{
  for (std::vector<expr>::iterator it = vec.begin(), ed = vec.end(); it != ed; it++) {
    if (it->inst == inst) return it;
  }
  return vec.end();
}

static std::vector<expr>::iterator find_ops(
    std::vector<expr> &vec, unsigned opcode, Value *op0, Value *op1)
{
  for (std::vector<expr>::iterator it = vec.begin(), ed = vec.end(); it != ed; it++) {
    if (it->opcode == opcode && it->op0 == op0 && it->op1 == op1) return it;
  }
  return vec.end();
}

// Replace all uses toward the instruction
static void replace_use(Instruction *replaced, Instruction *replacer)
{
  errs() << "\nAll uses from the following users have been replaced.\n";
  for (User *user : replaced->users()) {
    errs() << "User " << user;
    if (Instruction *user_inst = dyn_cast<Instruction>(user)) {
      errs() << " (" << *user_inst << ")";
      for (Use &user_inst_use : user_inst->operands()) {
        if (dyn_cast<Instruction>(user_inst_use.get()) == replaced) {
          user_inst_use.set(replacer);
        }
      }
    }
    errs() << "\n";
  }
}

namespace {
  struct MyCSE : public BasicBlockPass {
    static char ID;
    MyCSE() : BasicBlockPass(ID) {}

    bool runOnBasicBlock(BasicBlock &BB) override {
      std::vector<Instruction*> dead;
      std::vector<expr> vec;
      std::vector<expr>::iterator vit;
      
      for (BasicBlock::iterator it = BB.begin(), ed = BB.end(); it != ed; it++) {
        Instruction *inst = &*it;
        int op_num = inst->getNumOperands();
        expr newexpr;

        errs() << "Instruction:\n" << inst << " (" << *inst << ")\n";
        if (inst->getOpcode() == Instruction::Alloca) {
          // VN table should contain VNs of all alloca instructions.
          newexpr.inst = inst;
          newexpr.opcode = Instruction::Alloca;
          newexpr.op0 = inst->getOperand(0);
          newexpr.op1 = NULL;
          vec.insert(vec.end(), newexpr);
        } else if (inst->getOpcode() == Instruction::Store) {
          // Be careful
          vit = find_ops(vec, Instruction::Load, inst->getOperand(1), NULL);
          if (vit != vec.end()) {
            vec.erase(vit);
            errs() << "\nEntry " << vit->inst << " is removed from VN table.\n";
          }
        } else if (op_num >= 1 && op_num <= 2) {
          Value *vop[2];
          Instruction *iop[2];
          int i;

          for (i = 0; i < op_num; i++) {
            vop[i] = inst->getOperand(i);
            iop[i] = dyn_cast<Instruction>(vop[i]);
          }
          for (; i < 2; i++) {
            vop[i] = NULL;
            iop[i] = NULL;
          }

          for (i = 0; i < op_num; i++) {
            if (iop[i] != NULL && find_inst(vec, iop[i]) == vec.end()) {
              goto insert_inst;
            }
          }
          if ((vit = find_ops(vec, inst->getOpcode(), vop[0], vop[1])) == vec.end()) {
            goto insert_inst;
          }
          replace_use(inst, vit->inst);
          dead.insert(dead.end(), inst);
          goto end_if;

insert_inst:
          newexpr.inst = inst;
          newexpr.opcode = inst->getOpcode();
          newexpr.op0 = vop[0];
          newexpr.op1 = vop[1];
          vec.insert(vec.end(), newexpr);
          end_if:;
        }

        errs() << "\nVN table:\n";
        for (vit = vec.begin(); vit != vec.end(); vit++) {
          errs() << vit->inst << "\t(" << *(vit->inst) << ")\t" << vit->opcode \
                 << "\t" << vit->op0 << "\t" << vit->op1 << "\n";
        }
        errs() << "==================================================================\n";
      }

      for (std::vector<Instruction*>::iterator it = dead.begin(), ed = dead.end();
           it != ed; it++) {
        (*it)->eraseFromParent();
      }

      vec.clear();
      dead.clear();
      return true;
    }

    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.setPreservesCFG();
    }
  };
}

char MyCSE::ID = 0;
static RegisterPass<MyCSE> X("mycse", "My Common Subexpression Elimination");
