#include <string>
#include "targets/type_checker.h"
#include ".auto/all_nodes.h"
#include <cdk/types/primitive_type.h>

#include "udf_parser.tab.h"

#define ASSERT_UNSPEC { if (node->type() != nullptr && !node->is_typed(cdk::TYPE_UNSPEC)) return; }

//---------------------------------------------------------------------------

bool udf::type_checker::checkPointers(std::shared_ptr<cdk::basic_type> t1, std::shared_ptr<cdk::basic_type> t2) {
  auto ptr1 = t1, ptr2 = t2;

  while (ptr1->name() == cdk::TYPE_POINTER && ptr2->name() == cdk::TYPE_POINTER) {
    ptr1 = cdk::reference_type::cast(ptr1)->referenced();
    ptr2 = cdk::reference_type::cast(ptr2)->referenced();
  }
  return ptr1->name() == ptr2->name() || ptr2->name() == cdk::TYPE_UNSPEC;
}

//---------------------------------------------------------------------------

void udf::type_checker::do_sequence_node(cdk::sequence_node *const node, int lvl) {
  for (auto n : node->nodes())
    n->accept(this, lvl);
}

//---------------------------------------------------------------------------

void udf::type_checker::do_nil_node(cdk::nil_node *const node, int lvl) {
  // EMPTY
}
void udf::type_checker::do_data_node(cdk::data_node *const node, int lvl) {
  // EMPTY
}

//---------------------------------------------------------------------------

void udf::type_checker::do_integer_node(cdk::integer_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void udf::type_checker::do_double_node(cdk::double_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
}

void udf::type_checker::do_string_node(cdk::string_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(4, cdk::TYPE_STRING));
}

//---------------------------------------------------------------------------

void udf::type_checker::do_not_node(cdk::not_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->argument()->accept(this, lvl + 2);
  if (!node->argument()->is_typed(cdk::TYPE_INT)) {
    throw std::string("wrong type in unary logical expression");
  }
  node->type(node->argument()->type());
}

void udf::type_checker::processLogicalBinaryExpression(cdk::binary_operation_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  if (!node->left()->is_typed(cdk::TYPE_INT)) {
    throw std::string("integer expression expected in logical binary expression");
  }

  node->right()->accept(this, lvl + 2);
  if (!node->right()->is_typed(cdk::TYPE_INT)) {
    throw std::string("integer expression expected in logical binary expression");
  }

  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void udf::type_checker::do_and_node(cdk::and_node *const node, int lvl) {
  processLogicalBinaryExpression(node, lvl);
}
void udf::type_checker::do_or_node(cdk::or_node *const node, int lvl) {
  processLogicalBinaryExpression(node, lvl);
}

//---------------------------------------------------------------------------

void udf::type_checker::processUnaryExpression(cdk::unary_operation_node *const node, int lvl) {
  node->argument()->accept(this, lvl + 2);
  if (node->argument()->is_typed(cdk::TYPE_INT))
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  else if (node->argument()->is_typed(cdk::TYPE_DOUBLE))
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  else if (node->argument()->is_typed(cdk::TYPE_TENSOR)) {
    auto tensor = cdk::tensor_type::cast(node->argument()->type());
    node->type(cdk::tensor_type::create(tensor->dims()));
  }
  else
    throw std::string("wrong type in unary expression");
}

void udf::type_checker::do_unary_minus_node(cdk::unary_minus_node *const node, int lvl) {
  processUnaryExpression(node, lvl);
}

void udf::type_checker::do_unary_plus_node(cdk::unary_plus_node *const node, int lvl) {
  processUnaryExpression(node, lvl);
}

//---------------------------------------------------------------------------

void udf::type_checker::processIRTBinaryExpression(cdk::binary_operation_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);

  if (node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_TENSOR)) {
    // check if tensors have the same dimensions 
    auto t1_type = std::dynamic_pointer_cast<cdk::tensor_type>(node->left()->type());
    auto t2_type = std::dynamic_pointer_cast<cdk::tensor_type>(node->right()->type());
    const auto &dims1 = t1_type->dims();
    const auto &dims2 = t2_type->dims();
    if (dims1 != dims2) {
      throw std::string("tensors in multiplication/division must have the same dimensions");
    }
    auto tensor = cdk::tensor_type::cast(node->left()->type());
    node->type(cdk::tensor_type::create(tensor->dims()));
  }
  else if ((node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_INT)) ||
           (node->right()->is_typed(cdk::TYPE_TENSOR) && node->left()->is_typed(cdk::TYPE_INT)) ||
           (node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_DOUBLE)) ||
           (node->right()->is_typed(cdk::TYPE_TENSOR) && node->left()->is_typed(cdk::TYPE_DOUBLE))) {

    auto tensor = node->left()->is_typed(cdk::TYPE_TENSOR) ? 
      cdk::tensor_type::cast(node->left()->type()) : cdk::tensor_type::cast(node->right()->type());
    node->type(cdk::tensor_type::create(tensor->dims()));
  }
  else if (node->left()->is_typed(cdk::TYPE_DOUBLE) || node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    if (node->left()->is_typed(cdk::TYPE_DOUBLE) || node->right()->is_typed(cdk::TYPE_DOUBLE)
        || node->right()->is_typed(cdk::TYPE_INT) || node->left()->is_typed(cdk::TYPE_INT))
      node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  }

  else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_INT))
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));

  else
    throw std::string("wrong type in IRT binary expression");
}

