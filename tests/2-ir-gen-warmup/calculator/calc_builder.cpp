#include "calc_builder.hpp"

/***

override指定了子类的这个虚函数是重写的父类的。
当不希望某个类被继承或不希望某个虚函数被重写，可以再类名和虚函数后添加final关键字。

::表明类的作用域（标明类的变量、函数）或命名空间作用域（注明所使用的类、函数属于哪一个命名空间）。

shared_ptr是一种智能指针（smart pointer），作用有如同指针，但会记录有多少个shared_ptrs共同指向一个对象。这便是所谓的引用计数（reference counting）。
一旦最后一个这样的指针被销毁，也就是一旦某个对象的引用计数变为0，这个对象会被自动删除。

std::unique_ptr 实现了独享所有权的语义。
一个非空的 std::unique_ptr 总是拥有它所指向的资源。转移一个 std::unique_ptr 将会把所有权也从源指针转移给目标指针（源指针被置空）。
拷贝一个 std::unique_ptr 将不被允许，因为如果你拷贝一个 std::unique_ptr ,那么拷贝结束后，这两个 std::unique_ptr 都会指向相同的资源，它们都认为自己拥有这块资源（所以都会企图释放）。
因此 std::unique_ptr 是一个仅能移动（move_only）的类型。当指针析构时，它所拥有的资源也被销毁。默认情况下，资源的析构是伴随着调用 std::unique_ptr 内部的原始指针的 delete 操作的。

***/

std::unique_ptr<Module>
CalcBuilder::build(CalcAST &ast) {
    module = std::unique_ptr<Module>(new Module("Cminus code"));
    builder = new IRBuilder(nullptr, module.get());
    auto TyVoid = Type::get_void_type(module.get());
    TyInt32 = Type::get_int32_type(module.get());

    std::vector<Type *> output_params;
    output_params.push_back(TyInt32);
    auto output_type = FunctionType::get(TyVoid, output_params);
    auto output_fun =
        Function::create(
                output_type,
                "output",
                module.get());
    auto main = Function::create(FunctionType::get(TyInt32, {}),
                                    "main", module.get());
    auto bb = BasicBlock::create(module.get(), "entry", main);
    builder->set_insert_point(bb);
    ast.run_visitor(*this);
    builder->create_call(output_fun, {val});
    builder->create_ret(ConstantInt::get(0, module.get()));
    return std::move(module);
} 
void CalcBuilder::visit(CalcASTInput &node) {
    node.expression->accept(*this);
}
void CalcBuilder::visit(CalcASTExpression &node) {
    if (node.expression == nullptr) {
        node.term->accept(*this);
    } else {
        node.expression->accept(*this);
        auto l_val = val;
        node.term->accept(*this);
        auto r_val = val;
        switch (node.op) {
        case OP_PLUS:
            val = builder->create_iadd(l_val, r_val);
            break;
        case OP_MINUS:
            val = builder->create_isub(l_val, r_val);
            break;
        }
    }
}

void CalcBuilder::visit(CalcASTTerm &node) {
    if (node.term == nullptr) {
        node.factor->accept(*this);
    } else {
        node.term->accept(*this);
        auto l_val = val;
        node.factor->accept(*this);
        auto r_val = val;
        switch (node.op) {
        case OP_MUL:
            val = builder->create_imul(l_val, r_val);
            break;
        case OP_DIV:
            val = builder->create_isdiv(l_val, r_val);
            break;
        }
    }
}

void CalcBuilder::visit(CalcASTNum &node) {
    val = ConstantInt::get(node.val, module.get());
}
