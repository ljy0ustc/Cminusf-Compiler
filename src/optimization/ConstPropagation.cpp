#include "ConstPropagation.hpp"
#include "logging.hpp"
#include <unordered_map>
#include <unordered_set>

// 给出了返回整形值的常数折叠实现，大家可以参考，在此基础上拓展
// 当然如果同学们有更好的方式，不强求使用下面这种方式
ConstantInt *ConstFolder::computeINT(
    Instruction::OpID op,
    ConstantInt *value1,
    ConstantInt *value2)
{

    int c_value1 = value1->get_value();
    int c_value2 = value2->get_value();
    switch (op)
    {
    case Instruction::add:
        return ConstantInt::get(c_value1 + c_value2, module_);
        break;
    case Instruction::sub:
        return ConstantInt::get(c_value1 - c_value2, module_);
        break;
    case Instruction::mul:
        return ConstantInt::get(c_value1 * c_value2, module_);
        break;
    case Instruction::sdiv:
        return ConstantInt::get((int)(c_value1 / c_value2), module_);
        break;
    default:
        return nullptr;
        break;
    }
}

//TODO:ljy
ConstantFP *ConstFolder::computeFP(
    Instruction::OpID op,
    ConstantFP *value1,
    ConstantFP *value2)
{

    float c_value1 = value1->get_value();
    float c_value2 = value2->get_value();
    switch (op)
    {
    case Instruction::fadd:
        return ConstantFP::get(c_value1 + c_value2, module_);
        break;
    case Instruction::fsub:
        return ConstantFP::get(c_value1 - c_value2, module_);
        break;
    case Instruction::fmul:
        return ConstantFP::get(c_value1 * c_value2, module_);
        break;
    case Instruction::fdiv:
        return ConstantFP::get((int)(c_value1 / c_value2), module_);
        break;
    default:
        return nullptr;
        break;
    }
}

ConstantInt *ConstFolder::computeCMP(
    CmpInst::CmpOp op,
    ConstantInt *value1,
    ConstantInt *value2)
{

    int c_value1 = value1->get_value();
    int c_value2 = value2->get_value();
    switch (op)
    {
    case CmpInst::EQ:
        return ConstantInt::get((bool)(c_value1 == c_value2), module_);
        break;
    case CmpInst::NE:
        return ConstantInt::get((bool)(c_value1 != c_value2), module_);
        break;
    case CmpInst::GT:
        return ConstantInt::get((bool)(c_value1 > c_value2), module_);
        break;
    case CmpInst::GE:
        return ConstantInt::get((bool)(c_value1 >= c_value2), module_);
        break;
    case CmpInst::LT:
        return ConstantInt::get((bool)(c_value1 < c_value2), module_);
        break;
    case CmpInst::LE:
        return ConstantInt::get((bool)(c_value1 <= c_value2), module_);
        break;
    default:
        return nullptr;
        break;
    }
}

ConstantInt *ConstFolder::computeFCMP(
    FCmpInst::CmpOp op,
    ConstantFP *value1,
    ConstantFP *value2)
{

    float c_value1 = value1->get_value();
    float c_value2 = value2->get_value();
    switch (op)
    {
    case FCmpInst::EQ:
        return ConstantInt::get((bool)(c_value1 == c_value2), module_);
        break;
    case FCmpInst::NE:
        return ConstantInt::get((bool)(c_value1 != c_value2), module_);
        break;
    case FCmpInst::GT:
        return ConstantInt::get((bool)(c_value1 > c_value2), module_);
        break;
    case FCmpInst::GE:
        return ConstantInt::get((bool)(c_value1 >= c_value2), module_);
        break;
    case FCmpInst::LT:
        return ConstantInt::get((bool)(c_value1 < c_value2), module_);
        break;
    case FCmpInst::LE:
        return ConstantInt::get((bool)(c_value1 <= c_value2), module_);
        break;
    default:
        return nullptr;
        break;
    }
}



// 用来判断value是否为ConstantFP，如果不是则会返回nullptr
ConstantFP *cast_constantfp(Value *value)
{
    auto constant_fp_ptr = dynamic_cast<ConstantFP *>(value);
    if (constant_fp_ptr)
    {
        return constant_fp_ptr;
    }
    else
    {
        return nullptr;
    }
}
ConstantInt *cast_constantint(Value *value)
{
    auto constant_int_ptr = dynamic_cast<ConstantInt *>(value);
    if (constant_int_ptr)
    {
        return constant_int_ptr;
    }
    else
    {
        return nullptr;
    }
}

// 用来将value置为ConstantInt
//TODO:ljy
ConstantInt *ConstFolder::set_constantint(Value * value)
{
    if(cast_constantint(value))
    {
        return cast_constantint(value);
    }
    else
    {
        float val = dynamic_cast<ConstantFP *>(value)->get_value();
        return ConstantInt::get((int)val, module_);
    }
}

