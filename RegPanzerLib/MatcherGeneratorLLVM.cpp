#include "MatcherGeneratorLLVM.hpp"
#include "PushDisableLLVMWarnings.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/ConvertUTF.h>
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
		SequenceContersArray,
		GroupsArray,
		SubroutineCallReturnChainHead,
		SubroutineCallStateSaveChainHead,
	};
};

struct SubroutineCallReturnChainNodeFieldIndex
{
	enum
	{
		NextFunction,
		Prev,
	};
};


struct SubroutineCallStateSaveChainNodeFieldIndex
{
	enum
	{
		SequenceContersArray,
		GroupsArray,
		Prev,
	};
};

using IRBuilder= llvm::IRBuilder<>;

class Generator
{
public:
	explicit Generator(llvm::Module& module);

	void GenerateMatcherFunction(const RegexGraphBuildResult& regex_graph, const std::string& function_name);

private:
	void CreateStateType(const RegexGraphBuildResult& regex_graph);

	llvm::Function* GetOrCreateNodeFunction(const GraphElements::NodePtr& node);

	void BuildNodeFunctionBody(const GraphElements::NodePtr& node, llvm::Function* function);

	void BuildNodeFunctionBodyImpl(
		IRBuilder& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::AnySymbol& node);

	void BuildNodeFunctionBodyImpl(
		IRBuilder& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::SpecificSymbol& node);

	void BuildNodeFunctionBodyImpl(
		IRBuilder& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::OneOf& node);

	void BuildNodeFunctionBodyImpl(
		IRBuilder& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::Alternatives& node);

	void BuildNodeFunctionBodyImpl(
		IRBuilder& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::GroupStart& node);

	void BuildNodeFunctionBodyImpl(
		IRBuilder& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::GroupEnd& node);

	void BuildNodeFunctionBodyImpl(
		IRBuilder& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::BackReference& node);

	void BuildNodeFunctionBodyImpl(
		IRBuilder& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::Look& node);

	void BuildNodeFunctionBodyImpl(
		IRBuilder& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::ConditionalElement& node);

	void BuildNodeFunctionBodyImpl(
		IRBuilder& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::SequenceCounterReset& node);

	void BuildNodeFunctionBodyImpl(
		IRBuilder& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::SequenceCounter& node);

	void BuildNodeFunctionBodyImpl(
		IRBuilder& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::NextWeakNode& node);

	void BuildNodeFunctionBodyImpl(
		IRBuilder& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::PossessiveSequence& node);

	void BuildNodeFunctionBodyImpl(
		IRBuilder& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::AtomicGroup& node);

	void BuildNodeFunctionBodyImpl(
		IRBuilder& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::SubroutineEnter& node);

	void BuildNodeFunctionBodyImpl(
		IRBuilder& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::SubroutineLeave& node);

	void BuildNodeFunctionBodyImpl(
		IRBuilder& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::StateSave& node);

	void BuildNodeFunctionBodyImpl(
		IRBuilder& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::StateRestore& node);

	void CreateNextCallRet(
		IRBuilder& llvm_ir_builder, llvm::Value* state_ptr, const GraphElements::NodePtr& next_node);

	void CopyState(IRBuilder& llvm_ir_builder, llvm::Value* dst, llvm::Value* src);

	llvm::ConstantInt* GetConstant(llvm::IntegerType* type, uint64_t value) const;
	llvm::Constant* GetZeroGEPIndex() const;
	llvm::Constant* GetFieldGEPIndex(uint32_t field_index) const;

private:
	llvm::LLVMContext& context_;
	llvm::Module& module_;

	llvm::IntegerType* const gep_index_type_;
	llvm::IntegerType* const ptr_size_int_type_;
	llvm::IntegerType* const char_type_;
	llvm::PointerType* const char_type_ptr_;
	llvm::IntegerType* const code_point_type_;
	llvm::StructType* const group_type_;

	llvm::StructType* state_type_= nullptr;
	llvm::FunctionType* node_function_type_= nullptr;
	llvm::StructType* subroutine_call_return_chain_node_type_= nullptr;
	llvm::StructType* subroutine_call_state_save_chain_node_type_= nullptr;

	std::unordered_map<GraphElements::SequenceId, uint32_t> sequence_id_to_counter_filed_number_;
	std::unordered_map<size_t, uint32_t> group_number_to_field_number_;

	std::unordered_map<GraphElements::NodePtr, llvm::Function*> node_functions_;
};

Generator::Generator(llvm::Module& module)
	: context_(module.getContext())
	, module_(module)
	, gep_index_type_(llvm::IntegerType::getInt32Ty(context_))
	, ptr_size_int_type_(module.getDataLayout().getIntPtrType(context_, 0))
	, char_type_(llvm::Type::getInt8Ty(context_))
	, char_type_ptr_(llvm::PointerType::get(char_type_, 0))
	, code_point_type_(llvm::Type::getInt32Ty(context_))
	, group_type_(llvm::StructType::get(char_type_ptr_, char_type_ptr_))
{}

void Generator::GenerateMatcherFunction(const RegexGraphBuildResult& regex_graph, const std::string& function_name)
{
	// Body of state struct depends on actual regex.
	CreateStateType(regex_graph);

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

	IRBuilder llvm_ir_builder(start_basic_block);

	const auto state_ptr= llvm_ir_builder.CreateAlloca(state_type_, 0, "state");

	// Initialize state.
	{
		// Set begin/end pointers.

		const auto str_begin_ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::StrBegin)});
		llvm_ir_builder.CreateStore(&*root_function->arg_begin(), str_begin_ptr);

		const auto str_end_ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::StrEnd)});
		llvm_ir_builder.CreateStore(&*std::next(root_function->arg_begin()), str_end_ptr);
	}
	{
		// Zero groups.

		const uint64_t groups_array_size= state_type_->elements()[StateFieldIndex::GroupsArray]->getArrayNumElements();
		for(uint64_t i= 0; i < groups_array_size; ++i)
		{
			const auto group_ptr=
				llvm_ir_builder.CreateGEP(
					state_ptr,
					{
						GetZeroGEPIndex(),
						GetFieldGEPIndex(StateFieldIndex::GroupsArray),
						GetFieldGEPIndex(uint32_t(i)),
					});

			const auto group_begin_ptr= llvm_ir_builder.CreateGEP(group_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(0)});
			const auto group_end_ptr  = llvm_ir_builder.CreateGEP(group_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(1)});

			const auto null= llvm::Constant::getNullValue(char_type_ptr_);
			llvm_ir_builder.CreateStore(null, group_begin_ptr);
			llvm_ir_builder.CreateStore(null, group_end_ptr);
		}
	}
	{
		// Zero subroutine call return chain head.
		const auto ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::SubroutineCallReturnChainHead)});
		const auto null= llvm::Constant::getNullValue(llvm::PointerType::get(subroutine_call_return_chain_node_type_, 0));
		llvm_ir_builder.CreateStore(null, ptr);
	}
	{
		// Zero subroutine call state save chain head.
		const auto ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::SubroutineCallStateSaveChainHead)});
		const auto null= llvm::Constant::getNullValue(llvm::PointerType::get(subroutine_call_state_save_chain_node_type_, 0));
		llvm_ir_builder.CreateStore(null, ptr);
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

	// Clear internal structures.
	state_type_= nullptr;
	node_function_type_= nullptr;
	subroutine_call_return_chain_node_type_= nullptr;
	subroutine_call_state_save_chain_node_type_= nullptr;
	sequence_id_to_counter_filed_number_.clear();
	group_number_to_field_number_.clear();
	node_functions_.clear();
}

