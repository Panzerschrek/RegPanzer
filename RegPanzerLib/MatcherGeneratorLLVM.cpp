#include "MatcherGeneratorLLVM.hpp"
#include "PushDisableLLVMWarnings.hpp"
#include <llvm/IR/IRBuilder.h>
#include "PopLLVMWarnings.hpp"

namespace RegPanzer
{

namespace
{

const char* GetNodeName(const GraphElements::NodePtr& node);

const char* GetNodeName(const GraphElements::AnySymbol&) { return "any_symbol"; }
const char* GetNodeName(const GraphElements::SpecificSymbol&) { return "specific_symbol"; }
const char* GetNodeName(const GraphElements::OneOf&) { return "one_of"; }
const char* GetNodeName(const GraphElements::Alternatives&) { return "alternatives"; }
const char* GetNodeName(const GraphElements::GroupStart&) { return "group_start"; }
const char* GetNodeName(const GraphElements::GroupEnd&) { return "group_end"; }
const char* GetNodeName(const GraphElements::BackReference&) { return "back_reference"; }
const char* GetNodeName(const GraphElements::Look&) { return "look"; }
const char* GetNodeName(const GraphElements::ConditionalElement&) { return "condtinonal_element"; }
const char* GetNodeName(const GraphElements::SequenceCounterReset&) { return "sequence_counter_reset"; }
const char* GetNodeName(const GraphElements::SequenceCounter&) { return "loop_counter_block"; }
const char* GetNodeName(const GraphElements::NextWeakNode& node) { return GetNodeName(node.next.lock()); }
const char* GetNodeName(const GraphElements::PossessiveSequence&) { return "possessive_sequence"; }
const char* GetNodeName(const GraphElements::AtomicGroup&) { return "atomic_group"; }
const char* GetNodeName(const GraphElements::SubroutineEnter&) { return "subroutine_enter"; }
const char* GetNodeName(const GraphElements::SubroutineLeave&) { return "subroutine_leave"; }
const char* GetNodeName(const GraphElements::StateSave&) { return "state_save"; }
const char* GetNodeName(const GraphElements::StateRestore&) { return "state_restore"; }

const char* GetNodeName(const GraphElements::NodePtr& node)
{
	if(node == nullptr)
		return "true";

	return std::visit([](const auto& el){ return GetNodeName(el); }, *node);
}

struct StateFieldIndex
{
	enum
	{
		StrBegin,
		StrEnd,
	};
};

class Generator
{
public:
	explicit Generator(llvm::Module& module);

	void GenerateMatcherFunction(const RegexGraphBuildResult& regex_graph, const std::string& function_name);

private:
	llvm::Function* GetOrCreateNodeFunction(const GraphElements::NodePtr& node);

	void BuildNodeFunctionBody(const GraphElements::NodePtr& node, llvm::Function* function);

	void BuildNodeFunctionBodyImpl(
		llvm::IRBuilder<>& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::AnySymbol& node);

	void BuildNodeFunctionBodyImpl(
		llvm::IRBuilder<>& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::SpecificSymbol& node);

	void BuildNodeFunctionBodyImpl(
		llvm::IRBuilder<>& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::OneOf& node);

	void BuildNodeFunctionBodyImpl(
		llvm::IRBuilder<>& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::Alternatives& node);

	void BuildNodeFunctionBodyImpl(
		llvm::IRBuilder<>& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::NextWeakNode& node);

	template<typename T>
	void BuildNodeFunctionBodyImpl(
		llvm::IRBuilder<>& llvm_ir_builder, llvm::Value* const state_ptr,
		const T&);

	void CreateNextCallRet(
		llvm::IRBuilder<>& llvm_ir_builder,
		llvm::Value* state_ptr,
		const GraphElements::NodePtr& next_node);

	void CopyState(llvm::IRBuilder<>& llvm_ir_builder, llvm::Value* dst, llvm::Value* src);

	llvm::Constant* GetZeroGEPIndex() const;
	llvm::Constant* GetFieldGEPIndex(uint32_t field_index) const;

private:
	llvm::LLVMContext& context_;
	llvm::Module& module_;

