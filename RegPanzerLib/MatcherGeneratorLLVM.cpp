#include "MatcherGeneratorLLVM.hpp"
#include "PushDisableLLVMWarnings.hpp"
#include <llvm/IR/IRBuilder.h>
#include "PopLLVMWarnings.hpp"

namespace RegPanzer
{

namespace
{

class Generator
{
public:
	explicit Generator(llvm::Module& module)
		: context_(module.getContext())
		, module_(module)
		, char_type_(llvm::Type::getInt8Ty(context_))
		, char_type_ptr_(llvm::PointerType::get(char_type_, 0))
	{}

	void GenerateMatcherFunction(
		const GraphElements::NodePtr& regex_graph_root,
		const std::string& function_name)
	{
		// Body of state struct depends on actual regex.
		state_type_= llvm::StructType::create(context_, "State");
		state_type_->setBody({ char_type_ptr_, char_type_ptr_});

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

		const auto state_ptr= llvm_ir_builder.CreateAlloca(state_type_);

		const auto match_res= llvm_ir_builder.CreateCall(GetOrCreateNodeFunction(regex_graph_root), { state_ptr });
		llvm_ir_builder.CreateCondBr(match_res, found_block, not_found_block);

		llvm_ir_builder.SetInsertPoint(found_block);
		llvm_ir_builder.CreateRet(llvm::ConstantPointerNull::get(char_type_ptr_));

		llvm_ir_builder.SetInsertPoint(not_found_block);
		llvm_ir_builder.CreateRet(llvm::ConstantPointerNull::get(char_type_ptr_));
	}

private:

	llvm::Function* GetOrCreateNodeFunction(const GraphElements::NodePtr& node)
	{
		if(const auto it= node_functions_.find(node); it != node_functions_.end())
			return it->second;

		const auto function= llvm::Function::Create(node_function_type_, llvm::GlobalValue::ExternalLinkage, "", module_); // TODO - set name
		node_functions_.emplace(node, function);
		BuildNodeFunctionBody(node, function);
		return function;
	}

	void BuildNodeFunctionBody(const GraphElements::NodePtr& node, llvm::Function* const function)
	{
		// TODO
		(void)node;
		(void)function;
	}

private:
	llvm::LLVMContext& context_;
	llvm::Module& module_;

	llvm::IntegerType* const char_type_;
	llvm::PointerType* const char_type_ptr_;

	llvm::StructType* state_type_= nullptr;
	llvm::FunctionType* node_function_type_= nullptr;

	std::unordered_map<GraphElements::NodePtr, llvm::Function*> node_functions_;
};


} // namespace

void GenerateMatcherFunction(
	llvm::Module& module,
	const GraphElements::NodePtr& regex_graph_root,
	const std::string& function_name)
{
	Generator generator(module);
	generator.GenerateMatcherFunction(regex_graph_root, function_name);
}

} // namespace RegPanzer
