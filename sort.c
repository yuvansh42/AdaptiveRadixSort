#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#if defined(__GNUC__) || defined(__clang__)
  #define INLINE static inline __attribute__((always_inline))
#else
  #define INLINE static inline
#endif

#define RADIX 65536u
#define MASK 0xFFFFu
#define SMALL 64
#define NEARLY_DIV 32   // more aggressive (fixes your 859 ms issue)

typedef struct {
    uint32_t *tmp;
    uint32_t *c0;
    uint32_t *c1;
} WS;

/* ---------- TIME ---------- */
INLINE uint64_t now_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1e9 + ts.tv_nsec;
}

/* ---------- RNG ---------- */
static uint64_t s = 0x9E3779B97F4A7C15ull;

INLINE uint32_t rng() {
    uint64_t x = s;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    s = x;
    return (uint32_t)((x * 0x2545F4914F6CDD1Dull) >> 32);
}

/* ---------- SMALL SORT ---------- */
INLINE void ins(uint32_t *a, size_t n) {
    for (size_t i = 1; i < n; i++) {
        uint32_t x = a[i];
        size_t j = i;
        while (j && a[j-1] > x) {
            a[j] = a[j-1];
            j--;
        }
        a[j] = x;
    }
}

/* ---------- REVERSE ---------- */
INLINE void rev(uint32_t *a, size_t n) {
    size_t i = 0, j = n-1;
    while (i < j) {
        uint32_t t = a[i];
        a[i++] = a[j];
        a[j--] = t;
    }
}

/* ---------- NEARLY SORT FIX ---------- */
INLINE void fix(uint32_t *a, size_t n) {
    for (size_t i = 1; i < n; i++) {
        if (a[i-1] <= a[i]) continue;
        uint32_t x = a[i];
        size_t j = i;
        while (j && a[j-1] > x) {
            a[j] = a[j-1];
            j--;
        }
        a[j] = x;
    }
}

/* ----------RADIX ---------- */
INLINE void radix(uint32_t *a, size_t n, WS *w) {
    uint32_t *tmp = w->tmp;
    uint32_t *c0 = w->c0;
    uint32_t *c1 = w->c1;

    memset(c0, 0, RADIX * sizeof(uint32_t));
    memset(c1, 0, RADIX * sizeof(uint32_t));

    uint32_t *p = a, *e = a + n;

    // 🔥 unrolled histogram
    while (p + 4 <= e) {
        uint32_t x0 = p[0], x1 = p[1], x2 = p[2], x3 = p[3];

        c0[x0 & MASK]++; c1[x0 >> 16]++;
        c0[x1 & MASK]++; c1[x1 >> 16]++;
        c0[x2 & MASK]++; c1[x2 >> 16]++;
        c0[x3 & MASK]++; c1[x3 >> 16]++;

        p += 4;
    }
    while (p < e) {
        uint32_t x = *p++;
        c0[x & MASK]++;
        c1[x >> 16]++;
    }

    uint32_t sum = 0;
    for (size_t i = 0; i < RADIX; i++) {
        uint32_t t = c0[i];
        c0[i] = sum;
        sum += t;
    }

    for (size_t i = 0; i < n; i++) {
        uint32_t x = a[i];
        tmp[c0[x & MASK]++] = x;
    }

    sum = 0;
    for (size_t i = 0; i < RADIX; i++) {
        uint32_t t = c1[i];
        c1[i] = sum;
        sum += t;
    }

    for (size_t i = 0; i < n; i++) {
        uint32_t x = tmp[i];
        a[c1[x >> 16]++] = x;
    }
}

/* ---------- weird stuff that made me lose my sanity ---------- */
void sort(uint32_t *a, size_t n, WS *w) {
    if (n <= SMALL) { ins(a,n); return; }

    int sorted = 1, revs = 1, eq = 1;
    size_t desc = 0;

    for (size_t i = 1; i < n; i++) {
        if (a[i-1] > a[i]) { sorted = 0; desc++; }
        if (a[i-1] < a[i]) revs = 0;
        if (a[i] != a[0]) eq = 0;
    }

    if (sorted || eq) return;
    if (revs) { rev(a,n); return; }

    if (desc < n / NEARLY_DIV) {
        fix(a,n);
        return;
    }

    radix(a,n,w);
}

/* ---------- BENCH ---------- */
int cmp(const void *a, const void *b) {
    uint32_t x = *(uint32_t*)a;
    uint32_t y = *(uint32_t*)b;
    return (x>y)-(x<y);
}

void run(const char *name, uint32_t *d, size_t n, WS *w) {
    uint32_t *a = malloc(n*4), *b = malloc(n*4);
    memcpy(a,d,n*4); memcpy(b,d,n*4);

    uint64_t t1 = now_ns();
    sort(a,n,w);
    uint64_t t2 = now_ns();

    uint64_t t3 = now_ns();
    qsort(b,n,4,cmp);
    uint64_t t4 = now_ns();

    printf("\n[%s]\n", name);
    printf("mine: %.3f ms\n", (t2-t1)/1e6);
    printf("qsort: %.3f ms\n", (t4-t3)/1e6);
    printf("speedup: %.2fx\n", (double)(t4-t3)/(t2-t1));

    free(a); free(b);
}

/* ---------- MAIN ---------- */
int main() {
    size_t n = 10000000;

    WS w;
    w.tmp = malloc(n*4);
    w.c0 = malloc(RADIX*4);
    w.c1 = malloc(RADIX*4);

    uint32_t *d = malloc(n*4);

    for (size_t i=0;i<n;i++) d[i]=rng();
    run("Random",d,n,&w);

    for (size_t i=0;i<n;i++) d[i]=i;
    run("Sorted",d,n,&w);

    for (size_t i=0;i<n;i++) d[i]=n-i;
    run("Reverse",d,n,&w);

    for (size_t i=0;i<n;i++) d[i]=42;
    run("Equal",d,n,&w);
}
