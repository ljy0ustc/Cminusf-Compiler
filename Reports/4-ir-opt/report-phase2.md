# Lab4 实验报告

小组成员 姓名 学号
廖佳怡PB19151776
邓博以PB19061260

## 实验要求

请按照自己的理解，写明本次实验需要干什么

实现3个pass：常量传播与死代码删除，循环不变式外提，活跃变量分析。
1. 常量传播与死代码删除
如果一个变量的值可以在编译优化阶段直接计算出，那么就直接将该变量替换为常量（即计算出的结果值）。补充以下几点需要注意的地方：
a. 只需要考虑过程内的常量传播，可以不用考虑数组，全局变量只需要考虑块内的常量传播。
b. 整形浮点型都需要考虑。
c. 对于a = 1 / 0的情形，可以不考虑，即可以做处理也可以不处理。
2. 循环不变式外提
要能够实现将与循环无关的表达式提取到循环的外面。不用考虑数组与全局变量。
3. 活跃变量分析
能够实现分析 bb 块的入口和出口的活跃变量，特别是对phi指令的处理。

## 实验难点

实验中遇到哪些挑战

* 如何处理常量的store和load？如何删除死代码块？
* 如何判断某指令是循环不变式？
* 在活跃变量分析中如何处理phi指令。

## 实验设计

