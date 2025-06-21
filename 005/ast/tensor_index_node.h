#pragma once

#include <cdk/ast/expression_node.h>

namespace udf {

  /**
   * Class for describing tensor index node.
   * Represents an expression that returns the value of a tensor at a specific index.
   */
  class tensor_index_node : public cdk::lvalue_node {
    cdk::expression_node *_tensor;
    cdk::sequence_node *_indices;

  public:
    tensor_index_node(int lineno, cdk::expression_node *tensor, cdk::sequence_node *indices) :
        cdk::lvalue_node(lineno), _tensor(tensor), _indices(indices) {}

    cdk::expression_node *tensor() { return _tensor; }
    cdk::sequence_node *indices() { return _indices; }

    void accept(basic_ast_visitor *sp, int level) {
      sp->do_tensor_index_node(this, level);
    }

  };

} // udf