void udf::type_checker::processIRBinaryExpression(cdk::binary_operation_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  if (!node->left()->is_typed(cdk::TYPE_INT) && !node->left()->is_typed(cdk::TYPE_DOUBLE)) 
    throw std::string("wrong type in left argument of IR binary expression");

  node->right()->accept(this, lvl + 2);
  if (!node->right()->is_typed(cdk::TYPE_INT) && !node->right()->is_typed(cdk::TYPE_DOUBLE)) 
    throw std::string("wrong type in right argument of IR binary expression");
  
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void udf::type_checker::processIBinaryExpression(cdk::binary_operation_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  if (!node->left()->is_typed(cdk::TYPE_INT)) 
    throw std::string("wrong type in left argument of I binary expression");

  node->right()->accept(this, lvl + 2);
  if (!node->right()->is_typed(cdk::TYPE_INT)) 
    throw std::string("wrong type in right argument of I binary expression");

  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void udf::type_checker::processBinaryComparativeExpression(cdk::binary_operation_node *const node, int lvl) {
  ASSERT_UNSPEC;
  processIRBinaryExpression(node, lvl);
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void udf::type_checker::processBinaryEqualityExpression(cdk::binary_operation_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);

  if (node->left()->type() != node->right()->type()) {
    throw std::string("arguments of equality expression must be of the same type");
  }

  if (!checkPointers(node->left()->type(), node->right()->type())) {
    throw std::string("arguments of equality expression must be of the same type");
  }
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void udf::type_checker::processBinaryAddSubExpression(cdk::binary_operation_node *const node, int lvl, bool subtraction) {
  ASSERT_UNSPEC;

  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);

  if (node->left()->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_INT)) {
    node->type(node->left()->type());
  } 
  else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_POINTER)) {
    node->type(node->right()->type());
  } 
  else if (node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_TENSOR)) {
    auto t1_type = std::dynamic_pointer_cast<cdk::tensor_type>(node->left()->type());
    auto t2_type = std::dynamic_pointer_cast<cdk::tensor_type>(node->right()->type());
    const auto &dims1 = t1_type->dims();
    const auto &dims2 = t2_type->dims();
    if (dims1 != dims2) {
      throw std::string("tensors in addition/subtraction must have the same dimensions");
    }
    auto tensor = cdk::tensor_type::cast(node->left()->type());
    node->type(cdk::tensor_type::create(tensor->dims()));
  }
  else if ((node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_INT)) ||
           (node->right()->is_typed(cdk::TYPE_TENSOR) && node->left()->is_typed(cdk::TYPE_INT)) ||
           (node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_DOUBLE)) ||
           (node->right()->is_typed(cdk::TYPE_TENSOR) && node->left()->is_typed(cdk::TYPE_DOUBLE))) {
    auto tensor = node->left()->is_typed(cdk::TYPE_TENSOR) ? 
      cdk::tensor_type::cast(node->left()->type()) : cdk::tensor_type::cast(node->right()->type());
    node->type(cdk::tensor_type::create(tensor->dims()));
  }
  else if (node->left()->is_typed(cdk::TYPE_INT) || node->left()->is_typed(cdk::TYPE_UNSPEC)) {
    if (node->right()->is_typed(cdk::TYPE_INT)) {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
      return;
    }
    else if (node->right()->is_typed(cdk::TYPE_DOUBLE)) {
      node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
      return;
    }
    else if (node->right()->is_typed(cdk::TYPE_UNSPEC)) {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
      node->left()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
      node->right()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
      return;
    }
  } 
  else if (node->left()->is_typed(cdk::TYPE_DOUBLE)) {
    
    if (node->right()->is_typed(cdk::TYPE_DOUBLE) || node->right()->is_typed(cdk::TYPE_INT)) {
      node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
      return;
    }
    else if (node->right()->is_typed(cdk::TYPE_UNSPEC)) {
      node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
      node->right()->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
      return;
    }
  }
  else {
    if (subtraction) {
      if ((node->left()->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_POINTER)) &&
          (checkPointers(node->left()->type(), node->right()->type()))) {
        
            node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
            return;
      }
    }
    throw std::string("wrong types in binary expression");
  }
}

