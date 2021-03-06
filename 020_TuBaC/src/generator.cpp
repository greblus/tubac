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

#include <boost/format.hpp>

#include <sstream>

#include "generator.h"
#include "synthesizer.h"
#include "algorithm.h"

generator::generator(std::ostream& _stream, const config& _cfg):
	cfg(_cfg),
	E_(cfg.get_endline()), 
	pokey_initialized(false),
	synth(_stream, _cfg.get_indent(), E_)
{
	loop_context.push(LOOP_CONTEXT::OUTSIDE);

	write_atari_registers();
	write_atari_constants();
	write_code_header();
	write_internal_variables();
	
	register_generator_runtime();
}

generator::~generator()
{
	write_code_footer();
	write_runtime();
	write_run_segment();
}

const std::string& generator::token(const token_provider::TOKENS& token) const
{
	return cfg.get_token_provider().get(token);
}

void generator::write_code_header() const {
	synth.synth(false) << token(token_provider::TOKENS::PROGRAM_START) << " equ $" << std::hex << PROGRAM_START << std::dec << E_;
	synth.synth() << "org " << token(token_provider::TOKENS::PROGRAM_START) << E_;
	synth.synth(false) << ".zpvar = $" << std::hex << ZERO_PAGE_START << std::dec << E_;
	
	synth.synth() << "mva #10 PTABW" << E_;

	write_stacks_initialization();
}

void generator::write_stacks() const
{
	for(const auto& s: stacks)
	{
		synth.synth() << "; STACK: " << s.second.get_name() << E_;
		synth.synth(false) << s.second.get_name() << E_;
		synth.synth(false) << ':' << s.second.get_capacity() << cfg.get_indent() << "dta ";
		call_n(cfg.get_number_interpretation()->get_size()-1, [&]{synth.synth(false) << "b(0),";});
		synth.synth(false) << "b(0)" << E_;
	}
}

void generator::write_stacks_initialization() const {
	for (const auto& s : stacks)
	{
		spawn_compiler_variable(s.second.get_pointer(), true);
		init_pointer(s.second.get_pointer(), s.second.get_name());
	}
}

void generator::write_run_segment() const
{
	// Synth run address
	synth.synth() << "org RUNAD" << E_;
	synth.synth() << "dta a(" << token(token_provider::TOKENS::PROGRAM_START) << ')' << E_;
}

void generator::write_code_footer()
{
	// Infinite loop at the end so the processor
	// won't fall into wilderness
	synth.synth(false) << token(token_provider::TOKENS::PROGRAM_END) << " jmp " << token(token_provider::TOKENS::PROGRAM_END) << E_;

	// Prepare internal data and structures
	write_integers();
	write_variables();
	write_stacks();
}

void generator::write_integers()
{
	synth.synth(false) << "; Fixed integers" << E_;

	for (auto& i : integers)
	{
		synth.synth(false) << token(token_provider::TOKENS::INTEGER);
		if ('-' == i[0])
		{
			synth.synth(false) << "NEG_" << i.substr(1);
		}
		else
		{
			synth.synth(false) << i;
		}
		synth.synth(false) << E_;
		synth.synth() << cfg.get_number_interpretation()->get_initializer(i) << E_;
	}
}

void generator::write_variables()
{
	synth.synth(false) << "; Variables" << E_;

	for (auto& i : variables)
	{
		synth.synth(false) << token(token_provider::TOKENS::VARIABLE) << i << E_;
		synth.synth() << cfg.get_number_interpretation()->get_initializer() << E_;
	}
}

void generator::new_integer(const std::string& i)
{
	integers.insert(i);
}

void generator::new_variable(const std::string& v)
{
	variables.insert(v);
}

void generator::new_line(const int& i) const
{
	synth.synth(false) << token(token_provider::TOKENS::LINE_INDICATOR) << i << E_;
}

void generator::write_atari_registers() const
{
	synth.synth(false) << "; ATARI registers" << E_;
	for (const auto& r : ATARI_REGISTERS)
	{
		synth.synth(false) << r.first << " equ $" << std::hex << r.second <<std::dec << E_;
	}
}

void generator::write_atari_constants() const
{
	synth.synth(false) << "; ATARI constants" << E_;
	for (const auto& r : ATARI_CONSTANTS)
	{
		synth.synth(false) << r.first << " equ $" << std::hex << r.second << std::dec << E_;
	}
}