	llvm::IntegerType* const gep_index_type_;
	llvm::IntegerType* const char_type_;
	llvm::PointerType* const char_type_ptr_;

	llvm::StructType* state_type_= nullptr;
	llvm::FunctionType* node_function_type_= nullptr;

	std::unordered_map<GraphElements::NodePtr, llvm::Function*> node_functions_;
};

Generator::Generator(llvm::Module& module)
	: context_(module.getContext())
	, module_(module)
	, gep_index_type_(llvm::IntegerType::getInt32Ty(context_))
	, char_type_(llvm::Type::getInt8Ty(context_))
	, char_type_ptr_(llvm::PointerType::get(char_type_, 0))
{}

void Generator::GenerateMatcherFunction(const RegexGraphBuildResult& regex_graph, const std::string& function_name)
{
	// Body of state struct depends on actual regex.
	state_type_= llvm::StructType::create(context_, "State");
	state_type_->setBody({char_type_ptr_, char_type_ptr_});

	const auto state_ptr_type= llvm::PointerType::get(state_type_, 0);

	// All match node functions looks like this:
	// bool MatchNode0123(State& state);
	node_function_type_= llvm::FunctionType::get(llvm::Type::getInt1Ty(context_), {state_ptr_type}, false);

	// Root function look like this:
	// const char* Match(const char* begin, const char* end);
	// It returns "nullptr" if no match found or pointer to match end.

	const auto root_function_type=
		llvm::FunctionType::get(
			char_type_ptr_,
			{char_type_ptr_, char_type_ptr_},
			false);

	const auto root_function= llvm::Function::Create(root_function_type, llvm::GlobalValue::ExternalLinkage, function_name, module_);

	const auto start_basic_block= llvm::BasicBlock::Create(context_, "", root_function);
	const auto found_block= llvm::BasicBlock::Create(context_, "found", root_function);
	const auto not_found_block= llvm::BasicBlock::Create(context_, "not_found", root_function);

	llvm::IRBuilder<> llvm_ir_builder(start_basic_block);

	const auto state_ptr= llvm_ir_builder.CreateAlloca(state_type_, 0, "state");

	// Initialize state.
	{
		const auto str_begin_ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::StrBegin)});
		llvm_ir_builder.CreateStore(&*root_function->arg_begin(), str_begin_ptr);

		const auto str_end_ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::StrEnd)});
		llvm_ir_builder.CreateStore(&*std::next(root_function->arg_begin()), str_end_ptr);
	}

	// Call match function.
	const auto match_res= llvm_ir_builder.CreateCall(GetOrCreateNodeFunction(regex_graph.root), {state_ptr}, "root_call_res");
	llvm_ir_builder.CreateCondBr(match_res, found_block, not_found_block);

	// Return result.
	{
		llvm_ir_builder.SetInsertPoint(found_block);
		const auto str_begin_ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::StrBegin)});
		const auto str_begin_value= llvm_ir_builder.CreateLoad(str_begin_ptr);
		llvm_ir_builder.CreateRet(str_begin_value);
	}
	{
		llvm_ir_builder.SetInsertPoint(not_found_block);
		llvm_ir_builder.CreateRet(llvm::ConstantPointerNull::get(char_type_ptr_));
	}
}

llvm::Function* Generator::GetOrCreateNodeFunction(const GraphElements::NodePtr& node)
{
	if(const auto it= node_functions_.find(node); it != node_functions_.end())
		return it->second;

	if(node != nullptr)
	{
		if(const auto next_weak_node= std::get_if<GraphElements::NextWeakNode>(node.get()))
		{
			const auto next= next_weak_node->next.lock();
			assert(next != nullptr);
			const auto function= GetOrCreateNodeFunction(next);
			node_functions_.emplace(node, function);
			return function;
		}
	}

	const auto function= llvm::Function::Create(node_function_type_, llvm::GlobalValue::ExternalLinkage, GetNodeName(node), module_);
	node_functions_.emplace(node, function);
	BuildNodeFunctionBody(node, function);
	return function;
}

