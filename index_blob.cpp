/* This file is part of the Casa Mia Datastore Project at UBC. It is distributed under the terms of
 * version 2 of the GNU GPL. See the file LICENSE for details. */

#include "blob_buffer.h"
#include "index_blob.h"

index_blob::index_blob(size_t count)
	: modified(true), resized(true), count(count)
{
	indices = new sub[count];
}

index_blob::index_blob(size_t count, const blob & x)
	: base(x), modified(false), resized(false), count(count)
{
	indices = new sub[count];
	if(!x.exists())
	{
		/* same as default constructor */
		modified = true;
		resized = true;
		return;
	}
	size_t offset = count * sizeof(uint32_t);
	for(size_t i = 0; i < count; i++)
	{
		uint32_t size = base.index<uint32_t>(i);
		if(size)
		{
			indices[i].delayed = true;
			indices[i]._size = --size;
			indices[i]._offset = offset;
			offset += size;
		}
		else
			assert(!indices[i].value.exists());
	}
}

index_blob::index_blob(const index_blob & x)
	: modified(false), resized(false), count(x.count)
{
	base = x.flatten();
	if(count)
	{
		indices = new sub[count];
		for(size_t i = 0; i < count; i++)
			indices[i] = x.indices[i];
	}
	else
		indices = NULL;
}

index_blob & index_blob::operator=(const index_blob & x)
{
	if(this == &x)
		return *this;
	if(indices)
		delete[] indices;
	base = x.flatten();
	modified = false;
	resized = false;
	count = x.count;
	if(count)
	{
		indices = new sub[count];
		for(size_t i = 0; i < count; i++)
			indices[i] = x.indices[i];
	}
	else
		indices = NULL;
	return *this;
}

blob index_blob::flatten() const
{
	if(!modified)
		return base;
	if(!resized)
	{
		blob_buffer buffer(base);
		size_t offset = count * sizeof(uint32_t);
		/* hopefully avoid copying by breaking sharing before modifying */
		base = blob();
		for(size_t i = 0; i < count; i++)
		{
			if(indices[i].modified)
			{
				assert(!indices[i].delayed);
				buffer.overwrite(offset, indices[i].value);
				indices[i].modified = false;
			}
			offset += indices[i].value.size();
		}
		modified = false;
		base = buffer;
	}
	else
	{
		size_t offset = count * sizeof(uint32_t);
		for(size_t i = 0; i < count; i++)
			offset += indices[i].value.size();
		blob_buffer buffer(offset);
		for(size_t i = 0; i < count; i++)
		{
			/* we use 0 for "does not exist" so other sizes are incremented by 1 */
			uint32_t size = indices[i].exists() ? indices[i].size() + 1 : 0;
			indices[i].modified = false;
			buffer << size;
		}
		offset = count * sizeof(uint32_t);
		for(size_t i = 0; i < count; i++)
			/* use get(i) rather than indices[i].value in case they are delayed */
			buffer.append(get(i));
		modified = false;
		resized = false;
		base = buffer;
	}
	return base;
}
