#pragma once

#include <cdk/ast/expression_node.h>
#include <cdk/ast/lvalue_node.h>

namespace udf {
  
    /**
    * Class for describing address of lvalue nodes.
    */
    class address_of_node: public cdk::expression_node {
        cdk::lvalue_node *_lvalue;
        
    public:
        inline address_of_node(int lineno, cdk::lvalue_node *lvalue) :
            cdk::expression_node(lineno), _lvalue(lvalue) {
        }
        
        inline cdk::lvalue_node *lvalue() {
        return _lvalue;
        }

        void accept(basic_ast_visitor *sp, int level) {
            sp->do_address_of_node(this, level);
        }
    };

} // udf    