* 常量传播
    实现思路和相应代码：
    1. 常量传播和死指令删除
    (1)Binary,cmp,fp<->si,zext等类型指令的常量传播和删除：
    枚举每个函数的每个基本块中的每条指令，分类讨论。
    用`ConstFolder`类进行常量计算，将要删去的指令保存在`delete_instr_set`中，`replace_all_use_with`在所有用到该常量的地方进行替换。
    ```c++
    if(instr->isBinary())
    {
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
            auto val_res = folder.computeFP(instr->get_instr_type(),folder.set_const(instr->get_operand(0)),folder.set_constantfp(instr->get_operand(1)));
            delete_instr_set.insert(instr);
            instr->replace_all_use_with(val_res);
        }
    }
    else if(instr->is_cmp()||instr->is_fcmp())
    {
        auto val1_int = cast_constantint(instr->get_operand(0));
        auto val2_int = cast_constantint(instr->get_operand(1));
        auto val1_fp = cast_constantfp(instr->get_operand(0));
        auto val2_fp = cast_constantfp(instr->get_operand(1));
        if(instr->is_cmp())
        {
            if(val1_int && val2_int)
            {
                auto val_res = folder.computeCMP(dynamic_cast<CmpInst *>(instr)->get_cmp_val1_int,val2_int);
                delete_instr_set.insert(instr);
                instr->replace_all_use_with(val_res);
            }
        }
        else if(instr->is_fcmp())
        {
            if(val1_fp && val2_fp)
            {
                auto val_res = folder.computeFCMP(dynamic_cast<FCmpInst *>(instr)->get_cmp,val1_fp,val2_fp);
                delete_instr_set.insert(instr);
                instr->replace_all_use_with(val_res);
            }
        }
    }
    else if(instr->is_fp2si()||instr->is_si2fp())
    {
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
        auto val_int = cast_constantint(instr->get_operand(0));
        if(val_int)
        {
            auto val_res = folder.set_constantint(instr->get_operand(0));
            delete_instr_set.insert(instr);
            instr->replace_all_use_with(val_res);
        }
    }
    ```
    (2)load和store指令的处理。
    用`GlobalINT`和`GlobalFP`来管理全局变量。
    ```c++
    else if(instr->is_store()||instr->is_load())
    {
        if(instr->is_load())
        {
            auto LoadPinstr->get_operand(0);
        (!dynamic_cast<GlobalVariab(LoadPtr))
            {
                if(GlobalINT.find(Loa!=GlobalINT.end())
                {
                    auto LoadVal_icast_consta(GlobalINT[LoadPtr]);
                    if(LoadVal_int)
                    {
                        delete_instrinsert(instr);
                        instr->replace_se_with(LoadVal;
                    }
                }
                else if(GlobalFP(LoadPtr)!=GlobalFP.end())
                {
                    auto LoadVal_cast_constantfp(Glo[LoadPtr]);
                    if(LoadVal_fp)
                    {
                        delete_instrinsert(instr);
                        instr->replace_se_with(LoadVal_fp);
                    }
                }
            }
            
            
        }
        else if(instr->is_store())
        {
            auto StoreVal_icast_consta(instr->get_operand(0));
            auto StoreVal_cast_const(instr->get_operand(0));
            auto StorePinstr->get_operand(1);
            if(StoreVal_int || StoreVal_fp)
            {
                if(StoreVal_int)
                {
                    if(GlobalINT(StorePtr) != Globaend())
                    {
                        GlobalINT[Store= StoreVal_int;
                    }
                    else
                    {
                        GlobalINT.in{StorStoreVal_int});
                    }
                }
                else if(StoreVal_fp)
                {
                    if(GlobalFP(StorePtr) != Globend())
                    {
                        GlobalFP[Store= StoreVal_fp;
                    }
                    else
                    {
                        GlobalFP.in{StorStoreVal_fp});
                    }
                }
            }
            else if(GlobalINT.find(Stor!=GlobalINT.end())
            {
                GlobalINT.erase(StorePtr);
            }
            else if(GlobalFP.find(Stor!=GlobalFP.end())
            {
                GlobalFP.erase(StorePtr);
            }
        }
    }
    ```
    (3)死指令删除
    ```c++
    for(auto instr : delete_instr_set)
    {
        bb->delete_instr(instr);
    }
    ```
    2. 死基本块删除
    (1)冗余跳转删除
    ```c++
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
                    auto true_br = dynamic_cast<BasicBlock (br_ins->get_operand(1));
                    auto false_br = dynamic_cast<BasicBlock (br_ins->get_operand(2));
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
    ```
    (2) 无前驱块删除
    将无前驱的块删除，并对该块的所有后继块，删除其phi指令中该块的参数。
    ```c++
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
                        for (int i = 0; i < ins->get_num_operand(/ 2; i++)
                        {
                            if (ins->get_operand(2 * i + 1) =*bb_list_it)
                            
                                ins->remove_operands(2 * i, 2 * i + 1);
                            }
                        }
                    }
                    
                }
                
            }
            func->remove(*bb_list_it);
            
        }
        
    }
    ```
    优化前后的IR对比（举一个例子）并辅以简单说明：
    (1).cminus文件：
    ```
    void main(void){
        int i;
        int idx;
    
        i = 0;
        idx = 0;
    
        while(i < 100000000)
        {
            idx = 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1     + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1     + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1     + 1 + 1 + 1 + 1 + 1 ;
            i=i+idx*idx*idx*idx*idx*idx*idx*idx/    (idx*idx*idx*idx*idx*idx*idx*idx);
        }
    	output(idx*idx);
        return ;
    }
    ```
    (2)优化前.ll文件：
    ```
    ; ModuleID = 'cminus'
    source_filename = "testcase-1.cminus"
    
    declare i32 @input()
    
    declare void @output(i32)
    
    declare void @outputFloat(float)
    
    declare void @neg_idx_except()
    
    define void @main() {
    label_entry:
      br label %label2
    label2:                                                   ; preds = %label_entry, %label7
      %op78 = phi i32 [ 0, %label_entry ], [ %op73,     %label7 ]
      %op79 = phi i32 [ 0, %label_entry ], [ %op40,     %label7 ]
      %op4 = icmp slt i32 %op78, 100000000
      %op5 = zext i1 %op4 to i32
      %op6 = icmp ne i32 %op5, 0
      br i1 %op6, label %label7, label %label74
    label7:                                                   ; preds = %label2
      %op8 = add i32 1, 1
      %op9 = add i32 %op8, 1
      %op10 = add i32 %op9, 1
      %op11 = add i32 %op10, 1
      %op12 = add i32 %op11, 1
      %op13 = add i32 %op12, 1
      %op14 = add i32 %op13, 1
      %op15 = add i32 %op14, 1
      %op16 = add i32 %op15, 1
      %op17 = add i32 %op16, 1
      %op18 = add i32 %op17, 1
      %op19 = add i32 %op18, 1
      %op20 = add i32 %op19, 1
      %op21 = add i32 %op20, 1
      %op22 = add i32 %op21, 1
      %op23 = add i32 %op22, 1
      %op24 = add i32 %op23, 1
      %op25 = add i32 %op24, 1
      %op26 = add i32 %op25, 1
      %op27 = add i32 %op26, 1
      %op28 = add i32 %op27, 1
      %op29 = add i32 %op28, 1
      %op30 = add i32 %op29, 1
      %op31 = add i32 %op30, 1
      %op32 = add i32 %op31, 1
      %op33 = add i32 %op32, 1
      %op34 = add i32 %op33, 1
      %op35 = add i32 %op34, 1
      %op36 = add i32 %op35, 1
      %op37 = add i32 %op36, 1
      %op38 = add i32 %op37, 1
      %op39 = add i32 %op38, 1
      %op40 = add i32 %op39, 1
      %op44 = mul i32 %op40, %op40
      %op46 = mul i32 %op44, %op40
      %op48 = mul i32 %op46, %op40
      %op50 = mul i32 %op48, %op40
      %op52 = mul i32 %op50, %op40
      %op54 = mul i32 %op52, %op40
      %op56 = mul i32 %op54, %op40
      %op59 = mul i32 %op40, %op40
      %op61 = mul i32 %op59, %op40
      %op63 = mul i32 %op61, %op40
      %op65 = mul i32 %op63, %op40
      %op67 = mul i32 %op65, %op40
      %op69 = mul i32 %op67, %op40
      %op71 = mul i32 %op69, %op40
      %op72 = sdiv i32 %op56, %op71
      %op73 = add i32 %op78, %op72
      br label %label2
    label74:                                                   ; preds = %label2
      %op77 = mul i32 %op79, %op79
      call void @output(i32 %op77)
      ret void
    }
    ```
    (3)优化后.ll文件：
    ```
    ; ModuleID = 'cminus'
    source_filename = "testcase-1.cminus"
    
    declare i32 @input()
    
    declare void @output(i32)
    
    declare void @outputFloat(float)
    
    declare void @neg_idx_except()
    
    define void @main() {
    label_entry:
      br label %label2
    label2:                                                    ; preds = %label_entry, %label7
      %op78 = phi i32 [ 0, %label_entry ], [ %op73,      %label7 ]
      %op79 = phi i32 [ 0, %label_entry ], [ 34,     %label7 ]
      %op4 = icmp slt i32 %op78, 100000000
      %op5 = zext i1 %op4 to i32
      %op6 = icmp ne i32 %op5, 0
      br i1 %op6, label %label7, label %label74
    label7:                                                    ; preds = %label2
      %op73 = add i32 %op78, 1
      br label %label2
    label74:                                                    ; preds = %label2
      %op77 = mul i32 %op79, %op79
      call void @output(i32 %op77)
      ret void
    }
    ```
    (4)运行时间：
    before optimization:1.40s
    after optimization:0.28s