void Generator::CreateStateType(const RegexGraphBuildResult& regex_graph)
{
	state_type_= llvm::StructType::create(context_, "State");

	// All match node functions looks like this:
	// bool MatchNode0123(State& state);
	node_function_type_= llvm::FunctionType::get(llvm::Type::getInt1Ty(context_), {llvm::PointerType::get(state_type_, 0)}, false);

	llvm::SmallVector<llvm::Type*, 6> members;

	members.push_back(char_type_ptr_); // StrBegin
	members.push_back(char_type_ptr_); // StrEnd

	{
		const GroupStat& regex_stat= regex_graph.group_stats.at(0);
		const size_t number_of_counters= regex_stat.internal_sequences.size();
		const auto sequence_counters_array= llvm::ArrayType::get(ptr_size_int_type_, uint64_t(number_of_counters));
		members.push_back(sequence_counters_array);

		for(const GraphElements::SequenceId sequence_id : regex_stat.internal_sequences)
			sequence_id_to_counter_filed_number_.emplace(sequence_id, uint32_t(sequence_id_to_counter_filed_number_.size()));
	}
	{
		for(const auto& group_pair : regex_graph.group_stats)
			if(group_pair.second.backreference_count > 0)
				group_number_to_field_number_.emplace(group_pair.first, size_t(group_number_to_field_number_.size()));

		const auto groups_array= llvm::ArrayType::get(group_type_, uint64_t(group_number_to_field_number_.size()));
		members.push_back(groups_array);
	}
	{
		// TODO - create this field only if subroutine calls are needed.
		subroutine_call_return_chain_node_type_= llvm::StructType::create(context_, "SubroutineCallReturnChainNode");
		const auto node_ptr_type= llvm::PointerType::get(subroutine_call_return_chain_node_type_, 0);
		subroutine_call_return_chain_node_type_->setBody({llvm::PointerType::get(node_function_type_, 0), node_ptr_type});

		members.push_back(node_ptr_type);
	}
	{
		// TODO - create this field only if state save nodes are used.
		subroutine_call_state_save_chain_node_type_= llvm::StructType::create(context_, "SubroutineCallStateSaveChainNode");
		const auto node_ptr_type= llvm::PointerType::get(subroutine_call_state_save_chain_node_type_, 0);

		llvm::Type* const elements[]
		{
			members[StateFieldIndex::SequenceContersArray],
			members[StateFieldIndex::GroupsArray],
			node_ptr_type,
		};
		subroutine_call_state_save_chain_node_type_->setBody(elements);

		members.push_back(node_ptr_type);
	}

	state_type_->setBody(members);
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

	// Use private linkage for all node functions to avoid possible name conflicts.
	const auto function= llvm::Function::Create(node_function_type_, llvm::GlobalValue::PrivateLinkage, GetNodeName(node), module_);
	node_functions_.emplace(node, function);
	BuildNodeFunctionBody(node, function);
	return function;
}

void Generator::BuildNodeFunctionBody(const GraphElements::NodePtr& node, llvm::Function* const function)
{
	const auto basic_block= llvm::BasicBlock::Create(context_, "", function);
	IRBuilder llvm_ir_builder(basic_block);

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
	IRBuilder& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::AnySymbol& node)
{
	GraphElements::OneOf one_of;
	one_of.next= node.next;
	one_of.inverse_flag= true;
	BuildNodeFunctionBodyImpl(llvm_ir_builder, state_ptr, one_of);
}

void Generator::BuildNodeFunctionBodyImpl(
	IRBuilder& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::SpecificSymbol& node)
{
	const auto function= llvm_ir_builder.GetInsertBlock()->getParent();

	const auto str_begin_ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::StrBegin)});
	const auto str_begin_value= llvm_ir_builder.CreateLoad(str_begin_ptr);

	const auto str_end_ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::StrEnd)});
	const auto str_end_value= llvm_ir_builder.CreateLoad(str_end_ptr);

	const auto check_content_block= llvm::BasicBlock::Create(context_, "check_content", function);
	const auto ok_block= llvm::BasicBlock::Create(context_, "ok", function);
	const auto fail_block= llvm::BasicBlock::Create(context_, "fail", function);

	char buff[UNI_MAX_UTF8_BYTES_PER_CODE_POINT+4];
	char* buff_ptr= buff;
	llvm::ConvertCodePointToUTF8(node.code, buff_ptr);
	const auto char_size= uint32_t(buff_ptr - buff);

	llvm::Value* next_str_begin_value= nullptr;
	if(char_size == 1)
	{
		next_str_begin_value= llvm_ir_builder.CreateGEP(str_begin_value, GetFieldGEPIndex(1), "next_str_begin_value");

		const auto is_empty= llvm_ir_builder.CreateICmpEQ(str_begin_value, str_end_value);
		llvm_ir_builder.CreateCondBr(is_empty, fail_block, check_content_block);

		// Check content block.
		llvm_ir_builder.SetInsertPoint(check_content_block);
		const auto char_value= llvm_ir_builder.CreateLoad(str_begin_value, "char_value");
		const auto is_same_symbol= llvm_ir_builder.CreateICmpEQ(char_value, GetConstant(char_type_, node.code));
		llvm_ir_builder.CreateCondBr(is_same_symbol, ok_block, fail_block);
	}
	else
	{
		next_str_begin_value= llvm_ir_builder.CreateGEP(str_begin_value, GetFieldGEPIndex(char_size), "next_str_begin_value");

		const auto not_enough_condition= llvm_ir_builder.CreateICmpULT(str_begin_value, next_str_begin_value);
		llvm_ir_builder.CreateCondBr(not_enough_condition, check_content_block, fail_block);

		// Check content block.
		llvm_ir_builder.SetInsertPoint(check_content_block);

		llvm::Value* all_eq_value= nullptr;
		for(uint32_t i= 0; i < char_size; ++i)
		{
			const auto char_ptr= llvm_ir_builder.CreateGEP(str_begin_value, GetFieldGEPIndex(i));
			const auto char_value= llvm_ir_builder.CreateLoad(char_ptr, "char_value");
			const auto eq= llvm_ir_builder.CreateICmpEQ(char_value, GetConstant(char_type_, uint64_t(buff[i])), "eq");
			if(all_eq_value == nullptr)
				all_eq_value = eq;
			else
				all_eq_value= llvm_ir_builder.CreateAnd(all_eq_value, eq);
		}
		llvm_ir_builder.CreateCondBr(all_eq_value, ok_block, fail_block);
	}

	// Ok block.
	llvm_ir_builder.SetInsertPoint(ok_block);
	llvm_ir_builder.CreateStore(next_str_begin_value, str_begin_ptr);
	CreateNextCallRet(llvm_ir_builder, state_ptr, node.next);

	// Fail block
	llvm_ir_builder.SetInsertPoint(fail_block);
	llvm_ir_builder.CreateRet(llvm::ConstantInt::getFalse(context_));
}

