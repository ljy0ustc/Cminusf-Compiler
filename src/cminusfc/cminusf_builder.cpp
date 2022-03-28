/*
 * 声明：本代码为 2021 秋 中国科大编译原理（李诚）课程实验参考实现。
 * 请不要以任何方式，将本代码上传到可以公开访问的站点或仓库
*/

#include "cminusf_builder.hpp"

#define CONST_FP(num) \
    ConstantFP::get((float)num, module.get())
#define CONST_INT(num) \
    ConstantInt::get(num, module.get())

// You can define global variables here
// to store state

// store temporary value
Value *tmp_val = nullptr;
// whether require lvalue
bool require_lvalue = false;
// function that is being built
Function *cur_fun = nullptr;
// detect scope pre-enter (for elegance only)
bool pre_enter_scope = false;

// types
Type *VOID_T;
Type *INT1_T;
Type *INT32_T;
Type *INT32PTR_T;
Type *FLOAT_T;
Type *FLOATPTR_T;

bool promote(IRBuilder *builder, Value **l_val_p, Value **r_val_p) {
    bool is_int;
    auto &l_val = *l_val_p;
    auto &r_val = *r_val_p;
    if (l_val->get_type() == r_val->get_type()) {
        is_int = l_val->get_type()->is_integer_type();
    } else {
        is_int = false;
        if (l_val->get_type()->is_integer_type())
            l_val = builder->create_sitofp(l_val, FLOAT_T);
        else
            r_val = builder->create_sitofp(r_val, FLOAT_T);
    }
    return is_int;
}


/*
 * use CMinusfBuilder::Scope to construct scopes
 * scope.enter: enter a new scope
 * scope.exit: exit current scope
 * scope.push: add a new binding to current scope
 * scope.find: find and return the value bound to the name
 */

void CminusfBuilder::visit(ASTProgram &node) {
    VOID_T = Type::get_void_type(module.get());
    INT1_T = Type::get_int1_type(module.get());
    INT32_T = Type::get_int32_type(module.get());
    INT32PTR_T = Type::get_int32_ptr_type(module.get());
    FLOAT_T = Type::get_float_type(module.get());
    FLOATPTR_T = Type::get_float_ptr_type(module.get());

    for (auto decl: node.declarations) {
        decl->accept(*this);
    }
}

void CminusfBuilder::visit(ASTNum &node) {
    if (node.type == TYPE_INT)
        tmp_val = CONST_INT(node.i_val);
    else
        tmp_val = CONST_FP(node.f_val);
}

void CminusfBuilder::visit(ASTVarDeclaration &node) {
    Type *var_type;
    if (node.type == TYPE_INT)
        var_type = Type::get_int32_type(module.get());
    else
        var_type = Type::get_float_type(module.get());
    if (node.num == nullptr) {
        if (scope.in_global()) {
            auto initializer = ConstantZero::get(var_type, module.get());
            auto var = GlobalVariable::create
                    (
                    node.id,
                    module.get(),
                    var_type,
                    false,
                    initializer);
            scope.push(node.id, var);
        } else {
            auto var = builder->create_alloca(var_type);
            scope.push(node.id, var);
        }
    } else {
        auto *array_type = ArrayType::get(var_type, node.num->i_val);
        if (scope.in_global()) {
            auto initializer = ConstantZero::get(array_type, module.get());
            auto var = GlobalVariable::create
                    (
                    node.id,
                    module.get(),
                    array_type,
                    false,
                    initializer);
            scope.push(node.id, var);
        } else {
            auto var = builder->create_alloca(array_type);
            scope.push(node.id, var);
        }
    }
}

void CminusfBuilder::visit(ASTFunDeclaration &node) {
    FunctionType *fun_type;
    Type *ret_type;
    std::vector<Type *> param_types;
    if (node.type == TYPE_INT)
        ret_type = INT32_T;
    else if (node.type == TYPE_FLOAT)
        ret_type = FLOAT_T;
    else
        ret_type = VOID_T;

    for (auto& param: node.params) {
        if (param->type == TYPE_INT) {
            if (param->isarray) {
                param_types.push_back(INT32PTR_T);
            } else {
                param_types.push_back(INT32_T);
            }
        } else {
            if (param->isarray) {
                param_types.push_back(FLOATPTR_T);
            } else {
                param_types.push_back(FLOAT_T);
            }
        }
    }

    fun_type = FunctionType::get(ret_type, param_types);
    auto fun =
        Function::create(
                fun_type,
                node.id,
                module.get());
    scope.push(node.id, fun);
    cur_fun = fun;
    auto funBB = BasicBlock::create(module.get(), "entry", fun);
    builder->set_insert_point(funBB);
    scope.enter();
    pre_enter_scope = true;
    std::vector<Value*> args;
    for (auto arg = fun->arg_begin();arg != fun->arg_end();arg++) {
        args.push_back(*arg);
    }
    for (int i = 0;i < node.params.size();++i) {
        if (node.params[i]->isarray) {
            Value *array_alloc;
            if (node.params[i]->type == TYPE_INT)
                array_alloc = builder->create_alloca(INT32PTR_T);
            else
                array_alloc = builder->create_alloca(FLOATPTR_T);
            builder->create_store(args[i], array_alloc);
            scope.push(node.params[i]->id, array_alloc);
        } else {
            Value *alloc;
            if (node.params[i]->type == TYPE_INT)
                alloc = builder->create_alloca(INT32_T);
            else
                alloc = builder->create_alloca(FLOAT_T);
            builder->create_store(args[i], alloc);
            scope.push(node.params[i]->id, alloc);
        }
    }
    node.compound_stmt->accept(*this);
    if (builder->get_insert_block()->get_terminator() == nullptr){
        if (cur_fun->get_return_type()->is_void_type())
            builder->create_void_ret();
        else if (cur_fun->get_return_type()->is_float_type())
            builder->create_ret(CONST_FP(0.));
        else
            builder->create_ret(CONST_INT(0));
    }
    scope.exit();
}

