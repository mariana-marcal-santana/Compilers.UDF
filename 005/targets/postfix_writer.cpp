#include <string>
#include <sstream>
#include "targets/type_checker.h"
#include "targets/postfix_writer.h"
#include ".auto/all_nodes.h"  // all_nodes.h is automatically generated
#include "targets/frame_size_calculator.h"
#include "targets/symbol.h"

#include "udf_parser.tab.h"

//---------------------------------------------------------------------------

void udf::postfix_writer::do_nil_node(cdk::nil_node * const node, int lvl) {
  // EMPTY
}
void udf::postfix_writer::do_data_node(cdk::data_node * const node, int lvl) {
  // EMPTY
}
void udf::postfix_writer::do_double_node(cdk::double_node * const node, int lvl) {
  if (_inFunctionBody)
    _pf.DOUBLE(node->value());
  else
    _pf.SDOUBLE(node->value());
}
void udf::postfix_writer::do_not_node(cdk::not_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl + 2); // the value we want to compare
  _pf.INT(0);                              // we want to compare it to false
  _pf.EQ(); // checks whether the last two values on the stack are equal
}
void udf::postfix_writer::do_and_node(cdk::and_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  const auto lbl = mklbl(++_lbl);
  node->left()->accept(this, lvl + 2);
  _pf.DUP32();
  _pf.JZ(lbl);
  node->right()->accept(this, lvl + 2);
  _pf.AND();
  _pf.ALIGN();
  _pf.LABEL(lbl);
}
void udf::postfix_writer::do_or_node(cdk::or_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  const auto lbl = mklbl(++_lbl);
  node->left()->accept(this, lvl + 2);
  _pf.DUP32();
  _pf.JNZ(lbl);
  node->right()->accept(this, lvl + 2);
  _pf.OR();
  _pf.ALIGN();
  _pf.LABEL(lbl);
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_sequence_node(cdk::sequence_node * const node, int lvl) {
  for (size_t i = 0; i < node->size(); i++) {
    node->node(i)->accept(this, lvl);
  }
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_integer_node(cdk::integer_node * const node, int lvl) {
  if (_inFunctionBody)
    _pf.INT(node->value());
  else
    _pf.SINT(node->value()); // declares a static integer value
}

void udf::postfix_writer::do_string_node(cdk::string_node * const node, int lvl) {
  
  const auto lbl = mklbl(++_lbl);

  _pf.RODATA();               
  _pf.ALIGN();               
  _pf.LABEL(lbl);           
  _pf.SSTRING(node->value());

  if (_inFunctionBody) { // local string
    _pf.TEXT();
    _pf.ADDR(lbl);
  } else { // global string
    _pf.DATA();
    _pf.SADDR(lbl);
  }
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_unary_minus_node(cdk::unary_minus_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  node->argument()->accept(this, lvl);
  if (node->argument()->is_typed(cdk::TYPE_INT))
    _pf.NEG();
  else if (node->argument()->is_typed(cdk::TYPE_DOUBLE))
    _pf.DNEG();
  else if (node->argument()->is_typed(cdk::TYPE_TENSOR)) {
    _pf.TRASH(4);
    _pf.DOUBLE(-1);
    node->argument()->accept(this, lvl);
    _functions_to_declare.insert("tensor_mul_scalar");
    _pf.CALL("tensor_mul_scalar");
    _pf.TRASH(12);
    _pf.LDFVAL32();
  }
}

void udf::postfix_writer::do_unary_plus_node(cdk::unary_plus_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl);
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_add_node(cdk::add_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  if(!node->is_typed(cdk::TYPE_TENSOR)) {
    node->left()->accept(this, lvl + 2);

    if (node->is_typed(cdk::TYPE_DOUBLE) && node->left()->is_typed(cdk::TYPE_INT))
      _pf.I2D();
    else if (node->is_typed(cdk::TYPE_POINTER) && !node->left()->is_typed(cdk::TYPE_POINTER)) {
      _pf.INT(std::max(1, static_cast<int>(cdk::reference_type::cast(node->right()->type())->referenced()->size())));
      _pf.MUL();
    }

    node->right()->accept(this, lvl + 2);

    if (node->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_INT))
      _pf.I2D();
    else if (node->is_typed(cdk::TYPE_POINTER) && !node->right()->is_typed(cdk::TYPE_POINTER)) {
      _pf.INT(std::max(1, static_cast<int>(cdk::reference_type::cast(node->left()->type())->referenced()->size())));
      _pf.MUL();
    }
  }

  if (node->type()->name() == cdk::TYPE_DOUBLE)
    _pf.DADD();
  else if (node->type()->name() != cdk::TYPE_TENSOR)
    _pf.ADD();
  else {
    if (node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_TENSOR)) {
      node->right()->accept(this, lvl);
      node->left()->accept(this, lvl);
      _functions_to_declare.insert("tensor_add");
      _pf.CALL("tensor_add");
      _pf.TRASH(8);
      _pf.LDFVAL32();
    }
    else {
      if (node->left()->is_typed(cdk::TYPE_TENSOR) && 
        (node->right()->is_typed(cdk::TYPE_INT) || node->right()->is_typed(cdk::TYPE_DOUBLE))) {
        node->right()->accept(this, lvl);
        if (node->right()->is_typed(cdk::TYPE_INT))
          _pf.I2D();
        node->left()->accept(this, lvl);
      }
      if (node->right()->is_typed(cdk::TYPE_TENSOR) && 
        (node->left()->is_typed(cdk::TYPE_INT) || node->left()->is_typed(cdk::TYPE_DOUBLE))) {
        node->left()->accept(this, lvl);
        if (node->left()->is_typed(cdk::TYPE_INT))
          _pf.I2D();
        node->right()->accept(this, lvl);
      }
      _functions_to_declare.insert("tensor_add_scalar");
      _pf.CALL("tensor_add_scalar");
      _pf.TRASH(12);
      _pf.LDFVAL32();
    }
  }
}

void udf::postfix_writer::do_sub_node(cdk::sub_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  node->left()->accept(this, lvl);
  node->right()->accept(this, lvl);

  if (!node->is_typed(cdk::TYPE_DOUBLE) && !node->is_typed(cdk::TYPE_TENSOR)) {
    _pf.SUB();

    if ((node->left()->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_POINTER)) &&
        cdk::reference_type::cast(node->left()->type())->referenced()->name() != cdk::TYPE_VOID) {

      _pf.INT(cdk::reference_type::cast(node->left()->type())->referenced()->size());
      _pf.DIV();
    }
  } 
  else if (node->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.DSUB();
  }
  else {
    if (node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_TENSOR)) {
      _functions_to_declare.insert("tensor_sub");
      _pf.CALL("tensor_sub");
      _pf.TRASH(8);
      _pf.LDFVAL32();
    }
    else if (node->right()->is_typed(cdk::TYPE_TENSOR)) {
      //Let's do it the other way around -TENSOR + DOUBLE ("unary minus" the tensor and then add the double)
      _pf.TRASH(8); //delete the right that we just accepted
      _pf.DOUBLE(-1);
      node->right()->accept(this, lvl);
      _functions_to_declare.insert("tensor_mul_scalar");
      _pf.CALL("tensor_mul_scalar");
      _pf.TRASH(12);
      node->left()->accept(this, lvl);
      if (node->left()->is_typed(cdk::TYPE_INT))
        _pf.I2D();
      _pf.LDFVAL32();
      _functions_to_declare.insert("tensor_add_scalar");
      _pf.CALL("tensor_add_scalar");
      _pf.TRASH(12);
      _pf.LDFVAL32();
    }
    else if (node->left()->is_typed(cdk::TYPE_TENSOR)) {
      node->right()->accept(this, lvl);
      if (node->right()->is_typed(cdk::TYPE_INT))
        _pf.I2D();
      node->left()->accept(this, lvl);
      _functions_to_declare.insert("tensor_sub_scalar");
      _pf.CALL("tensor_sub_scalar");
      _pf.TRASH(12);
      _pf.LDFVAL32();
    }
  }
}
void udf::postfix_writer::do_mul_node(cdk::mul_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  if (node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_TENSOR)){
    node->right()->accept(this, lvl + 2);
    node->left()->accept(this, lvl + 2);
  } 
  else if ((node->right()->is_typed(cdk::TYPE_DOUBLE) || node->right()->is_typed(cdk::TYPE_INT))
      && node->left()->is_typed(cdk::TYPE_TENSOR)) {
    node->right()->accept(this, lvl + 2);
    if (node->right()->type()->name() == cdk::TYPE_INT) 
      _pf.I2D();
    node->left()->accept(this, lvl + 2);
  } 
  else {
    node->left()->accept(this, lvl + 2);
    if ((node->left()->type()->name() == cdk::TYPE_DOUBLE || node->left()->type()->name() == cdk::TYPE_INT) &&
      (node->type()->name() == cdk::TYPE_TENSOR)) 
      _pf.I2D();
    if ((node->type()->name() == cdk::TYPE_DOUBLE && node->left()->type()->name() == cdk::TYPE_INT)) 
      _pf.I2D();

    node->right()->accept(this, lvl + 2);
    if (node->type()->name() == cdk::TYPE_DOUBLE && node->right()->type()->name() == cdk::TYPE_INT) 
      _pf.I2D();
  }

  if (node->type()->name() == cdk::TYPE_DOUBLE) {
    _pf.DMUL();
  } 
  else if(node->type()->name() == cdk::TYPE_INT) {
    _pf.MUL();
  }
  else {
    if(node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_TENSOR)){
        _functions_to_declare.insert("tensor_mul"); 
        _pf.CALL("tensor_mul");
        _pf.TRASH(8);
        _pf.LDFVAL32();
    } else {
        _functions_to_declare.insert("tensor_mul_scalar"); 
        _pf.CALL("tensor_mul_scalar");
        _pf.TRASH(12);
        _pf.LDFVAL32();
    }
  }
}
void udf::postfix_writer::do_div_node(cdk::div_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  if (!node->is_typed(cdk::TYPE_TENSOR)) {
    node->left()->accept(this, lvl);
    node->right()->accept(this, lvl);

    if (node->is_typed(cdk::TYPE_DOUBLE))
      _pf.DDIV();
    else if (!node->is_typed(cdk::TYPE_TENSOR))
      _pf.DIV();
  }
  else {
    if (node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_TENSOR)) {
      node->left()->accept(this, lvl);
      node->right()->accept(this, lvl);
      _functions_to_declare.insert("tensor_div");
      _pf.CALL("tensor_div");
      _pf.TRASH(8);
      _pf.LDFVAL32();
    }
    else if (node->left()->is_typed(cdk::TYPE_TENSOR)) {
      node->right()->accept(this, lvl);
      if (node->right()->is_typed(cdk::TYPE_INT))
        _pf.I2D();
      node->left()->accept(this, lvl);
      _functions_to_declare.insert("tensor_div_scalar");
      _pf.CALL("tensor_div_scalar");
      _pf.TRASH(12);
      _pf.LDFVAL32();
    }
    else if (node->right()->is_typed(cdk::TYPE_TENSOR)) {
      node->left()->accept(this, lvl);
      if (node->left()->is_typed(cdk::TYPE_INT)) 
        _pf.I2D();

      node->right()->accept(this, lvl);
      _functions_to_declare.insert("mem_init");
      _pf.CALL("mem_init");
      _functions_to_declare.insert("tensor_ones");
      size_t n_dims = cdk::tensor_type::cast(node->right()->type())->n_dims();

      for (int i = (int)n_dims - 1; i >= 0; --i) {
        _pf.INT(static_cast<int>(cdk::tensor_type::cast(node->right()->type())->dim(i)));
      }
      _pf.INT(static_cast<int>(n_dims));
      _pf.CALL("tensor_ones");   // create a tensor full of 1's
      _pf.TRASH(n_dims * 4 + 4);                // pop the 1 and n
      _pf.LDFVAL32();
      _functions_to_declare.insert("tensor_div");
      _pf.CALL("tensor_div"); 
      _pf.TRASH(8);
      _pf.LDFVAL32(); 

      _functions_to_declare.insert("tensor_mul_scalar");
      _pf.CALL("tensor_mul_scalar");
      _pf.TRASH(12);
      _pf.LDFVAL32();
    }
  }
}

