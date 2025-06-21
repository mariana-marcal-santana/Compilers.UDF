#pragma once

#include <cdk/ast/expression_node.h>

namespace udf {

  /**
   * Class for describing tensor rank nodes.
   * Represents an expression that returns the number of dimensions of a tensor.
   */
  class tensor_rank_node : public cdk::expression_node {
    cdk::expression_node *_tensor;

  public:
    tensor_rank_node(int lineno, cdk::expression_node *tensor) :
        cdk::expression_node(lineno), _tensor(tensor) {
    }

    cdk::expression_node *tensor() {
      return _tensor;
    }

    void accept(basic_ast_visitor *sp, int level) {
      sp->do_tensor_rank_node(this, level);
    }

  };

} // udf
