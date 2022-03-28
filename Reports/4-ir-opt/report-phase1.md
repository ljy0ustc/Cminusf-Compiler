# Lab4 实验报告-阶段一

#### 小组成员 
姓名:廖佳怡 学号:PB19151776
姓名:邓博以 学号:PB19061260

## 实验要求

请按照自己的理解，写明本次实验需要干什么
需要阅读LoopSearch和Mem2Reg相关代码，理解循环查找和SSA构造的思路，了解代码优化的过程和方式，掌握如何去调用LightIR的接口来开发优化Pass，为phase2打下基础。

## 思考题
### LoopSearch
1. `LoopSearch`中直接用于描述一个循环的数据结构是什么？需要给出其具体类型。
   
   ```c++
   std::unordered_set<BBset_t *> loop_set;//所有循环的集合，每一个元素就是一个循环
   using BBset_t = std::unordered_set<BasicBlock *>;//一个循环也是一个集合，每个元素是一个bb
   ```

2. 循环入口是重要的信息，请指出`LoopSearch`中如何获取一个循环的入口？需要指出具体代码，并解释思路。
   
   **核心过程：**
   
   循环的入口即其他块进入该循环的目标块，所以我们只需要考察循环中的每一个块的前驱块，如果它的某个前驱块不在该循环中，说明该块是循环的入口。
   
   第一种情况
   
   找出某个块，满足该块的某个前驱块不在该循环中，该块是循环的入口
   
   ```c++
   CFGNodePtr base = nullptr;
   for (auto n : *set)
   {
       for (auto prev : n->prevs)
       {
           if (set->find(prev) == set->end())
           {
               base = n;
           }
       }
   }
   ```
   
   第二种情况
   
   如果所有块的前驱块都在该循环中，由run函数的过程可知，这是因为当我们访问了一个外部循环之后，会将其循环入口删除，并且在其他块中删除该循环入口的存在（即不将其作为前驱和后继），所以这种情况说明该循环是内部循环，应该遍历所有被删除的循环入口（即reserved集合），如果某个循环入口的后继在该循环中，说明该后继就是该内层循环的入口。
   
   ```c++
   if (base != nullptr)
       return base;
   for (auto res : reserved)
   {
       for (auto succ : res->succs)
       {
           if (set->find(succ) != set->end())
           {
               base = succ;
           }
       }
   }
   ```
   
   **辅助过程：**
   
   在`LoopSearch.hpp`中为`LoopSearch`定义了public函数`BasicBlock* get_loop_base(BBset_t *loop) { return loop2base[loop]; }`，和private量`std::unordered_map<BBset_t *, BasicBlock *> loop2base;`。
   在`LoopSearch.cpp`中`void LoopSearch::run()`函数里有`loop2base.insert({bb_set, base->bb});`。`run()`里for循环枚举了模块的函数列表`func_list`中每一个函数`func`，对基本块数量大于0的函数，将函数通过`build_cfg`构建流图，在while循环中通过`strongly_connected_components`每次迭代剩余的nodes中的强连通分量，for循环迭代每一个强连通分量scc，将scc中所有node存储在`bb_set`中。通过`auto base = find_loop_base(scc, reserved);`将scc的base存储在`base`中，再用`loop2base.insert({bb_set, base->bb})`键值对加入loop2base存储。
   核心函数是`CFGNodePtr LoopSearch::find_loop_base(CFGNodePtrSet *set,CFGNodePtrSet &reserved)`。
   
   


3. 仅仅找出强连通分量并不能表达嵌套循环的结构。为了处理嵌套循环，`LoopSearch`在Tarjan algorithm的基础之上做了什么特殊处理？
   
   **大体思路：**
   
   在run函数中，我们每次找到一个循环就会将其循环入口删除，并且在其他块中删除该循环入口的存在（即不将其作为前驱和后继），然后再次调用strongly_connected_components(nodes, sccs)，由于循环入口被删除，所以原来该循环的强连通分量会被划分为更小的强连通分量，即内层循环。通过上述的方法进行迭代就可以找出所有的循环。
   
   **具体实现：**

   1. 用tarjan算法求出极大强连通分量，也就是剩余结点中的各个最外层循环。
   2.  枚举每个极大强连通分量scc。 
   3. 对每个scc用`find_loop_base`求出循环入口base。 
   4.  `reserved.insert(base);`和`nodes.erase(base);`将base从nodes挪到reserved。把base从nodes中删去相当于剥掉外层循环，把base加入到reserved相当于保留在回收站。 
   5.  重复上述过程。
   