void udf::type_checker::do_add_node(cdk::add_node *const node, int lvl) {
  processBinaryAddSubExpression(node, lvl, false);
}
void udf::type_checker::do_sub_node(cdk::sub_node *const node, int lvl) {
  processBinaryAddSubExpression(node, lvl, true);
}
void udf::type_checker::do_mul_node(cdk::mul_node *const node, int lvl) {
  processIRTBinaryExpression(node, lvl);
}
void udf::type_checker::do_div_node(cdk::div_node *const node, int lvl) {
  processIRTBinaryExpression(node, lvl);
}
void udf::type_checker::do_mod_node(cdk::mod_node *const node, int lvl) {
  processIBinaryExpression(node, lvl);
}

void udf::type_checker::do_lt_node(cdk::lt_node *const node, int lvl) {
  processBinaryComparativeExpression(node, lvl);
}
void udf::type_checker::do_le_node(cdk::le_node *const node, int lvl) {
  processBinaryComparativeExpression(node, lvl);
}
void udf::type_checker::do_ge_node(cdk::ge_node *const node, int lvl) {
  processBinaryComparativeExpression(node, lvl);
}
void udf::type_checker::do_gt_node(cdk::gt_node *const node, int lvl) {
  processBinaryComparativeExpression(node, lvl);
}

void udf::type_checker::do_ne_node(cdk::ne_node *const node, int lvl) {
  processBinaryEqualityExpression(node, lvl);
}
void udf::type_checker::do_eq_node(cdk::eq_node *const node, int lvl) {
  processBinaryEqualityExpression(node, lvl);
}

//---------------------------------------------------------------------------

void udf::type_checker::do_variable_node(cdk::variable_node *const node, int lvl) {
  
  ASSERT_UNSPEC;
  const std::string &id = node->name();
  std::shared_ptr<udf::symbol> symbol = _symtab.find(id);

  if (symbol != nullptr) {
    node->type(symbol->type());
  } else {
    throw id;
  }
}

void udf::type_checker::do_rvalue_node(cdk::rvalue_node *const node, int lvl) {
  ASSERT_UNSPEC;
  try {
    node->lvalue()->accept(this, lvl);
    node->type(node->lvalue()->type());
  } catch (const std::string &id) {
    throw "undeclared variable '" + id + "'";
  }
}