void CminusfBuilder::visit(ASTParam &node) { }

void CminusfBuilder::visit(ASTCompoundStmt &node) {
    bool need_exit_scope = !pre_enter_scope;
    if (pre_enter_scope) {
        pre_enter_scope = false;
    } else {
        scope.enter();
    }

    for (auto& decl: node.local_declarations) {
        decl->accept(*this);
    }

    for (auto& stmt: node.statement_list) {
        stmt->accept(*this);
        if (builder->get_insert_block()->get_terminator() != nullptr)
            break;
    }

    if (need_exit_scope) {
        scope.exit();
    }
}

void CminusfBuilder::visit(ASTExpressionStmt &node) {
    if (node.expression != nullptr)
        node.expression->accept(*this);
}

void CminusfBuilder::visit(ASTSelectionStmt &node) {
    node.expression->accept(*this);
    auto ret_val = tmp_val;
    auto trueBB = BasicBlock::create(module.get(), "", cur_fun);
    auto falseBB = BasicBlock::create(module.get(), "", cur_fun);
    auto contBB = BasicBlock::create(module.get(), "", cur_fun);
    Value *cond_val;
    if (ret_val->get_type()->is_integer_type())
        cond_val = builder->create_icmp_ne(ret_val, CONST_INT(0));
    else
        cond_val = builder->create_fcmp_ne(ret_val, CONST_FP(0.));

    if (node.else_statement == nullptr) {
        builder->create_cond_br(cond_val, trueBB, contBB);
    } else {
        builder->create_cond_br(cond_val, trueBB, falseBB);
    }
    builder->set_insert_point(trueBB);
    node.if_statement->accept(*this);

    if (builder->get_insert_block()->get_terminator() == nullptr)
        builder->create_br(contBB);

    if (node.else_statement == nullptr) {
        falseBB->erase_from_parent();
    } else {
        builder->set_insert_point(falseBB);
        node.else_statement->accept(*this);
        if (builder->get_insert_block()->get_terminator() == nullptr)
            builder->create_br(contBB);
    }

    builder->set_insert_point(contBB);
}

void CminusfBuilder::visit(ASTIterationStmt &node) {
    auto exprBB = BasicBlock::create(module.get(), "", cur_fun);
    if (builder->get_insert_block()->get_terminator() == nullptr)
        builder->create_br(exprBB);
    builder->set_insert_point(exprBB);
    node.expression->accept(*this);
    auto ret_val = tmp_val;
    auto trueBB = BasicBlock::create(module.get(), "", cur_fun);
    auto contBB = BasicBlock::create(module.get(), "", cur_fun);
    Value *cond_val;
    if (ret_val->get_type()->is_integer_type())
        cond_val = builder->create_icmp_ne(ret_val, CONST_INT(0));
    else
        cond_val = builder->create_fcmp_ne(ret_val, CONST_FP(0.));

    builder->create_cond_br(cond_val, trueBB, contBB);
    builder->set_insert_point(trueBB);
    node.statement->accept(*this);
    if (builder->get_insert_block()->get_terminator() == nullptr)
        builder->create_br(exprBB);
    builder->set_insert_point(contBB);
}

void CminusfBuilder::visit(ASTReturnStmt &node) {
    if (node.expression == nullptr) {
        builder->create_void_ret();
    } else {
        auto fun_ret_type = cur_fun->get_function_type()->get_return_type();
        node.expression->accept(*this);
        if (fun_ret_type != tmp_val->get_type()) {
            if (fun_ret_type->is_integer_type())
                tmp_val = builder->create_fptosi(tmp_val, INT32_T);
            else
                tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
        }

        builder->create_ret(tmp_val);
    }
}