void udf::postfix_writer::do_mod_node(cdk::mod_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  node->right()->accept(this, lvl);
  _pf.MOD();
}

void udf::postfix_writer::processCompares(cdk::binary_operation_node *const node, int lvl) {

  node->left()->accept(this, lvl);
  if (node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_INT)) {
    _pf.I2D();
  }
  node->right()->accept(this, lvl);
  if (node->right()->is_typed(cdk::TYPE_DOUBLE) && node->left()->is_typed(cdk::TYPE_INT)) {
    _pf.I2D();
  }

  if (node->left()->is_typed(cdk::TYPE_DOUBLE) || node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.DCMP();
    _pf.INT(0);
  }
}

void udf::postfix_writer::do_lt_node(cdk::lt_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  processCompares(node, lvl);
  _pf.LT();
}
void udf::postfix_writer::do_le_node(cdk::le_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  processCompares(node, lvl);
  _pf.LE();
}
void udf::postfix_writer::do_ge_node(cdk::ge_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  processCompares(node, lvl);
  _pf.GE();
}
void udf::postfix_writer::do_gt_node(cdk::gt_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  processCompares(node, lvl);
  _pf.GT();
}
void udf::postfix_writer::do_ne_node(cdk::ne_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  node->left()->accept(this, lvl);
  node->right()->accept(this, lvl);

  if (node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_TENSOR)) {
    auto t1_type = std::dynamic_pointer_cast<cdk::tensor_type>(node->left()->type());
    auto t2_type = std::dynamic_pointer_cast<cdk::tensor_type>(node->right()->type());
    const std::vector<size_t>& t1_dims = t1_type->dims();
    const std::vector<size_t>& t2_dims = t2_type->dims();
    if (t1_dims != t2_dims) {
      _pf.TRASH(8); // trash the two tensors
      _pf.INT(1);   // if the dimensions are not equal, return true    
    } 
    else {
      _functions_to_declare.insert("tensor_equals");
    _pf.CALL("tensor_equals");
    _pf.TRASH(8);
    _pf.LDFVAL32();
    _pf.INT(1);
    _pf.NE();
    }  
  }
  else
    _pf.NE();
}
void udf::postfix_writer::do_eq_node(cdk::eq_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  
  node->right()->accept(this, lvl);
  node->left()->accept(this, lvl);

  if (node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_TENSOR)) {
    auto t1_type = std::dynamic_pointer_cast<cdk::tensor_type>(node->left()->type());
    auto t2_type = std::dynamic_pointer_cast<cdk::tensor_type>(node->right()->type());
    const std::vector<size_t>& t1_dims = t1_type->dims();
    const std::vector<size_t>& t2_dims = t2_type->dims();
    if (t1_dims != t2_dims) {
      _pf.TRASH(8); // trash the two tensors
      _pf.INT(0);   // if the dimensions are not equal, return false    
    } 
    else {
      _functions_to_declare.insert("tensor_equals");
      _pf.CALL("tensor_equals");
      _pf.TRASH(8);
      _pf.LDFVAL32();
      _pf.INT(1); // we want to check if they are equal, so we compare with 1
      _pf.EQ(); // checks whether the last two values on the stack are equal
    }
  }
  else {
    _pf.EQ();
  }
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_variable_node(cdk::variable_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  const std::string &id = node->name();
  auto symbol = _symtab.find(id);
  if (symbol->global()) {
    _pf.ADDR(symbol->name());
  } else {
    _pf.LOCAL(symbol->offset());
  }
}