// 用来将value置为ConstantFP
//TODO:ljy
ConstantFP *ConstFolder::set_constantfp(Value * value)
{
    if(cast_constantfp(value))
    {
        return cast_constantfp(value);
    }
    else
    {
        float val = dynamic_cast<ConstantInt *>(value)->get_value();
        return ConstantFP::get((float)val, module_);
    }
}

void ConstPropagation::run()
{
    // 从这里开始吧！
    auto folder=ConstFolder(m_);
    std::unordered_set<Instruction *> delete_instr_set;
    std::unordered_map<Value *,ConstantInt *> GlobalINT;
    std::unordered_map<Value *,ConstantFP *> GlobalFP;

    //TODO:ljy - Const Propagation & Delete the dead instructions
    for(auto func : m_->get_functions())
    {
        for(auto bb : func->get_basic_blocks())
        {
            for(auto instr : bb->get_instructions())
            {
                if(instr->isBinary())
                {
                    LOG(DEBUG) << "Binary";
                    auto val1_int = cast_constantint(instr->get_operand(0));
                    auto val2_int = cast_constantint(instr->get_operand(1));
                    auto val1_fp = cast_constantfp(instr->get_operand(0));
                    auto val2_fp = cast_constantfp(instr->get_operand(1));
                    if(val1_int && val2_int)
                    {
                        auto val_res = folder.computeINT(instr->get_instr_type(),val1_int,val2_int);
                        delete_instr_set.insert(instr);
                        instr->replace_all_use_with(val_res);
                    }
                    else if((val1_int && val2_fp) || (val1_fp && val2_int) || (val1_fp && val2_fp))
                    {
                        auto val_res = folder.computeFP(instr->get_instr_type(),folder.set_constantfp(instr->get_operand(0)),folder.set_constantfp(instr->get_operand(1)));
                        delete_instr_set.insert(instr);
                        instr->replace_all_use_with(val_res);
                    }
                }
                else if(instr->is_cmp()||instr->is_fcmp())
                {
                    LOG(DEBUG) << "CMP";
                    auto val1_int = cast_constantint(instr->get_operand(0));
                    auto val2_int = cast_constantint(instr->get_operand(1));
                    auto val1_fp = cast_constantfp(instr->get_operand(0));
                    auto val2_fp = cast_constantfp(instr->get_operand(1));
                    if(instr->is_cmp())
                    {
                        if(val1_int && val2_int)
                        {
                            auto val_res = folder.computeCMP(dynamic_cast<CmpInst *>(instr)->get_cmp_op(),val1_int,val2_int);
                            delete_instr_set.insert(instr);
                            instr->replace_all_use_with(val_res);
                        }
                    }
                    else if(instr->is_fcmp())
                    {
                        if(val1_fp && val2_fp)
                        {
                            auto val_res = folder.computeFCMP(dynamic_cast<FCmpInst *>(instr)->get_cmp_op(),val1_fp,val2_fp);
                            delete_instr_set.insert(instr);
                            instr->replace_all_use_with(val_res);
                        }
                    }
                }
                else if(instr->is_fp2si()||instr->is_si2fp())
                {
                    LOG(DEBUG) << "TO";
                    auto val_int = cast_constantint(instr->get_operand(0));
                    auto val_fp = cast_constantfp(instr->get_operand(0));
                    if(instr->is_fp2si())
                    {
                        if(val_fp)
                        {
                            auto val_res = folder.set_constantint(instr->get_operand(0));
                            delete_instr_set.insert(instr);
                            instr->replace_all_use_with(val_res);
                        }
                    }
                    else if(instr->is_si2fp())
                    {
                        if(val_int)
                        {
                            auto val_res = folder.set_constantfp(instr->get_operand(0));
                            delete_instr_set.insert(instr);
                            instr->replace_all_use_with(val_res);
                        }
                    }
                }
                else if(instr->is_zext())
                {
                    LOG(DEBUG) << "ZEXT";
                    auto val_int = cast_constantint(instr->get_operand(0));
                    if(val_int)
                    {
                        auto val_res = folder.set_constantint(instr->get_operand(0));
                        delete_instr_set.insert(instr);
                        instr->replace_all_use_with(val_res);
                    }
                }
                /*else if(instr->is_call())
                {
                    for(int i=0;i<instr->get_num_operand();i++)
                    {
                        auto arg=instr->get_operand(i);
                        auto arg_int=cast_constantint(arg);
                        auto arg_fp=cast_constantfp(arg);
                        if(arg_int)
                        {

                        }
                        else if(arg_fp)
                        {

                        }
                    }
                }*/
                /*else if(instr->is_gep())
                {

                }*/
                else if(instr->is_store()||instr->is_load())
                {
                    if(instr->is_load())
                    {
                        LOG(DEBUG) << "LOAD";
                        auto LoadPtr = instr->get_operand(0);
                        if (!dynamic_cast<GlobalVariable *>(LoadPtr))
                        {
                            if(GlobalINT.find(LoadPtr)!=GlobalINT.end())
                            {
                                auto LoadVal_int = cast_constantint(GlobalINT[LoadPtr]);
                                if(LoadVal_int)
                                {
                                    delete_instr_set.insert(instr);
                                    instr->replace_all_use_with(LoadVal_int);
                                }
                            }
                            else if(GlobalFP.find(LoadPtr)!=GlobalFP.end())
                            {
                                auto LoadVal_fp = cast_constantfp(GlobalFP[LoadPtr]);
                                if(LoadVal_fp)
                                {
                                    delete_instr_set.insert(instr);
                                    instr->replace_all_use_with(LoadVal_fp);
                                }
                            }
                        }
                        
                        
                    }
                    else if(instr->is_store())
                    {
                        LOG(DEBUG) << "STORE";
                        auto StoreVal_int = cast_constantint(instr->get_operand(0));
                        auto StoreVal_fp = cast_constantfp(instr->get_operand(0));
                        auto StorePtr = instr->get_operand(1);
                        if(StoreVal_int || StoreVal_fp)
                        {
                            if(StoreVal_int)
                            {
                                if(GlobalINT.find(StorePtr) != GlobalINT.end())
                                {
                                    GlobalINT[StorePtr] = StoreVal_int;
                                }
                                else
                                {
                                    GlobalINT.insert({StorePtr,StoreVal_int});
                                }
                            }
                            else if(StoreVal_fp)
                            {
                                if(GlobalFP.find(StorePtr) != GlobalFP.end())
                                {
                                    GlobalFP[StorePtr] = StoreVal_fp;
                                }
                                else
                                {
                                    GlobalFP.insert({StorePtr,StoreVal_fp});
                                }
                            }
                        }
                        else if(GlobalINT.find(StorePtr)!=GlobalINT.end())
                        {
                            GlobalINT.erase(StorePtr);
                        }
                        else if(GlobalFP.find(StorePtr)!=GlobalFP.end())
                        {
                            GlobalFP.erase(StorePtr);
                        }
                    }
                }
            }//end for instr

            LOG(DEBUG) << "Before delete";
            //Delete the dead instructions 
            for(auto instr : delete_instr_set)
            {
                LOG(DEBUG) << instr->get_instr_op_name();
                bb->delete_instr(instr);
            }
            LOG(DEBUG) << "After delete";

        }//end for bb
    }//end for func
    
    //TODO:dby - Delete the dead BasicBlocks
    for (auto func : m_->get_functions())
    {
        for (auto bb : func->get_basic_blocks())
        {
            auto last_ins = bb->get_terminator();
            auto br_ins = dynamic_cast<BranchInst *>(last_ins);
            if (br_ins)
            {
                if (br_ins->is_cond_br())
                {
                    auto result = cast_constantint(br_ins->get_operand(0));
                    if (result)
                    {
                        auto true_br = dynamic_cast<BasicBlock *>(br_ins->get_operand(1));
                        auto false_br = dynamic_cast<BasicBlock *>(br_ins->get_operand(2));
                        if (result->get_value() == 0)
                        {
                            true_br->remove_pre_basic_block(bb);
                            bb->remove_succ_basic_block(true_br);
                            bb->delete_instr(br_ins);
                            IRBuilder *builder;
                            builder->set_insert_point(bb);
                            builder->create_br(false_br);
                        }
                        else
                        {
                            false_br->remove_pre_basic_block(bb);
                            bb->remove_succ_basic_block(false_br);
                            bb->delete_instr(br_ins);
                            IRBuilder *builder;
                            builder->set_insert_point(bb);
                            builder->create_br(true_br);
                        }
                        
                    }
                    
                }
                
            }
            
        }
        auto bb_list = func->get_basic_blocks();
        auto bb_list_it = bb_list.begin();
        for (bb_list_it++; bb_list_it != bb_list.end();bb_list_it++)
        {
            if ((*bb_list_it)->get_pre_basic_blocks().size() == 0)
            {
                
                for (auto succ:(*bb_list_it)->get_succ_basic_blocks())
                {
                    for (auto ins : succ->get_instructions())
                    {
                        if (ins->is_phi())
                        {
                            for (int i = 0; i < ins->get_num_operand() / 2; i++)
                            {
                                if (ins->get_operand(2 * i + 1) == *bb_list_it)
                                {

                                    ins->remove_operands(2 * i, 2 * i + 1);
                                }
                            }
                        }
                        
                    }
                    
                }
                func->remove(*bb_list_it);
                
            }
            
        }
    
    }
    


}
