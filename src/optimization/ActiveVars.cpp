#include "ActiveVars.hpp"

void ActiveVars::run()
{
    std::ofstream output_active_vars;
    output_active_vars.open("active_vars.json", std::ios::out);
    output_active_vars << "[";
    for (auto &func : this->m_->get_functions()) {
        if (func->get_basic_blocks().empty()) {
            continue;
        }
        else
        {
            func_ = func;  

            func_->set_instr_name();
            live_in.clear();
            live_out.clear();
            
            // 在此分析 func_ 的每个bb块的活跃变量，并存储在 live_in live_out 结构内
            std::map<BasicBlock *, std::set<Value *>> use, def;
            std::map<BasicBlock *, std::map<Value *, BasicBlock *>> phi_val_bb; //存储每个块的phi指令中变量对应的基本快
            //计算use和def
            for (auto bb : func_->get_basic_blocks())
            {
                for (auto ins : bb->get_instructions())
                {
                    if (ins->is_phi())
                    {
                        for (int i = 0; i < ins->get_num_operand() / 2; i++)
                        {
                            auto op = ins->get_operand(2 * i);
                            if(!dynamic_cast<ConstantInt *>(op) && !dynamic_cast<ConstantFP *>(op))
                            {
                                if (def[bb].find(op) == def[bb].end() && use[bb].find(op) == use[bb].end())
                                {
                                    phi_val_bb[bb][op] = dynamic_cast<BasicBlock *>(ins->get_operand(2 * i + 1));
                                }
                            }
                        }
                        if (def[bb].find(ins) == def[bb].end() && use[bb].find(ins) == use[bb].end())
                        {
                            def[bb].insert(ins);
                        }
                    }
                    else if (ins->is_store())
                    {
                        auto op = ins->get_operand(0);
                        if (!dynamic_cast<ConstantInt *>(op) && !dynamic_cast<ConstantFP *>(op))
                        {
                            if (def[bb].find(op) == def[bb].end() && use[bb].find(op) == use[bb].end())
                            {
                                use[bb].insert(op);
                            }
                            
                        }
                        op = ins->get_operand(1);
                        if (def[bb].find(op) == def[bb].end() && use[bb].find(op) == use[bb].end())
                        {
                            def[bb].insert(op);
                        }
                        
                    }
                    else if (ins->is_alloca())
                    {
                        def[bb].insert(ins);
                    }
                    else if (ins->is_ret())
                    {
                        for (auto op : ins->get_operands())
                        {
                            if (!dynamic_cast<ConstantInt *>(op) && !dynamic_cast<ConstantFP *>(op))
                            {
                                if (def[bb].find(op) == def[bb].end() && use[bb].find(op) == use[bb].end())
                                {
                                    use[bb].insert(op);
                                }
                                
                            }
                        }
                        
                    }
                    else if (ins->is_load())
                    {
                        auto op = ins->get_operand(0);
                        if (def[bb].find(op) == def[bb].end() && use[bb].find(op) == use[bb].end())
                        {
                            use[bb].insert(op);
                        }
                        if (def[bb].find(ins) == def[bb].end() && use[bb].find(ins) == use[bb].end())
                        {
                            def[bb].insert(ins);
                        }

                    }
                    else if (ins->is_br())
                    {
                        if (dynamic_cast<BranchInst *>(ins)->is_cond_br())
                        {
                            auto op = ins->get_operand(0);
                            if (!dynamic_cast<ConstantInt *>(op) && !dynamic_cast<ConstantFP *>(op))
                            {
                                if (def[bb].find(op) == def[bb].end() && use[bb].find(op) == use[bb].end())
                                {
                                    use[bb].insert(op);
                                }
                                
                            }
                        }
                        
                    }
                    else if (ins->isBinary() || ins->is_cmp() || ins->is_fcmp())
                    {
                        for (auto op : ins->get_operands())
                        {
                            if (!dynamic_cast<ConstantInt *>(op) && !dynamic_cast<ConstantFP *>(op))
                            {
                                if (def[bb].find(op) == def[bb].end() && use[bb].find(op) == use[bb].end())
                                {
                                    use[bb].insert(op);
                                }
                                
                            }
                        }
                        if (def[bb].find(ins) == def[bb].end() && use[bb].find(ins) == use[bb].end())
                        {
                            def[bb].insert(ins);
                        }
                    }
                    else if (ins->is_fp2si() || ins->is_si2fp() || ins->is_zext())
                    {
                        auto op = ins->get_operand(0);
                        if (def[bb].find(op) == def[bb].end() && use[bb].find(op) == use[bb].end())
                        {
                            use[bb].insert(op);
                        }
                        if (def[bb].find(ins) == def[bb].end() && use[bb].find(ins) == use[bb].end())
                        {
                            def[bb].insert(ins);
                        }
                    }
                    else if (ins->is_gep())
                    {
                        for (auto op : ins->get_operands())
                        {
                            if (!dynamic_cast<ConstantInt *>(op) && !dynamic_cast<ConstantFP *>(op))
                            {
                                if (def[bb].find(op) == def[bb].end() && use[bb].find(op) == use[bb].end())
                                {
                                    use[bb].insert(op);
                                }
                                
                            }
                        }
                        if (def[bb].find(ins) == def[bb].end() && use[bb].find(ins) == use[bb].end() && !ins->is_void())
                        {
                            def[bb].insert(ins);
                        }
                    }
                    else if (ins->is_call())
                    {
                        for (int i = 1; i < ins->get_num_operand(); i++)
                        {
                            auto op = ins->get_operand(i);
                            if (!dynamic_cast<ConstantInt *>(op) && !dynamic_cast<ConstantFP *>(op))
                            {
                                if (def[bb].find(op) == def[bb].end() && use[bb].find(op) == use[bb].end())
                                {
                                    use[bb].insert(op);
                                }
                                
                            }
                        }
                        if (def[bb].find(ins) == def[bb].end() && use[bb].find(ins) == use[bb].end() && !ins->is_void())
                        {
                            def[bb].insert(ins);
                        }
                    }
                    
                }
                
            }

            //计算in和out
            bool whether_change = true;
            while (whether_change)
            {
                whether_change = false;
                for (auto bb : func_->get_basic_blocks())
                {
                    int num = live_in[bb].size();
                    for (auto succ : bb->get_succ_basic_blocks())
                    {
                        
                        for (auto op : live_in[succ])
                        {
                            live_out[bb].insert(op);
                            // live_in[bb].insert(op);
                        }
                        for (auto val_bb : phi_val_bb[succ])
                        {
                            if (val_bb.second == bb)
                            {
                                live_out[bb].insert(val_bb.first);
                                // live_in[bb].insert(val_bb.first);
                            }
                        }
                        
                        
                    }

                    for (auto op : live_out[bb])
                    {
                        live_in[bb].insert(op);
                    }
                    for (auto op : def[bb])
                    {
                        live_in[bb].erase(op);
                    }
                    for (auto op : use[bb])
                    {
                        live_in[bb].insert(op);
                    }
                    if (num != live_in[bb].size())
                    {
                        whether_change = true;
                    }
                    
                }
                
            }
            for (auto bb : func_->get_basic_blocks())
            {
                for (auto val_bb : phi_val_bb[bb])
                {
                    live_in[bb].insert(val_bb.first);
                }
            }
            
            

            output_active_vars << print();
            output_active_vars << ",";
        }
    }
    output_active_vars << "]";
    output_active_vars.close();
    return ;
}

