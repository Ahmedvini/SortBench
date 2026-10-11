// Competition version: single-pass histograms + 3-pass 11-bit LSD radix + fast I/O
// SORT_MODE: 0 = radix 11-bit x3 (default), 1 = radix 8-bit x4, 2 = std::sort
// Defaults: 4-digit LUT serialization + mmap input. Disable with -DNO_SER4 / -DNO_MMAP.
// Build: g++ -O3 -march=native -std=c++17 [-DSORT_MODE=1] sort_fast.cpp -o sort_fast
// Run:   ./sort_fast input.txt output.txt     (add -DTIMING for per-stage ms on stderr)
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <algorithm>
#ifndef SORT_MODE
#define SORT_MODE 0
#endif
// Measured winners, on by default (SEPHIST gave no gain, so it stays off):
#ifndef NO_SER4
#define SER4
#endif
#ifndef NO_MMAP
#define USE_MMAP
#endif
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#ifdef TIMING
#include <chrono>
static double now() { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
#endif

#if SORT_MODE == 0
static uint32_t h0[2048], h1[2048], h2[1024];
#elif SORT_MODE == 1
static uint32_t h8[4][256];
#endif

int main(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "usage: %s in out\n", argv[0]); return 1; }
#ifdef TIMING
    double t0 = now();
#endif
    // ---- read whole file ----
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) return 1;
    struct stat st; fstat(fd, &st);
    size_t sz = st.st_size;
