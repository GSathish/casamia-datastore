/* This file is part of the Casa Mia Datastore Project at UBC.It is distributed under the terms of
 * version 2 of the GNU GPL. See the file LICENSE for details. */

#ifndef __STABLE_H
#define __STABLE_H

#ifndef __cplusplus
#error stable.h is a C++ header file
#endif

#include "dtype.h"
#include "dtable.h"
#include "ext_index.h"
#include "blob_comparator.h"

/* schema tables, like typed ctables (similar to old gtable) */

class stable
{
public:
	/* iterate through the columns of an stable */
	class column_iter
	{
	public:
		virtual bool valid() const = 0;
		/* see the note about dtable::iter in dtable.h */
		virtual bool next() = 0;
		virtual bool prev() = 0;
		virtual bool first() = 0;
		virtual bool last() = 0;
		virtual const istr & name() const = 0;
		virtual size_t row_count() const = 0;
		virtual dtype::ctype type() const = 0;
		virtual ext_index * index() const = 0;
		inline column_iter() {}
		virtual ~column_iter() {}
	private:
		void operator=(const column_iter &);
		column_iter(const column_iter &);
	};
	
	/* iterate through the actual data */
	class iter
	{
	public:
		virtual bool valid() const = 0;
		/* see the note about dtable::iter in dtable.h */
		virtual bool next() = 0;
		virtual bool prev() = 0;
		virtual bool first() = 0;
		virtual bool last() = 0;
		virtual dtype key() const = 0;
		virtual bool seek(const dtype & key) = 0;
		virtual bool seek(const dtype_test & test) = 0;
		virtual dtype::ctype key_type() const = 0;
		virtual const istr & column() const = 0;
		virtual dtype value() const = 0;
		inline iter() {}
		virtual ~iter() {}
	private:
		void operator=(const iter &);
		iter(const iter &);
	};
	
	virtual column_iter * columns() const = 0;
	virtual size_t column_count() const = 0;
	virtual size_t row_count(const istr & column) const = 0;
	virtual dtype::ctype column_type(const istr & column) const = 0;
	
	/* stables can keep track of column indices that you give them, or they
	 * can take care of creating and managing the indices themselves */
	virtual ext_index * column_index(const istr & column) const = 0;
	virtual int set_column_index(const istr & column, ext_index * index) = 0;
	
	virtual dtable::key_iter * keys() const = 0;
	virtual iter * iterator() const = 0;
	virtual iter * iterator(const dtype & key) const = 0;
	
	/* returns true if found, otherwise does not change *value */
	virtual bool find(const dtype & key, const istr & column, dtype * value) const = 0;
	virtual bool contains(const dtype & key) const = 0;
	
	virtual bool writable() const = 0;
	
	virtual int insert(const dtype & key, const istr & column, const dtype & value, bool append = false) = 0;
	/* remove just a column */
	virtual int remove(const dtype & key, const istr & column) = 0;
	/* remove the whole row */
	virtual int remove(const dtype & key) = 0;
	
	inline virtual int set_blob_cmp(const blob_comparator * cmp) { return -ENOSYS; }
	inline virtual const blob_comparator * get_blob_cmp() const { return NULL; }
	inline virtual const istr & get_cmp_name() const { return istr::null; }
	
	/* maintenance callback; does nothing by default */
	inline virtual int maintain(bool force = false) { return 0; }
	
	virtual dtype::ctype key_type() const = 0;
	inline stable() {}
	inline virtual ~stable() {}
	
private:
	void operator=(const stable &);
	stable(const stable &);
};

#endif /* __STABLE_H */
