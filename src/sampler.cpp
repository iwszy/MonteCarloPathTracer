#include "sampler.hpp"

Sampler::Sampler() : m_reversedDirectionNumbers(getBitReversedDirections()), m_globalSeed(getGlobalSeed()) {
    
}

float Sampler::get1D(uint8_t dim) {
    return static_cast<float>(scramble(sobol(m_shuffledIndex, dim), hashCombine(m_seed, hash(dim)))) / (1ULL << 32);
}

float Sampler::getRandom(uint32_t salt) {
    return static_cast<float>(scramble(m_bitReversedIndex, hashCombine(m_seed, hash(salt + 0x9e3779b9u)))) / (1ULL << 32);
}

glm::vec2 Sampler::get2D(uint8_t dim) {
    return {get1D(dim), get1D(dim + 1)};
}

void Sampler::initialize(uint32_t startSeed) {
    m_baseSeed = hashCombine(m_globalSeed, hash(startSeed));
}

void Sampler::setIndex(uint32_t index) {
    m_sequence = 0u;
    m_seed = m_baseSeed;
    m_bitReversedIndex = reverseBits(index);
    m_shuffledIndex = index;
}

void Sampler::shuffle() {
    m_seed = hashCombine(m_baseSeed, hash(++m_sequence));
    m_shuffledIndex = scramble(m_bitReversedIndex, m_seed);
}

uint32_t Sampler::sobol(uint32_t index, int dim) {
    uint32_t reversedIndex = reverseBits(index);
    uint32_t result = 0;
    for (int j = 0; reversedIndex != 0; reversedIndex >>= 1U, j++) {
    	result ^= (reversedIndex & 1U) * m_reversedDirectionNumbers[dim][j];
    }
    return result;
}

// 固定默认种子：同一场景、同一 spp 的渲染必须可复现。
// 此前这里用 std::random_device，导致每次运行的噪声实现都不同，A/B 对比与回归测试无法逐像素比较。
// 需要换一组噪声实现时，改这个常量即可。
uint32_t Sampler::getGlobalSeed() {
    static uint32_t globalSeed = 0u;
    return globalSeed;
}

std::array<std::array<uint32_t, 32>, Sampler::MAX_DIM> Sampler::getBitReversedDirections() {
    static std::array<std::array<uint32_t, 32>, MAX_DIM> reversedDirectionNumbers = generateBitReversedDirections();
    return reversedDirectionNumbers;
}


 std::array<std::array<uint32_t, 32>, Sampler::MAX_DIM> Sampler::generateBitReversedDirections() {
	// Based on code and data from: https://web.maths.unsw.edu.au/~fkuo/sobol/
	// The bits are also reversed at compile-time to optimize Owen-scrambling.
	//从"new-joe-kuo-6.21201"获取的第2到9维的参数
    constexpr uint32_t s[] = { 1, 2, 3, 3, 4, 4, 5, 5};
    constexpr uint32_t a[] = { 0, 1, 1, 2, 1, 4, 2, 4};
    constexpr uint32_t m[][s[std::size(s) - 1]] = {
        { 1 }, { 1, 3 },
        {1, 3, 1}, {1, 1, 1},
        {1, 1, 3, 3}, {1, 3, 5, 13},
        {1, 1, 5, 5, 17}, {1, 1, 5, 5, 5} };
	std::array<std::array<uint32_t, 32>, MAX_DIM> reversedDirectionNumbers;
    for (uint32_t dim = 0; dim < reversedDirectionNumbers.size(); dim++) {
        for (uint32_t bit = 0; bit < s[dim]; bit++) {
            reversedDirectionNumbers[dim][bit] = m[dim][bit] << (31 - bit);
        }
        for (uint32_t bit = s[dim]; bit < 32; bit++) {
            reversedDirectionNumbers[dim][bit] = reversedDirectionNumbers[dim][bit - s[dim]] ^ (reversedDirectionNumbers[dim][bit - s[dim]] >> s[dim]);
            for (uint32_t k = 1; k < s[dim]; k++) {
                reversedDirectionNumbers[dim][bit] ^= (((a[dim] >> (s[dim] - 1 - k)) & 1) * reversedDirectionNumbers[dim][bit - k]);
            }
        }
        for (uint32_t bit = 0; bit < 32; bit++) {
            reversedDirectionNumbers[dim][bit] = reverseBits(reversedDirectionNumbers[dim][bit]);
        }
    }
    return reversedDirectionNumbers;
}

uint32_t Sampler::scramble(uint32_t bitReversedX, uint32_t seed) {
    // Improved Laine-Karras hash by Nathan Vegdahl
	// https://psychopath.io/post/2021_01_30_building_a_better_lk_hash
    bitReversedX ^= bitReversedX * 0x3d20adea;
    bitReversedX += seed;
    bitReversedX *= (seed >> 16) | 1;
    bitReversedX ^= bitReversedX * 0x05526c56;
    bitReversedX ^= bitReversedX * 0x53a22864;
    return reverseBits(bitReversedX);
}

uint32_t Sampler::hash(uint32_t x) {
    x ^= x >> 15;
    x *= 0xd168aaad;
    x ^= x >> 15;
    x *= 0xaf723597;
    x ^= x >> 15;
    return x;
}

uint32_t Sampler::hashCombine(uint32_t seed, uint32_t v) {
    return seed ^ (v + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

uint32_t Sampler::reverseBits(uint32_t n) {
    n = (n & 0xaaaaaaaau) >> 1 | (n & 0x55555555u) << 1;
    n = (n & 0xccccccccu) >> 2 | (n & 0x33333333u) << 2;
    n = (n & 0xf0f0f0f0u) >> 4 | (n & 0x0f0f0f0fu) << 4;
    n = (n & 0xff00ff00u) >> 8 | (n & 0x00ff00ffu) << 8;
    return (n >> 16) | (n << 16);
}