* 循环不变式外提
    实现思路和相应代码：
    遍历每个循环。
    (1) 将循环中的所有左值记录在`defVal_set`中。
    ```c++
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
    ```

    (2) 求出该循环要移动的指令集合`move_instr_set`。
    其中判断是否要移动该指令的依据为：
    该指令的所有右值都不在`defVal_set`中，即不为其它指令的左值。符合该条件的时间，以`move_flag`为真来标记。
    另外，考虑到循环不变式可能相互之间有变量调用关系，故以do-while循环来迭代搜索，并以`change_flag`来标记是否要继续迭代搜索，每搜索到一个循环不变式则将其左值从`move_instr_set`中删除。
    ```c++
    bool change_flag = false;//一直迭代到查找出所有指令
    do{
        change_flag = false;
        for(auto bb : *loop)
        {
            for(auto instr : bb->get_instructio())
            {
                if(move_instr2bb.find(inst!=move_instr2bb.end())
                    continue;
                LOG(DEBUG) <instr->get_instr_op_name();
                bool move_flag=true;/instruction是否要移动
                //LOG(DEBUG) << "开始检查该指否要移动";
                if(instr->is_store()instr->is_load()||instr->is_br(|instr->is_ret()||instr->is_phi)
                {
                    move_flag=false;
                }
                else
                {
                    //检查其涉及的变量
                    for(int i=i<instr->get_num_operand();+)
                    {
                        autarg=instr->get_operand(;
                        if(defVal_set.find(ar!=defVal_set.end())
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
                    move_instr2bb.insert({instbb});
                    //bb->delete_instr(instr);
                    defVal_set.erase(instr);
                    //LOG(DEBUG) << "move_flag";
                }
                //LOG(DEBUG) << "move_flag操成";
            }//end for instr
        }//end for bb
    //可能有相互依赖的instruction
    }while(change_flag);
    ```

    (3)将`move_instr_set`中保存的循环不变式外提。因为之前搜索循环不变式的时候，构建了辅助map`move_instr2bb`记录了指令与BB的对应情况。故我们只需将`move_isntr_set`中的指令从对应的BB中删除，然后插入到pre_bb中即可。
    其中有两个问题,一个是如何确定pre_bb。由于该循环的入口块只有两个前驱，其中不属于该循环的基本块即为要插入的基本块。
    另一个问题是如何将循环不变式插入基本块。先删除该BB的terminator，然后按序插入循环不变式和terminator即可。
    ```c++
    for(auto pre_bb : base->get_pre_basic_blocks())
    {
        //因为base只对应两个pre，一个在循环内，在循环外，取外
        if(loop->find(pre_bb)==loop->end())
        {
            auto br_instr=pre_bb->get_terminat();
            pre_bb->delete_instr(br_instr);
            LOG(DEBUG) << "开始外提循环不变式";
            while(move_instr_set.size())
            {
                auto move_instr=move_instr_sefront();
                LOG(DEBUG) <move_instr->get_instr_op_name();
                //加入前驱bb
                pre_bb->add_instructi(move_instr);
                //从原bb中删除
                move_instr2bb[move_inst->delete_instr(move_instr);
                move_instr_set.pop();
            }
            pre_bb->add_instruction(br_instr);
            LOG(DEBUG) << "外提完成";
        }
    }//end for pre_bb
    ```
    优化前后的IR对比（举一个例子）并辅以简单说明：
    (1)cminusf代码：
    
    ```
    void main(void){
        int i;
        int j;
        int ret;
    
        i = 1;
    
        while(i<10000)
        {
            j = 0;
            while(j<10000)
            {
                ret = (i*i*i*i*i*i*i*i*i*i)/i/i/i/i/i/i/i/i/    i/i;
                j=j+1;
            }
            i=i+1;
	    }
        output(ret);
        return ;
    }
    ```
    (2)优化前.ll：
    ```
        ; ModuleID = 'cminus'
        source_filename = "testcase-1.cminus"
    
        declare i32 @input()
    
        declare void @output(i32)
    
        declare void @outputFloat(float)
    
        declare void @neg_idx_except()
    
        define void @main() {
        label_entry:
          br label %label3
        label3:                                                ;        preds = %label_entry, %label58
          %op61 = phi i32 [ %op64, %label58 ], [ undef,         %label_entry ]
          %op62 = phi i32 [ 1, %label_entry ], [ %op60, %label58        ]
          %op63 = phi i32 [ %op65, %label58 ], [ undef,         %label_entry ]
          %op5 = icmp slt i32 %op62, 10000
          %op6 = zext i1 %op5 to i32
          %op7 = icmp ne i32 %op6, 0
          br i1 %op7, label %label8, label %label9
        label8:                                                ;        preds = %label3
          br label %label11
        label9:                                                ;        preds = %label3
          call void @output(i32 %op61)
          ret void
        label11:                                                        ; preds = %label8, %label16
          %op64 = phi i32 [ %op61, %label8 ], [ %op55, %label16 ]
          %op65 = phi i32 [ 0, %label8 ], [ %op57, %label16 ]
          %op13 = icmp slt i32 %op65, 10000
          %op14 = zext i1 %op13 to i32
          %op15 = icmp ne i32 %op14, 0
          br i1 %op15, label %label16, label %label58
        label16:                                                        ; preds = %label11
          %op19 = mul i32 %op62, %op62
          %op21 = mul i32 %op19, %op62
          %op23 = mul i32 %op21, %op62
          %op25 = mul i32 %op23, %op62
          %op27 = mul i32 %op25, %op62
          %op29 = mul i32 %op27, %op62
          %op31 = mul i32 %op29, %op62
          %op33 = mul i32 %op31, %op62
          %op35 = mul i32 %op33, %op62
          %op37 = sdiv i32 %op35, %op62
          %op39 = sdiv i32 %op37, %op62
          %op41 = sdiv i32 %op39, %op62
          %op43 = sdiv i32 %op41, %op62
          %op45 = sdiv i32 %op43, %op62
          %op47 = sdiv i32 %op45, %op62
          %op49 = sdiv i32 %op47, %op62
          %op51 = sdiv i32 %op49, %op62
          %op53 = sdiv i32 %op51, %op62
          %op55 = sdiv i32 %op53, %op62
          %op57 = add i32 %op65, 1
          br label %label11
        label58:                                                        ; preds = %label11
          %op60 = add i32 %op62, 1
          br label %label3
        }
    ```
    
    优化后.ll:
    ```
        ; ModuleID = 'cminus'
        source_filename = "testcase-1.cminus"
        
        declare i32 @input()
    
        declare void @output(i32)
    
        declare void @outputFloat(float)
    
        declare void @neg_idx_except()
    
        define void @main() {
        label_entry:
          br label %label3
        label3:                                                ;        preds = %label_entry, %label58
          %op61 = phi i32 [ %op64, %label58 ], [ undef,         %label_entry ]
          %op62 = phi i32 [ 1, %label_entry ], [ %op60, %label58        ]
          %op63 = phi i32 [ %op65, %label58 ], [ undef,         %label_entry ]
          %op5 = icmp slt i32 %op62, 10000
          %op6 = zext i1 %op5 to i32
          %op7 = icmp ne i32 %op6, 0
          br i1 %op7, label %label8, label %label9
        label8:                                                ;        preds = %label3
          %op19 = mul i32 %op62, %op62
          %op21 = mul i32 %op19, %op62
          %op23 = mul i32 %op21, %op62
          %op25 = mul i32 %op23, %op62
          %op27 = mul i32 %op25, %op62
          %op29 = mul i32 %op27, %op62
          %op31 = mul i32 %op29, %op62
          %op33 = mul i32 %op31, %op62
          %op35 = mul i32 %op33, %op62
          %op37 = sdiv i32 %op35, %op62
          %op39 = sdiv i32 %op37, %op62
          %op41 = sdiv i32 %op39, %op62
          %op43 = sdiv i32 %op41, %op62
          %op45 = sdiv i32 %op43, %op62
          %op47 = sdiv i32 %op45, %op62
          %op49 = sdiv i32 %op47, %op62
          %op51 = sdiv i32 %op49, %op62
          %op53 = sdiv i32 %op51, %op62
          %op55 = sdiv i32 %op53, %op62
          br label %label11
        label9:                                                ;        preds = %label3
          call void @output(i32 %op61)
          ret void
        label11:                                                        ; preds = %label8, %label16
          %op64 = phi i32 [ %op61, %label8 ], [ %op55, %label16 ]
          %op65 = phi i32 [ 0, %label8 ], [ %op57, %label16 ]
          %op13 = icmp slt i32 %op65, 10000
          %op14 = zext i1 %op13 to i32
          %op15 = icmp ne i32 %op14, 0
          br i1 %op15, label %label16, label %label58
        label16:                                                        ; preds = %label11
          %op57 = add i32 %op65, 1
          br label %label11
        label58:                                                        ; preds = %label11
          %op60 = add i32 %op62, 1
          br label %label3
        }
    ```
     可见循环不变式`ret = (i*i*i*i*i*i*i*i*i*i)/i/i/i/i/i/i/i/i/i/i;`外提到`while(j<10000)`外，`while(i<10000)`内了。
    (3)运行时间：
        before optimization:4.91s
        after optimization:0.32
        提速：93.5%
    
