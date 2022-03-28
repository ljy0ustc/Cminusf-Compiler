#include <algorithm>
#include <queue>
#include "logging.hpp"
#include "LoopSearch.hpp"
#include "LoopInvHoist.hpp"

void LoopInvHoist::run()
{
    // 先通过LoopSearch获取循环的相关信息
    LoopSearch loop_searcher(m_, false);
    loop_searcher.run();

    // 接下来由你来补充啦！
    auto function_list = m_->get_functions();

    LOG(DEBUG) << "进入function循环之前";

    for(auto function : function_list)
    {
        std::unordered_set<BBset_t *> loop_set=loop_searcher.get_loops_in_func(function);
        
        LOG(DEBUG) << "进入loop循环之前";
        for(auto loop : loop_set)
        {    

            LOG(DEBUG) << "计算左值之前";
            //记录所有左值
            std::unordered_set<Value *> defVal_set;
            for(auto bb : *loop)
            {
                for(auto instr : bb->get_instructions())
                {
                    //Binary Instr
                    bool isBinary=instr->isBinary();
                    //cmp,fcmp
                    bool isCmp=instr->is_cmp()||instr->is_fcmp();
                    //phi
                    bool isPhi=instr->is_phi();
                    //call
                    bool isCall=instr->is_call();
                    //zext
                    bool isZext=instr->is_zext();
                    //fptosi,sitofp
                    bool isTo=instr->is_fp2si()||instr->is_si2fp();
                    if(isBinary||isCmp||isPhi||isCall||isZext||isTo)
                        defVal_set.insert(instr);
                }//end for instr
            }//end for bb

            LOG(DEBUG) << "对每个循环，左值记录完成，开始求移动指令集合";
            
            //求出该循环要移动的指令集合
            std::queue<Instruction *> move_instr_set;
            std::unordered_map<Instruction *,BasicBlock *> move_instr2bb;
            bool change_flag = false;//一直迭代到查找出所有指令
            do{
                change_flag = false;
                for(auto bb : *loop)
                {
                    for(auto instr : bb->get_instructions())
                    {
                        if(move_instr2bb.find(instr)!=move_instr2bb.end())
                            continue;
                        LOG(DEBUG) << instr->get_instr_op_name();
                        bool move_flag=true;//该instruction是否要移动
                        //LOG(DEBUG) << "开始检查该指令是否要移动";
                        if(instr->is_store()||instr->is_load()||instr->is_br()||instr->is_ret()||instr->is_phi())
                        {
                            move_flag=false;
                        }
                        else
                        {
                            //检查其涉及的变量
                            for(int i=0;i<instr->get_num_operand();i++)
                            {
                                auto arg=instr->get_operand(i);
                                if(defVal_set.find(arg)!=defVal_set.end())
                                {
                                    move_flag=false;
                                }
                            }
                        }//end if-else
                        //LOG(DEBUG) << "检查完成";
                        if(move_flag)
                        {
                            LOG(DEBUG) << "移动";
                            change_flag=true;
                            move_instr_set.push(instr);
                            move_instr2bb.insert({instr,bb});
                            //bb->delete_instr(instr);
                            defVal_set.erase(instr);
                            //LOG(DEBUG) << "退出move_flag";
                        }
                        //LOG(DEBUG) << "move_flag操作完成";
                    }//end for instr
                }//end for bb
            //可能有相互依赖的instruction
            }while(change_flag);

            //LOG(DEBUG) << "对每个循环，移动指令集合计算完成，开始外提循环不变式";

            //循环不变式外提
            auto base = loop_searcher.get_loop_base(loop);
            for(auto pre_bb : base->get_pre_basic_blocks())
            {
                //因为base只对应两个pre，一个在循环内，一个在循环外，取外
                if(loop->find(pre_bb)==loop->end())
                {
                    auto br_instr=pre_bb->get_terminator();
                    pre_bb->delete_instr(br_instr);
                    LOG(DEBUG) << "开始外提循环不变式";
                    while(move_instr_set.size())
                    {
                        auto move_instr=move_instr_set.front();
                        LOG(DEBUG) << move_instr->get_instr_op_name();
                        //加入前驱bb
                        pre_bb->add_instruction(move_instr);
                        //从原bb中删除
                        move_instr2bb[move_instr]->delete_instr(move_instr);
                        move_instr_set.pop();
                    }
                    pre_bb->add_instruction(br_instr);
                    LOG(DEBUG) << "外提完成";
                }
            }//end for pre_bb
        }// end for loop
    }//end for function
}