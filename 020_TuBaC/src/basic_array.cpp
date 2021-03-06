#include "basic_array.h"

const std::string& basic_array::get_name() const
{
	return _name;
}

void basic_array::set_name(const std::string& name)
{
	_name = name;
}

const bool& basic_array::is_two_dimensional() const
{
	return _two_dimensional;
}

void basic_array::set_two_dimensional(const bool two_dimensional)
{
	_two_dimensional = two_dimensional;
}

void basic_array::init()
{
	_name = "";
	_size[0] = _size[1] = 0;
	_two_dimensional = false;
}

void basic_array::set_size(int which, std::size_t size)
{
	_size[which] = size;
}

const std::size_t& basic_array::get_size(int which) const
{
	return _size[which];
}