void generator::put_integer_on_stack(const std::string& i) const {
	synth.synth(false) << "; Put integer '" << i << "' on stack" << E_;

	synth.synth() << "mwa #" << i << " FR0" << E_;
	push_from("FR0");
}

void generator::pop_to(const std::string& target, const generator::STACK& stack) const {
	synth.synth(false) << "; Pop from stack (" << stacks.at(stack).get_name() << ") into '" << target << '\'' << E_;
	
	// Do the pop
	synth.synth() << "mwa #" << target << ' ' << token(token_provider::TOKENS::PUSH_POP_VALUE_PTR) << E_;
	synth.synth() << "mwa #" << stacks.at(stack).get_pointer() << ' ' << token(token_provider::TOKENS::PUSH_POP_PTR_TO_INC_DEC) << E_;
	synth.synth() << "jsr POP_TO" << E_;
}

void generator::peek_to(const std::string& target, const generator::STACK& stack) const {
	synth.synth(false) << "; Peek from stack (" << stacks.at(stack).get_name() << ") into '" << target << '\'' << E_;

	// Do the pop
	synth.synth() << "mwa #" << target << ' ' << token(token_provider::TOKENS::PUSH_POP_VALUE_PTR) << E_;
	synth.synth() << "mwa #" << stacks.at(stack).get_pointer() << ' ' << token(token_provider::TOKENS::PUSH_POP_PTR_TO_INC_DEC) << E_;
	synth.synth() << "jsr PEEK_TO" << E_;
}

void generator::pop_to_variable(const std::string& target) const {
	synth.synth(false) << "; Pop from stack into variable '" << target << '\'' << E_;
	pop_to(token(token_provider::TOKENS::VARIABLE) + target);
}

void generator::push_from(const std::string& source, const generator::STACK& stack) const {
	synth.synth(false) << "; Push from '" << source << "' to stack (" << stacks.at(stack).get_name() << ')' << E_;

	// Do the push
	synth.synth() << "mwa #" << source << ' ' << token(token_provider::TOKENS::PUSH_POP_VALUE_PTR) << E_;
	synth.synth() << "mwa #" << stacks.at(stack).get_pointer() << ' ' << token(token_provider::TOKENS::PUSH_POP_PTR_TO_INC_DEC) << E_;
	synth.synth() << "jsr PUSH_FROM" << E_;
}

void generator::push_from_variable(const std::string& source) const {
	synth.synth(false) << "; Push from variable '" << source << "\' into stack" << E_;
	push_from(token(token_provider::TOKENS::VARIABLE) + source);
}

void generator::write_internal_variables() const {
	spawn_compiler_variable(token(token_provider::TOKENS::PUSH_POP_TARGET_STACK_PTR), true);
	spawn_compiler_variable(token(token_provider::TOKENS::PUSH_POP_PTR_TO_INC_DEC), true);
	spawn_compiler_variable(token(token_provider::TOKENS::PUSH_POP_VALUE_PTR), true);
}

void generator::spawn_compiler_variable(const std::string& name, bool zero_page) const {
	synth.synth(false) << "; Creating compiler variable '" << name << '\'';
	if (zero_page)
	{
		synth.synth(false) << " on ZP";
	}
	synth.synth(false) << E_;

	synth.synth(false) << "." <<
		(zero_page ? "zp" : "")
		<< "var " << name << " .word" << E_;
}

void generator::init_pointer(const std::string& name, const std::string& source) const {
	synth.synth(false) << "; Init pointer '" << name << "' with address of '" << source << '\'' << E_;
	synth.synth() << "lda <" << source << E_;
	synth.synth() << "sta " << name << E_;
	synth.synth() << "lda >" << source << E_;
	synth.synth() << "sta " << name << "+1" << E_;
}

void generator::addition() const {
	synth.synth(false) << "; Execute addition (FR0 + FR1). Result stored in FR0" << E_;
	synth.synth() << "jsr BADD" << E_;
}

void generator::subtraction() const {
	synth.synth(false) << "; Execute subtraction (FR0 - FR1). Result stored in FR0" << E_;
	synth.synth() << "jsr BSUB" << E_;
}