std::string ActiveVars::print()
{
    std::string active_vars;
    active_vars +=  "{\n";
    active_vars +=  "\"function\": \"";
    active_vars +=  func_->get_name();
    active_vars +=  "\",\n";

    active_vars +=  "\"live_in\": {\n";
    for (auto &p : live_in) {
        if (p.second.size() == 0) {
            continue;
        } else {
            active_vars +=  "  \"";
            active_vars +=  p.first->get_name();
            active_vars +=  "\": [" ;
            for (auto &v : p.second) {
                active_vars +=  "\"%";
                active_vars +=  v->get_name();
                active_vars +=  "\",";
            }
            active_vars += "]" ;
            active_vars += ",\n";   
        }
    }
    active_vars += "\n";
    active_vars +=  "    },\n";
    
    active_vars +=  "\"live_out\": {\n";
    for (auto &p : live_out) {
        if (p.second.size() == 0) {
            continue;
        } else {
            active_vars +=  "  \"";
            active_vars +=  p.first->get_name();
            active_vars +=  "\": [" ;
            for (auto &v : p.second) {
                active_vars +=  "\"%";
                active_vars +=  v->get_name();
                active_vars +=  "\",";
            }
            active_vars += "]";
            active_vars += ",\n";
        }
    }
    active_vars += "\n";
    active_vars += "    }\n";

    active_vars += "}\n";
    active_vars += "\n";
    return active_vars;
}