void CminusfBuilder::visit(ASTVar &node) {
    auto var = scope.find(node.id);
    assert(var != nullptr);
    auto is_int = var->get_type()->get_pointer_element_type()->is_integer_type();
    auto is_float = var->get_type()->get_pointer_element_type()->is_float_type();
    auto is_ptr = var->get_type()->get_pointer_element_type()->is_pointer_type();
    bool should_return_lvalue = require_lvalue;
    require_lvalue = false;
    if (node.expression == nullptr) {
        if (should_return_lvalue) {
            tmp_val = var;
            require_lvalue = false;
        } else {
            if (is_int || is_float || is_ptr) {
                tmp_val = builder->create_load(var);
            } else {
                tmp_val = builder->create_gep(var, {CONST_INT(0), CONST_INT(0)});
            }
        }
    } else {
        node.expression->accept(*this);
        auto val = tmp_val;
        Value *is_neg;
        auto exceptBB = BasicBlock::create(module.get(), "", cur_fun);
        auto contBB = BasicBlock::create(module.get(), "", cur_fun);
        if (val->get_type()->is_float_type())
            val = builder->create_fptosi(val, INT32_T);

        is_neg = builder->create_icmp_lt(val, CONST_INT(0));

        builder->create_cond_br(is_neg, exceptBB, contBB);
        builder->set_insert_point(exceptBB);
        auto neg_idx_except_fun = scope.find("neg_idx_except");
        builder->create_call( static_cast<Function *>(neg_idx_except_fun), {});
        if (cur_fun->get_return_type()->is_void_type())
            builder->create_void_ret();
        else if (cur_fun->get_return_type()->is_float_type())
            builder->create_ret(CONST_FP(0.));
        else
            builder->create_ret(CONST_INT(0));

        builder->set_insert_point(contBB);
        Value *tmp_ptr;
        if (is_int || is_float)
            tmp_ptr = builder->create_gep(var, {val} );
        else if (is_ptr) {
            auto array_load = builder->create_load(var);
            tmp_ptr = builder->create_gep(array_load, {val} );
        } else
            tmp_ptr = builder->create_gep(var, {CONST_INT(0), val});
        if (should_return_lvalue) {
            tmp_val = tmp_ptr;
            require_lvalue = false;
        } else {
            tmp_val = builder->create_load(tmp_ptr);
        }
    }
}

void CminusfBuilder::visit(ASTAssignExpression &node) {
    node.expression->accept(*this);
    auto expr_result = tmp_val;
    require_lvalue = true;
    node.var->accept(*this);
    auto var_addr = tmp_val;
    if (var_addr->get_type()->get_pointer_element_type() != expr_result->get_type()) {
        if (expr_result->get_type() == INT32_T)
            expr_result = builder->create_sitofp(expr_result, FLOAT_T);
        else
            expr_result = builder->create_fptosi(expr_result, INT32_T);
    }
    builder->create_store(expr_result, var_addr);
    tmp_val = expr_result;
}

void CminusfBuilder::visit(ASTSimpleExpression &node) {
    if (node.additive_expression_r == nullptr) {
        node.additive_expression_l->accept(*this);
    } else {
        node.additive_expression_l->accept(*this);
        auto l_val = tmp_val;
        node.additive_expression_r->accept(*this);
        auto r_val = tmp_val;
        bool is_int = promote(builder, &l_val, &r_val);
        Value *cmp;
        switch (node.op) {
        case OP_LT:
            if (is_int)
                cmp = builder->create_icmp_lt(l_val, r_val);
            else
                cmp = builder->create_fcmp_lt(l_val, r_val);
            break;
        case OP_LE:
            if (is_int)
                cmp = builder->create_icmp_le(l_val, r_val);
            else
                cmp = builder->create_fcmp_le(l_val, r_val);
            break;
        case OP_GE:
            if (is_int)
                cmp = builder->create_icmp_ge(l_val, r_val);
            else
                cmp = builder->create_fcmp_ge(l_val, r_val);
            break;
        case OP_GT:
            if (is_int)
                cmp = builder->create_icmp_gt(l_val, r_val);
            else
                cmp = builder->create_fcmp_gt(l_val, r_val);
            break;
        case OP_EQ:
            if (is_int)
                cmp = builder->create_icmp_eq(l_val, r_val);
            else
                cmp = builder->create_fcmp_eq(l_val, r_val);
            break;
        case OP_NEQ:
            if (is_int)
                cmp = builder->create_icmp_ne(l_val, r_val);
            else
                cmp = builder->create_fcmp_ne(l_val, r_val);
            break;
        }

        tmp_val = builder->create_zext(cmp, INT32_T);
    }
}

void CminusfBuilder::visit(ASTAdditiveExpression &node) {
    if (node.additive_expression == nullptr) {
        node.term->accept(*this);
    } else {
        node.additive_expression->accept(*this);
        auto l_val = tmp_val;
        node.term->accept(*this);
        auto r_val = tmp_val;
        bool is_int = promote(builder, &l_val, &r_val);
        switch (node.op) {
        case OP_PLUS:
            if (is_int)
                tmp_val = builder->create_iadd(l_val, r_val);
            else
                tmp_val = builder->create_fadd(l_val, r_val);
            break;
        case OP_MINUS:
            if (is_int)
                tmp_val = builder->create_isub(l_val, r_val);
            else
                tmp_val = builder->create_fsub(l_val, r_val);
            break;
        }
    }
}