void udf::type_checker::do_assignment_node(cdk::assignment_node *const node, int lvl) {
  ASSERT_UNSPEC;

  try {
    node->lvalue()->accept(this, lvl);
  } catch (const std::string &id) {
    throw std::string("variable used in assignment expression not declared previously");
  }
  node->rvalue()->accept(this, lvl + 4);

  auto alloc = dynamic_cast<udf::stack_alloc_node *>(node->rvalue());
  if (alloc != nullptr) {
    node->type(node->lvalue()->type());
    alloc->type(node->lvalue()->type());
    return;
  }

  if (node->lvalue()->is_typed(cdk::TYPE_INT)) {
    if (node->rvalue()->is_typed(cdk::TYPE_INT)) {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    }
    else if (node->rvalue()->is_typed(cdk::TYPE_UNSPEC)) {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
      node->rvalue()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    }
    else {
      throw std::string("wrong assignment to integer");
    }
  } 
  else if (node->lvalue()->is_typed(cdk::TYPE_POINTER)) {
    if (node->rvalue()->is_typed(cdk::TYPE_POINTER)) {
      node->type(node->rvalue()->type());
    } 
    else if (node->rvalue()->is_typed(cdk::TYPE_INT)) {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_POINTER));
    }
    else if (node->rvalue()->is_typed(cdk::TYPE_DOUBLE)) {
      node->type(cdk::primitive_type::create(1, cdk::TYPE_VOID));
    } 
    else if (node->rvalue()->is_typed(cdk::TYPE_UNSPEC)) {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_ERROR));
      node->rvalue()->type(cdk::primitive_type::create(4, cdk::TYPE_ERROR));
    } 
    else {
      throw std::string("wrong assignment to pointer");
    }
  } 
  else if (node->lvalue()->is_typed(cdk::TYPE_DOUBLE)) {

    if (node->rvalue()->is_typed(cdk::TYPE_DOUBLE) || node->rvalue()->is_typed(cdk::TYPE_INT)) {
      node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
    } else if (node->rvalue()->is_typed(cdk::TYPE_UNSPEC)) {
      node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
      node->rvalue()->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
    } else {
      throw std::string("wrong assignment to real");
    }
  } 
  else if (node->lvalue()->is_typed(cdk::TYPE_STRING)) {

    if (node->rvalue()->is_typed(cdk::TYPE_STRING)) {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_STRING));
    } else if (node->rvalue()->is_typed(cdk::TYPE_UNSPEC)) {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_STRING));
      node->rvalue()->type(cdk::primitive_type::create(4, cdk::TYPE_STRING));
    } else {
      throw std::string("wrong assignment to string");
    }

  }
  else if (node->lvalue()->is_typed(cdk::TYPE_TENSOR)) {
    node->type(node->lvalue()->type());
  }
  else {
    throw std::string("wrong types in assignment");
  }
}

//---------------------------------------------------------------------------

void udf::type_checker::do_evaluation_node(udf::evaluation_node *const node, int lvl) {
  node->argument()->accept(this, lvl + 2);
}

//---------------------------------------------------------------------------

void udf::type_checker::do_if_node(udf::if_node *const node, int lvl) {
  node->condition()->accept(this, lvl + 4);
  node->block()->accept(this, lvl + 4);
}

void udf::type_checker::do_if_else_node(udf::if_else_node *const node, int lvl) {
  node->condition()->accept(this, lvl + 4);
  node->thenblock()->accept(this, lvl + 4);
  node->elseblock()->accept(this, lvl + 4);
}

//---------------------------------------------------------------------------

void udf::type_checker::do_break_node(udf::break_node *const node, int lvl) {
  // EMPTY
}

void udf::type_checker::do_continue_node(udf::continue_node *const node, int lvl) {
  // EMPTY
}

void udf::type_checker::do_for_node(udf::for_node *const node, int lvl) {

  _symtab.push();

  if (node->init()) {
    node->init()->accept(this, lvl + 2);
  }
  if (node->condition()) {
    node->condition()->accept(this, lvl + 2);
  }
  if (node->increment()) {
    node->increment()->accept(this, lvl + 2);
  }
  node->instruction()->accept(this, lvl + 2);

  _symtab.pop();
}

