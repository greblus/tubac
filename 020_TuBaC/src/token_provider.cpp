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

#include "token_provider.h"

const std::string& token_provider::get(TOKENS token) const
{
	return TOKEN_MAP.at(token);
}

std::string token_provider::make_token(const std::string& name) const
{
	return TOKEN_INDICATOR + name;
}

