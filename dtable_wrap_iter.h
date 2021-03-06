/* This file is part of the Casa Mia Datastore Project at UBC.It is distributed under the terms of
 * version 2 of the GNU GPL. See the file LICENSE for details. */

#ifndef __DTABLE_WRAP_ITER_H
#define __DTABLE_WRAP_ITER_H

#ifndef __cplusplus
#error dtable_wrap_iter.h is a C++ header file
#endif

#include "dtable.h"

/* The dtable_wrap_iter class provides a generic iterator wrapper that passes
 * all calls through to the base iterator. Thus wrappers can easily be made by
 * subclassing dtable_wrap_iter and overriding only the methods of interest. */

/* this version passes all calls through except index-related calls */
class dtable_wrap_iter_noindex : public dtable::iter
{
public:
	inline virtual bool valid() const { return base->valid(); }
	inline virtual bool next() { return base->next(); }
	inline virtual bool prev() { return base->prev(); }
	inline virtual bool first() { return base->first(); }
	inline virtual bool last() { return base->last(); }
	inline virtual dtype key() const { return base->key(); }
	inline virtual bool seek(const dtype & key) { return base->seek(key); }
	inline virtual bool seek(const dtype_test & test) { return base->seek(test); }
	inline virtual dtype::ctype key_type() const { return base->key_type(); }
	inline virtual const blob_comparator * get_blob_cmp() const { return base->get_blob_cmp(); }
	inline virtual const istr & get_cmp_name() const { return base->get_cmp_name(); }
	inline virtual metablob meta() const { return base->meta(); }
	inline virtual blob value() const { return base->value(); }
	inline virtual const dtable * source() const { return base->source(); }
	inline virtual bool reject(blob * replacement) { return base->reject(replacement); }
	
	inline dtable_wrap_iter_noindex(dtable::iter * base, bool claim_base = false) : base(base), claim_base(claim_base) {}
	inline virtual ~dtable_wrap_iter_noindex() { if(base && claim_base) delete base; }
	
protected:
	dtable::iter * base;
	bool claim_base;
};

/* this version also passes on the index-related calls */
class dtable_wrap_iter : public dtable_wrap_iter_noindex
{
public:
	inline virtual bool seek_index(size_t index) { return base->seek_index(index); }
	inline virtual size_t get_index() const { return base->get_index(); }
	
	inline dtable_wrap_iter(dtable::iter * base, bool claim_base = false) : dtable_wrap_iter_noindex(base, claim_base) {}
	inline virtual ~dtable_wrap_iter() {}
};

template<class T>
static inline T * wrap_and_claim(dtable::iter * in)
{
	if(!in)
		return NULL;
	/* we assume "true" means claim_base = true */
	T * out = new T(in, true);
	if(!out)
		delete in;
	return out;
}

#endif /* __DTABLE_WRAP_ITER_H */
