/* This file is part of the Casa Mia Datastore Project at UBC. It is distributed under the terms of
 * version 2 of the GNU GPL. See the file LICENSE for details. */

#include "index_factory.h"
#include "factory_impl.h"

/* force the template to instantiate */
template class factory<index_factory_base>;

ext_index * index_factory_base::load(const istr & type, const dtable * dt_source, dtype::ctype pri_key_type, const params & config)
{
	const index_factory * factory = index_factory::lookup(type);
	return factory ? factory->open(dt_source, pri_key_type, config) : NULL;
}

ext_index * index_factory_base::load(const istr & type, dtable * dt_source, dtype::ctype pri_key_type, const params & config)
{
	const index_factory * factory = index_factory::lookup(type);
	return factory ? factory->open(dt_source, pri_key_type, config) : NULL;
}

ext_index * index_factory_base::load(const dtable * dt_source, dtype::ctype pri_key_type, const params & config)
{
	istr base;
	params base_config;
	if(!config.get("base", &base) || !config.get("base_config", &base_config))
		return NULL;
	return load(base, dt_source, pri_key_type, base_config);
}

ext_index * index_factory_base::load(dtable * dt_source, dtype::ctype pri_key_type, const params & config)
{
	istr base;
	params base_config;
	if(!config.get("base", &base) || !config.get("base_config", &base_config))
		return NULL;
	return load(base, dt_source, pri_key_type, base_config);
}
