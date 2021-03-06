/*
 *
 * Turbo Basic Compiler by mgr_inz_rafal.
 *
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <rchabowski@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 *														// mgr inz. Rafal
 * ----------------------------------------------------------------------------
 */
#pragma once

#include <string>
#include <map>

class token_provider
{
public:
	enum class TOKENS
	{
		PROGRAM_START,
		PROGRAM_END,
		EXPRESSION_STACK,
		EXPRESSION_STACK_PTR,
		RETURN_ADDRESS_STACK,
		RETURN_ADDRESS_STACK_PTR,
		FOR_CONDITION_STACK,
		FOR_CONDITION_STACK_PTR,
		FOR_COUNTER_STACK,
		FOR_COUNTER_STACK_PTR,
		FOR_STEP_STACK,
		FOR_STEP_STACK_PTR,
		INTEGER,
		PUSH_POP_PTR_TO_INC_DEC,
		PUSH_POP_VALUE_PTR,
		PUSH_POP_TARGET_STACK_PTR,
		LINE_INDICATOR,
		VARIABLE,
		AFTER_IF_INDICATOR,
		INSIDE_IF_INDICATOR,
		GENERIC_LABEL,
		WHILE_INDICATOR,
		AFTER_WHILE_INDICATOR,
		REPEAT_INDICATOR,
		AFTER_REPEAT_INDICATOR,
		DO_INDICATOR,
		AFTER_DO_INDICATOR,
		PROCEDURE,
		INTEGER_ARRAY
	};

private:
	const std::string TOKEN_INDICATOR = "___TUBAC___";
	std::map<TOKENS, std::string> TOKEN_MAP = {
		{ TOKENS::PROGRAM_START,			make_token("PROGRAM_START") },
		{ TOKENS::PROGRAM_END,				make_token("PROGRAM_ENDS_HERE") },
		{ TOKENS::EXPRESSION_STACK,			make_token("EXPRESSION_STACK") },
		{ TOKENS::EXPRESSION_STACK_PTR,		make_token("EXPRESSION_STACK_PTR") },
		{ TOKENS::RETURN_ADDRESS_STACK,		make_token("RETURN_ADDRESS_STACK") },
		{ TOKENS::RETURN_ADDRESS_STACK_PTR,	make_token("RETURN_ADDRESS_STACK_PTR") },
		{ TOKENS::FOR_CONDITION_STACK,		make_token("FOR_CONDITION_STACK") },
		{ TOKENS::FOR_CONDITION_STACK_PTR,	make_token("FOR_CONDITION_STACK_PTR") },
		{ TOKENS::FOR_COUNTER_STACK,		make_token("FOR_COUNTER_STACK") },
		{ TOKENS::FOR_COUNTER_STACK_PTR,	make_token("FOR_COUNTER_STACK_PTR") },
		{ TOKENS::FOR_STEP_STACK,			make_token("FOR_STEP_STACK") },
		{ TOKENS::FOR_STEP_STACK_PTR,		make_token("FOR_STEP_STACK_PTR") },
		{ TOKENS::INTEGER,					make_token("INTEGER_") },
		{ TOKENS::PUSH_POP_PTR_TO_INC_DEC,	make_token("PUSH_POP_PTR_TO_INC_DEC") },
		{ TOKENS::PUSH_POP_VALUE_PTR,		make_token("PUSH_POP_VALUE_PTR") },
		{ TOKENS::PUSH_POP_TARGET_STACK_PTR,make_token("PUSH_POP_TARGET_STACK_PTR") },
		{ TOKENS::LINE_INDICATOR,			make_token("LINE_NUMBER__") },
		{ TOKENS::VARIABLE,					make_token("VARIABLE_") },
		{ TOKENS::AFTER_IF_INDICATOR,		make_token("AFTER_IF_") },
		{ TOKENS::INSIDE_IF_INDICATOR,		make_token("INSIDE_IF_") },
		{ TOKENS::GENERIC_LABEL,			make_token("GENERIC_LABEL_") },
		{ TOKENS::WHILE_INDICATOR,			make_token("WHILE_INDICATOR_") },
		{ TOKENS::AFTER_WHILE_INDICATOR,	make_token("AFTER_WHILE_INDICATOR_") },
		{ TOKENS::REPEAT_INDICATOR,			make_token("REPEAT_INDICATOR_") },
		{ TOKENS::AFTER_REPEAT_INDICATOR,	make_token("AFTER_REPEAT_INDICATOR_") },
		{ TOKENS::DO_INDICATOR,				make_token("DO_INDICATOR_") },
		{ TOKENS::AFTER_DO_INDICATOR,		make_token("AFTER_DO_INDICATOR_") },
		{ TOKENS::PROCEDURE,				make_token("PROCEDURE_") },
		{ TOKENS::INTEGER_ARRAY,			make_token("INTEGER_ARRAY_") }
	};

	std::string make_token(const std::string& name) const;

public:
	const std::string& get(TOKENS token) const;
};