void generator::multiplication() const {
	synth.synth(false) << "; Execute multiplication (FR0 * FR1). Result stored in FR0" << E_;
	synth.synth() << "jsr BMUL" << E_;
}

void generator::division() const {
	synth.synth(false) << "; Execute division (FR0 / FR1). Result stored in FR0" << E_;
	synth.synth() << "jsr BDIV" << E_;
}

void generator::logical_and() const {
	synth.synth(false) << "; Execute logical and (FR0 AND FR1). Result stored in FR0" << E_;
	synth.synth() << "jsr LOGICAL_AND" << E_;
}

void generator::logical_or() const {
	synth.synth(false) << "; Execute logical or (FR0 OR FR1). Result stored in FR0" << E_;
	synth.synth() << "jsr LOGICAL_OR" << E_;
}

void generator::binary_xor() const {
	synth.synth(false) << "; Execute binary exclusive or (FR0 EXOR FR1). Result stored in FR0" << E_;
	synth.synth() << "jsr BINARY_XOR" << E_;
}

void generator::binary_and() const {
	synth.synth(false) << "; Execute binary and (FR0 & FR1). Result stored in FR0" << E_;
	synth.synth() << "jsr BINARY_AND" << E_;
}

void generator::binary_or() const {
	synth.synth(false) << "; Execute binary or (FR0 ! FR1). Result stored in FR0" << E_;
	synth.synth() << "jsr BINARY_OR" << E_;
}

void generator::compare_equal() const {
	synth.synth(false) << "; Comparing FR0 and FR1 for equality" << E_;
	synth.synth() << "lda #0" << E_;
	synth.synth() << "sta INTEGER_COMPARE_TMP" << E_;
	synth.synth() << "jsr COMPARE_FR0_FR1" << E_;
}

void generator::compare_less() const {
	synth.synth(false) << "; Comparing FR0 and FR1 for less" << E_;
	synth.synth() << "lda #1" << E_;
	synth.synth() << "sta INTEGER_COMPARE_TMP" << E_;
	synth.synth() << "jsr COMPARE_FR0_FR1" << E_;
}

void generator::compare_greater() const {
	synth.synth(false) << "; Comparing FR0 and FR1 for greater" << E_;
	synth.synth() << "lda #-1" << E_;
	synth.synth() << "sta INTEGER_COMPARE_TMP" << E_;
	synth.synth() << "jsr COMPARE_FR0_FR1" << E_;
}

void generator::assign_to_array(const std::string& a) const {
	synth.synth() << "mwa #" << token(token_provider::TOKENS::INTEGER_ARRAY) << a << "+4 ARRAY_ASSIGNMENT_TMP_ADDRESS" << E_;
	synth.synth() << "mwa " << token(token_provider::TOKENS::INTEGER_ARRAY) << a << " ARRAY_ASSIGNMENT_TMP_SIZE" << E_;
	synth.synth() << "jsr INIT_ARRAY_OFFSET" << E_;
	synth.synth() << "ldy #" << cfg.get_number_interpretation()->get_size()-1 << E_;
	synth.synth(false) << "@" << E_;
	synth.synth() << "lda ARRAY_ASSIGNMENT_TMP_VALUE,y" << E_;
	synth.synth() << "sta (ARRAY_ASSIGNMENT_TMP_ADDRESS),y" << E_;
	synth.synth() << "dey" << E_;
	synth.synth() << "cpy #-1" << E_;
	synth.synth() << "bne @-" << E_;
}

void generator::retrieve_from_array(const std::string& a) const {
	synth.synth() << "mwa #" << token(token_provider::TOKENS::INTEGER_ARRAY) << a << "+4 ARRAY_ASSIGNMENT_TMP_ADDRESS" << E_;
	synth.synth() << "mwa " << token(token_provider::TOKENS::INTEGER_ARRAY) << a << " ARRAY_ASSIGNMENT_TMP_SIZE" << E_;
	synth.synth() << "jsr INIT_ARRAY_OFFSET" << E_;
	synth.synth() << "ldy #" << cfg.get_number_interpretation()->get_size()-1 << E_;
	synth.synth(false) << "@" << E_;
	synth.synth() << "lda (ARRAY_ASSIGNMENT_TMP_ADDRESS),y" << E_;
	synth.synth() << "sta FR0,y" << E_;
	synth.synth() << "dey" << E_;
	synth.synth() << "cpy #-1" << E_;
	synth.synth() << "bne @-" << E_;
}

