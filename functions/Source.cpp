#include<cassert>
#include<cstdint>
#include<iostream>
#include<random>

#include <mmintrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>
#include <pmmintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>
#include <nmmintrin.h>
#include <wmmintrin.h>
#include <immintrin.h>

uint64_t get_some_moves(const uint64_t player, const uint64_t mask, const int dir)
{
	uint64_t flip_l, flip_r;
	uint64_t mask_l, mask_r;
	const int dir2 = dir + dir;

	flip_l = mask & (player << dir);      flip_r = mask & (player >> dir);
	flip_l |= mask & (flip_l << dir);     flip_r |= mask & (flip_r >> dir);
	mask_l = mask & (mask << dir);        mask_r = mask & (mask >> dir);
	flip_l |= mask_l & (flip_l << dir2);  flip_r |= mask_r & (flip_r >> dir2);
	flip_l |= mask_l & (flip_l << dir2);  flip_r |= mask_r & (flip_r >> dir2);

	return (flip_l << dir) | (flip_r >> dir);
}
uint64_t get_moves(const uint64_t player, const uint64_t opponent)
{
	const uint64_t mask = opponent & 0x7E7E7E7E7E7E7E7Eull;

	return (get_some_moves(player, mask, 1)
		| get_some_moves(player, opponent, 8)
		| get_some_moves(player, mask, 7)
		| get_some_moves(player, mask, 9))
		& ~(player | opponent);
}

int bit_count(const uint64_t b)
{
	uint64_t c = b
		- ((b >> 1) & 0x7777777777777777ULL)
		- ((b >> 2) & 0x3333333333333333ULL)
		- ((b >> 3) & 0x1111111111111111ULL);
	c = ((c + (c >> 4)) & 0x0F0F0F0F0F0F0F0FULL) * 0x0101010101010101ULL;

	return  (int)(c >> 56);
}

__m256i flipVertical(const __m256i dbd) {
	const __m256i byteswap_table = _mm256_set_epi8(
		8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7
	);
	return _mm256_shuffle_epi8(dbd, byteswap_table);
}
__m256i upperbit(__m256i p) {
	p = _mm256_or_si256(p, _mm256_srli_epi64(p, 1));
	p = _mm256_or_si256(p, _mm256_srli_epi64(p, 2));
	p = _mm256_or_si256(p, _mm256_srli_epi64(p, 4));
	p = _mm256_andnot_si256(_mm256_srli_epi64(p, 1), p);
	p = flipVertical(p);
	p = _mm256_and_si256(p, _mm256_sub_epi64(_mm256_setzero_si256(), p));
	return flipVertical(p);
}
uint64_t hor(const __m256i lhs) {
	__m128i lhs_xz_yw = _mm_or_si128(_mm256_castsi256_si128(lhs), _mm256_extractf128_si256(lhs, 1));
	return uint64_t(_mm_cvtsi128_si64(lhs_xz_yw) | _mm_extract_epi64(lhs_xz_yw, 1));
}
uint64_t flip(const uint64_t player, const uint64_t opponent, const int pos) {
	const __m256i pppp = _mm256_set1_epi64x(player);
	const __m256i oooo = _mm256_set1_epi64x(opponent);
	const __m256i OM = _mm256_and_si256(oooo, _mm256_set_epi64x(
		0xFFFFFFFFFFFFFFFFULL, 0x7E7E7E7E7E7E7E7EULL, 0x7E7E7E7E7E7E7E7EULL, 0x7E7E7E7E7E7E7E7EULL));
	__m256i flipped, outflank, mask;
	mask = _mm256_srli_epi64(_mm256_set_epi64x(
		0x0080808080808080ULL, 0x7F00000000000000ULL, 0x0102040810204000ULL, 0x0040201008040201ULL), 63 - pos);
	outflank = _mm256_and_si256(upperbit(_mm256_andnot_si256(OM, mask)), pppp);
	flipped = _mm256_and_si256(_mm256_slli_epi64(_mm256_sub_epi64(_mm256_setzero_si256(), outflank), 1), mask);
	mask = _mm256_slli_epi64(_mm256_set_epi64x(
		0x0101010101010100ULL, 0x00000000000000FEULL, 0x0002040810204080ULL, 0x8040201008040200ULL), pos);
	outflank = _mm256_and_si256(_mm256_and_si256(mask, _mm256_add_epi64(_mm256_or_si256(
		OM, _mm256_andnot_si256(mask, _mm256_set1_epi8(int8_t(0xFF)))), _mm256_set1_epi64x(1))), pppp);
	flipped = _mm256_or_si256(flipped, _mm256_and_si256(_mm256_sub_epi64(outflank, _mm256_add_epi64(
		_mm256_cmpeq_epi64(outflank, _mm256_setzero_si256()), _mm256_set1_epi64x(1))), mask));
	return hor(flipped);
}

