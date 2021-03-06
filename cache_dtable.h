/* This file is part of the Casa Mia Datastore Project at UBC. It is distributed under the terms of
 * version 2 of the GNU GPL. See the file LICENSE for details. */

#ifndef __CACHE_DTABLE_H
#define __CACHE_DTABLE_H

#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>

#ifndef __cplusplus
#error cache_dtable.h is a C++ header file
#endif

#include <queue>
#include <ext/hash_map>

#include "dtable_factory.h"

/* The cache dtable sits on top of another dtable, and merely adds caching. */

class cache_dtable : public dtable
{
public:
	virtual iter * iterator(ATX_OPT) const;
	virtual bool present(const dtype & key, bool * found, ATX_OPT) const;
	virtual blob lookup(const dtype & key, bool * found, ATX_OPT) const;
	inline virtual bool writable() const { return base->writable(); }
	virtual int insert(const dtype & key, const blob & blob, bool append = false, ATX_OPT);
	virtual int remove(const dtype & key, ATX_OPT);
	
	/* cache_dtable is aware of abortable transactions */
	inline virtual abortable_tx create_tx() { return base->create_tx(); }
	inline virtual int check_tx(ATX_REQ) const { return base->check_tx(atx); }
	inline virtual int commit_tx(ATX_REQ) { return base->commit_tx(atx); }
	inline virtual void abort_tx(ATX_REQ) { return base->abort_tx(atx); }
	
	inline virtual int set_blob_cmp(const blob_comparator * cmp)
	{
		int value = base->set_blob_cmp(cmp);
		if(value >= 0)
		{
			value = dtable::set_blob_cmp(cmp);
			assert(value >= 0);
		}
		return value;
	}
	
	inline virtual int maintain(bool force = false) { return base->maintain(force); }
	
	DECLARE_WRAP_FACTORY(cache_dtable);
	
	inline cache_dtable() : base(NULL), chain(this), cache(10, blob_cmp, blob_cmp) {}
	int init(int dfd, const char * file, const params & config, sys_journal * sysj);
	
protected:
	void deinit();
	inline virtual ~cache_dtable()
	{
		if(base)
			deinit();
	}
	
private:
	struct entry
	{
		blob value;
		bool found;
	};
	
	void add_cache(const dtype & key, const blob & value, bool found) const;
	
	typedef __gnu_cxx::hash_map<const dtype, entry, dtype_hashing_comparator, dtype_hashing_comparator> cache_map;
	
	dtable * base;
	mutable chain_callback chain;
	size_t cache_size;
	mutable cache_map cache;
	mutable std::queue<dtype> order;
};

#endif /* __CACHE_DTABLE_H */