void generator::random() const {
	synth.synth() << "jsr PUT_RANDOM_IN_FR0" << E_;
}

void generator::FP_to_ASCII() const {
	synth.synth() << "jsr FASC" << E_;
}

void generator::init_print() const {
	synth.synth() << R"(
	lda PTABW
	sta AUXBR
	lda #0
	sta COX
)";
}

void generator::print_LBUFF() const {
	synth.synth(false) << "; Printing string located at LBUFF" << E_;
	synth.synth() << "jsr PUTSTRING" << E_;
}

void generator::FR0_boolean_invert() const {
	synth.synth(false) << "; Inverting logical (boolean) value stored in FR0" << E_;
	synth.synth() << "jsr FR0_boolean_invert" << E_;
}

void generator::goto_line(const int& i) const {
	synth.synth(false) << "; Go to line " << i << E_;
	synth.synth() << "jmp " << token(token_provider::TOKENS::LINE_INDICATOR) << i << E_;
}

void generator::gosub(const int& i) const {
	synth.synth(false) << "; Go sub line " << i << E_;
	synth.synth() << "jsr " << token(token_provider::TOKENS::LINE_INDICATOR) << i << E_;
}

void generator::gosub(const std::string& s) const {
	synth.synth(false) << "; Go sub procedure " << s << E_;
	synth.synth() << "jsr " << token(token_provider::TOKENS::PROCEDURE) << s << E_;
}

void generator::write_runtime() const {
	synth.synth(false) << "; Here come the compiler runtime functions" << E_;
	cfg.get_runtime()->synth_implementation();
}

void generator::sound()
{
	if(!pokey_initialized)
	{
		pokey_initialized = true;
		synth.synth() << "jsr POKEY_INIT" << E_;
	}
	synth.synth() << "jsr SOUND" << E_;
}

void generator::poke() const {
	synth.synth() << "jsr POKE" << E_;
}

void generator::dpoke() const {
	synth.synth() << "jsr DPOKE" << E_;
}

void generator::peek() const {
	synth.synth() << "jsr PEEK" << E_;
}

void generator::dpeek() const {
	synth.synth() << "jsr DPEEK" << E_;
}

void generator::stick() const {
	synth.synth() << "jsr STICK" << E_;
}

void generator::strig() const {
	synth.synth() << "jsr STRIG" << E_;
}

void generator::after_if()
{
	// If this particular if didn't have ELSE statement, we need
	// to synth it just before the ENDIF
	if(ifs_with_else.find(stack_if.top()) == ifs_with_else.end())
	{
		synth.synth(false) << token(token_provider::TOKENS::INSIDE_IF_INDICATOR) << stack_if.top() << E_;
	}
	synth.synth(false) << token(token_provider::TOKENS::AFTER_IF_INDICATOR) << stack_if.top() << E_;
	stack_if.pop();
}

void generator::inside_if()
{
	synth.synth() << "jmp " << token(token_provider::TOKENS::AFTER_IF_INDICATOR) << stack_if.top() << E_;
	synth.synth(false) << token(token_provider::TOKENS::INSIDE_IF_INDICATOR) << stack_if.top() << E_;
	ifs_with_else.insert(stack_if.top());
}

void generator::skip_if_on_false()
{
	pop_to("FR0");
	stack_if.push(counter_after_if++);
	synth.synth(false) << "; Skip execution if logical value is false " << E_;
	synth.synth() << "jsr Is_FR0_true" << E_;
	synth.synth() << "cmp #0" << E_;
	synth.synth() << "jeq " << token(token_provider::TOKENS::INSIDE_IF_INDICATOR) << (counter_after_if - 1) << E_;
}

void generator::for_loop_condition()
{
	last_generic_label = get_next_generic_label();

	pop_to("FR0");
	push_from("FR0", generator::STACK::FOR_CONDITION);

	synth.synth() << "mwa #" << last_generic_label << " FR0" << E_;
	push_from("FR0", generator::STACK::RETURN_ADDRESS_STACK);
}

void generator::for_step(bool default_step) const {
	if (default_step)
	{
		// Use "1"
		synth.synth() << "mwa #1 FR0" << E_;
	}
	else
	{
		// Calculated value is on the expression stack
		pop_to("FR0");
	}
	push_from("FR0", generator::STACK::FOR_STEP);
	synth.synth(false) << last_generic_label << E_;
}