void udf::type_checker::do_input_node(udf::input_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(0, cdk::TYPE_UNSPEC));
}


void udf::type_checker::do_nullptr_node(udf::nullptr_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::reference_type::create(4, nullptr));
}

void udf::type_checker::do_stack_alloc_node(udf::stack_alloc_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->argument()->accept(this, lvl + 2);

  if (node->argument()->is_typed(cdk::TYPE_UNSPEC)) {
    node->argument()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } 
  else if (!node->argument()->is_typed(cdk::TYPE_INT)) {
    throw std::string("wrong type in argument of unary expression");
  }

  node->type(cdk::reference_type::create(4, cdk::primitive_type::create(1, cdk::TYPE_VOID)));
}

void udf::type_checker::do_return_node(udf::return_node *const node, int lvl) {

  if (node->retval()) {

    if (_function->type() != nullptr && _function->is_typed(cdk::TYPE_VOID)) 
      throw std::string("initializer specified for void function.");

    node->retval()->accept(this, lvl + 2);

    if (_function->type() == nullptr) {
      _function->set_type(node->retval()->type());
      return;
    }

    if (_inBlockReturnType == nullptr) {
      _inBlockReturnType = node->retval()->type();
    } else {
      if (_inBlockReturnType != node->retval()->type()) {
        _function->set_type(cdk::primitive_type::create(0, cdk::TYPE_ERROR));  // probably irrelevant
        throw std::string("all return statements in a function must return the same type.");
      }
    }

    if (_function->is_typed(cdk::TYPE_INT)) {
      if (!node->retval()->is_typed(cdk::TYPE_INT)) throw std::string("wrong type for initializer (integer expected).");
    } 
    else if (_function->is_typed(cdk::TYPE_DOUBLE)) {
      if (!node->retval()->is_typed(cdk::TYPE_INT) && !node->retval()->is_typed(cdk::TYPE_DOUBLE)) {
        throw std::string("wrong type for initializer (integer or double expected).");
      }
    } 
    else if (_function->is_typed(cdk::TYPE_STRING)) {
      if (!node->retval()->is_typed(cdk::TYPE_STRING)) {
        throw std::string("wrong type for initializer (string expected).");
      }
    } 
    else if (_function->is_typed(cdk::TYPE_POINTER)) {
      int ft = 0, rt = 0;
      auto ftype = _function->type();
      while (ftype->name() == cdk::TYPE_POINTER) {
        ft++;
        ftype = cdk::reference_type::cast(ftype)->referenced();
      }
      auto rtype = node->retval()->type();

      while (rtype != nullptr && rtype->name() == cdk::TYPE_POINTER) {
        rt++;
        rtype = cdk::reference_type::cast(rtype)->referenced();
      }
      bool compatible = (ft == rt) && (rtype == nullptr || (rtype != nullptr && ftype->name() == rtype->name()));
      if (!compatible) throw std::string("wrong type for return expression (pointer expected).");

    } else {
      throw std::string("unknown type for initializer.");
    }
  } else if (! _function->is_typed(cdk::TYPE_VOID)) {
    throw std::string("empty return in non-void function");
  }
}

