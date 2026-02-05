#include "string8.hpp"

#define XOR ^

auto inline fnv_1a_32bit(string8 s) -> u32 {
    const u32 FNV_offset_basis = 0x811c9dc5;
    const u32 FNV_prime = 0x01000193;

    u32 hash = FNV_offset_basis;

    for (i32 i = 0; i < s.size; i++) {
        unsigned char c = s.data[i];
        hash = hash XOR c;
        hash = hash * FNV_prime;
    }
    return hash;
}

auto inline fnv_1a_64bit(string8 s) -> u64 {
    const u64 FNV_offset_basis = 0xcbf29ce484222325;
    const u64 FNV_prime = 0x00000100000001b3;

    u64 hash = FNV_offset_basis;

    // NOTE: Potentially have to add detection for if the hash becomes zero. Appearently a weakness of the algorithm.
    for (i32 i = 0; i < s.size; i++) {
        unsigned char c = s.data[i];
        hash = hash XOR c;
        hash = hash * FNV_prime;
    }
    return hash;
}

auto inline hash32(string8 s) {
    return fnv_1a_32bit(s);
}

auto inline hash64(string8 s) {
    return fnv_1a_64bit(s);
}

// TODO: Testing
//
/*Test a hash function in terms of properties and invariants, not “correctness” in the cryptographic sense (unless you have official test vectors).*/
/**/
/*1) Determinism*/
/**/
/*Same input → same output (including across runs if it’s supposed to be stable).*/
/**/
/*For many random inputs: hash(x) == hash(x) and repeated calls match.*/
/**/
/*If you persist hashes, run the same tests in a fresh process.*/
/**/
/*2) Known-answer tests (test vectors)*/
/**/
/*If the hash is standardized (SHA-256, xxHash, MurmurHash3, CityHash, etc.), use published vectors.*/
/**/
/*Include: empty input, short inputs, long inputs, binary with \0, non-ASCII, incremental patterns.*/
/**/
/*If it’s your own algorithm, create “golden” outputs from a trusted reference implementation and lock them in.*/
/**/
/*3) Boundary and encoding cases*/
/**/
/*Cover inputs that often break implementations:*/
/**/
/*Empty buffer*/
/**/
/*Length 1, 2, 3, … up through a few block boundaries (e.g., 63/64/65 bytes, 127/128/129)*/
/**/
/*Very large input (streaming or chunked)*/
/**/
/*Inputs with embedded zeros, high-bit bytes (0x80..0xFF)*/
/**/
/*Unicode strings: verify you hash bytes vs code points consistently (UTF-8 vs UTF-16)*/
/**/
/*4) Avalanche behavior (non-crypto)*/
/**/
/*A small change in input should flip many output bits (for decent non-crypto hashes too).*/
/**/
/*Method:*/
/**/
/*Pick random inputs x.*/
/**/
/*Create x' by flipping 1 bit (or changing 1 byte).*/
/**/
/*Compute h = hash(x), h' = hash(x').*/
/**/
/*Measure Hamming distance of outputs.*/
/**/
/*Expectations:*/
/**/
/*For an n-bit output, average distance near n/2.*/
/**/
/*Set a loose acceptance band (statistical): e.g., for 64-bit hash, average maybe ~32 with tolerance.*/
/**/
/*This won’t prove quality, but catches broken mixing.*/
/**/
/*5) Collision checks (limited, statistical)*/
/**/
/*You cannot “test for no collisions” (collisions must exist), but you can detect obvious problems.*/
/**/
/*Generate N random inputs, hash them, count duplicates.*/
/**/
/*For uniform k-bit hashes, expected collisions ~ N(N-1)/(2*2^k).*/
/**/
/*Examples:*/
/**/
/*For 32-bit hashes, collisions appear quickly; that’s normal.*/
/**/
/*For 64-bit, with N=1,000,000, expected collisions ≈ 0.027 (rare). If you see many, it’s bad.*/
/**/
/*Keep this as a smoke test; don’t make it flaky:*/
/**/
/*Use fixed RNG seed.*/
/**/
/*Pick N so expectations are stable.*/
/**/
/*6) Distribution sanity checks*/
/**/
/*Basic checks for bias:*/
/**/
/*Bucket test: take top b bits (or low b bits) and histogram into 2^b buckets (e.g., b=12 → 4096 buckets).*/
/**/
/*Verify counts are near uniform (chi-square style) or at least within loose bounds.*/
/**/
/*Also test both low and high bits; some hashes have weak low bits.*/
/**/
/*7) Incremental/streaming consistency (if applicable)*/
/**/
/*If your hash supports chunking:*/
/**/
/*hash(data) should equal hash(chunk1 + chunk2 + ...) when processed incrementally.*/
/**/
/*Test with random chunk sizes, including 1-byte chunks.*/
/**/
/*8) Seed handling (if the hash is keyed/seeded)*/
/**/
/*Same seed + same input → same output.*/
/**/
/*Different seeds → different outputs (often for almost all inputs).*/
/**/
/*Edge seeds: 0, 1, max, alternating bits.*/
/**/
/*9) Performance and non-functional tests (optional)*/
/**/
/*Not unit tests, but useful:*/
/**/
/*Time hashing typical sizes*/
/**/
/*Ensure no allocations (if required)*/
/**/
/*Thread-safety (if used concurrently)*/
/**/
/*10) Fuzzing and differential testing*/
/**/
/*Best way to find subtle bugs:*/
/**/
/*Compare your implementation to a reference (or a second independent implementation).*/
/**/
/*Feed random bytes of random lengths and assert equality.*/
/**/
/*Minimal “good” test set (practical)*/
/**/
/*Determinism + cross-run stability (if required).*/
/**/
/*Test vectors (or goldens).*/
/**/
/*Boundary lengths around internal block sizes.*/
/**/
/*Incremental vs one-shot (if supported).*/
/**/
/*Collision smoke test (fixed seed, reasonable N).*/
/**/
/*Avalanche average distance check (fixed seed, moderate samples).*/
