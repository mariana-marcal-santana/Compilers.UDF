#pragma once

#include <cdk/ast/basic_node.h>
#include <cdk/ast/expression_node.h>

namespace udf {

  /**
   * Class for describing write statements.
   */
  class write_node : public cdk::basic_node {
    cdk::sequence_node *_args;
    bool _newline;

  public:
    write_node(int lineno, cdk::sequence_node *args, bool newline = false) :
        cdk::basic_node(lineno), _args(args), _newline(newline) {
    }

    cdk::sequence_node *args() {
      return _args;
    }
    bool newline() {
      return _newline;
    }

    void accept(basic_ast_visitor *sp, int level) {
      sp->do_write_node(this, level);
    }
  };

} // udf