void Generator::BuildNodeFunctionBody(const GraphElements::NodePtr& node, llvm::Function* const function)
{
	const auto basic_block= llvm::BasicBlock::Create(context_, "", function);
	llvm::IRBuilder<> llvm_ir_builder(basic_block);

	if(node == nullptr)
	{
		// for end node create "return true" function.
		llvm_ir_builder.CreateRet(llvm::ConstantInt::getTrue(context_));
		return;
	}

	const auto state_ptr= &*function->arg_begin();
	state_ptr->setName("state");

	std::visit([&](const auto& el){ BuildNodeFunctionBodyImpl(llvm_ir_builder, state_ptr, el); }, *node);
}

void Generator::BuildNodeFunctionBodyImpl(
	llvm::IRBuilder<>& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::AnySymbol& node)
{
	const auto function= llvm_ir_builder.GetInsertBlock()->getParent();

	const auto str_begin_ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::StrBegin)});
	const auto str_begin_value= llvm_ir_builder.CreateLoad(str_begin_ptr);

	const auto str_end_ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::StrEnd)});
	const auto str_end_value= llvm_ir_builder.CreateLoad(str_end_ptr);

	// TODO - support UTF-8.

	const auto is_empty= llvm_ir_builder.CreateICmpEQ(str_begin_value, str_end_value);

	const auto empty_block= llvm::BasicBlock::Create(context_, "empty", function);
	const auto non_empty_block= llvm::BasicBlock::Create(context_, "non_empty", function);

	llvm_ir_builder.CreateCondBr(is_empty, empty_block, non_empty_block);

	llvm_ir_builder.SetInsertPoint(empty_block);
	llvm_ir_builder.CreateRet(llvm::ConstantInt::getFalse(context_));

	llvm_ir_builder.SetInsertPoint(non_empty_block);

	const auto new_str_begin_value= llvm_ir_builder.CreateGEP(str_begin_value, GetFieldGEPIndex(1));
	llvm_ir_builder.CreateStore(new_str_begin_value, str_begin_ptr);

	CreateNextCallRet(llvm_ir_builder, state_ptr, node.next);
}

void Generator::BuildNodeFunctionBodyImpl(
	llvm::IRBuilder<>& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::SpecificSymbol& node)
{
	const auto function= llvm_ir_builder.GetInsertBlock()->getParent();

	const auto str_begin_ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::StrBegin)});
	const auto str_begin_value= llvm_ir_builder.CreateLoad(str_begin_ptr);

	const auto str_end_ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::StrEnd)});
	const auto str_end_value= llvm_ir_builder.CreateLoad(str_end_ptr);

	const auto is_empty= llvm_ir_builder.CreateICmpEQ(str_begin_value, str_end_value);

	const auto empty_block= llvm::BasicBlock::Create(context_, "empty", function);
	const auto non_empty_block= llvm::BasicBlock::Create(context_, "non_empty", function);

	llvm_ir_builder.CreateCondBr(is_empty, empty_block, non_empty_block);

	llvm_ir_builder.SetInsertPoint(empty_block);
	llvm_ir_builder.CreateRet(llvm::ConstantInt::getFalse(context_));

	llvm_ir_builder.SetInsertPoint(non_empty_block);

	// TODO - support UTF-8.

	const auto char_value= llvm_ir_builder.CreateLoad(str_begin_value, "char_value");
	const auto is_same_symbol=
		llvm_ir_builder.CreateICmpEQ(
			char_value,
			llvm::ConstantInt::get(char_type_, llvm::APInt(char_type_->getBitWidth(), node.code)));

	const auto ne_block= llvm::BasicBlock::Create(context_, "ne", function);
	const auto eq_block= llvm::BasicBlock::Create(context_, "eq", function);

	llvm_ir_builder.CreateCondBr(is_same_symbol, eq_block, ne_block);

	llvm_ir_builder.SetInsertPoint(ne_block);
	llvm_ir_builder.CreateRet(llvm::ConstantInt::getFalse(context_));

	llvm_ir_builder.SetInsertPoint(eq_block);

	const auto new_str_begin_value= llvm_ir_builder.CreateGEP(str_begin_value, GetFieldGEPIndex(1));
	llvm_ir_builder.CreateStore(new_str_begin_value, str_begin_ptr);

	CreateNextCallRet(llvm_ir_builder, state_ptr, node.next);
}

