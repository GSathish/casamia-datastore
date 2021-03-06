/* This file is part of the Casa Mia Datastore Project at UBC. It is distributed under the terms of
 * version 2 of the GNU GPL. See the file LICENSE for details. */

#ifndef __PARAMS_H
#define __PARAMS_H

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef __cplusplus
#error params.h is a C++ header file
#endif

#include <map>

#include "istr.h"
#include "token_stream.h"

class params
{
public:
	inline void remove(const istr & name)
	{
		values.erase(name);
	}
	
	template<class T>
	inline void set(const istr & name, T value)
	{
		values[name] = value;
	}
	/* cause a compilation error if "class_name" is not the name of a type */
#define set_class(name, class_name) set(name, (const char *) (const class_name **) #class_name)
	/* verify that "class_name" is an available factory, like params::parse() */
	bool set_dt(const istr & name, const istr & class_name);
	bool set_ct(const istr & name, const istr & class_name);
	bool set_idx(const istr & name, const istr & class_name);
	
	/* these return false on a type mismatch */
	bool get(const istr & name, bool * value, bool dfl = false) const;
	bool get(const istr & name, int * value, int dfl = 0) const;
	bool get(const istr & name, float * value, float dfl = 0) const;
	bool get(const istr & name, istr * value, const istr & dfl = NULL) const;
	bool get(const istr & name, blob * value, const blob & dfl = blob()) const;
	bool get(const istr & name, params * value, const params & dfl = params()) const;
	
	/* get sequences of config names, starting at 0 and either fetching exactly a requested number
	 * or stopping when a name does not exist (in the latter case, the default value is ignored) */
	inline bool get_seq(const istr & prefix, const istr & postfix, size_t count, bool variable, std::vector<bool> * value, bool dfl = false) const
	{ return get_seq_impl<bool>(prefix, postfix, count, variable, value, dfl); }
	inline bool get_seq(const istr & prefix, const istr & postfix, size_t count, bool variable, std::vector<int> * value, int dfl = 0) const
	{ return get_seq_impl<int>(prefix, postfix, count, variable, value, dfl); }
	inline bool get_seq(const istr & prefix, const istr & postfix, size_t count, bool variable, std::vector<float> * value, float dfl = 0) const
	{ return get_seq_impl<float>(prefix, postfix, count, variable, value, dfl); }
	inline bool get_seq(const istr & prefix, const istr & postfix, size_t count, bool variable, std::vector<istr> * value, const istr & dfl = NULL) const
	{ return get_seq_impl<istr>(prefix, postfix, count, variable, value, dfl); }
	inline bool get_seq(const istr & prefix, const istr & postfix, size_t count, bool variable, std::vector<blob> * value, const blob & dfl = blob()) const
	{ return get_seq_impl<blob>(prefix, postfix, count, variable, value, dfl); }
	inline bool get_seq(const istr & prefix, const istr & postfix, size_t count, bool variable, std::vector<params> * value, const params & dfl = params()) const
	{ return get_seq_impl<params>(prefix, postfix, count, variable, value, dfl); }
	
	bool has(const istr & name) const;
	
	/* a wrapper that allows blobs and optionally strings */
	bool get_blob_or_string(const istr & name, blob * value, const blob & dfl = blob()) const;
	
	/* a wrapper that allows integers and optionally big-endian blobs */
	bool get_int_or_blob(const istr & name, int * value, int dfl = 0) const;
	
	inline bool contains(const istr & name) const
	{
		return values.count(name) > 0;
	}
	
	inline void reset()
	{
		values.clear();
	}
	
	/* just for debugging; prints to stdout, with no newline */
	void print() const;
	/* just for debugging; prints the available classes to stdout */
	static void print_classes();
	
	/* on error, returns the negative of the line number causing the error */
	static int parse(const char * input, params * result);
	
private:
	enum keyword { ERROR, BOOL, INT, FLOAT, STRING, CLASS, CLASS_DT, CLASS_CT, CLASS_IDX, BLOB, CONFIG };
	static inline enum keyword parse_type(const char * type);
	static int parse(token_stream * tokens, params * result);
	
	template<class T>
	bool get_seq_impl(const istr & prefix, const istr & postfix, size_t count, bool variable,
	                  std::vector<T> * value, const T & dfl) const;
	
	/* can't be defined until later */
	struct param;
	
	bool simple_find(const istr & name, const param ** p) const;
	
	typedef std::map<istr, param, strcmp_less> value_map;
	value_map values;
};

/* this macro can be used with params::parse() to help do multi-line input strings */
#define LITERAL(x) #x

struct params::param
{
	enum { BOOL, INT, FLT, STR, BLB, PRM } type;
	union
	{
		bool b;
		int i;
		float f;
	};
	/* alas, we can't put these in the union */
	istr s;
	blob bl;
	params p;
	inline param() : type(INT), i(0) {}
	inline param(bool x) : type(BOOL), b(x) {}
	inline param(int x) : type(INT), i(x) {}
	inline param(float x) : type(FLT), f(x) {}
	inline param(const istr & x) : type(STR), i(0), s(x) {}
	/* this is necessary so const char * doesn't end up being used as bool */
	inline param(const char * x) : type(STR), i(0), s(x) {}
	inline param(const blob & x) : type(BLB), i(0), bl(x) {}
	inline param(const params & x) : type(PRM), i(0), p(x) {}
};

#endif /* __PARAMS_H */
