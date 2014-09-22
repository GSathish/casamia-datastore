/* This file is part of the Casa Mia Datastore Project at UBC. It is distributed under the terms of
 * version 2 of the GNU GPL. See the file LICENSE for details. */

#define _ATFILE_SOURCE

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <sys/time.h>

#include "main.h"
#include "tpch.h"
#include "openat.h"
#include "transaction.h"

#include "dtable_factory.h"
#include "ctable_factory.h"
#include "sys_journal.h"

/* this class reads |-delimited data lines, as generated by dbgen */
template<size_t max_columns, size_t line_length = 380>
class tbl_reader {
public:
	tbl_reader(const char * file)
	{
		line_number = 0;
		input = fopen(file, "r");
		if(input)
			next();
	}
	bool valid() const
	{
		return !!input;
	}
	size_t number() const
	{
		return line_number;
	}
	bool next()
	{
		int i = 0;
		if(!input)
			return false;
		if(!fgets(line, sizeof(line), input))
		{
			fclose(input);
			input = NULL;
			return false;
		}
		line_number++;
		for(size_t c = 0; c < max_columns; c++)
		{
			column[c] = &line[i];
			for(; line[i] && line[i] != '|'; i++);
			if(line[i])
				line[i++] = 0;
		}
		return true;
	}
	const char * get(size_t index) const
	{
		assert(index < max_columns);
		return column[index];
	}
	~tbl_reader()
	{
		if(input)
			fclose(input);
	}
private:
	FILE * input;
	size_t line_number;
	char line[line_length];
	const char * column[max_columns];
};

/* FIXME: an ASCII dtable would be nice, ignoring the high bit of each byte (difficulty: how to calculate decoded size?) */
#include "tpch_config.h"

struct tpch_table_info {
	const char * name;
	istr type;
	const char * config;
};
enum tpch_table {
	PART = 0,
	CUSTOMER = 1,
	ORDERS = 2,
	LINEITEM = 3
};
static const tpch_table_info tpch_column_tables[4] = {
	{"part", "column_ctable", tpch_part_column_config},
	{"customer", "column_ctable", tpch_customer_column_config},
	{"orders", "column_ctable", tpch_orders_column_config},
	{"lineitem", "column_ctable", tpch_lineitem_column_config}
};
static const tpch_table_info tpch_row_tables[4] = {
	{"part", "simple_ctable", tpch_part_row_config},
	{"customer", "simple_ctable", tpch_customer_row_config},
	{"orders", "simple_ctable", tpch_orders_row_config},
	{"lineitem", "simple_ctable", tpch_lineitem_row_config}
};
static const tpch_table_info * tpch_tables = tpch_column_tables;

int command_tpchtype(int argc, const char * argv[])
{
	const char * now = "";
	const char * type = (tpch_tables == tpch_column_tables) ? "column" : "row";
	if(argc > 1)
	{
		now = "now ";
		type = argv[1];
		if(!strcmp(argv[1], "row"))
			tpch_tables = tpch_row_tables;
		else if(!strcmp(argv[1], "column"))
			tpch_tables = tpch_column_tables;
		else
		{
			printf("Unknown table type: %s\n", argv[1]);
			type = NULL;
		}
	}
	if(type)
		printf("TPC-H tests will %suse %s-based tables.\n", now, type);
	return 0;
}

static ctable * create_and_open(const tpch_table_info & info)
{
	int r;
	params config;
	ctable * table;
	
	r = params::parse(info.config, &config);
	printf("params::parse = %d\n", r);
	config.print();
	printf("\n");
	
	r = tx_start();
	printf("tx_start = %d\n", r);
	r = ctable_factory::setup(info.type, AT_FDCWD, info.name, config, dtype::UINT32);
	printf("ctable::create = %d\n", r);
	r = tx_end(0);
	printf("tx_end = %d\n", r);
	
	table = ctable_factory::load(info.type, AT_FDCWD, info.name, config, sys_journal::get_global_journal());
	printf("ctable_factory::load = %p\n", table);
	
	return table;
}

static void maintain_restart_tx(ctable * table)
{
	printf("maintain... ");
	fflush(stdout);
	int r = table->maintain(true);
	printf("\rmaintain = %d\n", r);
	r = tx_end(0);
	if(r < 0)
		printf("tx_end = %d\n", r);
	else
	{
		r = tx_start();
		if(r < 0)
			printf("tx_start = %d\n", r);
	}
}