void udf::type_checker::do_sizeof_node(udf::sizeof_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->expression()->accept(this, lvl + 2);
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void udf::type_checker::do_write_node(udf::write_node *const node, int lvl) {

  for (size_t i = 0; i < node->args()->size(); i++) {
    auto child = dynamic_cast<cdk::expression_node*>(node->args()->node(i));

    child->accept(this, lvl);

    if (child->is_typed(cdk::TYPE_UNSPEC)) {
      child->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    } 
    else if (!child->is_typed(cdk::TYPE_INT) && !child->is_typed(cdk::TYPE_DOUBLE)
          && !child->is_typed(cdk::TYPE_STRING) && !child->is_typed(cdk::TYPE_TENSOR)) {
      throw std::string("wrong type for argument " + std::to_string(i + 1) + " of write instruction");
    }
  }
}

void udf::type_checker::do_function_declaration_node(udf::function_declaration_node *const node, int lvl) {
  std::string id;

  if (node->identifier() == "udf") 
    id = "_main";
  else 
    id = node->identifier();

  auto function = udf::make_symbol(node->qualifier(), node->type(), id, false, true, true); 

  std::vector < std::shared_ptr < cdk::basic_type >> argtypes;
  for (size_t i = 0; i < node->arguments()->size(); i++) 
    argtypes.push_back(node->argument(i)->type());
  function->set_argument_types(argtypes);

  std::shared_ptr<udf::symbol> previous = _symtab.find(function->name());
  if (previous) {
    throw std::string("conflicting declaration for '" + function->name() + "'");
  }
  else {
    _symtab.insert(function->name(), function);
    _parent->set_new_symbol(function);  
  }
}

void udf::type_checker::do_variable_declaration_node(udf::variable_declaration_node *const node, int lvl) {

  if (node->initializer() != nullptr) {   

    node->initializer()->accept(this, lvl + 2); 

    if (node->type() == nullptr) {
      node->type(node->initializer()->type());
    }
    else if (node->is_typed(cdk::TYPE_INT)) {
      if (!node->initializer()->is_typed(cdk::TYPE_INT)) 
        throw std::string("wrong type for initializer (integer expected).");
    } 
    else if (node->is_typed(cdk::TYPE_DOUBLE)) {
      if (!node->initializer()->is_typed(cdk::TYPE_INT) && !node->initializer()->is_typed(cdk::TYPE_DOUBLE))
        throw std::string("wrong type for initializer (integer or double expected).");
    } 
    else if (node->is_typed(cdk::TYPE_STRING)) {
      if (!node->initializer()->is_typed(cdk::TYPE_STRING))
        throw std::string("wrong type for initializer (string expected).");
    } 
    else if (node->is_typed(cdk::TYPE_POINTER)) {
      if (!node->initializer()->is_typed(cdk::TYPE_POINTER)) {
        auto in = (cdk::literal_node<int>*)node->initializer();
        if (in == nullptr || in->value() != 0) 
          throw std::string("wrong type for initializer (pointer expected).");
      }
    }
    else if (node->is_typed(cdk::TYPE_TENSOR)) {
      if (!node->initializer()->is_typed(cdk::TYPE_TENSOR))
        throw std::string("wrong type for initializer (tensor expected).");
    }
    else {
      throw std::string("unknown type for initializer.");
    }
  }

  auto id = node->identifier();

  auto symbol = udf::make_symbol(node->qualifier(), node->type(), id, (bool)node->initializer(), false);
  if (_symtab.insert(id, symbol)) {
    _parent->set_new_symbol(symbol);
  } else {
    throw std::string("variable '" + id + "' redeclared");
  }
}

void udf::type_checker::do_block_node(udf::block_node *const node, int lvl) {
}

void udf::type_checker::do_function_definition_node(udf::function_definition_node * const node, int lvl) {
  
  std::string id;

  if (node->identifier() == "udf")
    id = "_main";
  else
    id = node->identifier();

  _inBlockReturnType = nullptr;

  auto function = udf::make_symbol(node->qualifier(), node->type(), id, true, true, false);

  std::vector < std::shared_ptr < cdk::basic_type >> argtypes;
  for (size_t ax = 0; ax < node->arguments()->size(); ax++)
    argtypes.push_back(node->argument(ax)->type());
  function->set_argument_types(argtypes);

  std::shared_ptr<udf::symbol> previous = _symtab.find(function->name());
  if (previous) {
    if (previous->function() && previous->forward()) { 
      _symtab.replace(function->name(), function);
      _parent->set_new_symbol(function);
    } else {
      throw std::string("conflicting definition for '" + function->name() + "'");
    }
  } else {
    _symtab.insert(function->name(), function);
    _parent->set_new_symbol(function);
  }
}

void udf::type_checker::do_function_call_node(udf::function_call_node * const node, int lvl) {
  ASSERT_UNSPEC;

  const std::string &id = node->identifier();
  auto symbol = _symtab.find(id);
  if (symbol == nullptr) throw std::string("symbol '" + id + "' is undeclared.");
  if (!symbol->function()) throw std::string("symbol '" + id + "' is not a function.");

  node->type(symbol->type());

  if (node->arguments()->size() == symbol->number_of_arguments()) {
    node->arguments()->accept(this, lvl + 4);
    for (size_t ax = 0; ax < node->arguments()->size(); ax++) {
      if (node->argument(ax)->type() == symbol->argument_type(ax)) continue;
      if (symbol->argument_is_typed(ax, cdk::TYPE_DOUBLE) && node->argument(ax)->is_typed(cdk::TYPE_INT)) continue;
      throw std::string("type mismatch for argument " + std::to_string(ax + 1) + " of '" + id + "'.");
    }
  } else {
    throw std::string(
        "number of arguments in call (" + std::to_string(node->arguments()->size()) + ") must match declaration ("
            + std::to_string(symbol->number_of_arguments()) + ").");
  }
}

void udf::type_checker::do_tensor_capacity_node(udf::tensor_capacity_node * const node, int lvl) {
  ASSERT_UNSPEC;

  node->tensor()->accept(this, lvl + 2);
  if (!node->tensor()->is_typed(cdk::TYPE_TENSOR)) {
    throw std::string("tensor capacity requires a tensor argument");
  }
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void udf::type_checker::do_tensor_contraction_node(udf::tensor_contraction_node * const node, int lvl) {
  ASSERT_UNSPEC;
  node->tensor1()->accept(this, lvl + 2);
  node->tensor2()->accept(this, lvl + 2);

  if (!node->tensor1()->is_typed(cdk::TYPE_TENSOR) || !node->tensor2()->is_typed(cdk::TYPE_TENSOR)) {
    throw std::string("tensor contraction requires two tensor arguments");
  }

  auto t1_type = std::dynamic_pointer_cast<cdk::tensor_type>(node->tensor1()->type());
  auto t2_type = std::dynamic_pointer_cast<cdk::tensor_type>(node->tensor2()->type());

  if (!t1_type || !t2_type) {
    throw std::string("tensor contraction requires two tensor arguments");
  }

  const auto &dims1 = t1_type->dims();
  const auto &dims2 = t2_type->dims();
  if (dims1.back() != dims2.front()) {
    throw std::string("tensor contraction requires last dimension of the first tensor to match the first dimension of the second tensor");
  }

  auto tensor = cdk::tensor_type::cast(node->tensor1()->type());
  node->type(cdk::tensor_type::create(tensor->dims()));
}

void udf::type_checker::do_tensor_dims_node(udf::tensor_dims_node * const node, int lvl) {
  ASSERT_UNSPEC;

  node->tensor()->accept(this, lvl + 2);
  if (!node->tensor()->is_typed(cdk::TYPE_TENSOR)) {
    throw std::string("tensor dimensions requires a tensor argument");
  }
  node->type(cdk::primitive_type::create(4, cdk::TYPE_POINTER));
}

void udf::type_checker::do_tensor_dim_node(udf::tensor_dim_node * const node, int lvl) {
  ASSERT_UNSPEC;
  
  node->tensor()->accept(this, lvl + 2);
  if (!node->tensor()->is_typed(cdk::TYPE_TENSOR)) {
    throw std::string("tensor dimension requires a tensor argument");
  }
  node->index()->accept(this, lvl + 2);
  if (!node->index()->is_typed(cdk::TYPE_INT)) {
    throw std::string("integer expected in tensor dimension index");
  }

  if (auto idx_int = dynamic_cast<cdk::integer_node*>(node->index())) {
    int idx = idx_int->value();
    auto tensor_type = std::dynamic_pointer_cast<cdk::tensor_type>(node->tensor()->type());
    if (tensor_type) {
      size_t rank = tensor_type->dims().size();
      if (static_cast<size_t>(idx) >= rank || static_cast<size_t>(idx) < 0) {
        throw std::string("invalid dimension");
      }
    }
  }

  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT)); 
}

