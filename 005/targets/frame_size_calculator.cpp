#include <string>
#include "targets/frame_size_calculator.h"
#include "targets/type_checker.h"
#include "targets/symbol.h"
#include ".auto/all_nodes.h"

udf::frame_size_calculator::~frame_size_calculator() {
  os().flush();
}

void udf::frame_size_calculator::do_add_node(cdk::add_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_and_node(cdk::and_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_assignment_node(cdk::assignment_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_data_node(cdk::data_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_div_node(cdk::div_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_double_node(cdk::double_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_eq_node(cdk::eq_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_ge_node(cdk::ge_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_gt_node(cdk::gt_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_variable_node(cdk::variable_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_integer_node(cdk::integer_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_le_node(cdk::le_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_lt_node(cdk::lt_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_mod_node(cdk::mod_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_mul_node(cdk::mul_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_ne_node(cdk::ne_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_unary_minus_node(cdk::unary_minus_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_unary_plus_node(cdk::unary_plus_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_nil_node(cdk::nil_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_not_node(cdk::not_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_or_node(cdk::or_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_rvalue_node(cdk::rvalue_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_string_node(cdk::string_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_sub_node(cdk::sub_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_evaluation_node(udf::evaluation_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_write_node(udf::write_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_input_node(udf::input_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_address_of_node(udf::address_of_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_function_call_node(udf::function_call_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_function_declaration_node(udf::function_declaration_node *const node, int lvl) {
  _localsize += node->type()->size();
}
void udf::frame_size_calculator::do_index_node(udf::index_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_continue_node(udf::continue_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_nullptr_node(udf::nullptr_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_return_node(udf::return_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_stack_alloc_node(udf::stack_alloc_node *const node, int lvl) {
  // EMPTY
}
void udf::frame_size_calculator::do_break_node(udf::break_node *const node, int lvl) {
  // EMPTY
}

void udf::frame_size_calculator::do_sequence_node(cdk::sequence_node *const node, int lvl) {
  for (size_t i = 0; i < node->size(); i++) {
    cdk::basic_node *n = node->node(i);
    if (n == nullptr) break;
    n->accept(this, lvl + 2);
  }
}

void udf::frame_size_calculator::do_block_node(udf::block_node *const node, int lvl) {
  if (node->declarations()) node->declarations()->accept(this, lvl + 2);
  if (node->instructions()) node->instructions()->accept(this, lvl + 2);
}

void udf::frame_size_calculator::do_for_node(udf::for_node *const node, int lvl) {
  _symtab.push();
  node->init()->accept(this, lvl + 2);
  _symtab.pop();
  node->instruction()->accept(this, lvl + 2);
}

void udf::frame_size_calculator::do_if_node(udf::if_node *const node, int lvl) {
  node->block()->accept(this, lvl + 2);
}

void udf::frame_size_calculator::do_if_else_node(udf::if_else_node *const node, int lvl) {
  node->thenblock()->accept(this, lvl + 2);
  if (node->elseblock()) node->elseblock()->accept(this, lvl + 2);
}

void udf::frame_size_calculator::do_variable_declaration_node(udf::variable_declaration_node *const node, int lvl) {
  //_symtab.push();
  ASSERT_SAFE_EXPRESSIONS;  
  //_symtab.pop();
  _localsize += node->type()->size();
}

void udf::frame_size_calculator::do_function_definition_node(udf::function_definition_node *const node, int lvl) {
  node->block()->accept(this, lvl + 2);
}

void udf::frame_size_calculator::do_sizeof_node(udf::sizeof_node *const node, int lvl) {
  // EMPTY
}

void udf::frame_size_calculator::do_tensor_capacity_node(udf::tensor_capacity_node *const node, int lvl) {
  // EMPTY
}

void udf::frame_size_calculator::do_tensor_contraction_node(udf::tensor_contraction_node *const node, int lvl) {
  // EMPTY
}

void udf::frame_size_calculator::do_tensor_dims_node(udf::tensor_dims_node *const node, int lvl) {
  // EMPTY
}

void udf::frame_size_calculator::do_tensor_dim_node(udf::tensor_dim_node *const node, int lvl) {
  // EMPTY
}

void udf::frame_size_calculator::do_tensor_index_node(udf::tensor_index_node *const node, int lvl) {
  // EMPTY
}

void udf::frame_size_calculator::do_tensor_rank_node(udf::tensor_rank_node *const node, int lvl) {
  // EMPTY
}

void udf::frame_size_calculator::do_tensor_reshape_node(udf::tensor_reshape_node *const node, int lvl) {
  // EMPTY
}

void udf::frame_size_calculator::do_tensor_node(udf::tensor_node *const node, int lvl) {
  // EMPTY
}