static void create_part()
{
	ctable * table = create_and_open(tpch_tables[PART]);
	int r = tx_start();
	printf("tx_start = %d\n", r);
	
	tbl_reader<9> part_tbl("tpch/part.tbl");
	while(part_tbl.valid())
	{
		uint32_t key = atoi(part_tbl.get(0));
		uint32_t p_size;
		float p_retailprice;
		const char * char_values[8];
		ctable::colval values[8];
		for(size_t i = 0; i < 8; i++)
		{
			values[i].index = i;
			char_values[i] = part_tbl.get(i + 1);
		}
		values[0].value = blob(char_values[0]); /* p_name */
		values[1].value = blob(char_values[1]); /* p_mfgr */
		values[2].value = blob(char_values[2]); /* p_brand */
		values[3].value = blob(char_values[3]); /* p_type */
		p_size = strtol(char_values[4], NULL, 0);
		values[4].value = blob(sizeof(p_size), &p_size);
		values[5].value = blob(char_values[5]); /* p_container */
		p_retailprice = strtod(char_values[6], NULL);
		values[6].value = blob(sizeof(p_retailprice), &p_retailprice);
		values[7].value = blob(char_values[7]); /* p_comment */
		r = table->insert(key, values, 8, true);
		if(r < 0)
			printf("insert = %d\n", r);
		if(!(part_tbl.number() % 500000))
			maintain_restart_tx(table);
		part_tbl.next();
	}
	
	printf("digest_on_close\n");
	delete table;
	r = tx_end(0);
	printf("tx_end = %d\n", r);
}

static void create_customer()
{
	ctable * table = create_and_open(tpch_tables[CUSTOMER]);
	int r = tx_start();
	printf("tx_start = %d\n", r);
	
	tbl_reader<8> customer_tbl("tpch/customer.tbl");
	while(customer_tbl.valid())
	{
		uint32_t key = atoi(customer_tbl.get(0));
		uint32_t c_nationkey;
		float c_acctbal;
		const char * char_values[7];
		ctable::colval values[7];
		for(size_t i = 0; i < 7; i++)
		{
			values[i].index = i;
			char_values[i] = customer_tbl.get(i + 1);
		}
		values[0].value = blob(char_values[0]); /* c_name */
		values[1].value = blob(char_values[1]); /* c_address */
		c_nationkey = strtol(char_values[2], NULL, 0);
		values[2].value = blob(sizeof(c_nationkey), &c_nationkey);
		values[3].value = blob(char_values[3]); /* c_phone */
		c_acctbal = strtod(char_values[4], NULL);
		values[4].value = blob(sizeof(c_acctbal), &c_acctbal);
		values[5].value = blob(char_values[5]); /* c_mktsegment */
		values[6].value = blob(char_values[6]); /* c_comment */
		r = table->insert(key, values, 7, true);
		if(r < 0)
			printf("insert = %d\n", r);
		if(!(customer_tbl.number() % 500000))
			maintain_restart_tx(table);
		customer_tbl.next();
	}
	
	printf("digest_on_close\n");
	delete table;
	r = tx_end(0);
	printf("tx_end = %d\n", r);
}

static void create_orders()
{
	ctable * table = create_and_open(tpch_tables[ORDERS]);
	int r = tx_start();
	printf("tx_start = %d\n", r);
	
	tbl_reader<9> orders_tbl("tpch/orders.tbl");
	while(orders_tbl.valid())
	{
		uint32_t key = atoi(orders_tbl.get(0));
		uint32_t o_custkey, o_shippriority;
		float o_totalprice;
		const char * char_values[8];
		ctable::colval values[8];
		for(size_t i = 0; i < 8; i++)
		{
			values[i].index = i;
			char_values[i] = orders_tbl.get(i + 1);
		}
		o_custkey = strtol(char_values[0], NULL, 0);
		values[0].value = blob(sizeof(o_custkey), &o_custkey);
		values[1].value = blob(char_values[1]); /* o_orderstatus */
		o_totalprice = strtod(char_values[2], NULL);
		values[2].value = blob(sizeof(o_totalprice), &o_totalprice);
		values[3].value = blob(char_values[3]); /* o_orderdate */
		values[4].value = blob(char_values[4]); /* o_orderpriority */
		values[5].value = blob(char_values[5]); /* o_clerk */
		o_shippriority = strtol(char_values[6], NULL, 0);
		values[6].value = blob(sizeof(o_shippriority), &o_shippriority);
		values[7].value = blob(char_values[7]); /* o_comment */
		r = table->insert(key, values, 8, true);
		if(r < 0)
			printf("insert = %d\n", r);
		if(!(orders_tbl.number() % 500000))
			maintain_restart_tx(table);
		orders_tbl.next();
	}
	
	printf("digest_on_close\n");
	delete table;
	r = tx_end(0);
	printf("tx_end = %d\n", r);
}