void Generator::BuildNodeFunctionBodyImpl(
	IRBuilder& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::OneOf& node)
{
	static const CharType c_bit_masks[9]=
	{
		(1 << 0) - 1,
		(1 << 1) - 1,
		(1 << 2) - 1,
		(1 << 3) - 1,
		(1 << 4) - 1,
		(1 << 5) - 1,
		(1 << 6) - 1,
		(1 << 7) - 1,
		(1 << 8) - 1,
	};

	bool has_multi_byte_code_points= false;
	{
		const CharType c_last_onle_byte_code_point= 0x7F;
		for(const CharType c : node.variants)
			has_multi_byte_code_points|= c > c_last_onle_byte_code_point;
		for(const auto& range : node.ranges)
			has_multi_byte_code_points= range.second >=c_last_onle_byte_code_point;
	}

	const auto function= llvm_ir_builder.GetInsertBlock()->getParent();

	const auto str_begin_ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::StrBegin)});
	const auto str_begin_value= llvm_ir_builder.CreateLoad(str_begin_ptr);

	const auto str_end_ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::StrEnd)});
	const auto str_end_value= llvm_ir_builder.CreateLoad(str_end_ptr);

	auto found_block= llvm::BasicBlock::Create(context_, "found");
	const auto empty_block= llvm::BasicBlock::Create(context_, "empty");
	const auto non_empty_block= llvm::BasicBlock::Create(context_, "non_empty", function);

	const auto is_empty= llvm_ir_builder.CreateICmpEQ(str_begin_value, str_end_value);
	llvm_ir_builder.CreateCondBr(is_empty, empty_block, non_empty_block);

	// Non-empty block.
	llvm_ir_builder.SetInsertPoint(non_empty_block);
	const auto char_value= llvm_ir_builder.CreateLoad(str_begin_value, "char_value");

	llvm::Value* new_str_begin_value= nullptr;
	if(has_multi_byte_code_points || node.inverse_flag)
	{
		// In negative checks or in checks with multi-byte code points extract code point from UTF-8 and compare it against UTF-32 constants.

		const auto block_0= llvm::BasicBlock::Create(context_, "block_0", function);
		const auto block_1_check= llvm::BasicBlock::Create(context_, "block_check1", function);
		const auto block_1= llvm::BasicBlock::Create(context_, "block_1", function);
		const auto block_1_after_size_check= llvm::BasicBlock::Create(context_, "block_1_after_size_check", function);
		const auto block_2_check= llvm::BasicBlock::Create(context_, "block_2_check", function);
		const auto block_2= llvm::BasicBlock::Create(context_, "block_2", function);
		const auto block_2_after_size_check= llvm::BasicBlock::Create(context_, "block_2_after_size_check", function);
		const auto block_3_check= llvm::BasicBlock::Create(context_, "block_3_check", function);
		const auto block_3= llvm::BasicBlock::Create(context_, "block_3", function);
		const auto block_3_after_size_check= llvm::BasicBlock::Create(context_, "block_3_after_size_check", function);
		const auto block_invalid_utf8= llvm::BasicBlock::Create(context_, "block_invalid_utf8", function);
		const auto extract_end_block= llvm::BasicBlock::Create(context_, "extract_end", function);

		const auto first_char_value= llvm_ir_builder.CreateZExt(char_value, code_point_type_, "first_char_value");

		// Block 0 check.
		const auto and_mask0= llvm_ir_builder.CreateAnd(first_char_value, GetConstant(code_point_type_, 0b10000000));
		const auto cond_0= llvm_ir_builder.CreateICmpEQ(and_mask0, GetConstant(code_point_type_, 0));
		llvm_ir_builder.CreateCondBr(cond_0, block_0, block_1_check);

		// Block 0.
		llvm_ir_builder.SetInsertPoint(block_0);
		const auto char_code0= first_char_value;
		const auto str_begin0= llvm_ir_builder.CreateGEP(str_begin_value, GetFieldGEPIndex(1), "str_begin0");
		llvm_ir_builder.CreateBr(extract_end_block);

		// Block 1 check.
		llvm_ir_builder.SetInsertPoint(block_1_check);
		const auto and_mask1= llvm_ir_builder.CreateAnd(first_char_value, GetConstant(code_point_type_, 0b11100000));
		const auto cond_1= llvm_ir_builder.CreateICmpEQ(and_mask1, GetConstant(code_point_type_, 0b11000000));
		llvm_ir_builder.CreateCondBr(cond_1, block_1, block_2_check);

		// Block 1.
		llvm_ir_builder.SetInsertPoint(block_1);
		const auto str_begin1= llvm_ir_builder.CreateGEP(str_begin_value, GetFieldGEPIndex(2), "str_begin1");
		llvm_ir_builder.CreateCondBr(
			llvm_ir_builder.CreateICmpULE(str_begin1, str_end_value),
			block_1_after_size_check,
			empty_block);

		// Block 1 after size check.
		llvm_ir_builder.SetInsertPoint(block_1_after_size_check);
		const auto char_code1= [&]
		{
			const auto b0= first_char_value;
			const auto b1=
				llvm_ir_builder.CreateZExt(
					llvm_ir_builder.CreateLoad(llvm_ir_builder.CreateGEP(str_begin_value, GetFieldGEPIndex(1))),
					code_point_type_);
			const auto and0= llvm_ir_builder.CreateAnd(b0, GetConstant(code_point_type_, c_bit_masks[5]));
			const auto and1= llvm_ir_builder.CreateAnd(b1, GetConstant(code_point_type_, c_bit_masks[6]));
			const auto shift0= llvm_ir_builder.CreateShl(and0, GetConstant(code_point_type_, 6));
			const auto shift1= and1;
			return llvm_ir_builder.CreateOr(shift0, shift1, "char_code1");
		}();
		llvm_ir_builder.CreateBr(extract_end_block);

		// Block 2 check.
		llvm_ir_builder.SetInsertPoint(block_2_check);
		const auto and_mask2= llvm_ir_builder.CreateAnd(first_char_value, GetConstant(code_point_type_, 0b11110000));
		const auto cond_2= llvm_ir_builder.CreateICmpEQ(and_mask2, GetConstant(code_point_type_, 0b11100000));
		llvm_ir_builder.CreateCondBr(cond_2, block_2, block_3_check);

		// Block 2.
		llvm_ir_builder.SetInsertPoint(block_2);
		const auto str_begin2= llvm_ir_builder.CreateGEP(str_begin_value, GetFieldGEPIndex(3), "str_begin2");
		llvm_ir_builder.CreateCondBr(
			llvm_ir_builder.CreateICmpULE(str_begin2, str_end_value),
			block_2_after_size_check,
			empty_block);

		// Block 2 after size check.
		llvm_ir_builder.SetInsertPoint(block_2_after_size_check);
		const auto char_code2= [&]
		{
			const auto b0= first_char_value;
			const auto b1=
				llvm_ir_builder.CreateZExt(
					llvm_ir_builder.CreateLoad(llvm_ir_builder.CreateGEP(str_begin_value, GetFieldGEPIndex(1))),
					code_point_type_);
			const auto b2=
				llvm_ir_builder.CreateZExt(
					llvm_ir_builder.CreateLoad(llvm_ir_builder.CreateGEP(str_begin_value, GetFieldGEPIndex(2))),
					code_point_type_);
			const auto and0= llvm_ir_builder.CreateAnd(b0, GetConstant(code_point_type_, c_bit_masks[4]));
			const auto and1= llvm_ir_builder.CreateAnd(b1, GetConstant(code_point_type_, c_bit_masks[6]));
			const auto and2= llvm_ir_builder.CreateAnd(b2, GetConstant(code_point_type_, c_bit_masks[6]));
			const auto shift0= llvm_ir_builder.CreateShl(and0, GetConstant(code_point_type_, 12));
			const auto shift1= llvm_ir_builder.CreateShl(and1, GetConstant(code_point_type_,  6));
			const auto shift2= and2;
			return llvm_ir_builder.CreateOr(llvm_ir_builder.CreateOr(shift0, shift1), shift2, "char_code2");
		}();
		llvm_ir_builder.CreateBr(extract_end_block);

		// Block 3 check.
		llvm_ir_builder.SetInsertPoint(block_3_check);
		const auto and_mask3= llvm_ir_builder.CreateAnd(first_char_value, GetConstant(code_point_type_, 0b11111000));
		const auto cond_3= llvm_ir_builder.CreateICmpEQ(and_mask3, GetConstant(code_point_type_, 0b11110000));
		llvm_ir_builder.CreateCondBr(cond_3, block_3, block_invalid_utf8);

		// Block 3.
		llvm_ir_builder.SetInsertPoint(block_3);
		const auto str_begin3= llvm_ir_builder.CreateGEP(str_begin_value, GetFieldGEPIndex(4), "str_begin2");
		llvm_ir_builder.CreateCondBr(
			llvm_ir_builder.CreateICmpULE(str_begin3, str_end_value),
			block_3_after_size_check,
			empty_block);

		// Block 3 after size check.
		llvm_ir_builder.SetInsertPoint(block_3_after_size_check);
		const auto char_code3= [&]
		{
			const auto b0= first_char_value;
			const auto b1=
				llvm_ir_builder.CreateZExt(
					llvm_ir_builder.CreateLoad(llvm_ir_builder.CreateGEP(str_begin_value, GetFieldGEPIndex(1))),
					code_point_type_);
			const auto b2=
				llvm_ir_builder.CreateZExt(
					llvm_ir_builder.CreateLoad(llvm_ir_builder.CreateGEP(str_begin_value, GetFieldGEPIndex(2))),
					code_point_type_);
			const auto b3=
				llvm_ir_builder.CreateZExt(
					llvm_ir_builder.CreateLoad(llvm_ir_builder.CreateGEP(str_begin_value, GetFieldGEPIndex(3))),
					code_point_type_);
			const auto and0= llvm_ir_builder.CreateAnd(b0, GetConstant(code_point_type_, c_bit_masks[3]));
			const auto and1= llvm_ir_builder.CreateAnd(b1, GetConstant(code_point_type_, c_bit_masks[6]));
			const auto and2= llvm_ir_builder.CreateAnd(b2, GetConstant(code_point_type_, c_bit_masks[6]));
			const auto and3= llvm_ir_builder.CreateAnd(b3, GetConstant(code_point_type_, c_bit_masks[6]));
			const auto shift0= llvm_ir_builder.CreateShl(and0, GetConstant(code_point_type_, 18));
			const auto shift1= llvm_ir_builder.CreateShl(and1, GetConstant(code_point_type_, 12));
			const auto shift2= llvm_ir_builder.CreateShl(and2, GetConstant(code_point_type_,  6));
			const auto shift3= and3;
			return llvm_ir_builder.CreateOr(llvm_ir_builder.CreateOr(shift0, shift1), llvm_ir_builder.CreateOr(shift2, shift3), "char_code3");
		}();
		llvm_ir_builder.CreateBr(extract_end_block);

		// Invalid UTF-8 block.
		llvm_ir_builder.SetInsertPoint(block_invalid_utf8);
		const auto str_invalid_utf8= llvm_ir_builder.CreateGEP(str_begin_value, GetFieldGEPIndex(1), "str_invalid_utf8");
		llvm_ir_builder.CreateBr(extract_end_block);

		// Extract end block.
		llvm_ir_builder.SetInsertPoint(extract_end_block);

		const auto result_char_value= llvm_ir_builder.CreatePHI(code_point_type_, 5, "result_char_value");
		result_char_value->addIncoming(char_code0, block_0);
		result_char_value->addIncoming(char_code1, block_1_after_size_check);
		result_char_value->addIncoming(char_code2, block_2_after_size_check);
		result_char_value->addIncoming(char_code3, block_3_after_size_check);
		result_char_value->addIncoming(first_char_value, block_invalid_utf8);

		const auto new_str_begin_value_phi= llvm_ir_builder.CreatePHI(char_type_ptr_, 5, "new_str_begin_value");
		new_str_begin_value= new_str_begin_value_phi;
		new_str_begin_value_phi->addIncoming(str_begin0, block_0);
		new_str_begin_value_phi->addIncoming(str_begin1, block_1_after_size_check);
		new_str_begin_value_phi->addIncoming(str_begin2, block_2_after_size_check);
		new_str_begin_value_phi->addIncoming(str_begin3, block_3_after_size_check);
		new_str_begin_value_phi->addIncoming(str_invalid_utf8, block_invalid_utf8);

		for(const CharType c : node.variants)
		{
			const auto is_same_symbol= llvm_ir_builder.CreateICmpEQ(result_char_value, GetConstant(code_point_type_, c));

			const auto next_block= llvm::BasicBlock::Create(context_, "next", function);

			llvm_ir_builder.CreateCondBr(is_same_symbol, found_block, next_block);
			llvm_ir_builder.SetInsertPoint(next_block);
		}

		for(const auto& range : node.ranges)
		{
			const auto ge= llvm_ir_builder.CreateICmpUGE(result_char_value, GetConstant(code_point_type_, range.first ));
			const auto le= llvm_ir_builder.CreateICmpULE(result_char_value, GetConstant(code_point_type_, range.second));
			const auto in_range= llvm_ir_builder.CreateAnd(ge, le);

			const auto next_block= llvm::BasicBlock::Create(context_, "next", function);

			llvm_ir_builder.CreateCondBr(in_range, found_block, next_block);
			llvm_ir_builder.SetInsertPoint(next_block);
		}
	}
	else
	{
		// In positive checks with ASCII-only variants and ranges read only first byte and compare it against byte contants.

		new_str_begin_value= llvm_ir_builder.CreateGEP(str_begin_value, GetFieldGEPIndex(1), "new_str_begin_value");
		for(const CharType c : node.variants)
		{
			const auto is_same_symbol= llvm_ir_builder.CreateICmpEQ(char_value, GetConstant(char_type_, c));

			const auto next_block= llvm::BasicBlock::Create(context_, "next", function);

			llvm_ir_builder.CreateCondBr(is_same_symbol, found_block, next_block);
			llvm_ir_builder.SetInsertPoint(next_block);
		}

		for(const auto& range : node.ranges)
		{
			const auto ge= llvm_ir_builder.CreateICmpUGE(char_value, GetConstant(char_type_, range.first ));
			const auto le= llvm_ir_builder.CreateICmpULE(char_value, GetConstant(char_type_, range.second));
			const auto in_range= llvm_ir_builder.CreateAnd(ge, le);

			const auto next_block= llvm::BasicBlock::Create(context_, "next", function);

			llvm_ir_builder.CreateCondBr(in_range, found_block, next_block);
			llvm_ir_builder.SetInsertPoint(next_block);
		}
	}

	if(node.ranges.empty() && node.variants.empty())
	{
		delete found_block;
		found_block= nullptr;
	}

	llvm_ir_builder.GetInsertBlock()->setName("not_found");
	if(node.inverse_flag)
	{
		// Not found anything - continue.
		llvm_ir_builder.CreateStore(new_str_begin_value, str_begin_ptr);
		CreateNextCallRet(llvm_ir_builder, state_ptr, node.next);

		// Found something - return false.
		if(found_block != nullptr)
		{
			found_block->insertInto(function);
			llvm_ir_builder.SetInsertPoint(found_block);
			llvm_ir_builder.CreateRet(llvm::ConstantInt::getFalse(context_));
		}
	}
	else
	{
		// Not found anything - return false.
		llvm_ir_builder.CreateRet(llvm::ConstantInt::getFalse(context_));

		// Found - continue.
		if(found_block != nullptr)
		{
			found_block->insertInto(function);
			llvm_ir_builder.SetInsertPoint(found_block);
			llvm_ir_builder.CreateStore(new_str_begin_value, str_begin_ptr);
			CreateNextCallRet(llvm_ir_builder, state_ptr, node.next);
		}
	}

	// Empty block.
	empty_block->insertInto(function);
	llvm_ir_builder.SetInsertPoint(empty_block);
	llvm_ir_builder.CreateRet(llvm::ConstantInt::getFalse(context_));
}

