#pragma once

#include <cdk/ast/expression_node.h>

namespace udf {

  /**
   * Class for describing tensor dimension (dim) node.
   * Represents an expression that returns the dimension of a tensor.
   */
  class tensor_dim_node : public cdk::expression_node {
    cdk::expression_node *_tensor, *_index;

  public:
    tensor_dim_node(int lineno, cdk::expression_node *tensor, cdk::expression_node *index) :
        cdk::expression_node(lineno), _tensor(tensor), _index(index) {}

    cdk::expression_node *tensor() { return _tensor; }
    cdk::expression_node *index() { return _index; }

    void accept(basic_ast_visitor *sp, int level) {
      sp->do_tensor_dim_node(this, level);
    }

  };

} // udf
