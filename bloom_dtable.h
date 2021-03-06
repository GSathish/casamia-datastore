/* This file is part of the Casa Mia Datastore Project at UBC.It is distributed under the terms of
 * version 2 of the GNU GPL. See the file LICENSE for details. */

#ifndef __BLOOM_DTABLE_H
#define __BLOOM_DTABLE_H

#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>

#ifndef __cplusplus
#error bloom_dtable.h is a C++ header file
#endif

#include "dtable_factory.h"

/* The bloom filter dtable must be created with another read-only dtable, and
 * builds a bloom filter for the keys. Negative lookups are then very fast. */

#define BFDT_PERF_TEST 0

#define BLOOM_DTABLE_MAGIC 0x1138B893
#define BLOOM_DTABLE_VERSION 0

class bloom_dtable : public dtable
{
public:
	virtual iter * iterator(ATX_OPT) const
	{
		/* returns base->iterator() */
		return iterator_chain_usage(&chain, base, atx);
	}
	virtual bool present(const dtype & key, bool * found, ATX_OPT) const;
	virtual blob lookup(const dtype & key, bool * found, ATX_OPT) const;
	virtual blob index(size_t index) const { return base->index(index); }
	virtual bool contains_index(size_t index) const { return base->contains_index(index); }
	virtual size_t size() const { return base->size(); }
	
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
	
	/* bloom_dtable supports indexed access if its base does */
	static bool static_indexed_access(const params & config);
	
	static int create(int dfd, const char * file, const params & config, dtable::iter * source, const ktable * shadow = NULL);
	DECLARE_RO_FACTORY(bloom_dtable);
	
	inline bloom_dtable() : base(NULL), chain(this) {}
	int init(int dfd, const char * file, const params & config, sys_journal * sysj);
	
#if BFDT_PERF_TEST
	static bool perf_enable;
#endif
	
protected:
	void deinit();
	inline virtual ~bloom_dtable()
	{
		if(base)
			deinit();
	}
	
private:
	class bloom
	{
	public:
		bloom() : filter(NULL) {}
		/* for reading */
		int init(int dfd, const char * file, size_t * m, size_t * k);
		/* for writing */
		int init(size_t bytes);
		int write(int dfd, const char * file, size_t m, size_t k) const;
		void deinit();
		~bloom()
		{
			if(filter)
				deinit();
		}
		inline bool check(size_t number) const
		{
			return (filter[number / 8] & (1 << (number % 8))) ? true : false;
		}
		inline void set(size_t number)
		{
			filter[number / 8] |= 1 << (number % 8);
		}
		bool check(const uint8_t * hash, size_t k, size_t bits) const;
		void add(const uint8_t * hash, size_t k, size_t bits);
		bool check(const dtype & key, size_t k, size_t bits) const;
		void add(const dtype & key, size_t k, size_t bits);
	private:
		uint8_t * filter;
#if BFDT_PERF_TEST
		istr dir_name, file_name;
		mutable size_t total_lookups, blocked_lookups;
#endif
	};

	struct bloom_dtable_header
	{
		uint32_t magic;
		uint32_t version;
		uint32_t m, k;
	} __attribute__((packed));
	
	dtable * base;
	mutable chain_callback chain;
	bloom filter;
	/* m: number of bits in filter
	 * k: number of hash indices
	 * bits: size of each index */
	size_t m, k, bits;
};

#endif /* __BLOOM_DTABLE_H */
