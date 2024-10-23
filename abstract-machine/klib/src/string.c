#include <stddef.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t len = 0;
  while (*s != '\0') {
    len++;
    s++;
  }
  return len;
}

char *strcpy(char *dst, const char *src) {
  char *original_dest = dst;
  while ((*dst++ = *src++)) {
  
  }
  return original_dest;
}

char *strncpy(char *dst, const char *src, size_t n) {
  char *original_dest = dst;
  while (n-- && (*dst++ = *src++)) {
  }

  while (n--) {
    *dst++ = '\0';
  }
  return original_dest;
}

char *strcat(char *dst, const char *src) {
  char *original_dest = dst;
  while (*dst) {
    dst++;
  }

  while ((*dst++ = *src)) {
  }

  return original_dest;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }

  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  while (n--) {
    if (*s1 != *s2) {
      return *(unsigned char *)s1 - *(unsigned char *)s2;
    }

    if (*s1 == '\0') {
      return 0;
    }

    s1++;
    s2++;
  }

  return 0;
}

void *memset(void *s, int c, size_t n) {
  unsigned char *p = (unsigned char *)s;

  while (n--) {
    *p++ = (unsigned char)c;
  }

  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dst;
  const unsigned char *s = (const unsigned char *)src;

  if (d > s && d < s + n) {
    d += n;
    s += n;
    while (n--) {
      *(d++) = *(s++);
    }
  } else {
    while (n--) {
      *(d++) = *(s++);
    }
  }

  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  unsigned char *d = (unsigned char *)out;
  const unsigned char *s = (const unsigned char *)in;

  while (n--) {
    *d++ = *s++;
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *p1 = (const unsigned char *)s1;
  const unsigned char *p2 = (const unsigned char *)s1;

  while (n--) {
    if (*p1 != *p2) {
      return *p1 - *p2;
    }

    p1++;
    p2++;
  }

  return 0;
}

#endif
