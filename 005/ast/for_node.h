#pragma once

#include <cdk/ast/expression_node.h>
#include <cdk/ast/basic_node.h> 
#include <cdk/ast/sequence_node.h>

namespace udf {

  /**
   * Class for describing for-cycle nodes.
   */
  class for_node : public cdk::basic_node {
    cdk::sequence_node *_init;
    cdk::sequence_node *_condition;
    cdk::sequence_node *_increment;
    cdk::basic_node *_instruction;

  public:
    for_node(int lineno, cdk::sequence_node *init, cdk::sequence_node *condition,cdk::sequence_node *increment, cdk::basic_node *instruction) :
        basic_node(lineno), _init(init), _condition(condition), _increment(increment), _instruction(instruction) {
    }

    
    cdk::sequence_node *init() { return _init; }

    cdk::sequence_node *condition() { return _condition; }

    cdk::sequence_node *increment() { return _increment; }

    cdk::basic_node *instruction() { return _instruction; }

    void accept(basic_ast_visitor *sp, int level) { sp->do_for_node(this, level); }

  };

} // udf
