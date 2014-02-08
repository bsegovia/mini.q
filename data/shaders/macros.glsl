//##const char macros[] = {
#define UNIFORM(T, NAME) uniform T NAME;
#define UNIFORMI(T, NAME, LOC) uniform T NAME;
#define ATTRIB(T, NAME, LOC) VS_IN T NAME;
#define FRAGDATA(T, NAME, LOC) IF_NOT_WEBGL(out T NAME);
//##};

