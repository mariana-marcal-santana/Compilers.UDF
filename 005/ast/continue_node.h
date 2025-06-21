#pragma once

#include <cdk/ast/expression_node.h>

namespace udf {

    /**
     * Class for describing continue nodes.
     */
    class continue_node : public cdk::basic_node {
  
    public:
      continue_node(int lineno) :
          basic_node(lineno) {
      }

      void accept(basic_ast_visitor *sp, int level) { sp->do_continue_node(this, level); }
  
    };
  
  } // udf
  