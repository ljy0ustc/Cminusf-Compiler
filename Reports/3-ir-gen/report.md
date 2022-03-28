# Lab3 实验报告

##小组成员:
廖佳怡PB19151776
邓博以PB19061260


## 实验难点

实验中遇到哪些挑战

* 函数定义中作用域和compound_stmt作用域的划定
  
* 函数定义中参数的传递
  
* Selection和Iteration中的分支和跳转控制
  
* 返回值和参数的类型检查与转换
  
* 对于类型转换的处理，最容易忽略i1到i32的转换
  
* 对于可共享的全局变量的定义是否合理，如果定义的合理可以大幅减少代码的实现
  
* 对于数组类型的处理

* 对全局变量和局部变量的区别处理

## 实验设计

请写明为了顺利完成本次实验，加入了哪些亮点设计，并对这些设计进行解释。
可能的阐述方向有:

1. 如何设计全局变量
2. 遇到的难点以及解决方案
3. 如何降低生成 IR 中的冗余
4. ...

* 全局变量的设计
  * 函数参数传递
    `Type* param_type;`
    主要是为了连接`ASTFunDeclaration`和`ASTParam`，在`node.params`的每个`param`进行`accept`的时候，将参数类型存储在`param_type`中，便于生成函数参数的vector`paramsType`，生成的vector用于定义函数的类型。
    
  * 当前所在函数
    `Function* fun_cur;`
    在进行`BasicBlock::create`的时候由于需要指明所在函数，故通过全局变量`fun_cur`存储。
    
  * `Value *Val_ptr;//expression值的地址`
    
    `Value *Val=NULL;//expression最后的值`
  
    这两个全局变量较为相似，但所使用的情景不同，当一个变量作为右值或者参数（非指针）进行传递时，我们只需要得到其值，所以我们只需要使用Val；而当变量作为左值被赋值时，我们需要得到其地址，以便将其新的值赋给给变量，此时应该使用Val_ptr。
    
  * `bool in_func_scope=0;`
    这个布尔变量指明是否在function定义的时候就提前enter scope了，而不用在compound_smtm中重复进入。需要在function中enter_scope是因为参数也在函数的scope中。在compound_stmt中不仅在enter scope时需要判断`in_func_scope`，在exit scope的时候也需要判断`exit_in_cstmt`。
  
