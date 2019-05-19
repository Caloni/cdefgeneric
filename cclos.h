#ifndef _CCLOS_H_
#define _CCLOS_H_

typedef struct cclass_instance { const char* type; } cclass_instance;
static cclass_instance cclass = { "cclass" };

void cdefgeneric_initialize();
const char* defclass(const char* name, const char* deriv);
void defmethod(const char* name, void* fun, int argc, ...);
void call_next_method(const char* name, int argc, ...);
void call(const char* name, int argc, ...);


#endif