void CminusfBuilder::visit(ASTTerm &node) {
    if (node.term == nullptr) {
        node.factor->accept(*this);
    } else {
        node.term->accept(*this);
        auto l_val = tmp_val;
        node.factor->accept(*this);
        auto r_val = tmp_val;
        bool is_int = promote(builder, &l_val, &r_val);
        switch (node.op) {
        case OP_MUL:
            if (is_int)
                tmp_val = builder->create_imul(l_val, r_val);
            else
                tmp_val = builder->create_fmul(l_val, r_val);
            break;
        case OP_DIV:
            if (is_int)
                tmp_val = builder->create_isdiv(l_val, r_val);
            else
                tmp_val = builder->create_fdiv(l_val, r_val);
            break;
        }
    }
}

void CminusfBuilder::visit(ASTCall &node) {
    auto fun = static_cast<Function *>(scope.find(node.id));
    std::vector<Value *> args;
    auto param_type = fun->get_function_type()->param_begin();
    for (auto &arg: node.args) {
        arg->accept(*this);
        if (!tmp_val->get_type()->is_pointer_type() &&
            *param_type != tmp_val->get_type()) {
            if (tmp_val->get_type()->is_integer_type())
                tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
            else
                tmp_val = builder->create_fptosi(tmp_val, INT32_T);
        }
        args.push_back(tmp_val);
        param_type++;
    }

    tmp_val = builder->create_call(static_cast<Function *>(fun), args);
}

// #include "cminusf_builder.hpp"
// #include "logging.hpp"

// // use these macros to get constant value
// #define CONST_INT(num) \
//     ConstantInt::get(num, module.get())
// #define CONST_FP(num) \
//     ConstantFP::get((float)num, module.get())
// #define CONST_ZERO(type) \
//     ConstantZero::get(type, module.get())

// // You can define global variables here
// // to store state

// // 传递函数参数时使用
// Type* param;
// Type* param_type;

// Function* fun_cur;//当前所在的函数(为定义block使用)

// Value *Val_ptr;//expression值的地址
// Value *Val=NULL;//expression最后的值
// bool in_func_scope=0;//是否在function定义的时候就提前定义了而不用在compound_smtm中重复定义
// /*
//  * use CMinusfBuilder::Scope to construct scopes
//  * scope.enter: enter a new scope
//  * scope.exit: exit current scope
//  * scope.push: add a new binding to current scope
//  * scope.find: find and return the value bound to the name
//  */

// void CminusfBuilder::visit(ASTProgram &node) 
// { 
//     //遍历vector
//     for (auto declaration : node.declarations)
//     {
//         declaration->accept(*this);
//     }
// }

// void CminusfBuilder::visit(ASTNum &node) 
// { 
//     if(node.type == TYPE_INT)
//     {
//         //整型
//         Val = CONST_INT(node.i_val);
//     }
//     else if(node.type == TYPE_FLOAT)
//     {
//         //浮点型
//         Val = CONST_FP(node.f_val);
//     }
// }

// void CminusfBuilder::visit(ASTVarDeclaration &node) 
// { 
//     Type* varType;
//     if(node.type == TYPE_INT)
//     {
//         //定义整型
//         varType = Type::get_int32_type(module.get());
//     }
//     else if(node.type == TYPE_FLOAT)
//     {
//         //定义浮点型
//         varType = Type::get_float_type(module.get()); 
//     }
//     if(scope.in_global())
//     {
//         //全局
//         if(node.num)
//         {
//             //定义数组
//             auto *arrayType = ArrayType::get(varType, node.num->i_val);
//             auto initializer = ConstantZero::get(varType, module.get());
//             auto new_node = GlobalVariable::create(node.id, module.get(), arrayType, false, initializer);
//             scope.push(node.id,new_node);
//             LOG(DEBUG) << "成功定义全局数组";
//         }
//         else
//         {
//             //定义变量
//             auto initializer = ConstantZero::get(varType, module.get());
//             auto new_node = GlobalVariable::create(node.id, module.get(), varType, false, initializer);
//             scope.push(node.id,new_node);
//             LOG(DEBUG) << "成功定义全局变量";
//         }
//     }
//     else
//     {
//         //局部
//         if(node.num)
//         {
//             //定义数组
//             auto *arrayType = ArrayType::get(varType, node.num->i_val);
//             auto arrayAlloca = builder->create_alloca(arrayType);
//             scope.push(node.id,arrayAlloca);
//             LOG(DEBUG) << "成功定义局部数组";
//         }
//         else
//         {
//             //定义变量
//             auto aAlloca = builder->create_alloca(varType);
//             scope.push(node.id,aAlloca);
//             LOG(DEBUG) << "成功定义局部变量";
//         }
//     }
// }