void Generator::BuildNodeFunctionBodyImpl(
	IRBuilder& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::Alternatives& node)
{
	const auto function= llvm_ir_builder.GetInsertBlock()->getParent();

	const auto state_copy_ptr= llvm_ir_builder.CreateAlloca(state_type_, 0, "state_copy");

	if(node.next.size() == 1)
		CreateNextCallRet(llvm_ir_builder, state_ptr, node.next.front());
	else if(node.next.size() == 2)
	{
		// Optimization for sequences - avoid unnecessary state save/restore.

		const auto ok_block= llvm::BasicBlock::Create(context_, "ok", function);
		const auto fail_block= llvm::BasicBlock::Create(context_, "fail", function);

		// Try first alternative with state copy.
		CopyState(llvm_ir_builder, state_copy_ptr, state_ptr);
		const auto variant_res= llvm_ir_builder.CreateCall(GetOrCreateNodeFunction(node.next[0]), {state_copy_ptr});
		llvm_ir_builder.CreateCondBr(variant_res, ok_block, fail_block);

		// First alternative is fine - copy state copy to original state.
		llvm_ir_builder.SetInsertPoint(ok_block);
		CopyState(llvm_ir_builder, state_ptr, state_copy_ptr);
		llvm_ir_builder.CreateRet(llvm::ConstantInt::getTrue(context_));

		// Fail block - try second alternative with initial state.
		llvm_ir_builder.SetInsertPoint(fail_block);
		CreateNextCallRet(llvm_ir_builder, state_ptr, node.next[1]);
	}
	else
	{
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
}

void Generator::BuildNodeFunctionBodyImpl(
	IRBuilder& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::GroupStart& node)
{
	const auto group_ptr=
		llvm_ir_builder.CreateGEP(
			state_ptr,
			{
				GetZeroGEPIndex(),
				GetFieldGEPIndex(StateFieldIndex::GroupsArray),
				GetFieldGEPIndex(group_number_to_field_number_.at(node.index)),
			});

	const auto group_begin_ptr= llvm_ir_builder.CreateGEP(group_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(0)});
	const auto group_end_ptr  = llvm_ir_builder.CreateGEP(group_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(1)});

	const auto str_begin_ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::StrBegin)});
	const auto str_begin_value= llvm_ir_builder.CreateLoad(str_begin_ptr);

	llvm_ir_builder.CreateStore(str_begin_value, group_begin_ptr);
	llvm_ir_builder.CreateStore(str_begin_value, group_end_ptr  );

	CreateNextCallRet(llvm_ir_builder, state_ptr, node.next);
}

