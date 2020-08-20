#define CONCAT(a,b) a ## b
#define STRINGIZE_NX(a) #a
#define STRINGIZE(a) STRINGIZE_NX(a)

#define VERSION "1.0.3"
#define COMPILER "Microsoft (R) C/C++ Optimizing Compiler " STRINGIZE(_MSC_FULL_VER)
#define ID "baef4609163f542dc08f43aef02ce8da0581a2b5"
#define BUILD __DATE__ " " __TIME__