void udf::type_checker::do_tensor_index_node(udf::tensor_index_node * const node, int lvl) {
  ASSERT_UNSPEC;

  node->tensor()->accept(this, lvl + 2);
  if (!node->tensor()->is_typed(cdk::TYPE_TENSOR)) {
    throw std::string("tensor index requires a tensor argument");
  }
  for (size_t i = 0; i < node->indices()->size(); i++) {
    auto child = dynamic_cast<cdk::expression_node*>(node->indices()->node(i));
    child->accept(this, lvl);
    if (!child->is_typed(cdk::TYPE_INT)) {
      throw std::string("integer expected in tensor index");
    } 
  }
  node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
}

void udf::type_checker::do_tensor_rank_node(udf::tensor_rank_node * const node, int lvl) {
  ASSERT_UNSPEC;

  node->tensor()->accept(this, lvl + 2);
  if (!node->tensor()->is_typed(cdk::TYPE_TENSOR)) {
    throw std::string("tensor rank requires a tensor argument");
  }
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT)); 
}

void udf::type_checker::do_tensor_reshape_node(udf::tensor_reshape_node * const node, int lvl) {
  ASSERT_UNSPEC;

  node->tensor()->accept(this, lvl + 2);
  if (!node->tensor()->is_typed(cdk::TYPE_TENSOR)) {
    throw std::string("tensor reshape requires a tensor argument");
  }
  auto tensor_type = std::dynamic_pointer_cast<cdk::tensor_type>(node->tensor()->type());
  if (!tensor_type) {
    throw std::string("tensor reshape requires a tensor argument");
  }
  const auto &old_dims = tensor_type->dims();
  size_t old_capacity = 1;
  for (auto d : old_dims) { old_capacity *= d;}

  std::vector<size_t> new_dims;
  for (size_t i = 0; i < node->new_dims()->size(); i++) {
    auto child = dynamic_cast<cdk::expression_node*>(node->new_dims()->node(i));
    child->accept(this, lvl);
    if (!child->is_typed(cdk::TYPE_INT)) {
      throw std::string("reshape dimensions must be positive integer literals");
    }
    if (auto dim = dynamic_cast<cdk::integer_node*>(node->new_dims()->node(i))) {
      new_dims.push_back(dim->value());
    }
  }

  size_t new_capacity = 1;
  for (auto d : new_dims) new_capacity *= d;
  
  if (old_capacity != new_capacity) {
    throw std::string("reshape dimensions must match the original tensor capacity");
  }
  
  auto tensor = cdk::tensor_type::cast(node->tensor()->type());
  node->type(cdk::tensor_type::create(tensor->dims()));
}

