#pragma once

#include <cdk/ast/expression_node.h>
#include <vector>
#include <functional>

namespace udf {

    class tensor_node : public cdk::expression_node {
        cdk::sequence_node *_cells;
        cdk::sequence_node *_cell_values;
        std::vector<std::size_t> _dims;

    public:
        tensor_node(int lineno, cdk::sequence_node *cells) :
            cdk::expression_node(lineno), _cells(cells) {
              _cell_values = get_cell_values(cells);
              _dims = get_dims(cells);
        }

        cdk::sequence_node *cells() {
            return _cells;
        }

        cdk::sequence_node *cell_values() {
            return _cell_values;
        }
    
        const std::vector<std::size_t> &dims() const { return _dims; }
        
        void accept(basic_ast_visitor *sp, int level) {
            sp->do_tensor_node(this, level);
        }

    private:
        inline std::vector<std::size_t> get_dims(cdk::sequence_node *node) {
        std::vector<std::size_t> dims;

        std::function<void(cdk::basic_node *)> collect_dims = [&](cdk::basic_node *n) {
          auto seq = dynamic_cast<cdk::sequence_node *>(n);
          if (!seq) return; 

          dims.push_back(seq->size());

          if (seq->size() == 0) return;

          collect_dims(seq->node(0));
        };

        collect_dims(node);
        return dims;
      }

      static cdk::sequence_node *get_cell_values(cdk::sequence_node *node) {
        std::vector<cdk::basic_node *> flat;
        std::function<void(cdk::basic_node *)> flat_cells = [&](cdk::basic_node *n) {
          if (auto seq = dynamic_cast<cdk::sequence_node *>(n)) {
            for (auto *child : seq->nodes())
              flat_cells(child);
          } else {
            flat.push_back(n);
          }
        };
        flat_cells(node);

        cdk::sequence_node *result = nullptr;
        for (auto *n : flat)
          result = new cdk::sequence_node(n->lineno(), n, result);

        return result ? result : new cdk::sequence_node(node->lineno());
      }

    };

} // udf