* 遇到的难点以及解决方案

  * 对于类型转换的处理

    类型转换的处理较为繁琐，很容易忽略各种情况。对于i32和float的处理比较容易，而对于i1的处理就很容易忘记。首先是当i1作为右值进行赋值时，需要先转换（扩展）为i32，再根据左值的类型进一步处理。其次，当逻辑计算的结果作为另一个逻辑计算的分量时，也得首先转换（扩展）为i32，再根据另一分量的类型进一步处理。最后，当一个变量（i32或者float）作为条件判断表达式时，不能将其直接传递给br指令，必须先与0进行比较，将结果转化为i1，这样才能传递给br指令。

  * 对于数组类型的处理

    由于ir数组中的数组实际上是一个“二维数组”，在处理起来时会有一些繁琐，所以当数组声明完成后，我们就将其转化为一个基本类型的指针，并以该指针代替数组，这样一来在对数组元素赋值时就可以和普通的变量统一起来，不用特殊操作。但是实际上来说，这种方法只能适用于不存在多维数组的情况，如果有多维数组，那么我们还需要知道数组每一维的规模，仅仅一个基本类型的指针是远远不够的，但是由于此次实验不要求检查数组上界并且没有高维数组，所以我们才采用了该方法。

  * IR的冗余解决

    当数组声明完成后，我们就将其转化为一个基本类型的指针，并以该指针代替数组，这样一来就避免了每次调用数组时都需要将数组指针转化为一个基本类型的指针，可以减少gep命令的数量。
  
  * 函数定义中作用域和compound_stmt作用域的划定
    
    一般来说，compound_stmt中定义的变量应该属于该compound_stmt作用域中的变量。但由于对于一个函数来说，其参数需要我们在语义分析的时候重新为函数参数分配空间和定义为新的局部变量，也应与该函数的compound_stmt属于同一个作用域。故此处使用了全局变量`in_func_scope`来判断是否在分析该函数参数前已经进入其作用域了，避免在分析compound_stmt的时候进入新的域。

  * 函数定义中参数的传递
  
    首先当遍历每个param in ASTFunDeclaration node.params时，通过函数`void CminusfBuilder::visit(ASTParam &node)`来读取每个参数类型，存入全局变量`param_type`中，再在`void CminusfBuilder::visit(ASTFunDeclaration &node)`中将其压入vector`paramsType`，从而得到参数类型的vector`paramsType`，进而同之前分析得到的`retType`一起定义函数类型`funType`。
    在进入函数作用域之后，通过arg的for循环将参数压入参数vector`args`，然后再根据`paramsType`为每个参数开辟空间，将其存储。

  * Selection和Iteration中的分支和跳转控制

    在`void CminusfBuilder::visit(ASTSelectionStmt &node)`中，分成`if-else`型和`if`型两种选择来讨论。为`if`型创建了`trueBB`和`if_nextBB`，跳转为`builder->create_cond_br(cmp, trueBB, if_nextBB);`；为`if-else`型额外创建了`falseBB`，跳转为`builder->create_cond_br(cmp, trueBB, falseBB); `。
    在`void CminusfBuilder::visit(ASTIterationStmt &node) `中，通过分支跳转`builder->create_cond_br(cmp, in_iterBB, after_iterBB)`来进行循环。
    有几点特别要注意的地方，都是我们在不断试错中发现的。第一是在对expression进行条件判断时，需要将INT1类型的expression转换成INT32型的expression与INT32型ZERO比较大小，而float和INT32型expression就自然与相应类型的ZERO来比较。正如前面类型转换中的处理所提到。
    第二是，selection和iteration结束之后要记得break到nextBB。 
    第三，在为选择和循环中的分支和跳转控制创建BasicBlock的时候，不能自己命名Label。我们之前就是因为自己有给Label命名而导致多重选择或多重循环，还有同一个程序的多个选择和多个循环，发现都容易出现Label的混乱，以至于出现跳转错误。
    
  * 在`void CminusfBuilder::visit(ASTVarDeclaration &node)`中，需要由`scope.in_global()`来判断当前出于全局还是局部。全局变量通过`GlobalVariable::create(node.id, module.get(), arrayType或varType, false, initializer);`来定义，局部变量不仅要定义类型还要为其分配空间。


### 实验总结

此次实验有什么收获

通过本次实验完整地感受了中间代码的生成过程。与前两次初试牛刀的实验不同，本次实验需要我们对编译器的语法、语义分析有清晰的认知，需深入理解助教所设计的LightIR工具，并熟练掌握C++编程，对我们来说是很大的挑战，同样也是一种成长。
起初看到空白的16个函数一头雾水，几乎以DFS的方式仔细浏览了实验仓库中所调用的代码文件，特别是Lab2里的`ast.hpp`和`gcd_array_generator.cpp`。万事开头难，我们组在仔细阅读了文档之对实验要求有了大致了解后，两人深入探讨和交流，于是也大致明白应该如何构造我们的函数了。我们一个从前往后写，一个从后往前写，每写一个函数都加深了我们对编译的认知。
最后DEBUG的时候，虽然遇到了很多问题，特别是函数定义、选择和循环、还有进制转换上进行了不断的修改和尝试，最终所有问题迎刃而解。

### 实验反馈 （可选 不会评分）

对本次实验的建议

本次实验设计的很合理。

### 组间交流 （可选）

本次实验和哪些组（记录组长学号）交流了哪一部分信息

无