void generator::for_loop_counter(const std::string& counting_variable)
{
	loop_context.push(LOOP_CONTEXT::FOR);
	synth.synth() << "mwa #" << token(token_provider::TOKENS::VARIABLE) + counting_variable << " FR0" << E_;
	push_from("FR0", generator::STACK::FOR_COUNTER);
}

std::string generator::get_next_generic_label()
{
	return (boost::format("%1%_%2%") % token(token_provider::TOKENS::GENERIC_LABEL) % counter_generic_label++).str();
}

void generator::next()
{
	// Peek loop counter
	peek_to("FR1", generator::STACK::FOR_COUNTER);

	// Peek loop step
	peek_to("FR0", generator::STACK::FOR_STEP);

	// Increase loop counter
	synth.synth() << "ldy #0" << E_;
	synth.synth() << "adw (FR1),y FR0,y (FR1),y" << E_;

	// Move the counter value into FR0
	synth.synth() << "ldy #0" << E_;
	synth.synth() << "mwa (FR1),y FR0" << E_;

	// Peek loop condition
	peek_to("FR1", generator::STACK::FOR_CONDITION);

	// Compare (less or equal)
	compare_greater();
	FR0_boolean_invert();

	// If less then peek address from RETURN_ADDRESS_STACK and jump
	synth.synth() << "lda FR0" << E_;
	synth.synth() << "cmp #1" << E_;
	synth.synth() << "bne @+" << E_;
	peek_to("FR1", generator::STACK::RETURN_ADDRESS_STACK);

	//synth.synth() << "jmp (FR1)" << E_;		// DONE: Make safer jump via rts
	synth.synth() << R"(
	sbw FR1 #1
	lda FR1+1
	pha
	lda FR1
	pha
	rts
)";

	// Otherwise quick-pop addresses from three related stacks and continue
	loop_context.pop();
	synth.synth(false) << "@" << E_;
	synth.synth() << "jsr CLEAR_FOR_LOOP_STACKS" << E_;
}

void generator::register_generator_runtime() const
{
	// TODO: Rework this "get_indent()-crap. Consider enabling synth() to user-provided streams.
	std::stringstream ss;
	ss << "CLEAR_FOR_LOOP_STACKS" << E_;
	ss << cfg.get_indent() << "mwa #" << stacks.at(STACK::FOR_COUNTER).get_pointer() << ' ' << token(token_provider::TOKENS::PUSH_POP_PTR_TO_INC_DEC) << E_;
	ss << cfg.get_indent() << "jsr FAKE_POP" << E_;
	ss << cfg.get_indent() << "mwa #" << stacks.at(STACK::FOR_CONDITION).get_pointer() << ' ' << token(token_provider::TOKENS::PUSH_POP_PTR_TO_INC_DEC) << E_;
	ss << cfg.get_indent() << "jsr FAKE_POP" << E_;
	ss << cfg.get_indent() << "mwa #" << stacks.at(STACK::RETURN_ADDRESS_STACK).get_pointer() << ' ' << token(token_provider::TOKENS::PUSH_POP_PTR_TO_INC_DEC) << E_;
	ss << cfg.get_indent() << "jsr FAKE_POP" << E_;
	ss << cfg.get_indent() << "mwa #" << stacks.at(STACK::FOR_STEP).get_pointer() << ' ' << token(token_provider::TOKENS::PUSH_POP_PTR_TO_INC_DEC) << E_;
	ss << cfg.get_indent() << "jsr FAKE_POP" << E_;
	ss << cfg.get_indent() << "rts" << E_;

	cfg.get_runtime()->register_own_runtime_funtion(ss.str());
}

void generator::while_()
{
	loop_context.push(LOOP_CONTEXT::WHILE);
	stack_while.push(counter_while++);
	synth.synth(false) << token(token_provider::TOKENS::WHILE_INDICATOR) << stack_while.top() << E_;
}

void generator::while_condition()
{
	pop_to("FR0");
	synth.synth() << "jsr Is_FR0_true" << E_;
	synth.synth() << "cmp #0" << E_;
	synth.synth() << "jeq " << token(token_provider::TOKENS::AFTER_WHILE_INDICATOR) << stack_while.top() << E_;
}