static void create_lineitem()
{
	ctable * table = create_and_open(tpch_tables[LINEITEM]);
	int r = tx_start();
	printf("tx_start = %d\n", r);
	
	tbl_reader<16> lineitem_tbl("tpch/lineitem.tbl");
	while(lineitem_tbl.valid())
	{
		uint32_t key = lineitem_tbl.number();
		uint32_t l_orderkey, l_partkey, l_suppkey, l_linenumber;
		float l_quantity, l_extendedprice, l_discount, l_tax;
		const char * char_values[16];
		ctable::colval values[16];
		for(size_t i = 0; i < 16; i++)
		{
			values[i].index = i;
			char_values[i] = lineitem_tbl.get(i);
		}
		l_orderkey = strtol(char_values[0], NULL, 0);
		values[0].value = blob(sizeof(l_orderkey), &l_orderkey);
		l_partkey = strtol(char_values[1], NULL, 0);
		values[1].value = blob(sizeof(l_partkey), &l_partkey);
		l_suppkey = strtol(char_values[2], NULL, 0);
		values[2].value = blob(sizeof(l_suppkey), &l_suppkey);
		l_linenumber = strtol(char_values[3], NULL, 0);
		values[3].value = blob(sizeof(l_linenumber), &l_linenumber);
		l_quantity = strtod(char_values[4], NULL);
		values[4].value = blob(sizeof(l_quantity), &l_quantity);
		l_extendedprice = strtod(char_values[5], NULL);
		values[5].value = blob(sizeof(l_extendedprice), &l_extendedprice);
		l_discount = strtod(char_values[6], NULL);
		values[6].value = blob(sizeof(l_discount), &l_discount);
		l_tax = strtod(char_values[7], NULL);
		values[7].value = blob(sizeof(l_tax), &l_tax);
		values[8].value = blob(char_values[8]); /* l_returnflag */
		values[9].value = blob(char_values[9]); /* l_linestatus */
		values[10].value = blob(char_values[10]); /* l_shipdate */
		values[11].value = blob(char_values[11]); /* l_commitdate */
		values[12].value = blob(char_values[12]); /* l_receiptdate */
		values[13].value = blob(char_values[13]); /* l_shipinstruct */
		values[14].value = blob(char_values[14]); /* l_shipmode */
		values[15].value = blob(char_values[15]); /* l_comment */
		r = table->insert(key, values, 16, true);
		if(r < 0)
			printf("insert = %d\n", r);
		if(!(lineitem_tbl.number() % 500000))
			maintain_restart_tx(table);
		lineitem_tbl.next();
	}
	
	printf("digest_on_close\n");
	delete table;
	r = tx_end(0);
	printf("tx_end = %d\n", r);
}

/* import the part, customer, orders, and lineitem tables */
int command_tpchgen(int argc, const char * argv[])
{
	create_part();
	create_customer();
	create_orders();
	create_lineitem();
	return 0;
}

static ctable * open_in_tx(const tpch_table_info & info, bool print_config = false)
{
	int r;
	params config;
	ctable * table;
	
	r = params::parse(info.config, &config);
	printf("params::parse = %d\n", r);
	if(print_config)
	{
		config.print();
		printf("\n");
	}
	
	r = tx_start();
	printf("tx_start = %d\n", r);
	table = ctable_factory::load(info.type, AT_FDCWD, info.name, config, sys_journal::get_global_journal());
	printf("ctable_factory::load = %p\n", table);
	r = tx_end(0);
	printf("tx_end = %d\n", r);
	
	return table;
}

int command_tpchopen(int argc, const char * argv[])
{
	ctable * part = open_in_tx(tpch_tables[PART]);
	ctable * customer = open_in_tx(tpch_tables[CUSTOMER]);
	ctable * orders = open_in_tx(tpch_tables[ORDERS]);
	ctable * lineitem = open_in_tx(tpch_tables[LINEITEM]);
	delete lineitem;
	delete orders;
	delete customer;
	delete part;
	return 0;
}

