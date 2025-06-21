#pragma once

#include <cdk/ast/expression_node.h>

namespace udf {

  /**
   * Class for describing tensor reshape node.
   * Represents an expression that reshapes a tensor.
   */
  class tensor_reshape_node : public cdk::expression_node {
    cdk::expression_node *_tensor;
    cdk::sequence_node *_new_dims;

  public:
    tensor_reshape_node(int lineno, cdk::expression_node *tensor, cdk::sequence_node *new_dims) :
        cdk::expression_node(lineno), _tensor(tensor), _new_dims(new_dims) {
    }

    cdk::expression_node *tensor() { return _tensor; }
    cdk::sequence_node *new_dims() { return _new_dims; }

    void accept(basic_ast_visitor *sp, int level) {
      sp->do_tensor_reshape_node(this, level);
    }

  };

} // udf