// void CminusfBuilder::visit(ASTFunDeclaration &node) 
// { 
//     //函数返回值
//     Type* retType;
//     bool voidFlag = 0;
//     if(node.type == TYPE_INT)
//     {
//         //定义整型
//         retType = Type::get_int32_type(module.get());
//         LOG(DEBUG) << "函数返回值是INT";
//     }
//     else if(node.type == TYPE_FLOAT)
//     {
//         //定义浮点型
//         retType = Type::get_float_type(module.get()); 
//         LOG(DEBUG) << "函数返回值是FLOAT";
//     }
//     else if(node.type == TYPE_VOID)
//     {
//         //定义void型
//         retType = Type::get_void_type(module.get());
//         LOG(DEBUG) << "函数返回值是VOID";
//         voidFlag = 1;
//     }
//     /*bool voidFlag = 1;
//     retType = Type::get_void_type(module.get());*/

//     //函数参数
//     std::vector<Type *> paramsType;
//     for (auto param : node.params)
//     {
//         LOG(DEBUG) << "分析某个参数";
//         param->accept(*this);
//         paramsType.push_back(param_type);
//     }
//     LOG(DEBUG) << "paramsType的size"<< paramsType.size();

//     LOG(DEBUG) << "分析完参数";

//     auto funType = FunctionType::get(retType, paramsType);
//     LOG(DEBUG) << "定义好函数类型了";
//     auto Fun = Function::create(funType, node.id, module.get());
//     LOG(DEBUG) << "定义好函数了";
//     auto funBB = BasicBlock::create(module.get(), "funLabel", Fun);
//     LOG(DEBUG) << "创建BB";
//     builder->set_insert_point(funBB);
//     LOG(DEBUG) << "插入BB";
//     scope.push(node.id, Fun);
//     LOG(DEBUG) << "将函数注册进scope";
//     scope.enter();
//     LOG(DEBUG) << "进入新的scope";
//     fun_cur = Fun;
//     in_func_scope=1;

//     LOG(DEBUG) << "开始读取参数";
    
//     //读取参数
//     std::vector<Value *> args;
//     for (auto arg = Fun->arg_begin(); arg != Fun->arg_end(); arg++) 
//     {
//       args.push_back(*arg);
//     }
//     int i = 0;
//     for (auto param : node.params)
//     {
//         Type* paramType;
//         paramType = paramsType[i];
//         if (param->isarray)
//         {
//             scope.push(param->id, args[i]);
//         }
//         else
//         {
//             auto paramAlloca = builder->create_alloca(paramType);
//             builder->create_store(args[i], paramAlloca);
//             scope.push(param->id, paramAlloca);
//         }
        
//         i++;
//     }

//     LOG(DEBUG) << "开始读取函数内容";
    
//     //函数内容
//     node.compound_stmt->accept(*this);

//     //返回值(若compound_stmt中没有显式定义return)
//     //ret 与 br 都是 Terminator Instructions 也就是终止指令，在 llvm 基本块的定义里，基本块是单进单出的，因此只能有一条终止指令（ret 或 br）。当一个基本块有两条终止指令，clang 在做解析会认为第一个终结指令是此基本块的结束，并会开启一个新的匿名的基本块（并占用了下一个编号）。
//     if (!(builder->get_insert_block()->get_terminator()))
//     {
//         if (voidFlag)
//         {
//             builder->create_void_ret();
//             LOG(DEBUG) << "成功返回VOID";
//         }
//         else
//         {
//             builder->create_ret(CONST_ZERO(retType));
//             LOG(DEBUG) << "成功返回INT或FLOAT";
//         } 
//     }
      

//     scope.exit();
// }

// void CminusfBuilder::visit(ASTParam &node) 
// { 
//     if(node.type == TYPE_INT)
//     {
//         if(node.isarray)
//         {
//             //整型数组
//             param_type = Type::get_int32_ptr_type(module.get());
//             LOG(DEBUG) << "参数是INT数组";
//         }
//         else
//         {
//             //整型变量
//             param_type = Type::get_int32_type(module.get());
//             LOG(DEBUG) << "参数是INT变量";
//         }
//     }
//     else if(node.type == TYPE_FLOAT)
//     {
//         if(node.isarray)
//         {
//             //浮点型数组
//             param_type = Type::get_float_ptr_type(module.get());
//             LOG(DEBUG) << "参数是FLOAT数组";
//         }
//         else
//         {
//             //浮点型变量
//             param_type = Type::get_float_type(module.get());
//             LOG(DEBUG) << "参数是FLOAT变量";
//         }
//     }
//     else if(node.type == TYPE_VOID)
//     {
//         //VOID
//         param_type = Type::get_void_type(module.get());
//         LOG(DEBUG) << "参数是VOID";
//     }
// }

// void CminusfBuilder::visit(ASTCompoundStmt &node) 
// {
//     bool exit_in_cstmt=!in_func_scope;
//     if(in_func_scope)
//         in_func_scope=0;
//     else
//         scope.enter();
//     for (auto var_declaration : node.local_declarations)
//     {
//         var_declaration->accept(*this);
//     }
//     for (auto statement : node.statement_list)
//     {
//         statement->accept(*this);
//     }
//     if(exit_in_cstmt)
//         scope.exit();
// }

// void CminusfBuilder::visit(ASTExpressionStmt &node) 
// { 
//     if(node.expression)
//         node.expression->accept(*this);
// }