void Generator::BuildNodeFunctionBodyImpl(
	IRBuilder& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::GroupEnd& node)
{
	const auto group_end_ptr=
		llvm_ir_builder.CreateGEP(
			state_ptr,
			{
				GetZeroGEPIndex(),
				GetFieldGEPIndex(StateFieldIndex::GroupsArray),
				GetFieldGEPIndex(group_number_to_field_number_.at(node.index)),
				GetFieldGEPIndex(1),
			});

	const auto str_begin_ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::StrBegin)});
	const auto str_begin_value= llvm_ir_builder.CreateLoad(str_begin_ptr);

	llvm_ir_builder.CreateStore(str_begin_value, group_end_ptr);

	CreateNextCallRet(llvm_ir_builder, state_ptr, node.next);
}

void Generator::BuildNodeFunctionBodyImpl(
	IRBuilder& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::BackReference& node)
{
	// Generate here code, equivalent to.
	/*
		const auto group= state.groups[index];
		const auto str_begin= state.str_begin;
		const auto str_end= state.str_end;

		const auto str_size= str_end - str_begin;
		const auto group_size= group.end - group.end;
		if(group_size > str_size)
			return false;

		for(size_t i= 0; i < group_size; ++i)
		{
			if(str_begin[i] != group.begin[i])
				return false;
		}

		state.str_begin+= group_size;
		return MatchNextNode(state);
	*/

	const auto function= llvm_ir_builder.GetInsertBlock()->getParent();

	const auto group_ptr=
		llvm_ir_builder.CreateGEP(
			state_ptr,
			{
				GetZeroGEPIndex(),
				GetFieldGEPIndex(StateFieldIndex::GroupsArray),
				GetFieldGEPIndex(group_number_to_field_number_.at(node.index)),
			});

	const auto str_begin_ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::StrBegin)});
	const auto str_begin_value= llvm_ir_builder.CreateLoad(str_begin_ptr, "str_begin_value");

	const auto str_end_ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::StrEnd)});
	const auto str_end_value= llvm_ir_builder.CreateLoad(str_end_ptr, "str_end_value");

	const auto group_begin_ptr= llvm_ir_builder.CreateGEP(group_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(0)});
	const auto group_end_ptr  = llvm_ir_builder.CreateGEP(group_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(1)});

	const auto group_begin_value= llvm_ir_builder.CreateLoad(group_begin_ptr, "group_begin_value");
	const auto group_end_value  = llvm_ir_builder.CreateLoad(group_end_ptr  , "group_end_value"  );

	const auto str_size= llvm_ir_builder.CreatePtrDiff(str_end_value, str_begin_value, "str_size");
	const auto group_size= llvm_ir_builder.CreatePtrDiff(group_end_value, group_begin_value, "group_size");

	const auto start_block= llvm_ir_builder.GetInsertBlock();
	const auto loop_counter_check_block= llvm::BasicBlock::Create(context_, "loop_counter_check", function);
	const auto loop_body_block= llvm::BasicBlock::Create(context_, "loop_body", function);
	const auto loop_counter_increase_block= llvm::BasicBlock::Create(context_, "loop_counter_increase", function);
	const auto end_block= llvm::BasicBlock::Create(context_, "end", function);
	const auto fail_block= llvm::BasicBlock::Create(context_, "fail", function);

	const auto size_check_condition= llvm_ir_builder.CreateICmpULE(group_size, str_size, "group_size_not_greater_than_str_size");
	llvm_ir_builder.CreateCondBr(size_check_condition, loop_counter_check_block, fail_block);

	// Loop counter check block.
	llvm_ir_builder.SetInsertPoint(loop_counter_check_block);
	const auto loop_counter_current= llvm_ir_builder.CreatePHI(str_size->getType(), 2, "loop_counter_current");
	loop_counter_current->addIncoming(llvm::ConstantInt::getNullValue(str_size->getType()), start_block);

	const auto loop_end_condition= llvm_ir_builder.CreateICmpULT(loop_counter_current, group_size);

	llvm_ir_builder.CreateCondBr(loop_end_condition, loop_body_block, end_block);

	// Loop body block.
	llvm_ir_builder.SetInsertPoint(loop_body_block);
	const auto group_char_ptr= llvm_ir_builder.CreateGEP(group_begin_value, loop_counter_current);
	const auto group_char= llvm_ir_builder.CreateLoad(group_char_ptr, "group_char");

	const auto str_char_ptr= llvm_ir_builder.CreateGEP(str_begin_value, loop_counter_current);
	const auto str_char= llvm_ir_builder.CreateLoad(str_char_ptr, "str_char");

	const auto char_eq= llvm_ir_builder.CreateICmpEQ(group_char, str_char, "char_eq");
	llvm_ir_builder.CreateCondBr(char_eq, loop_counter_increase_block, fail_block);

	// Loop counter increase block.
	llvm_ir_builder.SetInsertPoint(loop_counter_increase_block);
	const auto loop_counter_next=
		llvm_ir_builder.CreateAdd(
			loop_counter_current,
			llvm::ConstantInt::get(loop_counter_current->getType(), 1),
			"loop_counter_next");
	loop_counter_current->addIncoming(loop_counter_next, loop_counter_increase_block);
	llvm_ir_builder.CreateBr(loop_counter_check_block);

	// End block.
	llvm_ir_builder.SetInsertPoint(end_block);
	const auto new_str_begin_value= llvm_ir_builder.CreateGEP(str_begin_value, group_size);
	llvm_ir_builder.CreateStore(new_str_begin_value, str_begin_ptr);
	CreateNextCallRet(llvm_ir_builder, state_ptr, node.next);

	// Fail block.
	llvm_ir_builder.SetInsertPoint(fail_block);
	llvm_ir_builder.CreateRet(llvm::ConstantInt::getFalse(context_));
}

