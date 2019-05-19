#include "cclos.h"
#include <stdio.h>

/* defclass(foo, cclass_instance); */
typedef struct foo_instance { cclass_instance type; } foo_instance;
static foo_instance foo = { "foo" };

/* defclass(bar, foo); */
typedef struct bar_instance { cclass_instance type; } bar_instance; 
static bar_instance bar = { "bar" };

void test_dispatch()
{
	bar_instance x = bar;
	bar_instance y = bar;
	call("thing", 2, &x, &y);
}

void thing_foo_foo(cclass_instance* x, cclass_instance* y)
{
	printf("Both are of type FOO\n");
}

void thing_bar_foo(cclass_instance* x, cclass_instance* y)
{
	printf("X is BAR, Y is FOO. Next...\n");
	call_next_method("thing", 2, x, y);
}

void thing_foo_bar(cclass_instance* x, cclass_instance* y)
{
	printf("X is FOO, Y is BAR. Next...\n");
	call_next_method("thing", 2, x, y);
}

int main()
{
	cdefgeneric_initialize();
	defclass("foo", "cclass");
	defclass("bar", "foo");
	defmethod("thing", &thing_foo_foo, 2, &foo, &foo);
	defmethod("thing", &thing_bar_foo, 2, &bar, &foo);
	defmethod("thing", &thing_foo_bar, 2, &foo, &bar);
	test_dispatch();
}