void udf::type_checker::do_tensor_node(udf::tensor_node *const node, int lvl) {
  ASSERT_UNSPEC
  auto cell_values = node->cell_values()->nodes();

  for (auto cell_value : cell_values) {
    auto expr = dynamic_cast<cdk::expression_node*>(cell_value);
    if (!expr) continue;
    expr->accept(this, lvl + 2);
    if (expr->type()->name() != cdk::TYPE_INT && expr->type()->name() != cdk::TYPE_DOUBLE) {
      throw std::string("tensor cell values must be integer or double expressions");
    }
  }
  node->type(cdk::tensor_type::create(node->dims()));
}

void udf::type_checker::do_address_of_node(udf::address_of_node * const node, int lvl) {
  ASSERT_UNSPEC;
  node->lvalue()->accept(this, lvl + 2);
  node->type(cdk::reference_type::create(4, node->lvalue()->type()));
}

void udf::type_checker::do_index_node(udf::index_node * const node, int lvl) {
  ASSERT_UNSPEC;
  node->base()->accept(this, lvl + 2);
  node->index()->accept(this, lvl + 2);

  if (!node->base()->is_typed(cdk::TYPE_POINTER)) {
    throw std::string("pointer expression expected in pointer indexing");
  }

  if (!node->index()->is_typed(cdk::TYPE_INT)) {
    throw std::string("integer expected in index");
  }

  auto ref = cdk::reference_type::cast(node->base()->type());
  node->type(ref->referenced());
}