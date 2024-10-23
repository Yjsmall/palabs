#include <stddef.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  if (s == NULL) {
    return 0;
  }

  size_t len = 0;
  while (*s != '\0') {
    len++;
    s++;
  }
  return len;
}

char *strcpy(char *dst, const char *src) {
  char *original_dest = dst;
  while (*src != '\0') {
    *dst++ = *src++;
  }
  *dst = '\0';
  return original_dest;
}

char *strncpy(char *dst, const char *src, size_t n) {
  size_t i = 0;
  for (i = 0; i < n && src[i] != '\0'; i++) {
    dst[i] = src[i];
  }

  for (; i < n; i++) {
    dst[i] = '\0';
  }

  return dst;
}

char *strcat(char *dst, const char *src) {
  char *original_dest = dst;
  while (*dst) {
    dst++;
  }

  while (*src) {
    *dst++ = *src++;
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
    unsigned char* p = (unsigned char*)s;

    for (size_t i = 0; i < n; i++) {
        p[i] = (unsigned char)c;
    }
    return s;
}

void *memcpy(void *out, const void *in, size_t n) {
    if (!out || !in)
        return NULL;
    // 确保out和in指向的内存地址不重叠
    if ((char*)out < (char*)in || (char*)out >= (char*)in + n) {
        // 没有重叠，直接复制
        for (size_t i = 0; i < n; ++i) {
            ((char*)out)[i] = ((char*)in)[i];
        }
    } else {
        // 有重叠，从后向前复制以避免数据被覆盖
        for (size_t i = n; i != 0; --i) {
            ((char*)out)[i - 1] = ((char*)in)[i - 1];
        }
    }
    return out;
}

void *memmove(void *dst, const void *src, size_t n) {
    return memcpy(dst, src, n);
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