void Generator::BuildNodeFunctionBodyImpl(
	llvm::IRBuilder<>& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::OneOf& node)
{
	const auto function= llvm_ir_builder.GetInsertBlock()->getParent();

	const auto str_begin_ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::StrBegin)});
	const auto str_begin_value= llvm_ir_builder.CreateLoad(str_begin_ptr);

	const auto str_end_ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::StrEnd)});
	const auto str_end_value= llvm_ir_builder.CreateLoad(str_end_ptr);

	const auto is_empty= llvm_ir_builder.CreateICmpEQ(str_begin_value, str_end_value);

	const auto empty_block= llvm::BasicBlock::Create(context_, "empty", function);
	const auto non_empty_block= llvm::BasicBlock::Create(context_, "non_empty", function);

	llvm_ir_builder.CreateCondBr(is_empty, empty_block, non_empty_block);

	llvm_ir_builder.SetInsertPoint(empty_block);
	llvm_ir_builder.CreateRet(llvm::ConstantInt::getFalse(context_));

	llvm_ir_builder.SetInsertPoint(non_empty_block);

	// TODO - support UTF-8.

	const auto char_value= llvm_ir_builder.CreateLoad(str_begin_value, "char_value");

	const auto found_block= llvm::BasicBlock::Create(context_, "found");

	for(const CharType c : node.variants)
	{
		const auto is_same_symbol=
			llvm_ir_builder.CreateICmpEQ(
				char_value,
				llvm::ConstantInt::get(char_type_, llvm::APInt(char_type_->getBitWidth(), c)));

		const auto next_block= llvm::BasicBlock::Create(context_, "next", function);

		llvm_ir_builder.CreateCondBr(is_same_symbol, found_block, next_block);
		llvm_ir_builder.SetInsertPoint(next_block);
	}

	for(const auto& range : node.ranges)
	{
		const auto range_begin_constant= llvm::ConstantInt::get(char_type_, llvm::APInt(char_type_->getBitWidth(), range.first ));
		const auto range_end_constant  = llvm::ConstantInt::get(char_type_, llvm::APInt(char_type_->getBitWidth(), range.second));

		const auto ge= llvm_ir_builder.CreateICmpUGE(char_value, range_begin_constant);
		const auto le= llvm_ir_builder.CreateICmpULE(char_value, range_end_constant  );
		const auto in_range= llvm_ir_builder.CreateAnd(ge, le);

		const auto next_block= llvm::BasicBlock::Create(context_, "next", function);

		llvm_ir_builder.CreateCondBr(in_range, found_block, next_block);
		llvm_ir_builder.SetInsertPoint(next_block);
	}

	if(node.inverse_flag)
	{
		// Not found anything - continue.
		llvm_ir_builder.GetInsertBlock()->setName("not_found");

		const auto new_str_begin_value= llvm_ir_builder.CreateGEP(str_begin_value, GetFieldGEPIndex(1));
		llvm_ir_builder.CreateStore(new_str_begin_value, str_begin_ptr);
		CreateNextCallRet(llvm_ir_builder, state_ptr, node.next);

		// Found something - return false.
		found_block->insertInto(function);
		llvm_ir_builder.SetInsertPoint(found_block);
		llvm_ir_builder.CreateRet(llvm::ConstantInt::getFalse(context_));
	}
	else
	{
		// Not found anything - return false.
		llvm_ir_builder.GetInsertBlock()->setName("not_found");
		llvm_ir_builder.CreateRet(llvm::ConstantInt::getFalse(context_));

		// Found - continue.
		found_block->insertInto(function);
		llvm_ir_builder.SetInsertPoint(found_block);

		const auto new_str_begin_value= llvm_ir_builder.CreateGEP(str_begin_value, GetFieldGEPIndex(1));
		llvm_ir_builder.CreateStore(new_str_begin_value, str_begin_ptr);

		CreateNextCallRet(llvm_ir_builder, state_ptr, node.next);
	}
}

