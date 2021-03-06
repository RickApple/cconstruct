// This code will cause an error when running the CConstruct binary to construct projects
#define GENERATE_ERROR \
  int* i = NULL;       \
  *i     = 5;
