#include <map>
#include <string>
#include <vector>
#include <stdarg.h>

extern "C"
{

#include "cclos.h"

	typedef std::vector<cclass_instance*> Argv;
	struct Method
	{
		void* fun;
		Argv argv;
		void* next;
	};

	typedef void (*FP_Argv0)();
	typedef void (*FP_Argv1)(cclass_instance*);
	typedef void (*FP_Argv2)(cclass_instance*, cclass_instance*);
	typedef void (*FP_Argv3)(cclass_instance*, cclass_instance*, cclass_instance*);

	typedef std::map<std::string, std::string> ClassMap;
	typedef std::vector<Method> Methods;
	typedef std::map<std::string, Methods> MethodMap;

	static ClassMap g_classes;
	static MethodMap g_methods;


	const char* defclass(const char* name, const char* deriv)
	{
		g_classes[name] = deriv;
		return name;
	}

	#define extractargv(argv, argc) \
		va_list vl; \
		va_start(vl, argc); \
		for (int i = 0; i < argc; ++i) \
		{ \
			cclass_instance* arg = va_arg(vl, cclass_instance*); \
			argv.push_back(arg); \
		}

	bool argmatch(Argv& a1, Argv& a2)
	{
		for (size_t i = 0; i < a1.size(); ++i)
			if (a1[i]->type != a2[i]->type)
				return false;
		return true;
	}

	int calcdistance_arg(cclass_instance* arg, cclass_instance* underpromo)
	{
		int ret = 0;
		std::string a1 = arg->type;
		std::string a2 = underpromo->type;
		while ( a1 != a2 )
		{
			a1 = g_classes[a1];
			if (a1.empty()) return -1;
			++ret;
		}
		return ret;
	}

	int calcdistance(Argv& args, Argv& underpromo)
	{
		if (args.size() != underpromo.size()) return -1;
		int ret = 0;
		for (size_t i = 0; i < args.size(); ++i)
		{
			int dist = calcdistance_arg(args[i], underpromo[i]);
			if (dist == -1) return -1;
			ret += dist;
		}
		return ret;
	}

	void defmethod(const char* name, void* fun, int argc, ...)
	{
		Method method = { fun };
		extractargv(method.argv, argc);
		method.next = NULL;

		int nextdist = 0;
		for (Method& m : g_methods[name])
		{
			int dist = calcdistance(method.argv, m.argv);
			if (dist >= 0)
			{
				if( nextdist == 0 || dist < nextdist )
				{
					method.next = m.fun;
					nextdist = dist;
				}
			}
		}

		g_methods[name].push_back(method);
	}

	void callmethod(const char* name, Method& method)
	{
		if (method.argv.size() == 0)
		{
			FP_Argv0 fun = (FP_Argv0)method.fun;
			fun();
		}
		else if (method.argv.size() == 1)
		{
			FP_Argv1 fun = (FP_Argv1)method.fun;
			fun(method.argv[0]);
		}
		else if (method.argv.size() == 2)
		{
			FP_Argv2 fun = (FP_Argv2)method.fun;
			fun(method.argv[0], method.argv[1]);
		}
		else if (method.argv.size() == 3)
		{
			FP_Argv3 fun = (FP_Argv3)method.fun;
			fun(method.argv[0], method.argv[1], method.argv[2]);
		}
	}

	Method* find_method(const char* name, void* fun)
	{
		Method* method = NULL;
		for (Method& m : g_methods[name])
		{
			if (m.fun == fun)
			{
				method = &m;
				break;
			}
		}
		return method;
	}

	void call_next_method(const char* name, void* fun, int argc, ...)
	{
		Method* method = find_method(name, fun);
		if (method && method->next)
		{
			extractargv(method->argv, argc);
			callmethod(name, *method);
		}
	}

	void call(const char* name, int argc, ...)
	{
		Methods& methods = g_methods[name];
		Argv argv;
		extractargv(argv, argc);
		for (Method& m : methods)
		{
			Method* meth = &m;

			if (meth->fun && argmatch(argv, meth->argv))
			{
				callmethod(name, *meth);
				goto end;
			}
			else
			{
				while (meth)
				{
					meth = find_method(name, meth->next);

					if (meth && meth->fun)
					{
						callmethod(name, *meth);
						goto end;
					}
				}
			}
		}
	end:;
	}
}