4. 某个基本块可以属于多层循环中，`LoopSearch`找出其所属的最内层循环的思路是什么？这里需要用到什么数据？这些数据在何时被维护？需要指出数据的引用与维护的代码，并简要分析。
   在`LoopSearch.hpp`有
   ```c++
   BBset_t *get_inner_loop(BasicBlock* bb){
        if(bb2base.find(bb) == bb2base.end())
            return nullptr;
        return base2loop[bb2base[bb]];
    }
   ```
   定义了get_inner_loop函数来返回基本块所处的最内层循环。
   而在`LoopSearch.cpp`的`run()`函数中，有
   
   ```c++
   // step 5: map each node to loop base
   for (auto bb : *bb_set)
   {
       if (bb2base.find(bb) == bb2base.end())
           bb2base.insert({bb, base->bb});
       else
           bb2base[bb] = base->bb;
   }
   ```
   它出于以Tarjan算法重复计算强连通分量的while循环内，故每次计算都是剥离了外层循环，循环到越后面更新的值是对应着内层循环的。
   由于std::unordered_map插入重复key的时候是忽略而非替换，故这里分类讨论了，如果bb不在bb2base的关键字里，则插入`{bb,base->bb}`键值对；如果bb在bb2base的关键字里，则更新该键值对的为`bb2base[bb]=base->bb`。这样就保持了bb2base中的`{bb,base->bb}`键值对始终为最新，即始终对应最内层循环。
   
   
### Mem2reg
1. 请简述概念：支配性、严格支配性、直接支配性、支配边界。
   * 支配性：如果从流图的起点开始，每条到达n的路径都要经过d，则称d时n的支配节点
   * 严格支配性：a支配b，且a不等于b，则a严格支配b
   * 直接支配性：所有严格支配b的节点中，和b距离最近的节点（有向路径最短）
   * 支配边界：通俗来讲，如果a支配b，c为b的直接后继，a不严格支配c，则c属于a的支配边界
  
2. `phi`节点是SSA的关键特征，请简述`phi`节点的概念，以及引入`phi`节点的理由。
   * 概念：在构造SSA的过程中，会在CFG的具有多个前驱的每个程序块起始处，为当前过程中定义或使用的每个名字y插入一个`phi`函数，如$y\leftarrow \phi (y,y)$。对于CFG中的每一个前驱块，`phi`函数都应该有一个参数与之对应。
   * 引入理由：当某个块中的某个变量在不同路径中被定值，我们要确定在该块中使用哪一条路径中的值，就需要借助`phi`函数。在汇合点处，不同的静态单赋值形式名由`phi`调和为同一个名字，即为其选择正确前驱块中的值定义。
   
3. 下面给出的cminus代码显然不是SSA的，后面是使用lab3的功能将其生成的LLVM IR（未加任何Pass），说明对一个变量的多次赋值变成了什么形式？
   对a的多次赋值过程中，每次对a的重新赋值都是将计算`1+2`和`a*4`得到的值存入`%op0`中。通过LLVM IR可以看出，对一个变量的多次赋值每次都遵循：先计算出右值，再将右值store到变量的地址中。
   
4. 对下面给出的cminus程序，使用lab3的功能，分别关闭/开启Mem2Reg生成LLVM IR。对比生成的两段LLVM IR，开启Mem2Reg后，每条load, store指令发生了变化吗？变化或者没变化的原因是什么？请分类解释。
   * 删除了函数参数的store、load指令。
    没开启`Mem2Reg`之前，会在函数开头alloca多个局部变量，用于存放函数的参数，当需要使用这些变量时，再通过load得到，而开启`Mem2Reg`后则直接使用函数的参数，不需要先store再load。
   * 删除了条件分支中的store、load指令。
    对于条件分支语句，使用了phi语句来得到最终的值，而不是在bb中store或者load
   * 直接删除某些变量的声明，从而减少store和load。
    对于b变量，它是一个局部变量，并且它的作用仅仅是获得一个值，然后将这个值作为函数参数传递给函数，所以`Mem2Reg`直接删除了该变量的声明，并且直接将其应该获得的值直接作为函数参数传递给函数。
   
