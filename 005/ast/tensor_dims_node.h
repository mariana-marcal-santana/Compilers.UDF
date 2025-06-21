#pragma once

#include <cdk/ast/expression_node.h>

namespace udf {

  /**
   * Class for describing tensor dimensions (dims) node.
   * Represents an expression that returns the dimensions vector of a tensor.
   */
  class tensor_dims_node : public cdk::expression_node {
    cdk::expression_node *_tensor;

  public:

    tensor_dims_node(int lineno, cdk::expression_node *tensor) :
        cdk::expression_node(lineno), _tensor(tensor) {}

    cdk::expression_node *tensor() { return _tensor; }

    void accept(basic_ast_visitor *sp, int level) {
      sp->do_tensor_dims_node(this, level);
    }

  };

} // udf