#ifdef USE_MMAP
    char* raw; size_t got = sz;
    if (sz % 4096 != 0) {   // zero-fill past EOF acts as terminator
        raw = (char*)mmap(nullptr, sz, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
    } else raw = nullptr;
    if (!raw || raw == MAP_FAILED) {
        raw = (char*)malloc(sz + 1); got = 0;
        while (got < sz) { ssize_t r = read(fd, raw + got, sz - got); if (r <= 0) break; got += r; }
        raw[got] = 0;
    }
    close(fd);
#else
    char* raw = (char*)malloc(sz + 1);
    size_t got = 0;
    while (got < sz) { ssize_t r = read(fd, raw + got, sz - got); if (r <= 0) break; got += r; }
    close(fd);
    raw[got] = 0;
#endif

    // ---- parse + histograms in the same loop ----
    size_t cap = got / 2 + 16;               // safe upper bound on count
    uint32_t* a = (uint32_t*)malloc(cap * 4);
    uint32_t* b = (uint32_t*)malloc(cap * 4);
    size_t n = 0;
    const char* p = raw; const char* end = raw + got;
    while (p < end) {
        unsigned c = (unsigned char)*p;
        if (c - '0' > 9u && c != '-' && c != '+') { ++p; continue; }
        bool neg = false;
        if (c == '-') { neg = true; ++p; } else if (c == '+') ++p;
        if ((unsigned)((unsigned char)*p - '0') > 9u) continue;
        int64_t v = 0;
        while ((unsigned)((unsigned char)*p - '0') <= 9u) { v = v * 10 + (*p - '0'); ++p; }
        uint32_t k = (uint32_t)(int32_t)(neg ? -v : v) ^ 0x80000000u;
        a[n++] = k;
#if SORT_MODE == 0 && !defined(SEPHIST)
        h0[k & 2047]++; h1[(k >> 11) & 2047]++; h2[k >> 22]++;
#elif SORT_MODE == 0
#elif SORT_MODE == 1
        h8[0][k & 255]++; h8[1][(k >> 8) & 255]++; h8[2][(k >> 16) & 255]++; h8[3][k >> 24]++;
#endif
    }
#if defined(SEPHIST) && SORT_MODE == 0
    for (size_t i = 0; i < n; i++) { uint32_t k = a[i]; h0[k & 2047]++; h1[(k >> 11) & 2047]++; h2[k >> 22]++; }
#endif
#ifdef TIMING
    double t1 = now();
#endif
    // ---- 3-pass LSD radix (skip trivial passes) ----
    [[maybe_unused]] auto pass = [&](uint32_t* src, uint32_t* dst, uint32_t* h, int buckets, int shift) {
        uint32_t sum = 0;
        for (int i = 0; i < buckets; i++) { uint32_t c = h[i]; h[i] = sum; sum += c; }
        const uint32_t mask = buckets - 1;
        for (size_t i = 0; i < n; i++) { uint32_t k = src[i]; dst[h[(k >> shift) & mask]++] = k; }
    };
    [[maybe_unused]] auto trivial = [&](uint32_t* h, int buckets, uint32_t any) {
        return n == 0 || h[(any)] == n;
    };
    uint32_t* src = a; uint32_t* dst = b;
#if SORT_MODE == 0
    if (n) {
        uint32_t k0 = a[0];
        if (!trivial(h0, 2048, k0 & 2047)) { pass(src, dst, h0, 2048, 0);  std::swap(src, dst); }
        if (!trivial(h1, 2048, (k0 >> 11) & 2047)) { pass(src, dst, h1, 2048, 11); std::swap(src, dst); }
        if (!trivial(h2, 1024, k0 >> 22)) { pass(src, dst, h2, 1024, 22); std::swap(src, dst); }
    }
#elif SORT_MODE == 1
    if (n) {
        uint32_t k0 = a[0];
        for (int d = 0; d < 4; d++) {
            if (trivial(h8[d], 256, (k0 >> (8 * d)) & 255)) continue;
            pass(src, dst, h8[d], 256, 8 * d); std::swap(src, dst);
        }
    }
#else
    std::sort(a, a + n);
#endif
#ifdef TIMING
    double t2 = now();
#endif
    // ---- serialize ----
#ifdef SER4
    static char lut4[40000];
    for (int i = 0; i < 10000; i++) { lut4[4*i]='0'+i/1000; lut4[4*i+1]='0'+(i/100)%10; lut4[4*i+2]='0'+(i/10)%10; lut4[4*i+3]='0'+i%10; }
    char* out = (char*)malloc(n * 12 + 16);
    char* o = out;
    for (size_t i = 0; i < n; i++) {
        int32_t x = (int32_t)(src[i] ^ 0x80000000u);
        uint32_t u = (uint32_t)x;
        if (x < 0) { *o++ = '-'; u = 0u - u; }
        char tmp[12]; char* q = tmp + 12;
        while (u >= 10000) { uint32_t r = u % 10000; u /= 10000; q -= 4; memcpy(q, lut4 + 4*r, 4); }
        if (u >= 1000) { q -= 4; memcpy(q, lut4 + 4*u, 4); }
        else if (u >= 100) { q -= 3; memcpy(q, lut4 + 4*u + 1, 3); }
        else if (u >= 10) { q -= 2; memcpy(q, lut4 + 4*u + 2, 2); }
        else *--q = '0' + u;
        size_t len = tmp + 12 - q;
        memcpy(o, q, len); o += len; *o++ = '\n';
    }
#else
    static char lut[200];
    for (int i = 0; i < 100; i++) { lut[2*i] = '0' + i / 10; lut[2*i+1] = '0' + i % 10; }
    char* out = (char*)malloc(n * 12 + 16);
    char* o = out;
    for (size_t i = 0; i < n; i++) {
        int32_t x = (int32_t)(src[i] ^ 0x80000000u);
        uint32_t u = (uint32_t)x;
        if (x < 0) { *o++ = '-'; u = 0u - u; }
        char tmp[12]; char* q = tmp + 12;
        while (u >= 100) { uint32_t r = u % 100; u /= 100; q -= 2; memcpy(q, lut + 2*r, 2); }
        if (u >= 10) { q -= 2; memcpy(q, lut + 2*u, 2); } else *--q = '0' + u;
        size_t len = tmp + 12 - q;
        memcpy(o, q, len); o += len; *o++ = '\n';
    }
#endif
#ifdef TIMING
    double t3 = now();
#endif
    // ---- write ----
    int fo = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fo < 0) return 1;
    size_t tot = o - out, w = 0;
    while (w < tot) { ssize_t r = write(fo, out + w, tot - w); if (r <= 0) return 1; w += r; }
    close(fo);
#ifdef TIMING
    double t4 = now();
    fprintf(stderr, "read+parse %.2f | sort %.2f | serialize %.2f | write %.2f | total %.2f ms\n",
            t1 - t0, t2 - t1, t3 - t2, t4 - t3, t4 - t0);
#endif
    return 0;
}
