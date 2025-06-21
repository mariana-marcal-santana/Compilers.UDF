#pragma once

#include <cdk/ast/typed_node.h>
#include <cdk/ast/expression_node.h>
#include <cdk/types/basic_type.h>
#include <string>

namespace udf {
    
  /**
   * Class for describing variable declaration nodes.
   */
  class variable_declaration_node : public cdk::typed_node {
    int _qualifier; 
    std::string _identifier;
    cdk::expression_node *_initializer;

    public:
        variable_declaration_node(int lineno, int qualifier,
                                std::shared_ptr<cdk::basic_type> var_type,
                                const std::string &identifier,
                                cdk::expression_node *initializer) :
                cdk::typed_node(lineno), _qualifier(qualifier), _identifier(identifier), _initializer(initializer) {
            type(var_type);  
        }

        int qualifier() const { return _qualifier; }

        const std::string &identifier() const { return _identifier; }

        cdk::expression_node *initializer() { return _initializer; }

        void accept(basic_ast_visitor *sp, int level) {
            sp->do_variable_declaration_node(this, level);
        }
    };
    
} // udf

