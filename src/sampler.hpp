#pragma once

#include <cstdint>
#include <array>
#include <glm/glm.hpp>

/// <summary>
/// 采样器类，提供采样1个数与采样2个数的方法
///	Hash-based Owen-scrambled Sobol sequence generator based on:
/// Practical Hash-based Owen Scrambling - Brent Burley
/// https://jcgt.org/published/0009/04/01/
/// </summary>
class Sampler{
public:
	Sampler();

	/// <summary>
	/// 获取一个指定维度的Sobol序列值
	/// </summary>
	/// <param name="dim">维度</param>
	/// <returns>一个指定维度的Sobol序列值</returns>
	float get1D(uint8_t dim);
	/// <summary>
	/// 获取两个指定维度的Sobol序列值
	/// </summary>
	/// <param name="dim">维度</param>
	/// <returns>两个指定维度的Sobol序列值</returns>
	glm::vec2 get2D(uint8_t dim);
	/// <summary>
	/// Called with e.g. linear pixel index before sampling pixel
	/// </summary>
	/// <param name="startSeed">起始种子</param>
	void initialize(uint32_t startSeed);
	/// <summary>
	/// Called with e.g. ray path index before each pixel sample
	/// </summary>
	/// <param name="index">索引</param>
	void setIndex(uint32_t index);
	/// <summary>
	/// Called at the beginning of e.g. each ray bounce/scatter to effectively 
	/// decorrelate the previously sampled dimensions from the next ("padding").
	/// </summary>
	void shuffle();
private:
	/// <summary>
	/// 所支持的Sobol序列的最高维度
	/// </summary>
	static constexpr uint32_t MAX_DIM = 8;
	/// <summary>
	/// Sobol序列的位反转方向数
	/// </summary>
	std::array<std::array<uint32_t, 32>, MAX_DIM> m_reversedDirectionNumbers;
	inline static thread_local uint32_t m_baseSeed = 0u, m_seed = 0u, m_sequence = 0u, m_bitReversedIndex = 0u, m_shuffledIndex = 0u;
	/// <summary>
	/// 全局种子
	/// </summary>
	uint32_t m_globalSeed;

	/// <summary>
	/// 用于保证只会初始化一次全局种子
	/// </summary>
	/// <returns>全局种子</returns>
	static uint32_t getGlobalSeed();
	/// <summary>
	/// 用于保证只会初始化一次方向数
	/// </summary>
	/// <returns>Sobol序列的方向数</returns>
	static std::array<std::array<uint32_t, 32>, MAX_DIM> getBitReversedDirections();
	/// <summary>
	/// 生成Sobol序列的方向数
	/// </summary>
	/// <returns>Sobol序列的方向数</returns>
	static std::array<std::array<uint32_t, 32>, MAX_DIM> generateBitReversedDirections();

	/// <summary>
	/// nested_uniform_scramble, but mostly avoids the first bit-reversal.
	/// </summary>
	/// <param name="bitReversedX">位反转后的数</param>
	/// <param name="seed">种子</param>
	/// <returns>扰动后的Sobol序列值</returns>
	uint32_t scramble(uint32_t bitReversedX, uint32_t seed);
	/// <summary>
	/// 计算给定数的哈希值
	///	2-round constants with lowest bias from:
	/// https://github.com/skeeto/hash-prospector
	/// </summary>
	/// <param name="x">要计算哈希值的数</param>
	/// <returns>给定数的哈希值</returns>
	uint32_t hash(uint32_t x);
	/// <summary>
	/// Boost hash combine
	/// </summary>
	/// <param name="seed">种子</param>
	/// <param name="v">另一个哈希值</param>
	/// <returns>结合后的哈希值</returns>
	uint32_t hashCombine(uint32_t seed, uint32_t v);
	/// <summary>
	/// 根据给定索引与维度生成Sobol序列值
	/// </summary>
	/// <param name="index">索引</param>
	/// <param name="dim">维度</param>
	/// <returns>生成的Sobol序列值</returns>
	uint32_t sobol(uint32_t index, int dim);
	/// <summary>
	/// 将给定数位反转
	/// </summary>
	/// <param name="n">给定数</param>
	/// <returns>位反转后的数</returns>
	static uint32_t reverseBits(uint32_t n);
};