void generator::wend()
{
	loop_context.pop();
	const auto while_count = stack_while.top();
	stack_while.pop();
	synth.synth() << "jmp " << token(token_provider::TOKENS::WHILE_INDICATOR) << while_count << E_;
	synth.synth(false) << token(token_provider::TOKENS::AFTER_WHILE_INDICATOR) << while_count << E_;
}

void generator::exit()
{
	switch(loop_context.top())
	{
	case LOOP_CONTEXT::OUTSIDE:
		throw std::runtime_error("EXIT found outside the loop");
	case LOOP_CONTEXT::FOR:
		synth.synth() << "jmp @+" << E_;
		break;
	case LOOP_CONTEXT::WHILE:
		synth.synth() << "jmp " << token(token_provider::TOKENS::AFTER_WHILE_INDICATOR) << stack_while.top() << E_;
		break;
	case LOOP_CONTEXT::REPEAT:
		synth.synth() << "jmp " << token(token_provider::TOKENS::AFTER_REPEAT_INDICATOR) << stack_repeat.top() << E_;
		break;
	case LOOP_CONTEXT::DO:
		synth.synth() << "jmp " << token(token_provider::TOKENS::AFTER_DO_INDICATOR) << stack_do.top() << E_;
		break;
	default:
		throw std::runtime_error("EXIT found in unknown loop context");
	}
}

void generator::repeat()
{
	loop_context.push(LOOP_CONTEXT::REPEAT);
	stack_repeat.push(counter_repeat++);
	synth.synth(false) << token(token_provider::TOKENS::REPEAT_INDICATOR) << stack_repeat.top() << E_;
}

void generator::until()
{
	pop_to("FR0");
	synth.synth() << "jsr Is_FR0_true" << E_;
	synth.synth() << "cmp #0" << E_;
	synth.synth() << "jeq " << token(token_provider::TOKENS::REPEAT_INDICATOR) << stack_repeat.top() << E_;
	synth.synth(false) << token(token_provider::TOKENS::AFTER_REPEAT_INDICATOR) << stack_repeat.top() << E_;
	stack_repeat.pop();
}

void generator::do_()
{
	loop_context.push(LOOP_CONTEXT::DO);
	stack_do.push(counter_do++);
	synth.synth(false) << token(token_provider::TOKENS::DO_INDICATOR) << stack_do.top() << E_;
}

void generator::loop()
{
	synth.synth() << "jmp " << token(token_provider::TOKENS::DO_INDICATOR) << stack_do.top() << E_;
	synth.synth(false) << token(token_provider::TOKENS::AFTER_DO_INDICATOR) << stack_do.top() << E_;
	stack_do.pop();
}

void generator::return_() const {
	synth.synth() << "rts" << E_;
}

void generator::proc(const std::string& s)
{
	stack_procedure.push(s);
	synth.synth(false) << token(token_provider::TOKENS::PROCEDURE) << s << E_;
}

void generator::end() const
{
	synth.synth() << "jmp " << token(token_provider::TOKENS::PROGRAM_END) << E_;
}

void generator::print_newline() const
{
	synth.synth() << "jsr PUTNEWLINE" << E_;
}

void generator::print_comma() const
{
	synth.synth() << "jsr PUTCOMMA" << E_;
}

void generator::init_integer_array(const basic_array& arr) const
{
	// TODO: Check whether such array has already been declared
	// TODO: Rework this "get_indent()-crap. Consider enabling synth() to user-provided streams.
	std::stringstream ss;
	ss << get_array_token(arr.get_name()) << E_;
	ss << cfg.get_indent() << "dta a(" << arr.get_size(0)+1 << "),a(" << arr.get_size(1)+1 << ')' << E_;
	ss << ':' << ((arr.get_size(0)+1)*(arr.get_size(1)+1)) << cfg.get_indent() << cfg.get_number_interpretation()->get_initializer() << E_;

	cfg.get_runtime()->register_own_runtime_funtion(ss.str());
}

std::string generator::get_array_token(const std::string& name) const
{
	return token(token_provider::TOKENS::INTEGER_ARRAY) + name;
}

void generator::put_zero_in_FR0() const
{
	synth.synth() << "jsr PUT_ZERO_IN_FR0" << E_;
}