void udf::postfix_writer::do_rvalue_node(cdk::rvalue_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->lvalue()->accept(this, lvl);
  if (node->is_typed(cdk::TYPE_DOUBLE))
    _pf.LDDOUBLE();
  else 
    _pf.LDINT();
}

void udf::postfix_writer::do_assignment_node(cdk::assignment_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->rvalue()->accept(this, lvl); // determine the new value

  if (!node->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.DUP32();
  } else {
    if (node->rvalue()->is_typed(cdk::TYPE_INT))
      _pf.I2D();
    _pf.DUP64();
  }

  node->lvalue()->accept(this, lvl + 2);
  if (!node->is_typed(cdk::TYPE_DOUBLE))
    _pf.STINT();
  else
    _pf.STDOUBLE();
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_evaluation_node(udf::evaluation_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  node->argument()->accept(this, lvl); // determine the value
  if (node->argument()->is_typed(cdk::TYPE_VOID)) {
  } else 
    _pf.TRASH(node->argument()->type()->size()); // delete the evaluated value
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_if_node(udf::if_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  int lbl1;
  node->condition()->accept(this, lvl);
  _pf.JZ(mklbl(lbl1 = ++_lbl));
  node->block()->accept(this, lvl + 2);
  _pf.LABEL(mklbl(lbl1));
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_if_else_node(udf::if_else_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  int lbl1, lbl2;
  node->condition()->accept(this, lvl);
  _pf.JZ(mklbl(lbl1 = ++_lbl));
  node->thenblock()->accept(this, lvl + 2);
  _pf.JMP(mklbl(lbl2 = ++_lbl));
  _pf.LABEL(mklbl(lbl1));
  node->elseblock()->accept(this, lvl + 2);
  _pf.LABEL(mklbl(lbl1 = lbl2));
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_break_node(udf::break_node * const node, int lvl) {
  //
  if (_forIni.size() != 0) {
    _pf.JMP(mklbl(_forEnd.top())); // jump to for end
  } else
    error(node->lineno(), "'break' outside 'for'");
}

void udf::postfix_writer::do_continue_node(udf::continue_node * const node, int lvl) {
  if (_forIni.size() != 0) {
    _pf.JMP(mklbl(_forStep.top())); // jump to next cycle
  } else
    error(node->lineno(), "'continue' outside 'for'");
}

void udf::postfix_writer::do_for_node(udf::for_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  _forIni.push(++_lbl);
  _forStep.push(++_lbl);
  _forEnd.push(++_lbl);

  _symtab.push();

  node->init()->accept(this, lvl + 2);

  _pf.ALIGN();
  _pf.LABEL(mklbl(_forIni.top()));
  
  node->condition()->accept(this, lvl + 2);
  _pf.JZ(mklbl(_forEnd.top()));

  node->instruction()->accept(this, lvl + 2);

  _pf.ALIGN();
  _pf.LABEL(mklbl(_forStep.top()));
  node->increment()->accept(this, lvl + 2);
  
  _pf.JMP(mklbl(_forIni.top()));

  _pf.ALIGN();
  _pf.LABEL(mklbl(_forEnd.top()));

  _symtab.pop();

  _forIni.pop();
  _forStep.pop();
  _forEnd.pop();
}

void udf::postfix_writer::do_input_node(udf::input_node * const node, int lvl) {

  ASSERT_SAFE_EXPRESSIONS;

  if (node->is_typed(cdk::TYPE_DOUBLE)) {
    _functions_to_declare.insert("readd");
    _pf.CALL("readd");
    _pf.LDFVAL64();
  } else if (node->is_typed(cdk::TYPE_INT)) {
    _functions_to_declare.insert("readi");
    _pf.CALL("readi");
    _pf.LDFVAL32();
  } else {
    std::cerr << "FATAL: " << node->lineno() << ": cannot read type" << std::endl;
    return;
  }
}

void udf::postfix_writer::do_nullptr_node(udf::nullptr_node *const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  if (_inFunctionBody)
    _pf.INT(0);
  else
    _pf.SINT(0);
}

void udf::postfix_writer::do_stack_alloc_node(udf::stack_alloc_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl);

  _pf.INT(cdk::reference_type::cast(node->type())->referenced()->size());
  _pf.MUL();
  _pf.ALLOC();
  _pf.SP();
}

void udf::postfix_writer::do_return_node(udf::return_node * const node, int lvl) {

  ASSERT_SAFE_EXPRESSIONS;

  // should not reach here without returning a value (if not void)
  if (_function->type()->name() != cdk::TYPE_VOID) {
    node->retval()->accept(this, lvl + 2);

    if (_function->type()->name() == cdk::TYPE_INT || _function->type()->name() == cdk::TYPE_STRING || _function->type()->name() == cdk::TYPE_POINTER) {
      _pf.STFVAL32();
    } 
    else if (_function->type()->name() == cdk::TYPE_DOUBLE) {
      if (node->retval()->type()->name() == cdk::TYPE_INT) 
        _pf.I2D();
      _pf.STFVAL64();
    } 
    else {
      std::cerr << node->lineno() << ": should not happen: unknown return type" << std::endl;
    }
  }

  //_returnSeen = true;
  _pf.LEAVE();
  _pf.RET();
}

void udf::postfix_writer::do_sizeof_node(udf::sizeof_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  if (node->expression()->is_typed(cdk::TYPE_TENSOR)) {
    node->expression()->accept(this, lvl + 2);
    _functions_to_declare.insert("tensor_size");
    _pf.CALL("tensor_size");
    _pf.TRASH(4);
    _pf.LDFVAL32();
    _pf.INT(8);
    _pf.MUL();
  } else
    _pf.INT(node->expression()->type()->size());
}

void udf::postfix_writer::do_write_node(udf::write_node * const node, int lvl) {

  ASSERT_SAFE_EXPRESSIONS;

  for (size_t ix = 0; ix < node->args()->size(); ix++) {
    auto child = dynamic_cast<cdk::expression_node*>(node->args()->node(ix));

    child->accept(this, lvl); // expression to print
    
    if (child->is_typed(cdk::TYPE_INT)) {
      _functions_to_declare.insert("printi");
      _pf.CALL("printi");
      _pf.TRASH(4); // trash int
    } 
    else if (child->is_typed(cdk::TYPE_DOUBLE)) {
      _functions_to_declare.insert("printd");
      _pf.CALL("printd");
      _pf.TRASH(8); // trash double
    } 
    else if (child->is_typed(cdk::TYPE_STRING)) {
      _functions_to_declare.insert("prints");
      _pf.CALL("prints");
      _pf.TRASH(4); // trash char pointer
    }
    else if (child->is_typed(cdk::TYPE_TENSOR)) {
      _functions_to_declare.insert("tensor_print");
      _pf.CALL("tensor_print");
      _pf.TRASH(4); // trash tensor pointer
    }
    else {
      std::cerr << "cannot print expression of unknown type" << std::endl;
      return;
    }
  }

  if (node->newline()) {
    _functions_to_declare.insert("println");
    _pf.CALL("println");
  }
}

void udf::postfix_writer::do_function_declaration_node(udf::function_declaration_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  if (_inFunctionBody || _inFunctionArgs) {
    error(node->lineno(), "cannot declare function in body or in arguments");
    return;
  }

  if (!new_symbol()) return;

  auto function = new_symbol();
  _functions_to_declare.insert(function->name());
  reset_new_symbol();
}

void udf::postfix_writer::do_variable_declaration_node(udf::variable_declaration_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS; 

  auto id = node->identifier();
  int offset = 0, typesize = node->type()->size();
  
  if(_inFunctionBody) {
    // local variables
    _offset -= typesize; // reservar espaço para as vars locais
    offset = _offset; // guardar o offset
  }
  else if (_inFunctionArgs) {
    // arg de uma função
    offset = _offset; // guardar o offset
    _offset += typesize; // reservar espaço para os args
  }
  else {
    offset = 0;  // global var
  }
 
  auto symbol = new_symbol();  // o typechecker ja deu insert na symtab
  if (symbol) {
    symbol->set_offset(offset);  // def acima (para saber se é global/local/args...)
    reset_new_symbol(); 
  }

  if(_inFunctionBody) {
    // if local var, no action needed unless an initialier exists
    if (node->initializer()) {
      node->initializer()->accept(this, lvl); // sacar type do initializer

      if (symbol->is_typed(cdk::TYPE_INT) || symbol->is_typed(cdk::TYPE_STRING) || 
        symbol->is_typed(cdk::TYPE_POINTER) || symbol->is_typed(cdk::TYPE_TENSOR)) {
        _pf.LOCAL(symbol->offset());
        _pf.STINT();
      } 
      else if (symbol->is_typed(cdk::TYPE_DOUBLE)) {

        if (node->initializer()->is_typed(cdk::TYPE_INT)) // ex: double x = 5; node->double e initializer->5(int)
          _pf.I2D();

        _pf.LOCAL(symbol->offset());
        _pf.STDOUBLE();
      } 
      else {
        std::cerr << "cannot initialize" << std::endl;
      }
    }
    else {
      if(node->is_typed(cdk::TYPE_TENSOR)){
        auto tensor = cdk::tensor_type::cast(node->type());
        _pf.TEXT(); // function calls should be in text

        for (size_t i = tensor->n_dims(); i-- > 0; ) {
          _pf.INT(tensor->dims()[i]);
        }
        _pf.INT(tensor->n_dims());
        _functions_to_declare.insert("tensor_create"); 
        _pf.CALL("tensor_create");

        // trash the arguments
        _pf.TRASH(4 + tensor->n_dims()*4);
        _pf.LDFVAL32(); // puts the return value in the stack

        _pf.LOCAL(symbol->offset());
        _pf.STINT();
      }
    }
  } 
  else {
    // gloval vars neste else
    if (!_function) {
      if (node->initializer() == nullptr) {  // sem valor inicial
        _pf.BSS();         // data segment for uninitialized values
        _pf.ALIGN();
        _pf.LABEL(id);
        _pf.SALLOC(typesize); // declares an uninitialized vector with length size (in bytes)
      } else { 
        // com valor inicial
        if (node->is_typed(cdk::TYPE_INT) || node->is_typed(cdk::TYPE_DOUBLE) || node->is_typed(cdk::TYPE_POINTER)) {

          _pf.DATA();
          _pf.ALIGN();
          _pf.LABEL(id);  

          if (node->is_typed(cdk::TYPE_INT)) {
            node->initializer()->accept(this, lvl);
          } else if (node->is_typed(cdk::TYPE_POINTER)) {
            node->initializer()->accept(this, lvl);
          } else if (node->is_typed(cdk::TYPE_DOUBLE)) {
            if (node->initializer()->is_typed(cdk::TYPE_DOUBLE)) {
              node->initializer()->accept(this, lvl);
            } else if (node->initializer()->is_typed(cdk::TYPE_INT)) {
              cdk::integer_node * dclini = dynamic_cast<cdk::integer_node*>(node->initializer());
              cdk::double_node ddi(dclini->lineno(), dclini->value());
              ddi.accept(this, lvl);
            } else {
              std::cerr << node->lineno() << ": '" << id << "' has bad initializer for real value\n";
            }
          }
        } 
        else if (node->is_typed(cdk::TYPE_STRING)) {
          _pf.DATA();
          _pf.ALIGN();
          _pf.LABEL(id);
          node->initializer()->accept(this, lvl);
        } 
        else {
          std::cerr << node->lineno() << ": '" << id << "' has unexpected initializer\n";
        }
      }
    }
  }
}

void udf::postfix_writer::do_block_node(udf::block_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  _symtab.push();
  if (node->declarations())
    node->declarations()->accept(this, lvl + 2);
  if (node->instructions())
    node->instructions()->accept(this, lvl + 2);
  _symtab.pop();
}

void udf::postfix_writer::do_function_definition_node(udf::function_definition_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  if (_inFunctionBody || _inFunctionArgs) {
    error(node->lineno(), "cannot define function in body or in arguments");
    return;
  }

  // remember symbol so that args and body know
  _function = new_symbol();
  _functions_to_declare.erase(_function->name());  // just in case
  reset_new_symbol();

  _offset = 8; // prepare for arguments (4: remember to account for return address)
  _symtab.push(); // scope of args

  if (node->arguments()->size() > 0) {
    _inFunctionArgs = true; //FIXME really needed?
    for (size_t ix = 0; ix < node->arguments()->size(); ix++) {
      cdk::basic_node *arg = node->arguments()->node(ix);
      if (arg == nullptr) break; // this means an empty sequence of arguments
      arg->accept(this, 0); // the function symbol is at the top of the stack
    }
    _inFunctionArgs = false; //FIXME really needed?
  }

  _pf.TEXT();
  _pf.ALIGN();

  if (node->qualifier() == tPUBLIC) _pf.GLOBAL(_function->name(), _pf.FUNC());

  _pf.LABEL(_function->name());

  // compute stack size to be reserved for local variables
  frame_size_calculator lsc(_compiler, _symtab, _function);
  node->accept(&lsc, lvl);
  _pf.ENTER(lsc.localsize()); // total stack size reserved for local variables

  if(!_memInitialized && node->identifier() == "udf") {
    _functions_to_declare.insert("mem_init"); 
    _pf.CALL("mem_init");
    _memInitialized = true;
  }

  _offset = 0; // prepare for local variable

  //_returnSeen = false;
  _inFunctionBody = true;
  os() << "        ;; before body " << std::endl;
  node->block()->accept(this, lvl + 4); // block has its own scope
  os() << "        ;; after body " << std::endl;
  _inFunctionBody = false;

  // if (!_returnSeen) {
    _pf.LEAVE();
    _pf.RET();
  // }
  
  _symtab.pop(); // scope of arguments

  if (node->identifier() == "udf") {
    // declare external functions
    for (std::string s : _functions_to_declare)
      _pf.EXTERN(s);
  }
}


void udf::postfix_writer::do_function_call_node(udf::function_call_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  auto symbol = _symtab.find(node->identifier());

  size_t argsSize = 0;
  if (node->arguments()->size() > 0) {
    for (int ax = node->arguments()->size() - 1; ax >= 0; ax--) {
      cdk::expression_node *arg = dynamic_cast<cdk::expression_node*>(node->arguments()->node(ax));
      arg->accept(this, lvl + 2);
      if (symbol->argument_is_typed(ax, cdk::TYPE_DOUBLE) && arg->is_typed(cdk::TYPE_INT)) {
        _pf.I2D();
      }
      argsSize += symbol->argument_size(ax);
    }
  }
  _pf.CALL(node->identifier());
  if (argsSize != 0) {
    _pf.TRASH(argsSize);
  }

  if (symbol->is_typed(cdk::TYPE_INT) || symbol->is_typed(cdk::TYPE_POINTER) || symbol->is_typed(cdk::TYPE_STRING)) {
    _pf.LDFVAL32();
  } else if (symbol->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.LDFVAL64();
  } else if (symbol->is_typed(cdk::TYPE_VOID)) {
  } else  
    std::cerr << "FATAL: " << node->lineno() << ": unknown function return type" << std::endl;
}

void udf::postfix_writer::do_tensor_capacity_node(udf::tensor_capacity_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->tensor()->accept(this, lvl + 2);
  _functions_to_declare.insert("tensor_size");
  _pf.CALL("tensor_size");
  _pf.TRASH(4); // limpar pilha
  _pf.LDFVAL32(); // põe o ponteiro do inteiro resultante na pilha
}

void udf::postfix_writer::do_tensor_contraction_node(udf::tensor_contraction_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->tensor2()->accept(this, lvl + 2);
  node->tensor1()->accept(this, lvl + 2);

  _functions_to_declare.insert("tensor_matmul");
  _pf.CALL("tensor_matmul");
  _pf.TRASH(8); // limpar pilha, 4 bytes para cada tensor
  _pf.LDFVAL32(); // põe o ponteiro do tensor resultante na pilha
}

void udf::postfix_writer::do_tensor_dims_node(udf::tensor_dims_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  node->tensor()->accept(this, lvl + 2);
  _functions_to_declare.insert("tensor_get_dims");
  _pf.CALL("tensor_get_dims");
  _pf.TRASH(4);
  _pf.LDFVAL32();
}

void udf::postfix_writer::do_tensor_dim_node(udf::tensor_dim_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  node->index()->accept(this, lvl + 2); // aceitar o índice
  node->tensor()->accept(this, lvl + 2);
  _functions_to_declare.insert("tensor_get_dim_size");
  _pf.CALL("tensor_get_dim_size");
  _pf.TRASH(8);
  _pf.LDFVAL32();
}

void udf::postfix_writer::do_tensor_index_node(udf::tensor_index_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  for (size_t i = 0; i < node->indices()->size(); i++) {
    node->indices()->node(i)->accept(this, lvl + 2); // aceitar cada índice
  }
  node->tensor()->accept(this, lvl + 2);
  
  _functions_to_declare.insert("tensor_getptr");
  _pf.CALL("tensor_getptr");
  _pf.TRASH(node->indices()->size() * 4 + 4);
  _pf.LDFVAL32();
}

void udf::postfix_writer::do_tensor_rank_node(udf::tensor_rank_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->tensor()->accept(this, lvl + 2);
  _functions_to_declare.insert("tensor_get_n_dims");
  _pf.CALL("tensor_get_n_dims");
  _pf.TRASH(4); // limpar pilha
  _pf.LDFVAL32(); // põe o ponteiro do inteiro resultante na pilha
}

void udf::postfix_writer::do_tensor_reshape_node(udf::tensor_reshape_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  
  for (ssize_t i = node->new_dims()->size() - 1; i >= 0; i--) {
    node->new_dims()->node(i)->accept(this, lvl + 2); // aceitar cada dimensão
  }
  
  _pf.INT(node->new_dims()->size()); // número de dimensões
  node->tensor()->accept(this, lvl + 2);

  _functions_to_declare.insert("tensor_reshape");
  _pf.CALL("tensor_reshape");
  _pf.TRASH(node->new_dims()->size() * 4 + 4); // limpar pilha
  _pf.LDFVAL32(); // põe o ponteiro do tensor reformatado na pilha
}

void udf::postfix_writer::do_tensor_node(udf::tensor_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  _pf.TEXT();
  _functions_to_declare.insert("mem_init");
  _pf.CALL("mem_init");
  // por argumentos de tensor_create na pilha
  for (ssize_t i = node->dims().size() - 1; i >= 0; i--) {
    _pf.INT(node->dims()[i]);
  }
  _pf.INT(node->dims().size());

  // chama a função de criação de tensor
  _functions_to_declare.insert("tensor_create");
  _pf.CALL("tensor_create");
  _pf.TRASH(node->dims().size() * 4 + 4);
  _pf.LDFVAL32(); // põe o ponteiro do tensor criado na pilha

  for (size_t i = 0; i < node->cell_values()->nodes().size(); i++) {
    _pf.INT(i);

    auto expr = dynamic_cast<cdk::expression_node*>(node->cell_values()->node(i));
    expr->accept(this, lvl + 2);
    if (expr->is_typed(cdk::TYPE_INT)) {
      _pf.I2D();
    }

    _functions_to_declare.insert("tensor_put");
    _pf.CALL("tensor_put");
    _pf.TRASH(12);
  } 
}

void udf::postfix_writer::do_address_of_node(udf::address_of_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->lvalue()->accept(this, lvl + 2);
}

void udf::postfix_writer::do_index_node(udf::index_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->base()->accept(this, lvl);
  node->index()->accept(this, lvl);

  _pf.INT(node->type()->size());
  _pf.MUL();   
  _pf.ADD(); 
}