// void CminusfBuilder::visit(ASTSelectionStmt &node) 
// { 
//     auto trueBB = BasicBlock::create(module.get(), "", fun_cur);
//     auto if_nextBB = BasicBlock::create(module.get(), "", fun_cur);
//     node.expression->accept(*this);
//     Value* exp_val = Val;


//     if(node.else_statement)
//     {
//         auto falseBB = BasicBlock::create(module.get(), "", fun_cur);
//         //if-else型
//         if(exp_val->get_type()->is_integer_type())
//         {
//             //整型表达式
//             if (exp_val->get_type()==Type::get_int1_type(module.get()))
//             {
//                 exp_val=builder->create_zext(exp_val, Type::get_int32_type(module.get()));
//             }
//             auto icmp = builder->create_icmp_ne(exp_val, CONST_INT(0));
//             builder->create_cond_br(icmp, trueBB, falseBB); 
//         }
//         else if(exp_val->get_type()->is_float_type())
//         {
//             //浮点型表达式
//             auto fcmp = builder->create_fcmp_ne(exp_val, CONST_FP(0));
//             builder->create_cond_br(fcmp, trueBB, falseBB);
//         }
//         builder->set_insert_point(trueBB);
//         node.if_statement->accept(*this);
//         if (!(builder->get_insert_block()->get_terminator()))
// 			builder->create_br(if_nextBB);
//         builder->set_insert_point(falseBB);
//         node.else_statement->accept(*this);
//         if (!(builder->get_insert_block()->get_terminator()))
// 			builder->create_br(if_nextBB);
//     }
//     else
//     {
//         //if型(没有else)
//         if(exp_val->get_type()->is_integer_type())
//         {
//             if (exp_val->get_type()==Type::get_int1_type(module.get()))
//             {
//                 exp_val=builder->create_zext(exp_val, Type::get_int32_type(module.get()));
//             }
//             auto icmp = builder->create_icmp_ne(exp_val, CONST_INT(0));
//             builder->create_cond_br(icmp, trueBB, if_nextBB);
//         }
//         else if(exp_val->get_type()->is_float_type())
//         {
//             //浮点型表达式
//             auto fcmp = builder->create_fcmp_ne(exp_val, CONST_FP(0));
//             builder->create_cond_br(fcmp, trueBB, if_nextBB);
//         }
//         builder->set_insert_point(trueBB);
//         node.if_statement->accept(*this);
//         if (!(builder->get_insert_block()->get_terminator()))
// 			builder->create_br(if_nextBB);
//     }
//     builder->set_insert_point(if_nextBB);
// }

// void CminusfBuilder::visit(ASTIterationStmt &node) 
// { 
//     auto cmp_iterBB = BasicBlock::create(module.get(), "", fun_cur);
    
//     if (!(builder->get_insert_block()->get_terminator())) 
//         builder->create_br(cmp_iterBB);    
//     builder->set_insert_point(cmp_iterBB);
//     node.expression->accept(*this);
//     Value* exp_val = Val;

//     auto in_iterBB = BasicBlock::create(module.get(), "", fun_cur);
//     auto after_iterBB = BasicBlock::create(module.get(), "", fun_cur);
//     if(exp_val->get_type()->is_integer_type())
//     {
//         //整型表达式
//         if (exp_val->get_type()==Type::get_int1_type(module.get()))
//         {
//             exp_val=builder->create_zext(exp_val, Type::get_int32_type(module.get()));
//         }
//         auto icmp = builder->create_icmp_ne(exp_val, CONST_INT(0));
//         builder->create_cond_br(icmp, in_iterBB, after_iterBB);
        
//     }
//     else if(exp_val->get_type()->is_float_type())
//     {
//         //浮点型表达式
//         auto fcmp = builder->create_fcmp_ne(exp_val, CONST_FP(0));
//         builder->create_cond_br(fcmp, in_iterBB, after_iterBB);
//     }

    
//     builder->set_insert_point(in_iterBB);
//     node.statement->accept(*this);
//     if (!(builder->get_insert_block()->get_terminator()))
//         builder->create_br(cmp_iterBB);
//     builder->set_insert_point(after_iterBB);
// }

// void CminusfBuilder::visit(ASTReturnStmt &node) 
// { 
//     //LOG(DEBUG) << "进入Return函数";
//     //auto retBB = BasicBlock::create(module.get(), "retLabel", fun_cur);
//     //LOG(DEBUG) << "创建了retBB";
//     //builder->set_insert_point(retBB);
//     //LOG(DEBUG) << "插入了retBB";
//     if(node.expression)
//     {
//         //有返回值
//         LOG(DEBUG) << "非空返回";
//         node.expression->accept(*this);
//         Value* expVal = Val;
//         auto fun_retType = fun_cur->get_function_type()->get_return_type();
//         if((fun_retType->is_integer_type() && expVal->get_type()->is_integer_type())||(fun_retType->is_float_type() && expVal->get_type()->is_float_type()))
//             builder->create_ret(expVal);
//         else
//         {
//             if(expVal->get_type()->is_integer_type())
//             {
//                 expVal = builder->create_sitofp(expVal, Type::get_float_type(module.get()));
//             }
//             else
//             {
//                 expVal = builder->create_fptosi(expVal, Type::get_int32_type(module.get()));
//             }
//             builder->create_ret(expVal);
//         }
//     }
//     else
//     {
//         //空返回
//         LOG(DEBUG) << "空返回";
//         builder->create_void_ret();
//     }
    