5. 指出放置phi节点的代码，并解释是如何使用支配树的信息的。需要给出代码中的成员变量或成员函数名称。
   * 找到所有全局活跃变量`global_live_var_name`（store指令的`l_val`）和其定值的`bb`(`live_var_2blocks`)。
   * 对于每一个全局活跃变量，构造`work_list`，初始值为该全局活跃变量定值的所有`bb`。
   * 对于`work_list`中的每个bb，找到其df集合`dominators_->get_dominance_frontier(bb)`，通过分析可以知道，其df集合的每一个bb`bb_dominance_frontier_bb`都应该在最前方插入一条`phi`（如果已经有了就不用插入），由于每一条`phi`都相当于一条定值，所以应该把`df`集合中的每一个`bb`都插入`work_list`中，依次迭代即可。

    用到的支配树信息有get_dominance_frontier(bb)，用于找bb的支配边界。
   ```
    void Mem2Reg::generate_phi()
    {
        // step 1: find all global_live_var_name x and get their blocks 
        std::set<Value *> global_live_var_name;
        std::map<Value *, std::set<BasicBlock *>> live_var_2blocks;
        for ( auto bb : func_->get_basic_blocks() )
        {
            std::set<Value *> var_is_killed;
            for ( auto instr : bb->get_instructions() )
            {
                if ( instr->is_store() )
                {
                    // store i32 a, i32 *b
                    // a is r_val, b is l_val
                    auto r_val = static_cast<StoreInst *>(instr)->get_rval();
                    auto l_val = static_cast<StoreInst *>(instr)->get_lval();

                    if (!IS_GLOBAL_VARIABLE(l_val) && !IS_GEP_INSTR(l_val))
                    {
                        global_live_var_name.insert(l_val);
                        live_var_2blocks[l_val].insert(bb);
                    }
                }
            }
        }

        // step 2: insert phi instr
        std::map<std::pair<BasicBlock *,Value *>, bool> bb_has_var_phi; // bb has phi for var
        for (auto var : global_live_var_name )
        {
            std::vector<BasicBlock *> work_list;
            work_list.assign(live_var_2blocks[var].begin(), live_var_2blocks[var].end());
            for (int i =0 ; i < work_list.size() ; i++ )
            {   
                auto bb = work_list[i];
                for ( auto bb_dominance_frontier_bb : dominators_->get_dominance_frontier(bb))
                {
                    if ( bb_has_var_phi.find({bb_dominance_frontier_bb, var}) ==    bb_has_var_phi.end() )
                    { 
                        // generate phi for bb_dominance_frontier_bb & add  bb_dominance_frontier_bb to work list
                        auto phi = PhiInst::create_phi(var->get_type()  ->get_pointer_element_type(), bb_dominance_frontier_bb);
                        phi->set_lval(var);
                        bb_dominance_frontier_bb->add_instr_begin( phi );
                        work_list.push_back( bb_dominance_frontier_bb );
                        bb_has_var_phi[{bb_dominance_frontier_bb, var}] = true;
                    }
                }
            }
        }
    }
   ```
   

### 代码阅读总结

此次实验有什么收获

通过对两个pass的源码阅读和思考，为之后的phase2做准备。

##### LoopSearch
循环查找是属于一种分析类Pass，该类Pass是用来为之后的优化Pass获取CFG的必要信息，而不会对CFG本身造成改变；循环查找的主要目的就是为了找到程序当中的所有循环，以及这些循环的入口块（即循环的条件块）等信息。该分析Pass也是后续的循环不变式外提优化的基础。其重点是：
* 使用Tarjan算法求剥去已找到的循环入口块的剩余图的极大强连通分量
* 从外向内迭代，寻找循环入口块


##### Mem2Reg
Mem2Reg Pass构造了LLVM IR 的SSA格式(静态单赋值格式)，Mem2Reg调用了Dominators类来生成支配树信息。由于最大静态单赋值形式包含很多冗余，这里采用了半剪枝静态单赋值形式，其大致流程如下：
* Dominators计算支配信息，如支配边界、直接支配等，生成支配者树
* Mem2Reg的`generate_phi`函数放置`phi`函数，主要是找到全局活跃变量和插入`phi`函数
* Mem2Reg的`re_name`函数进行重命名
* Mem2Reg的`remove_alloca`函数删去冗余指令


### 实验反馈 （可选 不会评分）

对本次实验的建议

本次实验设计非常合理，收获颇丰。

### 组间交流 （可选）

本次实验和哪些组（记录组长学号）交流了哪一部分信息

无