void Generator::BuildNodeFunctionBodyImpl(
	IRBuilder& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::Look& node)
{
	const auto function= llvm_ir_builder.GetInsertBlock()->getParent();

	const auto state_copy_ptr= llvm_ir_builder.CreateAlloca(state_type_, 0, "state_copy");
	if(node.forward)
	{
		CopyState(llvm_ir_builder, state_copy_ptr, state_ptr);
		const auto call_res= llvm_ir_builder.CreateCall(GetOrCreateNodeFunction(node.look_graph), {state_copy_ptr});

		const auto ok_block= llvm::BasicBlock::Create(context_, "ok", function);
		const auto fail_block= llvm::BasicBlock::Create(context_, "fail", function);

		if(node.positive)
			llvm_ir_builder.CreateCondBr(call_res, ok_block, fail_block);
		else
			llvm_ir_builder.CreateCondBr(call_res, fail_block, ok_block);

		// Ok block.
		llvm_ir_builder.SetInsertPoint(ok_block);
		CreateNextCallRet(llvm_ir_builder, state_ptr, node.next);

		// Fail block.
		llvm_ir_builder.SetInsertPoint(fail_block);
		llvm_ir_builder.CreateRet(llvm::ConstantInt::getFalse(context_));
	}
	else
	{
		assert(false && "not implemented yet!");
	}
}

void Generator::BuildNodeFunctionBodyImpl(
	IRBuilder& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::ConditionalElement& node)
{
	const auto function= llvm_ir_builder.GetInsertBlock()->getParent();

	const auto state_copy_ptr= llvm_ir_builder.CreateAlloca(state_type_, 0, "state_copy");

	CopyState(llvm_ir_builder, state_copy_ptr, state_ptr);
	const auto call_res= llvm_ir_builder.CreateCall(GetOrCreateNodeFunction(node.condition_node), {state_copy_ptr});

	const auto true_block = llvm::BasicBlock::Create(context_, "true_block" , function);
	const auto false_block= llvm::BasicBlock::Create(context_, "false_block", function);
	llvm_ir_builder.CreateCondBr(call_res, true_block, false_block);

	// True block.
	llvm_ir_builder.SetInsertPoint(true_block );
	CreateNextCallRet(llvm_ir_builder, state_ptr, node.next_true);

	// False block.
	llvm_ir_builder.SetInsertPoint(false_block);
	CreateNextCallRet(llvm_ir_builder, state_ptr, node.next_false);
}

void Generator::BuildNodeFunctionBodyImpl(
	IRBuilder& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::SequenceCounterReset& node)
{
	const auto counter_ptr=
		llvm_ir_builder.CreateGEP(
			state_ptr,
			{
				GetZeroGEPIndex(),
				GetFieldGEPIndex(StateFieldIndex::SequenceContersArray),
				GetFieldGEPIndex(sequence_id_to_counter_filed_number_.at(node.id)),
			});

	llvm_ir_builder.CreateStore(llvm::ConstantInt::getNullValue(ptr_size_int_type_), counter_ptr);

	CreateNextCallRet(llvm_ir_builder, state_ptr, node.next);
}

void Generator::BuildNodeFunctionBodyImpl(
	IRBuilder& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::SequenceCounter& node)
{
	const auto function= llvm_ir_builder.GetInsertBlock()->getParent();

	const auto state_copy_ptr= llvm_ir_builder.CreateAlloca(state_type_, 0, "state_copy");

	const auto counter_ptr=
		llvm_ir_builder.CreateGEP(
			state_ptr,
			{
				GetZeroGEPIndex(),
				GetFieldGEPIndex(StateFieldIndex::SequenceContersArray),
				GetFieldGEPIndex(sequence_id_to_counter_filed_number_.at(node.id)),
			});

	const auto counter_value= llvm_ir_builder.CreateLoad(counter_ptr, "counter_value");
	const auto counter_value_next=
		llvm_ir_builder.CreateAdd(counter_value, GetConstant(ptr_size_int_type_, 1), "counter_value_next");
	llvm_ir_builder.CreateStore(counter_value_next, counter_ptr);

	if(node.min_elements > 0)
	{
		const auto less=
			llvm_ir_builder.CreateICmpULT(
				counter_value,
				GetConstant(ptr_size_int_type_, node.min_elements),
				"less");

		const auto less_block= llvm::BasicBlock::Create(context_, "less", function);
		const auto next_block= llvm::BasicBlock::Create(context_, "", function);

		llvm_ir_builder.CreateCondBr(less, less_block, next_block);

		llvm_ir_builder.SetInsertPoint(less_block);
		CreateNextCallRet(llvm_ir_builder, state_ptr, node.next_iteration);

		llvm_ir_builder.SetInsertPoint(next_block);
	}
	if(node.max_elements < Sequence::c_max)
	{
		const auto greater_equal=
			llvm_ir_builder.CreateICmpUGE(
				counter_value,
				GetConstant(ptr_size_int_type_, node.max_elements),
				"greater_equal");

		const auto greater_equal_block= llvm::BasicBlock::Create(context_, "greater_equal", function);
		const auto next_block= llvm::BasicBlock::Create(context_, "", function);

		llvm_ir_builder.CreateCondBr(greater_equal, greater_equal_block, next_block);

		llvm_ir_builder.SetInsertPoint(greater_equal_block);
		CreateNextCallRet(llvm_ir_builder, state_ptr, node.next_sequence_end);

		llvm_ir_builder.SetInsertPoint(next_block);
	}

	GraphElements::NodePtr branches[2];
	if(node.greedy)
	{
		branches[0]= node.next_iteration;
		branches[1]= node.next_sequence_end;
	}
	else
	{
		branches[0]= node.next_sequence_end;
		branches[1]= node.next_iteration;
	}

	CopyState(llvm_ir_builder, state_copy_ptr, state_ptr);
	const auto first_call_res= llvm_ir_builder.CreateCall(GetOrCreateNodeFunction(branches[0]), {state_copy_ptr});

	const auto ok_block= llvm::BasicBlock::Create(context_, "", function);
	const auto next_block= llvm::BasicBlock::Create(context_, "", function);

	llvm_ir_builder.CreateCondBr(first_call_res, ok_block, next_block);

	llvm_ir_builder.SetInsertPoint(ok_block);
	CopyState(llvm_ir_builder, state_ptr, state_copy_ptr);
	llvm_ir_builder.CreateRet(llvm::ConstantInt::getTrue(context_));

	llvm_ir_builder.SetInsertPoint(next_block);
	CreateNextCallRet(llvm_ir_builder, state_ptr, branches[1]);
}

