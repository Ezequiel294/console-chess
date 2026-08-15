#ifndef VERSION_H
#define VERSION_H

#ifndef CHESS_VERSION
#error "CHESS_VERSION is not defined; build with the project's Makefile so VERSION is compiled in"
#endif

/* The single compiled-in version string, declared once in VERSION at the
 * repository root and carried here by the Makefile. Other modules should
 * call chess_version() rather than reaching for CHESS_VERSION directly. */
static inline const char *chess_version(void) {
  return CHESS_VERSION;
}

#endif /* VERSION_H */
