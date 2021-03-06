/* This file is part of the Casa Mia Datastore Project at UBC.It is distributed under the terms of
 * version 2 of the GNU GPL. See the file LICENSE for details. */

#ifndef __STRINGTBL_H
#define __STRINGTBL_H

#include <stdint.h>
#include <sys/types.h>

#ifndef __cplusplus
#error stringtbl.h is a C++ header file
#endif

#include <vector>

#include "blob.h"

/* A string table is a section of a file which maintains a collection of unique
 * strings in sorted order. String tables are immutable once created. */

#define ST_LRU 16

class rofile;
class rwfile;
class blob_comparator;

class stringtbl
{
public:
	inline stringtbl() : fp(NULL) {}
	inline ~stringtbl()
	{
		if(fp)
			deinit();
	}
	
	int init(const rofile * fp, off_t start, bool do_lock = true);
	void deinit();
	
	inline size_t get_size()
	{
		return size;
	}
	
	inline bool is_binary()
	{
		return binary;
	}
	
	/* The return value of get() is good until at least ST_LRU
	 * more calls to get(), or one call to locate(). */
	const char * get(ssize_t index, bool do_lock = true) const;
	const blob & get_blob(ssize_t index, bool do_lock = true) const;
	ssize_t locate(const char * string, bool do_lock = true) const;
	ssize_t locate(const blob & search, const blob_comparator * blob_cmp = NULL, bool do_lock = true) const;
	
	/* these functions assume the input is sorted */
	static int create(rwfile * fp, const std::vector<istr> & strings);
	/* makes a binary table */
	static int create(rwfile * fp, const std::vector<blob> & blobs);
	
private:
	struct lru_ent
	{
		ssize_t index;
		const char * string;
		blob binary;
	};
	
	const rofile * fp;
	off_t start;
	ssize_t count;
	size_t size;
	uint8_t bytes[3];
	bool binary;
	mutable lru_ent lru[ST_LRU];
	int lru_next;
};

#endif /* __STRINGTBL_H */