* 活跃变量分析
    实现思路：
    
    对于活跃变量分析，我们以函数为界限。首先，遍历函数中所有块的每一条指令，确定每一个块的use、def和phi_val_bb（该集合与phi指令相关，具体会在后文解释）集合。其次，再次遍历所有块，通过use、def和phi_val_bb三个集合确定出每一个块的IN和OUT集合（此时的IN集合不是最终的IN集合），重复迭代上述过程，直到每一个块的IN和OUT集合都不发生改变。最后，借助phi_val_bb集合，补全每一个块的IN集合。
    
    具体实现：
    
    1. use、def和phi_val_bb
    
       ```c++
       std::map<BasicBlock *, std::set<Value *>> use, def;
       std::map<BasicBlock *, std::map<Value *, BasicBlock *>> phi_val_bb;
       ```
    
       上面一段段代码中的use和def的定义和课本上的定义类似，唯一的区别是此处的use集合中不包括phi指令的右值变量（但def集合包括phi指令的左值变量），这两个集合都用map实现，其中key是基本块，val是一个集合，用于存储具体的变量。
    
       phi_val_bb存储phi指令右值中的变量以及其对应的基本块，该数据结构也用map实现，其中key是基本块，val还是一个map，内层的map存储了该基本块中每一个phi指令右值中的变量以及其对应的基本块。
    
       下面一段代码是用于计算use、def和phi_val_bb的具体代码
    
       ```c++
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
       ```
    
       
    
    2. IN和OUT集合
    
       根据助教的提示我们可以知道，应该采用的数据流方程为$OUT[B] =\cup_{s是B的后继}IN[S]\cup_{s是B的后继} phi\_uses[S,B]$和$IN [B] = useB\cup(OUT [B]- defB )  $
    
       其中`IN[S]`是S中剔除`phi`指令后分析出的入口变量结果。`phi_uses[S,B]`表示S中的`phi`指令参数中`label`为B的对应变量。
    
       对于第一个数据流方程，由于在我们的代码实现中use集合不包括phi指令的右值变量，所以按照我们的代码计算出的IN集合是可以直接在第一个数据流方程中使用，而`phi_uses[S,B]`通过我们先前定义的phi_val_bb是很容易算出来的。
    
       对于第二个数据流方程，只需要按照数据流方程计算即可。
    
       按照这种方式可以正确的计算出OUT集合，但是由于我们计算出的IN集合不包括phi指令中的变量，所以还需要将phi指令中的右值变量补充进IN集合，这就是为什么最后还需要补全每一个块的IN集合。
    
       ```c++
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
       
       ```
    
       
    
    3. 补全每一个块的IN集合。
    
       ```c++
       for (auto bb : func_->get_basic_blocks())
       {
           for (auto val_bb : phi_val_bb[bb])
           {
               live_in[bb].insert(val_bb.first);
           }
       }
       ```
    
       

### 实验总结

此次实验有什么收获

通过本次实验我们实现了常量传播与死代码删除，循环不变式外提，和活跃变量分析，加深了我们对编译优化方法的了解，也学会了如何基于lightir开发优化pass。而且也学会了c++的STL的一些基本运用。在组内讨论和合作中，不仅提升了我们的团队写作能力，也使我们在分享交流中共同进步。

### 实验反馈 （可选 不会评分）

对本次实验的建议

本次实验设计很合理。

### 组间交流 （可选）

本次实验和哪些组（记录组长学号）交流了哪一部分信息

无