uint64_t flip_slow(const uint64_t player, const uint64_t opponent, const int pos) {
	const int dir[8] = { -9,-8,-7,-1,1,7,8,9 };
	const uint64_t edge[8] = {
		0x01010101010101ffull,
		0x00000000000000ffull,
		0x80808080808080ffull,
		0x0101010101010101ull,
		0x8080808080808080ull,
		0xff01010101010101ull,
		0xff00000000000000ull,
		0xff80808080808080ull
	};
	uint64_t flipped = 0;
	for (int d = 0; d < 8; ++d) {
		if (((1ULL << pos) & edge[d]) == 0) {
			uint64_t f = 0;
			int x = pos + dir[d];
			for (; (opponent & (1ULL << x)) && ((1ULL << x) & edge[d]) == 0; x += dir[d]) {
				f |= (1ULL << x);
			}
			if (player & (1ULL << x)) flipped |= f;
		}
	}
	return flipped;
}

void test(const int seed, const int length) {
	std::mt19937_64 rnd(seed);
	const uint64_t initial_occupied = 0x0000'0018'1800'0000ULL;

	uint64_t count_testnum_flip_nonzero = 0;
	uint64_t count_testnum_flip_zero = 0;

	for (int iteration = 0; iteration < length; ++iteration) {
		uint64_t player = 0, opponent = 0;
		for (uint64_t i = 1; i; i <<= 1) {
			switch (std::uniform_int_distribution<uint64_t>(0, (i & initial_occupied) ? 1 : (2 + (iteration % 2)))(rnd)) {
			case 0:
				player |= i;
				break;
			case 1:
				opponent |= i;
				break;
			}
		}
		assert((player & opponent) == 0);
		assert(((player | opponent) & initial_occupied) == initial_occupied);
		const uint64_t empty = ~(player | opponent);
		const uint64_t move_bb = get_moves(player, opponent);
		assert((empty | move_bb) == empty);
		assert(bit_count(move_bb) == _mm_popcnt_u64(move_bb));
		for (int pos = 0; pos < 64; ++pos)if ((1ULL << pos) & empty) {
			const uint64_t flip1 = flip(player, opponent, pos);
			const uint64_t flip2 = flip_slow(player, opponent, pos);
			assert(flip1 == flip2);
			assert((flip1 | opponent) == opponent);
			assert((flip1 != 0) == (((1ULL << pos) & move_bb) != 0));
			if (flip1 == 0) {
				++count_testnum_flip_zero;
			}
			else {
				++count_testnum_flip_nonzero;
			}
		}
	}
	std::cout << "test succeeded" << std::endl;
	std::cout << "count_testnum_flip_nonzero = " << count_testnum_flip_nonzero << std::endl;
	std::cout << "count_testnum_flip_zero = " << count_testnum_flip_zero << std::endl;
}

int main(int argc, char *argv[]) {

	test(42, 10000000);

	return 0;
}