void Generator::BuildNodeFunctionBodyImpl(
	IRBuilder& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::NextWeakNode& node)
{
	const auto next= node.next.lock();
	assert(next != nullptr);
	CreateNextCallRet(llvm_ir_builder, state_ptr, next);
}

void Generator::BuildNodeFunctionBodyImpl(
	IRBuilder& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::PossessiveSequence& node)
{
	const auto function= llvm_ir_builder.GetInsertBlock()->getParent();

	const auto state_copy_ptr= llvm_ir_builder.CreateAlloca(state_type_, 0, "state_copy");

	const auto counter_value_initial= llvm::ConstantInt::getNullValue(ptr_size_int_type_);

	const auto counter_check_block= llvm::BasicBlock::Create(context_, "counter_check", function);
	const auto iteration_block= llvm::BasicBlock::Create(context_, "iteration", function);
	const auto end_block= llvm::BasicBlock::Create(context_, "end");

	const auto start_block= llvm_ir_builder.GetInsertBlock();
	llvm_ir_builder.CreateBr(counter_check_block);

	// Counter check block.
	llvm_ir_builder.SetInsertPoint(counter_check_block);

	const auto counter_value_current= llvm_ir_builder.CreatePHI(ptr_size_int_type_, 2, "counter_value_current");
	counter_value_current->addIncoming(counter_value_initial, start_block);
	if(node.max_elements < Sequence::c_max)
	{
		const auto loop_end_condition=
			llvm_ir_builder.CreateICmpULT(
				counter_value_current,
				GetConstant(ptr_size_int_type_, node.max_elements),
				"less");
		llvm_ir_builder.CreateCondBr(loop_end_condition, iteration_block, end_block);
	}
	else
		llvm_ir_builder.CreateBr(iteration_block);

	// Iteration block.
	llvm_ir_builder.SetInsertPoint(iteration_block);

	CopyState(llvm_ir_builder, state_copy_ptr, state_ptr);
	const auto call_res= llvm_ir_builder.CreateCall(GetOrCreateNodeFunction(node.sequence_element), {state_copy_ptr});

	const auto ok_block= llvm::BasicBlock::Create(context_, "ok", function);
	const auto fail_block= llvm::BasicBlock::Create(context_, "fail", function);

	llvm_ir_builder.CreateCondBr(call_res, ok_block, fail_block);

	// Ok block.
	llvm_ir_builder.SetInsertPoint(ok_block);

	const auto counter_value_next=
		llvm_ir_builder.CreateAdd(counter_value_current, GetConstant(ptr_size_int_type_, 1), "counter_value_next");
	counter_value_current->addIncoming(counter_value_next, ok_block);

	CopyState(llvm_ir_builder, state_ptr, state_copy_ptr);
	llvm_ir_builder.CreateBr(counter_check_block);

	// Fail block.
	llvm_ir_builder.SetInsertPoint(fail_block);

	if(node.min_elements > 0)
	{
		const auto not_enough_elements_condition=
			llvm_ir_builder.CreateICmpULT(counter_value_current, GetConstant(ptr_size_int_type_, node.min_elements), "less_than_needed");

		const auto ret_false_block= llvm::BasicBlock::Create(context_, "ret_false", function);
		llvm_ir_builder.CreateCondBr(not_enough_elements_condition, ret_false_block, end_block);

		// Ret false block.
		llvm_ir_builder.SetInsertPoint(ret_false_block);
		llvm_ir_builder.CreateRet(llvm::ConstantInt::getFalse(context_));
	}
	else
		llvm_ir_builder.CreateBr(end_block);

	// End block.
	end_block->insertInto(function);
	llvm_ir_builder.SetInsertPoint(end_block);
	CreateNextCallRet(llvm_ir_builder, state_ptr, node.next);
}

void Generator::BuildNodeFunctionBodyImpl(
	IRBuilder& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::AtomicGroup& node)
{
	const auto function= llvm_ir_builder.GetInsertBlock()->getParent();

	const auto call_res= llvm_ir_builder.CreateCall(GetOrCreateNodeFunction(node.group_element), {state_ptr});

	const auto ok_block= llvm::BasicBlock::Create(context_, "ok", function);
	const auto fail_block= llvm::BasicBlock::Create(context_, "fail", function);
	llvm_ir_builder.CreateCondBr(call_res, ok_block, fail_block);

	// Ok block.
	llvm_ir_builder.SetInsertPoint(ok_block);
	CreateNextCallRet(llvm_ir_builder, state_ptr, node.next);

	// Fail block.
	llvm_ir_builder.SetInsertPoint(fail_block);
	llvm_ir_builder.CreateRet(llvm::ConstantInt::getFalse(context_));
}

void Generator::BuildNodeFunctionBodyImpl(
	IRBuilder& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::SubroutineEnter& node)
{
	const auto subroutine_call_return_chain_node=
		llvm_ir_builder.CreateAlloca(subroutine_call_return_chain_node_type_, 0, "subroutine_call_return_chain_node");

	const auto next_function= GetOrCreateNodeFunction(node.next);
	llvm_ir_builder.CreateStore(
		next_function,
		llvm_ir_builder.CreateGEP(
			subroutine_call_return_chain_node,
			{GetZeroGEPIndex(), GetFieldGEPIndex(SubroutineCallReturnChainNodeFieldIndex::NextFunction)}));

	const auto prev_node_ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::SubroutineCallReturnChainHead)});
	const auto prev_node_value= llvm_ir_builder.CreateLoad(prev_node_ptr);

	llvm_ir_builder.CreateStore(
		prev_node_value,
		llvm_ir_builder.CreateGEP(
			subroutine_call_return_chain_node,
			{GetZeroGEPIndex(), GetFieldGEPIndex(SubroutineCallReturnChainNodeFieldIndex::Prev)}));

	llvm_ir_builder.CreateStore(subroutine_call_return_chain_node, prev_node_ptr);

	CreateNextCallRet(llvm_ir_builder, state_ptr, node.subroutine_node);
}

void Generator::BuildNodeFunctionBodyImpl(
	IRBuilder& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::SubroutineLeave& node)
{
	(void)node;

	const auto node_ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::SubroutineCallReturnChainHead)});
	const auto node_value= llvm_ir_builder.CreateLoad(node_ptr);

	const auto next_function_ptr=
		llvm_ir_builder.CreateGEP(
			node_value,
			{GetZeroGEPIndex(), GetFieldGEPIndex(SubroutineCallReturnChainNodeFieldIndex::NextFunction)});
	const auto next_function= llvm_ir_builder.CreateLoad(next_function_ptr);

	const auto prev_node_ptr=
		llvm_ir_builder.CreateGEP(
			node_value,
			{GetZeroGEPIndex(), GetFieldGEPIndex(SubroutineCallReturnChainNodeFieldIndex::Prev)});
	const auto prev_node_value= llvm_ir_builder.CreateLoad(prev_node_ptr);

	llvm_ir_builder.CreateStore(prev_node_value, node_ptr);

	const auto call_res= llvm_ir_builder.CreateCall(next_function, {state_ptr});
	llvm_ir_builder.CreateRet(call_res);
}