// }

// void CminusfBuilder::visit(ASTVar &node) {
//     auto var = scope.find(node.id);
//     if (node.expression == nullptr)
//     {
//         Val_ptr = var;
//         if (var->get_type()->get_pointer_element_type()->is_array_type())
//         {
//             Val_ptr = builder->create_gep(var, {ConstantInt::get(0, module.get()), ConstantInt::get(0, module.get())});
//         }
//         else
//         {
//             Val = builder->create_load(Val_ptr);
//         }
        
//     }
//     else
//     {
//         node.expression->accept(*this);
//         auto expression = Val;
       
//         if (expression->get_type()->is_float_type())
//         {
//             expression = builder->create_fptosi(expression, Type::get_int32_type(module.get()));
//         }
//         auto icmp = builder->create_icmp_lt(expression, ConstantInt::get(0, module.get()));
//         auto current_fun = builder->get_insert_block()->get_parent();
//         auto trueBB = BasicBlock::create(module.get(), "", current_fun);
//         auto falseBB = BasicBlock::create(module.get(), "", current_fun);
//         builder->create_cond_br(icmp, trueBB, falseBB);

//         builder->set_insert_point(trueBB);
//         auto neg_idx_except = scope.find("neg_idx_except");
//         builder->create_call(neg_idx_except,{});
//         builder->create_br(falseBB);

//         builder->set_insert_point(falseBB);
//         if (var->get_type()->get_pointer_element_type()->is_array_type())
//         {
//             Val_ptr = builder->create_gep(var, {ConstantInt::get(0, module.get()), expression});
//         }
//         else
//         {
//             Val_ptr = builder->create_gep(var, {expression});
//         }
//         Val = builder->create_load(Val_ptr);
//     }
    
// }

// void CminusfBuilder::visit(ASTAssignExpression &node) {
//     node.var->accept(*this);
//     auto var_ptr = Val_ptr;
//     node.expression->accept(*this);
//     if (Val->get_type() == Type::get_int1_type(module.get()))
//     {
//         Val = builder->create_zext(Val, Type::get_int32_type(module.get()));
//     }
    
//     auto expression = Val;
//     if (var_ptr->get_type()->get_pointer_element_type() != expression->get_type())
//     {
//         if (expression->get_type()->is_integer_type())
//         {
//             expression = builder->create_sitofp(expression, Type::get_float_type(module.get()));
//         }
//         else
//         {
//             expression = builder->create_fptosi(expression, Type::get_int32_type(module.get()));
//         }
//     }
//     builder->create_store(expression, var_ptr);
//     Val = expression;
// }

// void CminusfBuilder::visit(ASTSimpleExpression &node) { 
//     if (node.additive_expression_r == nullptr)
// 	{
//         node.additive_expression_l->accept(*this);
//     }
//     else
//     {
//         node.additive_expression_l->accept(*this);
//         if (Val->get_type() == Type::get_int1_type(module.get()))
//         {
//             Val = builder->create_zext(Val, Type::get_int32_type(module.get()));
//         }
//         auto lval = Val;
//         node.additive_expression_r->accept(*this);
//         if (Val->get_type() == Type::get_int1_type(module.get()))
//         {
//             Val = builder->create_zext(Val, Type::get_int32_type(module.get()));
//         }
//         auto rval = Val;
//         if (lval->get_type() != rval->get_type())
//         {//判断两个操作数的类型，如不同，则都变成float
//             if (lval->get_type()->is_integer_type())
//             {
//                 lval = builder->create_sitofp(lval, Type::get_float_type(module.get()));
//             }
//             else
//             {
//                 rval = builder->create_sitofp(rval, Type::get_float_type(module.get()));
//             }
//         }
//         if (lval->get_type()->is_integer_type())
//         {
//             switch (node.op)
//             {
//             case OP_LE:
//                 Val = builder->create_icmp_le(lval, rval);
//                 break;
//             case OP_LT:
//                 Val = builder->create_icmp_lt(lval, rval);
//                 break;
//             case OP_GT:
//                 Val = builder->create_icmp_gt(lval, rval);
//                 break;
//             case OP_GE:
//                 Val = builder->create_icmp_ge(lval, rval);
//                 break;
//             case OP_EQ:
//                 Val = builder->create_icmp_eq(lval, rval);
//                 break;
//             case OP_NEQ:
//                 Val = builder->create_icmp_ne(lval, rval);
//                 break;
//             }
//         }
//         else
//         {
//             switch (node.op)
//             {
//             case OP_LE:
//                 Val = builder->create_fcmp_le(lval, rval);
//                 break;
//             case OP_LT:
//                 Val = builder->create_fcmp_lt(lval, rval);
//                 break;
//             case OP_GT:
//                 Val = builder->create_fcmp_gt(lval, rval);
//                 break;
//             case OP_GE:
//                 Val = builder->create_fcmp_ge(lval, rval);
//                 break;
//             case OP_EQ:
//                 Val = builder->create_fcmp_eq(lval, rval);
//                 break;
//             case OP_NEQ:
//                 Val = builder->create_fcmp_ne(lval, rval);
//                 break;
//             }
//         }
//     }
// }

