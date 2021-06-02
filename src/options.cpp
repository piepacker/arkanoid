#include <map>
#include <string>
#include <cstring>

#include "options.h"

static std::map<std::string, std::string> emu_option;

void reset_options() {
	emu_option.clear();
}

void set_option(char const * key, char const * value) {
	emu_option[std::string(key)] = std::string(value);
}

bool get_option(char const * key, char const ** value) {
	auto it = emu_option.find(std::string(key));
	if (it == emu_option.end()) {
		*value = NULL;
		return false;
	}

	*value = it->second.c_str();
	return true;
}