/* TPC-H queries we might feasibly do are #6, #14, and #17, but we'll
 * stick with a simple variant of #6 as that's what another paper did.
 * We need only the lineitem table for query #6, even though we have
 * others available. The lineitem table is the biggest anyway. */
int command_tpchtest(int argc, const char * argv[])
{
	ctable * lineitem = open_in_tx(tpch_tables[LINEITEM]);
	struct timeval start;
	ctable::p_iter * iter;
	size_t columns[16];
	double revenue = 0;
	size_t runs = 5;
	
	if(argc > 1)
	{
		runs = atoi(argv[1]);
		if(runs < 1)
		{
			printf("Invalid run count: %s\n", argv[1]);
			return 0;
		}
	}
	printf("Running each column count %zu times.\n", runs);
	
	/* This is TPC-H query #6:
	 * 
	 * select sum(l_extendedprice * l_discount) as revenue
	 * from lineitem where l_shipdate >= date '[DATE]' and
	 * l_shipdate < date '[DATE]' + interval '1' year and
	 * l_discount between [DISCOUNT] - 0.01 and [DISCOUNT] + 0.01 and
	 * l_quantity < [QUANTITY];
	 * 
	 * Where [DATE] is Jan 1 of [1993-1997],
	 * [DISCOUNT] is chosen in [.02-.09], and
	 * [QUANTITY] is 24 or 25.
	 * 
	 * We don't actually do that exactly though. We select some number of
	 * columns, and don't do anything with the data we get from them. We
	 * also pick rows at random, simulating a predicate that happened to
	 * pick those rows while allowing simple control of how many we get. */
	
	/* Well, first we do this quick query to make sure we get the right answer
	 * (11475087032.373623, with TPC-H scale factor 1); then we do that. */
	columns[0] = lineitem->index("l_extendedprice");
	columns[1] = lineitem->index("l_discount");
	gettimeofday(&start, NULL);
	iter = lineitem->iterator(columns, 2);
	while(iter->valid())
	{
		double extendedprice = iter->value(columns[0]).index<float>(0);
		double discount = iter->value(columns[1]).index<float>(0);
		revenue += extendedprice * discount;
		iter->next();
	}
	EXPECT_DOUBLE("revenue", 11475087032.373623, revenue);
	print_elapsed(&start);
	delete iter;
	
	/* OK, now run some of those tests */
	const char * column_order[16] = {"l_partkey", "l_orderkey", "l_suppkey", "l_linenumber",
	                                 "l_quantity", "l_extendedprice", "l_returnflag", "l_linestatus",
	                                 "l_shipinstruct", "l_shipmode", "l_comment", "l_discount",
	                                 "l_tax", "l_shipdate", "l_commitdate", "l_receiptdate"};
	for(size_t i = 0; i < 16; i++)
		columns[i] = lineitem->index(column_order[i]);
	for(size_t n = 1; n <= 16; n++)
	{
		struct timeval sum = {0, 0}, end;
		for(size_t t = 0; t < runs; t++)
		{
			printf("\n%zu columns, take %zu:", n, t + 1);
			for(size_t i = 0; i < n; i++)
				printf(" %s", lineitem->name(columns[i]).str());
			printf("\n");
			
			/* reopen it to clear any application-level caches */
			delete lineitem;
			drop_cache(tpch_tables[LINEITEM].name);
			lineitem = open_in_tx(tpch_tables[LINEITEM]);
			iter = lineitem->iterator(columns, n);
			
			gettimeofday(&start, NULL);
			while(iter->valid())
			{
				/* get the predicate value */
				iter->value(columns[0]);
				/* assume 1/10 qualifies */
				if(!(rand() % 10))
					/* get the remaining values */
					for(size_t i = 1; i < n; i++)
						iter->value(columns[i]);
				iter->next();
			}
			gettimeofday(&end, NULL);
			timeval_subtract(&end, &start);
			timeval_add(&sum, &end);
			print_timeval(&end, true, true);
			
			delete iter;
		}
		printf("\n%zu column average: ", n);
		timeval_divide(&sum, runs, true);
		print_timeval(&sum, true, true);
	}
	
	delete lineitem;
	return 0;
}