// void CminusfBuilder::visit(ASTAdditiveExpression &node) {
//     if (node.additive_expression == nullptr)
// 	{
//         node.term->accept(*this);
//     }
//     else
//     {
//         node.additive_expression->accept(*this);
//         if (Val->get_type() == Type::get_int1_type(module.get()))
//         {
//             Val = builder->create_zext(Val, Type::get_int32_type(module.get()));
//         }
//         auto lval = Val;
//         node.term->accept(*this);
//         if (Val->get_type() == Type::get_int1_type(module.get()))
//         {
//             Val = builder->create_zext(Val, Type::get_int32_type(module.get()));
//         }
//         auto rval = Val;
//         if (lval->get_type() != rval->get_type())
//         {//判断两个操作数的类型，如不同，则都变成float
//             if (lval->get_type()->is_integer_type())
//             {
//                 lval = builder->create_sitofp(lval, Type::get_float_type(module.get()));
//             }
//             else
//             {
//                 rval = builder->create_sitofp(rval, Type::get_float_type(module.get()));
//             }
//         }
//         if (lval->get_type()->is_integer_type())
//         {
//             switch (node.op)
//             {
//             case OP_PLUS:
//                 Val = builder->create_iadd(lval, rval);
//                 break;
            
//             default:
//                 Val = builder->create_isub(lval, rval);
//                 break;
//             }
//         }
//         else
//         {
//             switch (node.op)
//             {
//             case OP_PLUS:
//                 Val = builder->create_fadd(lval, rval);
//                 break;
            
//             default:
//                 Val = builder->create_fsub(lval, rval);
//                 break;
//             }
//         }
//     }
//  }

// void CminusfBuilder::visit(ASTTerm &node) {
//     if (node.term == nullptr)
//     {
//         node.factor->accept(*this);
//     }
//     else
//     {
//         node.term->accept(*this);
//         auto lval = Val;
//         node.factor->accept(*this);
//         auto rval = Val;
//         if (lval->get_type() != rval->get_type())
//         {//判断两个操作数的类型，如不同，则都变成float
//             if (lval->get_type()->is_integer_type())
//             {
//                 lval = builder->create_sitofp(lval, Type::get_float_type(module.get()));
//             }
//             else
//             {
//                 rval = builder->create_sitofp(rval, Type::get_float_type(module.get()));
//             }
//         }
//         if (lval->get_type()->is_integer_type())
//         {
//             switch (node.op)
//             {
//             case OP_MUL:
//                 Val = builder->create_imul(lval, rval);
//                 break;
            
//             default:
//                 Val = builder->create_isdiv(lval, rval);
//                 break;
//             }
//         }
//         else
//         {
//             switch (node.op)
//             {
//             case OP_MUL:
//                 Val = builder->create_fmul(lval, rval);
//                 break;
            
//             default:
//                 Val = builder->create_fdiv(lval, rval);
//                 break;
//             }
//         }
//     }
//  }

// void CminusfBuilder::visit(ASTCall &node) {
//     auto fun_name = static_cast<Function *>(scope.find(node.id));//找到函数
//     auto fun_tp = fun_name->get_function_type();//通过funtype可以检查实参的类型是否和形参一样
//     int para_num = 0;
//     std::vector<Value *> params;//存类型转换之后的实参
    
//     for (auto para : node.args)
//     {
//         para->accept(*this);
//         if (!(fun_tp->get_param_type(para_num)->is_pointer_type()) && Val->get_type() != fun_tp->get_param_type(para_num))
//         {//当参数类型不正确时，需要强制转换，此时无非就是int to float或float to int
//             LOG(DEBUG) << "强制转换";
//             if (Val->get_type()==Type::get_int1_type(module.get()))
//             {
//                 Val=builder->create_zext(Val, Type::get_int32_type(module.get()));
//             }
//             else if (fun_tp->get_param_type(para_num)->is_integer_type())
//             {
//                 LOG(DEBUG) << "强制转换成INT";
//                 Val = builder->create_fptosi(Val, Type::get_int32_type(module.get()));
//             }
//             else
//             {
//                 LOG(DEBUG) << "强制转换成FLOAT";
//                 Val = builder->create_sitofp(Val, Type::get_float_type(module.get()));
//             }
//             params.push_back(Val);
//         }
//         else if (fun_tp->get_param_type(para_num)->is_pointer_type())
//         {
//             LOG(DEBUG) << "参数是指针";
//             params.push_back(Val_ptr);
//         }
//         else
//         {
//             LOG(DEBUG) << "不用转换且不是指针";
//             params.push_back(Val);
//         }
        
//         para_num++;
//     }

//     Val = builder->create_call(fun_name, params);
// }