void Generator::BuildNodeFunctionBodyImpl(
	IRBuilder& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::StateSave& node)
{
	// Allocate node.
	const auto subroutine_state_save_chain_node=
		llvm_ir_builder.CreateAlloca(subroutine_call_state_save_chain_node_type_, 0, "subroutine_state_save_chain_node");

	// Save prev value.
	const auto prev_node_ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::SubroutineCallStateSaveChainHead)});
	const auto prev_node_value= llvm_ir_builder.CreateLoad(prev_node_ptr);

	llvm_ir_builder.CreateStore(
		prev_node_value,
		llvm_ir_builder.CreateGEP(
			subroutine_state_save_chain_node,
			{GetZeroGEPIndex(), GetFieldGEPIndex(SubroutineCallStateSaveChainNodeFieldIndex::Prev)}));

	llvm_ir_builder.CreateStore(subroutine_state_save_chain_node, prev_node_ptr);

	// Save payload.
	for(const GraphElements::SequenceId sequence_id : node.sequence_counters_to_save)
	{
		const auto sequence_field_index= GetFieldGEPIndex(sequence_id_to_counter_filed_number_.at(sequence_id));

		const auto sequence_counter_ptr=
			llvm_ir_builder.CreateGEP(
				state_ptr,
				{
					GetZeroGEPIndex(),
					GetFieldGEPIndex(StateFieldIndex::SequenceContersArray),
					sequence_field_index,
				});

		const auto sequence_counter= llvm_ir_builder.CreateLoad(sequence_counter_ptr, "sequence_counter");

		llvm_ir_builder.CreateStore(
			sequence_counter,
			llvm_ir_builder.CreateGEP(
				subroutine_state_save_chain_node,
				{
					GetZeroGEPIndex(),
					GetFieldGEPIndex(SubroutineCallStateSaveChainNodeFieldIndex::SequenceContersArray),
					sequence_field_index,
				}));
	}
	for(const size_t group_id : node.groups_to_save)
	{
		const auto group_index= GetFieldGEPIndex(group_number_to_field_number_.at(group_id));

		const auto group_ptr_src=
			llvm_ir_builder.CreateGEP(
				state_ptr,
				{
					GetZeroGEPIndex(),
					GetFieldGEPIndex(StateFieldIndex::GroupsArray),
					group_index,
				},
				"group_ptr_src");

		const auto group_ptr_dst=
			llvm_ir_builder.CreateGEP(
				subroutine_state_save_chain_node,
				{
					GetZeroGEPIndex(),
					GetFieldGEPIndex(SubroutineCallStateSaveChainNodeFieldIndex::GroupsArray),
					group_index,
				},
				"group_ptr_dst");

		const llvm::DataLayout& data_layout= module_.getDataLayout();

		const auto alignment= data_layout.getABITypeAlignment(group_type_); // TODO - is this right alignment?

		llvm_ir_builder.CreateMemCpy(
			group_ptr_dst, alignment,
			group_ptr_src, alignment,
			GetConstant(gep_index_type_, data_layout.getTypeAllocSize(group_type_)));
	}

	CreateNextCallRet(llvm_ir_builder, state_ptr, node.next);
}

void Generator::BuildNodeFunctionBodyImpl(
	IRBuilder& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::StateRestore& node)
{
	const auto node_ptr= llvm_ir_builder.CreateGEP(state_ptr, {GetZeroGEPIndex(), GetFieldGEPIndex(StateFieldIndex::SubroutineCallStateSaveChainHead)});
	const auto node_value= llvm_ir_builder.CreateLoad(node_ptr);

	// Restore prev chain state.
	const auto prev_node_ptr=
		llvm_ir_builder.CreateGEP(
			node_value,
			{GetZeroGEPIndex(), GetFieldGEPIndex(SubroutineCallStateSaveChainNodeFieldIndex::Prev)});
	const auto prev_node_value= llvm_ir_builder.CreateLoad(prev_node_ptr);

	llvm_ir_builder.CreateStore(prev_node_value, node_ptr);

	// Restore payload.
	for(const GraphElements::SequenceId sequence_id : node.sequence_counters_to_restore)
	{
		const auto sequence_field_index= GetFieldGEPIndex(sequence_id_to_counter_filed_number_.at(sequence_id));

		const auto sequence_counter_ptr=
			llvm_ir_builder.CreateGEP(
				node_value,
				{
					GetZeroGEPIndex(),
					GetFieldGEPIndex(SubroutineCallStateSaveChainNodeFieldIndex::SequenceContersArray),
					sequence_field_index,
				});

		const auto sequence_counter= llvm_ir_builder.CreateLoad(sequence_counter_ptr, "sequence_counter");

		llvm_ir_builder.CreateStore(
			sequence_counter,
			llvm_ir_builder.CreateGEP(
				state_ptr,
				{
					GetZeroGEPIndex(),
					GetFieldGEPIndex(StateFieldIndex::SequenceContersArray),
					sequence_field_index,
				}));
	}
	for(const size_t group_id : node.groups_to_restore)
	{
		const auto group_index= GetFieldGEPIndex(group_number_to_field_number_.at(group_id));

		const auto group_ptr_src=
			llvm_ir_builder.CreateGEP(
				node_value,
				{
					GetZeroGEPIndex(),
					GetFieldGEPIndex(SubroutineCallStateSaveChainNodeFieldIndex::GroupsArray),
					group_index,
				},
				"group_ptr_src");

		const auto group_ptr_dst=
			llvm_ir_builder.CreateGEP(
				state_ptr,
				{
					GetZeroGEPIndex(),
					GetFieldGEPIndex(StateFieldIndex::GroupsArray),
					group_index,
				},
				"group_ptr_dst");

		const llvm::DataLayout& data_layout= module_.getDataLayout();

		const auto alignment= data_layout.getABITypeAlignment(group_type_); // TODO - is this right alignment?

		llvm_ir_builder.CreateMemCpy(
			group_ptr_dst, alignment,
			group_ptr_src, alignment,
			GetConstant(gep_index_type_, data_layout.getTypeAllocSize(group_type_)));
	}

	CreateNextCallRet(llvm_ir_builder, state_ptr, node.next);
}

void Generator::CreateNextCallRet(
	IRBuilder& llvm_ir_builder, llvm::Value* const state_ptr, const GraphElements::NodePtr& next_node)
{
	const auto next_call= llvm_ir_builder.CreateCall(GetOrCreateNodeFunction(next_node), {state_ptr}, "next_call_res");
	llvm_ir_builder.CreateRet(next_call);
}

void Generator::CopyState(IRBuilder& llvm_ir_builder, llvm::Value* const dst, llvm::Value* const src)
{
	const llvm::DataLayout& data_layout= module_.getDataLayout();

	const auto alignment= data_layout.getABITypeAlignment(state_type_); // TODO - is this right alignment?

	llvm_ir_builder.CreateMemCpy(
		dst, alignment,
		src, alignment,
		GetConstant(gep_index_type_, data_layout.getTypeAllocSize(state_type_)));
}

llvm::ConstantInt* Generator::GetConstant(llvm::IntegerType* const type, const uint64_t value) const
{
	return llvm::ConstantInt::get(type, value);
}

llvm::Constant* Generator::GetZeroGEPIndex() const
{
	return llvm::Constant::getNullValue(gep_index_type_);
}

llvm::Constant* Generator::GetFieldGEPIndex(const uint32_t field_index) const
{
	return GetConstant(gep_index_type_, field_index);
}

} // namespace

void GenerateMatcherFunction(
	llvm::Module& module, const RegexGraphBuildResult& regex_graph, const std::string& function_name)
{
	Generator generator(module);
	generator.GenerateMatcherFunction(regex_graph, function_name);
}

} // namespace RegPanzer
