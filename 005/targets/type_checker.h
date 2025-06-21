#pragma once

#include "targets/basic_ast_visitor.h"

namespace udf {

  /**
   * Print nodes as XML elements to the output stream.
   */
  class type_checker: public basic_ast_visitor {
    cdk::symbol_table<udf::symbol> &_symtab;
    std::shared_ptr<udf::symbol> _function;
    std::shared_ptr<cdk::basic_type> _inBlockReturnType = nullptr;
    basic_ast_visitor *_parent;

  public:
    type_checker(std::shared_ptr<cdk::compiler> compiler, cdk::symbol_table<udf::symbol> &symtab, std::shared_ptr<udf::symbol> func, basic_ast_visitor *parent) :
        basic_ast_visitor(compiler), _symtab(symtab), _function(func), _parent(parent) {
    }

  public:
    ~type_checker() {
      os().flush();
    }

  protected:
    void processUnaryExpression(cdk::unary_operation_node *const node, int lvl);
    void processBinaryExpression(cdk::binary_operation_node *const node, int lvl);
    void processIRTBinaryExpression(cdk::binary_operation_node *const node, int lvl);
    void processIBinaryExpression(cdk::binary_operation_node *const node, int lvl);
    void processLogicalBinaryExpression(cdk::binary_operation_node *const node, int lvl);
    void processIRBinaryExpression(cdk::binary_operation_node *const node, int lvl);
    void processIRTPBinaryExpression(cdk::binary_operation_node *const node, int lvl);
    void processBinaryComparativeExpression(cdk::binary_operation_node *const node, int lvl);
    void processBinaryEqualityExpression(cdk::binary_operation_node *const node, int lvl);
    void processBinaryAddSubExpression(cdk::binary_operation_node *const node, int lvl, bool subtraction);
    bool checkPointers(std::shared_ptr<cdk::basic_type> t1, std::shared_ptr<cdk::basic_type> t2);
    template<typename T>
    void process_literal(cdk::literal_node<T> *const node, int lvl) {
    }

  public:
    // do not edit these lines
#define __IN_VISITOR_HEADER__
#include ".auto/visitor_decls.h"       // automatically generated
#undef __IN_VISITOR_HEADER__
    // do not edit these lines: end

  };

} // udf

//---------------------------------------------------------------------------
//     HELPER MACRO FOR TYPE CHECKING
//---------------------------------------------------------------------------

#define CHECK_TYPES(compiler, symtab, function, node) { \
  try { \
    udf::type_checker checker(compiler, symtab, function, this); \
    (node)->accept(&checker, 0); \
  } \
  catch (const std::string &problem) { \
    std::cerr << (node)->lineno() << ": " << problem << std::endl; \
    return; \
  } \
}

#define ASSERT_SAFE_EXPRESSIONS CHECK_TYPES(_compiler, _symtab, _function, node)