void Generator::BuildNodeFunctionBodyImpl(
	llvm::IRBuilder<>& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::Alternatives& node)
{
	const auto function= llvm_ir_builder.GetInsertBlock()->getParent();

	const auto state_copy_ptr= llvm_ir_builder.CreateAlloca(state_type_, 0, "state_copy");

	const auto found_block= llvm::BasicBlock::Create(context_, "found");

	for(const GraphElements::NodePtr& possible_next : node.next)
	{
		CopyState(llvm_ir_builder, state_copy_ptr, state_ptr);
		const auto variant_res= llvm_ir_builder.CreateCall(GetOrCreateNodeFunction(possible_next), {state_copy_ptr});
		const auto next_block= llvm::BasicBlock::Create(context_, "", function);

		llvm_ir_builder.CreateCondBr(variant_res, found_block, next_block);
		llvm_ir_builder.SetInsertPoint(next_block);
	}

	// Return "false" in last "next" block.
	llvm_ir_builder.GetInsertBlock()->setName("not_found");
	llvm_ir_builder.CreateRet(llvm::ConstantInt::getFalse(context_));

	// Return true if one of next variants were successfull.
	found_block->insertInto(function);
	llvm_ir_builder.SetInsertPoint(found_block);
	CopyState(llvm_ir_builder, state_ptr, state_copy_ptr);
	llvm_ir_builder.CreateRet(llvm::ConstantInt::getTrue(context_));
}

void Generator::BuildNodeFunctionBodyImpl(
	llvm::IRBuilder<>& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::NextWeakNode& node)
{
	const auto next= node.next.lock();
	assert(next != nullptr);
	CreateNextCallRet(llvm_ir_builder, state_ptr, next);
}

template<typename T>
void Generator::BuildNodeFunctionBodyImpl(llvm::IRBuilder<>&, llvm::Value* const, const T&)
{
	assert(false && "not implemented yet!");
}

void Generator::CreateNextCallRet(
	llvm::IRBuilder<>& llvm_ir_builder,
	llvm::Value* const state_ptr,
	const GraphElements::NodePtr& next_node)
{
	const auto next_call= llvm_ir_builder.CreateCall(GetOrCreateNodeFunction(next_node), {state_ptr}, "next_call_res");
	llvm_ir_builder.CreateRet(next_call);
}

void Generator::CopyState(llvm::IRBuilder<>& llvm_ir_builder, llvm::Value* const dst, llvm::Value* const src)
{
	// TODO - use memcpy intrisinc?
	for(uint32_t i= 0; i < state_type_->getNumElements(); ++i)
	{
		const auto dst_element_ptr= llvm_ir_builder.CreateGEP(dst, {GetZeroGEPIndex(), GetFieldGEPIndex(i)});
		const auto src_element_ptr= llvm_ir_builder.CreateGEP(src, {GetZeroGEPIndex(), GetFieldGEPIndex(i)});
		const auto value= llvm_ir_builder.CreateLoad(src_element_ptr);
		llvm_ir_builder.CreateStore(value, dst_element_ptr);
	}
}

llvm::Constant* Generator::GetZeroGEPIndex() const
{
	return llvm::Constant::getNullValue(gep_index_type_);
}

llvm::Constant* Generator::GetFieldGEPIndex(const uint32_t field_index) const
{
	return llvm::Constant::getIntegerValue(gep_index_type_, llvm::APInt(gep_index_type_->getBitWidth(), field_index));
}

} // namespace

void GenerateMatcherFunction(
	llvm::Module& module,
	const RegexGraphBuildResult& regex_graph,
	const std::string& function_name)
{
	Generator generator(module);
	generator.GenerateMatcherFunction(regex_graph, function_name);
}

} // namespace RegPanzer
