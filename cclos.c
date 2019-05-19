#include "cclos.h"
#include <stdarg.h>
#include <string.h>
#include <gmodule.h>


typedef GPtrArray Argv;

typedef struct _Method
{
	void* fun;
	Argv* argv;
} Method;

typedef void (*FP_Argv0)();
typedef void (*FP_Argv1)(cclass_instance*);
typedef void (*FP_Argv2)(cclass_instance*, cclass_instance*);
typedef void (*FP_Argv3)(cclass_instance*, cclass_instance*, cclass_instance*);

typedef GHashTable ClassMap;
typedef GPtrArray Methods;
typedef GHashTable MethodMap;

static ClassMap* g_classes;
static MethodMap* g_methods;
static Methods* g_calledMethods;


void cdefgeneric_initialize()
{
	g_classes = g_hash_table_new(g_str_hash, g_str_equal);
	g_methods = g_hash_table_new(g_str_hash, g_str_equal);
}


Method* method_new(void* fun, Argv* argv)
{
	Method* ret = (Method*)malloc(sizeof(Method));
	if (ret)
	{
		ret->fun = fun;
		ret->argv = argv ? argv : g_ptr_array_new();
	}
	return ret;
}

Argv* method_free(Method* method, gboolean free_seg)
{
	Argv* ret = free_seg ? g_ptr_array_free(method->argv, TRUE), NULL : method->argv;
	free(method);
	return ret;
}


const char* defclass(const char* name, const char* deriv)
{
	gboolean insertOk = g_hash_table_insert(g_classes, (gpointer*)name, (gpointer*)deriv);
	return name;
}

#define extractargv(argv, argc) \
		va_list vl; \
		va_start(vl, argc); \
		for (int i = 0; i < argc; ++i) \
		{ \
			cclass_instance* arg = va_arg(vl, cclass_instance*); \
			g_ptr_array_add(argv, arg); \
		}

void defmethod(const char* name, void* fun, int argc, ...)
{
	Method* method = method_new(fun, NULL);
	Methods* methods = (Methods*)g_hash_table_lookup(g_methods, name);
	extractargv(method->argv, argc);
	if (!methods)
	{
		methods = g_ptr_array_new();
		g_hash_table_insert(g_methods, (gpointer) name, (gpointer) methods);
	}
	g_ptr_array_add(methods, method);
}

void callmethod(const char* name, Method* method)
{
	g_ptr_array_add(g_calledMethods, (gpointer) method->fun);
	cclass_instance** argv = (cclass_instance * *)method->argv->pdata;

	if (method->argv->len == 0)
	{
		FP_Argv0 fun = (FP_Argv0)method->fun;
		fun();
	}
	else if (method->argv->len == 1)
	{
		FP_Argv1 fun = (FP_Argv1)method->fun;
		fun(argv[0]);
	}
	else if (method->argv->len == 2)
	{
		FP_Argv2 fun = (FP_Argv2)method->fun;
		fun(argv[0], argv[1]);
	}
	else if (method->argv->len == 3)
	{
		FP_Argv3 fun = (FP_Argv3)method->fun;
		fun(argv[0], argv[1], argv[2]);
	}
}

Method* find_method_by_fun(const char* name, void* fun)
{
	Method* method = NULL;
	Methods* methods = (Methods*) g_hash_table_lookup(g_methods, name);
	if (methods)
	{
		Method** meths = (Method**)methods->pdata;
		guint i = 0;
		while (i < methods->len)
		{
			Method* m = meths[i];
			if (m->fun == fun)
			{
				method = m;
				break;
			}
			++i;
		}
	}
	return method;
}

int calcdistance_arg(cclass_instance* arg, cclass_instance* underpromo)
{
	int ret = 0;
	char* promo = (char*)arg->type;

	while (strcmp(promo, underpromo->type) != 0)
	{
		promo = (char*)g_hash_table_lookup(g_classes, (gpointer*)promo);
		if (!promo || strlen(promo) == 0)
		{
			ret = -1;
			break;
		}
		++ret;
	}

	return ret;
}

int calcdistance(cclass_instance** args, cclass_instance** underpromo, int len)
{
	int ret = 0;
	for (int i = 0; i < len; ++i)
	{
		int dist = calcdistance_arg(args[i], underpromo[i]);
		if (dist == -1) return -1;
		ret += dist;
	}
	return ret;
}

void call_next_method(const char* name, int argc, ...)
{
	Method* method = method_new(NULL, NULL);
	extractargv(method->argv, argc);

	int nextdist = 666;
	Methods* methods = (Methods*)g_hash_table_lookup(g_methods, name);
	if (methods)
	{
		guint i = 0;
		while( i < methods->len )
		{
			Method* m = ((Method * *)methods->pdata)[i];
			gboolean alreadyCalled = FALSE;
			guint cms = 0;

			while (cms < g_calledMethods->len)
			{
				void* fun = ((void **)g_calledMethods->pdata)[cms];
				if (fun == m->fun)
				{
					alreadyCalled = TRUE;
					break;
				}
				++cms;
			}

			if ( ! alreadyCalled)
			{
				int dist = -1;

				if (method->argv->len == m->argv->len)
					dist = calcdistance((cclass_instance * *)method->argv->pdata, (cclass_instance * *)m->argv->pdata, method->argv->len);

				if (dist >= 0 && dist < nextdist)
				{
					if (dist < nextdist)
					{
						method->fun = m->fun;
						nextdist = dist;
					}
				}
			}

			++i;
		}

		if (method->fun)
			callmethod(name, method);
	}
}

void call(const char* name, int argc, ...)
{
	g_calledMethods = g_ptr_array_new();

	Method* method = method_new(NULL, NULL);
	extractargv(method->argv, argc);

	int nextdist = 666;
	Methods* methods = g_hash_table_lookup(g_methods, name);
	if (methods)
	{
		guint i = 0;
		while( i < methods->len )
		{
			Method* m = ((Method * *)methods->pdata)[i];
			int dist = -1;

			if (method->argv->len == m->argv->len)
				dist = calcdistance((cclass_instance * *)method->argv->pdata, (cclass_instance * *)m->argv->pdata, method->argv->len);

			if (dist >= 0 && dist < nextdist)
			{
				if (dist < nextdist)
				{
					method->fun = m->fun;
					nextdist = dist;
				}
			}

			++i;
		}

		if (method->fun)
			callmethod(name, method);
	}

	method_free(method, TRUE);
}

