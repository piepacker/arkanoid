#ifdef __cplusplus
extern "C" {
#endif
	void reset_options();
	void set_option(char const * key, char const * value);
	bool get_option(char const * key, char const ** value);
#ifdef __cplusplus
}
#endif
