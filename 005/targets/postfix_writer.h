#pragma once

#include "targets/basic_ast_visitor.h"

#include <sstream>
#include <stack>
#include <cdk/emitters/basic_postfix_emitter.h>
#include <set>

namespace udf {

  //!
  //! Traverse syntax tree and generate the corresponding assembly code.
  //!
  class postfix_writer: public basic_ast_visitor {
    cdk::symbol_table<udf::symbol> &_symtab;
    cdk::basic_postfix_emitter &_pf;
    int _lbl;

    bool _inFunctionBody = false, _inFunctionArgs = false;
    bool _inForInit;
    bool _returnSeen;
    std::stack<int> _forIni, _forStep, _forEnd;
    std::shared_ptr<udf::symbol> _function;
    std::string _currentBodyRetLabel;
    std::set<std::string> _functions_to_declare;
    int _offset;
    bool _memInitialized = false;

  public:
    postfix_writer(std::shared_ptr<cdk::compiler> compiler, cdk::symbol_table<udf::symbol> &symtab,
                   cdk::basic_postfix_emitter &pf) :
        basic_ast_visitor(compiler), _symtab(symtab), _pf(pf), _lbl(0), _inFunctionBody(false), _inForInit(false) {
    }

  public:
    ~postfix_writer() {
      os().flush();
    }
  protected:
  void processCompares(cdk::binary_operation_node *const node, int lvl);
  
  private:
    /** Method used to generate sequential labels. */
    inline std::string mklbl(int lbl) {
      std::ostringstream oss;
      if (lbl < 0)
        oss << ".L" << -lbl;
      else
        oss << "_L" << lbl;
      return oss.str();
    }
    void error(int lineno, std::string e) {
      std::cerr << lineno << ": " << e << std::endl;
    }
  public:
  // do not edit these lines
#define __IN_VISITOR_HEADER__
#include ".auto/visitor_decls.h"       // automatically generated
#undef __IN_VISITOR_HEADER__
  // do not edit these lines: end

  };

} // udf
