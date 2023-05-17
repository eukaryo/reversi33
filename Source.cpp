
#define _CRT_SECURE_NO_WARNINGS

#include<iostream>
#include<iomanip>
#include<chrono>
#include<fstream>

#include<vector>
#include<string>
#include<stack>
#include<map>
#include<set>
#include<unordered_map>
#include<unordered_set>

#include<algorithm>
#include<array>
#include<bitset>
#include<cassert>
#include<cstdint>
#include<exception>
#include<functional>
#include<limits>
#include<queue>
#include<numeric>
#include<tuple>
#include<regex>
#include<random>
#include<filesystem>

#include <execution>

#include <omp.h>

#ifdef _MSC_VER
#define NOMINMAX
#include <windows.h>
#endif

#include <mmintrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>
#include <pmmintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>
#include <nmmintrin.h>
#include <wmmintrin.h>
#include <immintrin.h>

//include <boost/multiprecision/cpp_int.hpp>
//typedef boost::multiprecision::cpp_int Bigint;

//#define REP(i, a, b) for (int64_t i = (int64_t)(a); i < (int64_t)(b); i++)
//#define rep(i, a) REP(i, 0, a)
//#define ALL(a) (a.begin()), (a.end())

std::vector<std::string>tokenlist;
std::vector<std::string>exprlist;

class BitVector1 {
public:

	std::string name;
	uint64_t idx;

	void init() {
		idx = tokenlist.size();
		tokenlist.push_back(name + ":" + std::to_string(idx));
	}

	BitVector1(std::string name_) : name{ name_ } { init(); }

	std::string tokenname() const {
		return tokenlist[idx];
	}
};

class BitVector64 {
public:

	std::string name;
	std::array<uint64_t, 64>idx;

	std::string tokenname(const uint64_t i) const {
		return tokenlist[idx[i]];
	}
	void init() {
		for (int i = 0; i < 64; ++i) {
			idx[i] = tokenlist.size();
			tokenlist.push_back(name + std::to_string(i) + ":" + std::to_string(idx[i]));
		}
	}

	void init(const uint64_t value) {
		init();
		for (int i = 0; i < 64; ++i) {
			if ((1ULL << i) & value) {
				exprlist.push_back(tokenname(i));
			}
			else {

				exprlist.push_back("~" + tokenname(i));
			}
		}
	}

	BitVector64(std::string name_) : name{ name_ } { init(); }
	BitVector64(std::string name_, uint64_t value) : name{ name_ } { init(value); }
};


BitVector64 bv_and(const BitVector64 x, const BitVector64 y, const std::string name) {
	BitVector64 result(name);
	for (int i = 0; i < 64; ++i) {
		exprlist.push_back(result.tokenname(i) + "=" + x.tokenname(i) + "&" + y.tokenname(i));
	}
	return result;
}

BitVector64 bv_and(const BitVector64 x, const uint64_t y, const std::string name) {
	BitVector64 result(name);
	for (int i = 0; i < 64; ++i) {
		const uint64_t bit = 1ULL << i;
		if (y & (1ULL << i)) {
			exprlist.push_back(result.tokenname(i) + "=" + x.tokenname(i));
		}
		else {
			exprlist.push_back("(~" + result.tokenname(i) + ")");
		}
	}
	return result;
}

BitVector64 bv_or(const BitVector64 x, const BitVector64 y, const std::string name) {
	BitVector64 result(name);
	for (int i = 0; i < 64; ++i) {
		exprlist.push_back(result.tokenname(i) + "=" + x.tokenname(i) + "|" + y.tokenname(i));
	}
	return result;
}

BitVector64 bv_xor(const BitVector64 x, const BitVector64 y, const std::string name) {
	BitVector64 result(name);
	for (int i = 0; i < 64; ++i) {
		//exprlist.push_back(result.tokenname(i) + "=(" + x.tokenname(i) + "&(~" + y.tokenname(i) + "))|((~" + x.tokenname(i) + ")&" + y.tokenname(i) + ")");

		//A=(B&(~C))|((~B)&C) は ((~A)|((B&(~C))|((~B)&C)))&(A|(~((B&(~C))|((~B)&C)))) と等価である。
		//CNFにすると
		//(A | B | ~C) & (A | ~B | C) & (~A | B | C) & (~A | ~B | ~C)
		const std::string A = result.tokenname(i);
		const std::string B = x.tokenname(i);
		const std::string C = y.tokenname(i);
		exprlist.push_back(A + "|" + B + "|(~" + C + ")");
		exprlist.push_back(A + "|(~" + B + ")|" + C);
		exprlist.push_back("(~" + A + ")|" + B + "|" + C);
		exprlist.push_back("(~" + A + ")|(~" + B + ")|(~" + C + ")");
	}
	return result;
}

BitVector64 bv_not(const BitVector64 x, const std::string name) {
	BitVector64 result(name);
	for (int i = 0; i < 64; ++i) {
		exprlist.push_back(result.tokenname(i) + "=(~" + x.tokenname(i) + ")");
	}
	return result;
}

BitVector64 bv_shli(const BitVector64 x, const int p, const std::string name) {
	assert(1 <= p && p <= 63);
	BitVector64 result(name);
	for (int i = 0; i < p; ++i) {
		exprlist.push_back("(~" + result.tokenname(i) + ")");
	}
	for (int i = p; i < 64; ++i) {
		exprlist.push_back(result.tokenname(i) + "=" + x.tokenname(i - p));
	}
	return result;
}

BitVector64 bv_shri(const BitVector64 x, const int p, const std::string name) {
	assert(1 <= p && p <= 63);
	BitVector64 result(name);
	for (int i = 0; i < (64 - p); ++i) {
		exprlist.push_back(result.tokenname(i) + "=" + x.tokenname(i + p));
	}
	for (int i = (64 - p); i < 64; ++i) {
		exprlist.push_back("(~" + result.tokenname(i) + ")");
	}
	return result;
}

void half_addr(const std::string tokenname_a, const std::string tokenname_b, const std::string tokenname_c, const std::string tokenname_s) {
	exprlist.push_back(tokenname_c + "=" + tokenname_a + "&" + tokenname_b);

	//exprlist.push_back(tokenname_s + "=(" + tokenname_a + "|" + tokenname_b + ")&(~" + tokenname_c + ")");
	//A=(B|C)&(~D) は ((~A)|((B|C)&(~D)))&(A|(~((B|C)&(~D)))) と等価である。
	//CNFにすると
	//(~A | ~D) & (A | ~B | D) & (A | ~C | D) & (~A | B | C)
	const std::string A = tokenname_s;
	const std::string B = tokenname_a;
	const std::string C = tokenname_b;
	const std::string D = tokenname_c;
	exprlist.push_back("(~" + A + ")|(~" + D + ")");
	exprlist.push_back(A + "|(~" + B + ")|" + D);
	exprlist.push_back(A + "|(~" + C + ")|" + D);
	exprlist.push_back("(~" + A + ")|" + B + "|" + C);
}

void full_addr(const std::string tokenname_a, const std::string tokenname_b, const std::string tokenname_x, const std::string tokenname_c, const std::string tokenname_s) {
	BitVector1 tmp_c1("tmp1_full_addr_c1");
	BitVector1 tmp_s("tmp1_full_addr_s");
	half_addr(tokenname_a, tokenname_b, tmp_c1.tokenname(), tmp_s.tokenname());
	BitVector1 tmp_c2("tmp1_full_addr_c2");
	half_addr(tmp_s.tokenname(), tokenname_x, tmp_c2.tokenname(), tokenname_s);
	exprlist.push_back(tokenname_c + "=" + tmp_c1.tokenname() + "|" + tmp_c2.tokenname());
}

BitVector64 bv_popcount(const BitVector64 x, const std::string name) {

	BitVector64 tmp1("tmp1_popcount_" + name);
	for (int i = 0; i < 64; i += 2) {
		half_addr(x.tokenname(i), x.tokenname(i + 1), tmp1.tokenname(i + 1), tmp1.tokenname(i));
	}
	BitVector64 tmp2("tmp2_popcount_" + name);
	for (int i = 0; i < 64; i += 4) {
		//2桁+2桁を書き下す
		BitVector1 carry("tmp2_popcount_carry_" + std::to_string(i) + "_" + name);
		half_addr(tmp1.tokenname(i), tmp1.tokenname(i + 2), carry.tokenname(), tmp2.tokenname(i));
		full_addr(tmp1.tokenname(i + 1), tmp1.tokenname(i + 3), carry.tokenname(), tmp2.tokenname(i + 2), tmp2.tokenname(i + 1));
		exprlist.push_back("(~" + tmp2.tokenname(i + 3) + ")");
	}
	BitVector64 tmp3("tmp3_popcount_" + name);
	for (int i = 0; i < 64; i += 8) {
		//3桁+3桁を書き下す
		BitVector1 carry1("tmp3_popcount_carry1_" + std::to_string(i) + "_" + name);
		BitVector1 carry2("tmp3_popcount_carry2_" + std::to_string(i) + "_" + name);
		half_addr(tmp2.tokenname(i), tmp2.tokenname(i + 4), carry1.tokenname(), tmp3.tokenname(i));
		full_addr(tmp2.tokenname(i + 1), tmp2.tokenname(i + 5), carry1.tokenname(), carry2.tokenname(), tmp3.tokenname(i + 1));
		full_addr(tmp2.tokenname(i + 2), tmp2.tokenname(i + 6), carry2.tokenname(), tmp3.tokenname(i + 3), tmp3.tokenname(i + 2));
		for (int j = 4; j < 8; ++j) {
			exprlist.push_back("(~" + tmp3.tokenname(i + j) + ")");
		}
	}
	BitVector64 tmp4("tmp4_popcount_" + name);
	for (int i = 0; i < 64; i += 16) {
		//4桁+4桁を書き下す
		BitVector1 carry1("tmp4_popcount_carry1_" + std::to_string(i) + "_" + name);
		BitVector1 carry2("tmp4_popcount_carry2_" + std::to_string(i) + "_" + name);
		BitVector1 carry3("tmp4_popcount_carry3_" + std::to_string(i) + "_" + name);
		half_addr(tmp3.tokenname(i), tmp3.tokenname(i + 8), carry1.tokenname(), tmp4.tokenname(i));
		full_addr(tmp3.tokenname(i + 1), tmp3.tokenname(i + 9), carry1.tokenname(), carry2.tokenname(), tmp4.tokenname(i + 1));
		full_addr(tmp3.tokenname(i + 2), tmp3.tokenname(i + 10), carry2.tokenname(), carry3.tokenname(), tmp4.tokenname(i + 2));
		full_addr(tmp3.tokenname(i + 3), tmp3.tokenname(i + 11), carry3.tokenname(), tmp4.tokenname(i + 4), tmp4.tokenname(i + 3));
		for (int j = 5; j < 16; ++j) {
			exprlist.push_back("(~" + tmp4.tokenname(i + j) + ")");
		}
	}
	BitVector64 tmp5("tmp5_popcount_" + name);
	for (int i = 0; i < 64; i += 32) {
		//5桁+5桁を書き下す
		BitVector1 carry1("tmp5_popcount_carry1_" + std::to_string(i) + "_" + name);
		BitVector1 carry2("tmp5_popcount_carry2_" + std::to_string(i) + "_" + name);
		BitVector1 carry3("tmp5_popcount_carry3_" + std::to_string(i) + "_" + name);
		BitVector1 carry4("tmp5_popcount_carry4_" + std::to_string(i) + "_" + name);
		half_addr(tmp4.tokenname(i), tmp4.tokenname(i + 16), carry1.tokenname(), tmp5.tokenname(i));
		full_addr(tmp4.tokenname(i + 1), tmp4.tokenname(i + 17), carry1.tokenname(), carry2.tokenname(), tmp5.tokenname(i + 1));
		full_addr(tmp4.tokenname(i + 2), tmp4.tokenname(i + 18), carry2.tokenname(), carry3.tokenname(), tmp5.tokenname(i + 2));
		full_addr(tmp4.tokenname(i + 3), tmp4.tokenname(i + 19), carry3.tokenname(), carry4.tokenname(), tmp5.tokenname(i + 3));
		full_addr(tmp4.tokenname(i + 4), tmp4.tokenname(i + 20), carry4.tokenname(), tmp5.tokenname(i + 5), tmp5.tokenname(i + 4));
		for (int j = 6; j < 32; ++j) {
			exprlist.push_back("(~" + tmp5.tokenname(i + j) + ")");
		}
	}

	BitVector64 result(name);
	//6桁+6桁を書き下す
	BitVector1 carry1("tmp6_popcount_carry1_" + name);
	BitVector1 carry2("tmp6_popcount_carry2_" + name);
	BitVector1 carry3("tmp6_popcount_carry3_" + name);
	BitVector1 carry4("tmp6_popcount_carry4_" + name);
	BitVector1 carry5("tmp6_popcount_carry5_" + name);
	half_addr(tmp5.tokenname(0), tmp5.tokenname(32), carry1.tokenname(), result.tokenname(0));
	full_addr(tmp5.tokenname(1), tmp5.tokenname(33), carry1.tokenname(), carry2.tokenname(), result.tokenname(1));
	full_addr(tmp5.tokenname(2), tmp5.tokenname(34), carry2.tokenname(), carry3.tokenname(), result.tokenname(2));
	full_addr(tmp5.tokenname(3), tmp5.tokenname(35), carry3.tokenname(), carry4.tokenname(), result.tokenname(3));
	full_addr(tmp5.tokenname(4), tmp5.tokenname(36), carry4.tokenname(), carry5.tokenname(), result.tokenname(4));
	full_addr(tmp5.tokenname(5), tmp5.tokenname(37), carry5.tokenname(), result.tokenname(6), result.tokenname(5));
	for (int j = 7; j < 64; ++j) {
		exprlist.push_back("(~" + result.tokenname(j) + ")");
	}
	return result;
}

BitVector64 increment(const BitVector64 x, const std::string name) {
	BitVector64 carry("tmp_increment_carry_" + name);
	BitVector64 result(name);
	exprlist.push_back(carry.tokenname(0) + "=" + x.tokenname(0));
	exprlist.push_back(result.tokenname(0) + "=~" + x.tokenname(0));
	for (int i = 1; i < 64; ++i) {
		exprlist.push_back(carry.tokenname(i) + "=" + x.tokenname(i) + "&" + carry.tokenname(i - 1));

		//exprlist.push_back(result.tokenname(i) + "=(" + x.tokenname(i) + "&(~" + carry.tokenname(i - 1) + "))|((~" + x.tokenname(i) + ")&" + carry.tokenname(i - 1) + ")");

		//A=(B&(~C))|((~B)&C) は ((~A)|((B&(~C))|((~B)&C)))&(A|(~((B&(~C))|((~B)&C)))) と等価である。
		//CNFにすると
		//(A | B | ~C) & (A | ~B | C) & (~A | B | C) & (~A | ~B | ~C)
		const std::string A = result.tokenname(i);
		const std::string B = x.tokenname(i);
		const std::string C = carry.tokenname(i - 1);
		exprlist.push_back(A + "|" + B + "|(~" + C + ")");
		exprlist.push_back(A + "|(~" + B + ")|" + C);
		exprlist.push_back("(~" + A + ")|" + B + "|" + C);
		exprlist.push_back("(~" + A + ")|(~" + B + ")|(~" + C + ")");
	}
	return result;
}

BitVector64 bv_neg(const BitVector64 x, const std::string name) {
	return increment(bv_not(x, "tmp_neg_not_" + name), name);
}

void ifthenelse(const std::string tokenname_a, const std::string tokenname_b, const std::string tokenname_c, const std::string tokenname_dest) {
	// dest:=if a then b else c
	//exprlist.push_back(tokenname_dest + "=(" + tokenname_a + "&" + tokenname_b + ")|((~" + tokenname_a + ")&" + tokenname_c + ")");

	//A=(B&C)|((~B)&D) は ((~A)|((B&C)|((~B)&D)))&(A|(~((B&C)|((~B)&D)))) と等価である。
	//CNFにすると
	//(A | B | ~D) & (~A | B | D) & (~A | C | D) & (A | ~B | ~C) & (~A | ~B | C)
	//Cがfalseの場合
	//(A | B | ~D) & (~A | B | D) & (~A | D) & (~A | ~B)

	std::string A = tokenname_dest;
	std::string B = tokenname_a;
	std::string C = tokenname_b;
	std::string D = tokenname_c;

	const std::regex token = std::regex(R"([a-zA-Z_][a-zA-Z0-9_:]*)");

	assert(std::regex_match(A, token));
	assert(std::regex_match(B, token));
	assert(std::regex_match(D, token));

	if (C == "0") {
		exprlist.push_back(A + "|" + B + "|(~" + D + ")");
		exprlist.push_back("(~" + A + ")|" + B + "|" + D);
		exprlist.push_back("(~" + A + ")|" + D);
		exprlist.push_back("(~" + A + ")|(~" + B + ")");
	}
	else {
		assert(std::regex_match(C, token));
		exprlist.push_back(A + "|" + B + "|(~" + D + ")");
		exprlist.push_back("(~" + A + ")|" + B + "|" + D);
		exprlist.push_back("(~" + A + ")|" + C + "|" + D);
		exprlist.push_back(A + "|(~" + B + ")|(~" + C + ")");
		exprlist.push_back("(~" + A + ")|(~" + B + ")|" + C);
	}
}

BitVector64 ifthenelse(const BitVector64 a, const BitVector64 b, const BitVector64 c, const std::string name) {
	// if a then b else c
	BitVector64 result(name);
	for (int i = 0; i < 64; ++i) {
		ifthenelse(a.tokenname(i), b.tokenname(i), c.tokenname(i), result.tokenname(i));
	}
	return result;
}

BitVector64 ifthenelse(const BitVector1 a, const BitVector64 b, const BitVector64 c, const std::string name) {
	// if a then b else c
	BitVector64 result(name);
	for (int i = 0; i < 64; ++i) {
		ifthenelse(a.tokenname(), b.tokenname(i), c.tokenname(i), result.tokenname(i));
	}
	return result;
}

BitVector64 least_significant_bit(const BitVector64 x, const std::string name) {
	//xのlsbだけが立っているbitboardを返す。x==0なら0を返す。
	BitVector64 tmp("tmp_lsb_" + name);
	BitVector64 result(name);
	exprlist.push_back(tmp.tokenname(0) + "=" + x.tokenname(0));
	for (int i = 1; i < 64; ++i) {
		exprlist.push_back(tmp.tokenname(i) + "=" + x.tokenname(i) + "|" + tmp.tokenname(i - 1));
	}
	exprlist.push_back(result.tokenname(0) + "=" + tmp.tokenname(0));
	for (int i = 1; i < 64; ++i) {
		exprlist.push_back(result.tokenname(i) + "=" + tmp.tokenname(i) + "&(~" + tmp.tokenname(i - 1) + ")");
	}
	return result;
}

BitVector64 most_significant_bit(const BitVector64 x, const std::string name) {
	//xのmsbだけが立っているbitboardを返す。x==0なら0を返す。
	BitVector64 tmp("tmp_msb_" + name);
	BitVector64 result(name);
	exprlist.push_back(tmp.tokenname(63) + "=" + x.tokenname(63));
	for (int i = 62; i >= 0; --i) {
		exprlist.push_back(tmp.tokenname(i) + "=" + x.tokenname(i) + "|" + tmp.tokenname(i + 1));
	}
	exprlist.push_back(result.tokenname(63) + "=" + tmp.tokenname(63));
	for (int i = 62; i >= 0; --i) {
		exprlist.push_back(result.tokenname(i) + "=" + tmp.tokenname(i) + "&(~" + tmp.tokenname(i + 1) + ")");
	}
	return result;
}

BitVector1 reduce_or(const BitVector64 x, const std::string name) {

	BitVector64 bits("tmp_reduce_" + name);

	for (int i = 0; i < 32; ++i) {
		exprlist.push_back(bits.tokenname(i + 32) + "=" + x.tokenname(i) + "|" + x.tokenname(i + 32));
	}
	for (int i = 0; i < 16; ++i) {
		exprlist.push_back(bits.tokenname(i + 16) + "=" + bits.tokenname(i + 32) + "|" + bits.tokenname(i + 16 + 32));
	}
	for (int i = 0; i < 8; ++i) {
		exprlist.push_back(bits.tokenname(i + 8) + "=" + bits.tokenname(i + 16) + "|" + bits.tokenname(i + 8 + 16));
	}
	for (int i = 0; i < 4; ++i) {
		exprlist.push_back(bits.tokenname(i + 4) + "=" + bits.tokenname(i + 8) + "|" + bits.tokenname(i + 4 + 8));
	}
	for (int i = 0; i < 2; ++i) {
		exprlist.push_back(bits.tokenname(i + 2) + "=" + bits.tokenname(i + 4) + "|" + bits.tokenname(i + 2 + 4));
		exprlist.push_back("(~" + bits.tokenname(i) + ")");
	}
	BitVector1 result(name);
	exprlist.push_back(result.tokenname() + "=" + bits.tokenname(2) + "|" + bits.tokenname(3));
	return result;
}

BitVector64 zero_or_nonzero(const BitVector64 x, const std::string name) {
	BitVector1 tmp = reduce_or(x, "tmp_zero_or_nonzero_" + name);
	BitVector64 result(name);
	exprlist.push_back(result.tokenname(0) + "=" + tmp.tokenname());
	for (int i = 1; i < 64; ++i) {
		exprlist.push_back("(~" + result.tokenname(i) + ")");
	}
	return result;
}

BitVector64 decrement(const BitVector64 x, const std::string name) {
	BitVector64 tmp("tmp_decrement1_" + name);
	BitVector64 lsb("tmp_decrement2_" + name);
	BitVector64 result(name);
	exprlist.push_back(tmp.tokenname(0) + "=" + x.tokenname(0));
	for (int i = 1; i < 64; ++i) {
		exprlist.push_back(tmp.tokenname(i) + "=" + x.tokenname(i) + "|" + tmp.tokenname(i - 1));
	}
	exprlist.push_back(lsb.tokenname(0) + "=" + tmp.tokenname(0));
	for (int i = 1; i < 64; ++i) {
		exprlist.push_back(lsb.tokenname(i) + "=" + tmp.tokenname(i) + "&(~" + tmp.tokenname(i - 1) + ")");
	}
	/*
	x=  0b01010000のとき
	tmp=0b11110000
	lsb=0b00010000
	*/
	//ここもっと効率的に書けるかも
	return bv_xor(x, bv_or(lsb, bv_not(tmp, "tmp_decrement3_" + name), "tmp_decrement4_" + name), name);
}

BitVector1 greater(const BitVector64 a, const BitVector64 b, const std::string name) {
	//a>b

	BitVector64 difference = bv_xor(a, b, "tmp_greater_diff_" + name);
	BitVector64 msb_difference = most_significant_bit(difference, "tmp_greater_msb_" + name);

	return reduce_or(bv_and(a, msb_difference, "tmp_msb_and_a_" + name), name);
}

void equal_constraint(const BitVector64 a, const BitVector64 b) {
	for (int i = 0; i < 64; ++i) {
		exprlist.push_back(a.tokenname(i) + "=" + b.tokenname(i));
	}
}
void equal_constraint(const BitVector64 a, const uint64_t b) {
	for (int i = 0; i < 64; ++i) {
		if ((1ULL << i) & b) {
			exprlist.push_back(a.tokenname(i));
		}
		else {
			exprlist.push_back("~" + a.tokenname(i));
		}
	}
}
void equal_constraint(const BitVector1 a, const BitVector1 b) {
	exprlist.push_back(a.tokenname() + "=" + b.tokenname());
}
void greater_or_equal_constraint(const BitVector64 a, const BitVector64 b, const std::string name) {
	BitVector1 f1 = greater(a, b, "tmp_goe_1_" + name);
	BitVector1 f2 = reduce_or(bv_xor(a, b, "tmp_goe_2_" + name), "tmp_goe_3_" + name);
	exprlist.push_back(f1.tokenname() + "|(~" + f2.tokenname() + ")");
}

BitVector64 bv_shlv(const BitVector64 x, const BitVector64 p, const std::string name) {
	BitVector64 tmp1("tmp1_shlv_" + name);
	BitVector64 tmp2("tmp2_shlv_" + name);
	BitVector64 tmp3("tmp3_shlv_" + name);
	BitVector64 tmp4("tmp4_shlv_" + name);
	BitVector64 tmp5("tmp5_shlv_" + name);
	BitVector64 result(name);
	for (int i = 0; i < 64; ++i) {
		ifthenelse(p.tokenname(0), i < 1 ? "0" : x.tokenname(i - 1), x.tokenname(i), tmp1.tokenname(i));
	}
	for (int i = 0; i < 64; ++i) {
		ifthenelse(p.tokenname(1), i < 2 ? "0" : tmp1.tokenname(i - 2), tmp1.tokenname(i), tmp2.tokenname(i));
	}
	for (int i = 0; i < 64; ++i) {
		ifthenelse(p.tokenname(2), i < 4 ? "0" : tmp2.tokenname(i - 4), tmp2.tokenname(i), tmp3.tokenname(i));
	}
	for (int i = 0; i < 64; ++i) {
		ifthenelse(p.tokenname(3), i < 8 ? "0" : tmp3.tokenname(i - 8), tmp3.tokenname(i), tmp4.tokenname(i));
	}
	for (int i = 0; i < 64; ++i) {
		ifthenelse(p.tokenname(4), i < 16 ? "0" : tmp4.tokenname(i - 16), tmp4.tokenname(i), tmp5.tokenname(i));
	}
	for (int i = 0; i < 64; ++i) {
		ifthenelse(p.tokenname(5), i < 32 ? "0" : tmp5.tokenname(i - 32), tmp5.tokenname(i), result.tokenname(i));
	}
	return result;
}

BitVector64 bv_shrv(const BitVector64 x, const BitVector64 p, const std::string name) {
	BitVector64 tmp1("tmp1_shrv_" + name);
	BitVector64 tmp2("tmp2_shrv_" + name);
	BitVector64 tmp3("tmp3_shrv_" + name);
	BitVector64 tmp4("tmp4_shrv_" + name);
	BitVector64 tmp5("tmp5_shrv_" + name);
	BitVector64 result(name);
	for (int i = 0; i < 64; ++i) {
		ifthenelse(p.tokenname(0), i >= 64 - 1 ? "0" : x.tokenname(i + 1), x.tokenname(i), tmp1.tokenname(i));
	}
	for (int i = 0; i < 64; ++i) {
		ifthenelse(p.tokenname(1), i >= 64 - 2 ? "0" : tmp1.tokenname(i + 2), tmp1.tokenname(i), tmp2.tokenname(i));
	}
	for (int i = 0; i < 64; ++i) {
		ifthenelse(p.tokenname(2), i >= 64 - 4 ? "0" : tmp2.tokenname(i + 4), tmp2.tokenname(i), tmp3.tokenname(i));
	}
	for (int i = 0; i < 64; ++i) {
		ifthenelse(p.tokenname(3), i >= 64 - 8 ? "0" : tmp3.tokenname(i + 8), tmp3.tokenname(i), tmp4.tokenname(i));
	}
	for (int i = 0; i < 64; ++i) {
		ifthenelse(p.tokenname(4), i >= 64 - 16 ? "0" : tmp4.tokenname(i + 16), tmp4.tokenname(i), tmp5.tokenname(i));
	}
	for (int i = 0; i < 64; ++i) {
		ifthenelse(p.tokenname(5), i >= 64 - 32 ? "0" : tmp5.tokenname(i + 32), tmp5.tokenname(i), result.tokenname(i));
	}
	return result;
}

BitVector64 bv_byteswap(const BitVector64 x, const std::string name) {
	BitVector64 result(name);
	for (int i = 0; i < 64; i += 8) {
		for (int j = 0; j < 8; ++j) {
			exprlist.push_back(result.tokenname(i + j) + "=" + x.tokenname(56 - i + j));
		}
	}
	return result;
}

BitVector64 onebit_bsf(const BitVector64 x, const std::string name) {
	//xはたかだか1つのビットしか立っていないと仮定する。
	//立っているビットの位置を返す。立っていないならゼロを返す。
	BitVector64 tmp("tmp_onebit_bsf_" + name);
	exprlist.push_back("(~" + tmp.tokenname(63) + ")");
	for (int i = 62; i >= 0; --i) {
		exprlist.push_back(tmp.tokenname(i) + "=" + x.tokenname(i + 1) + "|" + tmp.tokenname(i + 1));
	}
	return bv_popcount(tmp, name);
}

BitVector64 sub_63_x(const BitVector64 x, const std::string name) {
	//xは63以下の符号なし整数だと仮定する。
	//63-xを求めて返す。
	BitVector64 result(name);
	for (int i = 0; i < 6; ++i) {
		exprlist.push_back(result.tokenname(i) + "=~" + x.tokenname(i));
	}
	for (int i = 6; i < 64; ++i) {
		exprlist.push_back("~" + result.tokenname(i));
	}
	return result;
}

void make(const uint64_t threshold) {

	constexpr int MAX_TURN = 42;

	std::vector<BitVector64>bb_player, bb_opponent, pop_move;

	constexpr uint64_t init_X = 0x0000'0000'0800'0000ULL;
	constexpr uint64_t init_O = 0x0000'0038'1000'0000ULL;
	bb_player.push_back(BitVector64("bb_player_init", init_X));
	bb_opponent.push_back(BitVector64("bb_opponent_init", init_O));
	pop_move.push_back(BitVector64("pop_move_init", 1ULL));


	std::array<uint64_t, 4>directions1{ 1,7,8,9 };
	std::array<uint64_t, 4>directions2{ 2,14,16,18 };
	std::array<uint64_t, 4>flip_const1{
		0xFFFF'FFFF'FFFF'FFFFULL,
		0x7E7E'7E7E'7E7E'7E7EULL,
		0x7E7E'7E7E'7E7E'7E7EULL,
		0x7E7E'7E7E'7E7E'7E7EULL
	};
	std::array<uint64_t, 4>flip_const2{
		0x0080'8080'8080'8080ULL,
		0x7F00'0000'0000'0000ULL,
		0x0102'0408'1020'4000ULL,
		0x0040'2010'0804'0201ULL
	};
	std::array<uint64_t, 4>flip_const3{
		0x0101'0101'0101'0100ULL,
		0x0000'0000'0000'00FEULL,
		0x0002'0408'1020'4080ULL,
		0x8040'2010'0804'0200ULL
	};

	std::vector<BitVector64>bv_flipconst2;
	for (int i = 0; i < 4; ++i) {
		bv_flipconst2.push_back(BitVector64("bv_flipconst2_" + std::to_string(i), flip_const2[i]));
	}
	std::vector<BitVector64>bv_flipconst3;
	for (int i = 0; i < 4; ++i) {
		bv_flipconst3.push_back(BitVector64("bv_flipconst3_" + std::to_string(i), flip_const3[i]));
	}

	for (int turn = 0; turn <= MAX_TURN; ++turn) {
		const std::string _turn = "_turn" + std::to_string(turn) + "_";
		const std::string _next_turn = "_turn" + std::to_string(turn + 1) + "_";
		BitVector64 bb_occupied = bv_or(bb_player[turn], bb_opponent[turn], "_bb_occupied" + _turn);
		BitVector64 bb_empty = bv_not(bb_occupied, "_bb_empty" + _turn);
		BitVector64 masked_bb_opponent = bv_and(bb_opponent[turn], 0x7E7E'7E7E'7E7E'7E7EULL, "_masked_bb_opponent" + _turn);

		std::vector<BitVector64>flip_l_0;
		for (int i = 0; i < 4; ++i)
			flip_l_0.push_back(
				bv_and(
					i == 2 ? bb_opponent[turn] : masked_bb_opponent,
					bv_shli(bb_player[turn], directions1[i], "tmp_flip_l_0_" + std::to_string(i) + _turn),
					"flip_l_0_" + std::to_string(i) + _turn
				)
			);
		std::vector<BitVector64>flip_r_0;
		for (int i = 0; i < 4; ++i)
			flip_r_0.push_back(
				bv_and(
					i == 2 ? bb_opponent[turn] : masked_bb_opponent,
					bv_shri(bb_player[turn], directions1[i], "tmp_flip_r_0_" + std::to_string(i) + _turn),
					"flip_r_0_" + std::to_string(i) + _turn
				)
			);
		std::vector<BitVector64>flip_l_1;
		for (int i = 0; i < 4; ++i)
			flip_l_1.push_back(
				bv_or(
					flip_l_0[i],
					bv_and(
						i == 2 ? bb_opponent[turn] : masked_bb_opponent,
						bv_shli(flip_l_0[i], directions1[i], "tmp_flip_l_1_shli_" + std::to_string(i) + _turn),
						"tmp_flip_l_1_and_" + std::to_string(i) + _turn
					),
					"flip_l_1_" + std::to_string(i) + _turn
				)
			);
		std::vector<BitVector64>flip_r_1;
		for (int i = 0; i < 4; ++i)
			flip_r_1.push_back(
				bv_or(
					flip_r_0[i],
					bv_and(
						i == 2 ? bb_opponent[turn] : masked_bb_opponent,
						bv_shri(flip_r_0[i], directions1[i], "tmp_flip_r_1_shri_" + std::to_string(i) + _turn),
						"tmp_flip_r_1_and_" + std::to_string(i) + _turn
					),
					"flip_r_1_" + std::to_string(i) + _turn
				)
			);
		std::vector<BitVector64>mask_l;
		for (int i = 0; i < 4; ++i)
			mask_l.push_back(
				bv_and(
					i == 2 ? bb_opponent[turn] : masked_bb_opponent,
					bv_shli(i == 2 ? bb_opponent[turn] : masked_bb_opponent, directions1[i], "tmp_mask_l_" + std::to_string(i) + _turn),
					"mask_l_" + std::to_string(i) + _turn
				)
			);
		std::vector<BitVector64>mask_r;
		for (int i = 0; i < 4; ++i)
			mask_r.push_back(
				bv_and(
					i == 2 ? bb_opponent[turn] : masked_bb_opponent,
					bv_shri(i == 2 ? bb_opponent[turn] : masked_bb_opponent, directions1[i], "tmp_mask_r_" + std::to_string(i) + _turn),
					"mask_r_" + std::to_string(i) + _turn
				)
			);
		std::vector<BitVector64>flip_l_2;
		for (int i = 0; i < 4; ++i)
			flip_l_2.push_back(
				bv_or(
					flip_l_1[i],
					bv_and(
						mask_l[i],
						bv_shli(flip_l_1[i], directions2[i], "tmp_flip_l_2_shli_" + std::to_string(i) + _turn),
						"tmp_flip_l_2_and_" + std::to_string(i) + _turn
					),
					"flip_l_2_" + std::to_string(i) + _turn
				)
			);
		std::vector<BitVector64>flip_r_2;
		for (int i = 0; i < 4; ++i)
			flip_r_2.push_back(
				bv_or(
					flip_r_1[i],
					bv_and(
						mask_r[i],
						bv_shri(flip_r_1[i], directions2[i], "tmp_flip_r_2_shri_" + std::to_string(i) + _turn),
						"tmp_flip_r_2_and_" + std::to_string(i) + _turn
					),
					"flip_r_2_" + std::to_string(i) + _turn
				)
			);
		std::vector<BitVector64>flip_l_3;
		for (int i = 0; i < 4; ++i)
			flip_l_3.push_back(
				bv_or(
					flip_l_2[i],
					bv_and(
						mask_l[i],
						bv_shli(flip_l_2[i], directions2[i], "tmp_flip_l_3_shli_" + std::to_string(i) + _turn),
						"tmp_flip_l_3_and_" + std::to_string(i) + _turn
					),
					"flip_l_3_" + std::to_string(i) + _turn
				)
			);
		std::vector<BitVector64>flip_r_3;
		for (int i = 0; i < 4; ++i)
			flip_r_3.push_back(
				bv_or(
					flip_r_2[i],
					bv_and(
						mask_r[i],
						bv_shri(flip_r_2[i], directions2[i], "tmp_flip_r_3_shri_" + std::to_string(i) + _turn),
						"tmp_flip_r_3_and_" + std::to_string(i) + _turn
					),
					"flip_r_3_" + std::to_string(i) + _turn
				)
			);
		std::vector<BitVector64>some_moves;
		for (int i = 0; i < 4; ++i)
			some_moves.push_back(
				bv_or(
					bv_shli(flip_l_3[i], directions1[i], "tmp_some_moves_shli_" + std::to_string(i) + _turn),
					bv_shri(flip_r_3[i], directions1[i], "tmp_some_moves_shri_" + std::to_string(i) + _turn),
					"some_moves_" + std::to_string(i) + _turn
				)
			);
		BitVector64 all_moves = bv_and(
			bv_or(
				bv_or(some_moves[0], some_moves[1], "tmp_all_moves_oror01" + _turn),
				bv_or(some_moves[2], some_moves[3], "tmp_all_moves_oror23" + _turn),
				"tmp_all_moves_or" + _turn
			),
			bb_empty,
			"all_moves" + _turn
		);
		BitVector64 popcount_moves = bv_popcount(all_moves, "popcount_moves" + _turn);

		pop_move.push_back(
			ifthenelse(
				greater(popcount_moves, pop_move.back(), "tmp_pop_move_" + std::to_string(turn + 1)),
				popcount_moves,
				pop_move.back(),
				"pop_move_" + std::to_string(turn + 1)
			)
		);

		BitVector64 move_onebit("move_onebit" + _turn);
		
		//着手はたかだか1箇所である
		equal_constraint(least_significant_bit(move_onebit, "tmp_onebit_lsb" + _turn), move_onebit);

		//着手するなら合法手である
		equal_constraint(bv_and(move_onebit, all_moves, "tmp_onebit_and" + _turn), move_onebit);

		//合法手が存在するなら必ず着手し、パスするのは合法手が存在しない場合のみである
		equal_constraint(reduce_or(move_onebit, "tmp_pass_reduce_or1" + _turn), reduce_or(all_moves, "tmp_pass_reduce_or2" + _turn));

		BitVector64 move_pos = onebit_bsf(move_onebit, "move_pos" + _turn);
		BitVector64 sub_63_move_pos = sub_63_x(move_pos, "tmp_sub_63_move_pos_" + _turn);

		std::vector<BitVector64>OM;
		for (int i = 0; i < 4; ++i)
			OM.push_back(
				bv_and(
					bb_opponent[turn],
					flip_const1[i],
					"OM_" + std::to_string(i) + _turn
				)
			);
		std::vector<BitVector64>mask1;
		for (int i = 0; i < 4; ++i)
			mask1.push_back(
				bv_shrv(
					bv_flipconst2[i],
					sub_63_move_pos,
					"mask1_" + std::to_string(i) + _turn
				)
			);
		std::vector<BitVector64>upperbit_argument;
		for (int i = 0; i < 4; ++i)
			upperbit_argument.push_back(
				bv_and(
					bv_not(OM[i], "tmp_not_OM_" + std::to_string(i) + _turn),
					mask1[i],
					"upperbit_argument_" + std::to_string(i) + _turn
				)
			);
		std::vector<BitVector64>upperbit_tmp1;
		for (int i = 0; i < 4; ++i)
			upperbit_tmp1.push_back(
				bv_or(
					upperbit_argument[i],
					bv_shri(upperbit_argument[i], 1, "tmp_upperbit_tmp1_shri_" + std::to_string(i) + _turn),
					"upperbit_tmp1_" + std::to_string(i) + _turn
				)
			);
		std::vector<BitVector64>upperbit_tmp2;
		for (int i = 0; i < 4; ++i)
			upperbit_tmp2.push_back(
				bv_or(
					upperbit_tmp1[i],
					bv_shri(upperbit_tmp1[i], 2, "tmp_upperbit_tmp2_shri_" + std::to_string(i) + _turn),
					"upperbit_tmp2_" + std::to_string(i) + _turn
				)
			);
		std::vector<BitVector64>upperbit_tmp3;
		for (int i = 0; i < 4; ++i)
			upperbit_tmp3.push_back(
				bv_or(
					upperbit_tmp2[i],
					bv_shri(upperbit_tmp2[i], 4, "tmp_upperbit_tmp3_shri_" + std::to_string(i) + _turn),
					"upperbit_tmp3_" + std::to_string(i) + _turn
				)
			);
		std::vector<BitVector64>upperbit_tmp4;
		for (int i = 0; i < 4; ++i)
			upperbit_tmp4.push_back(
				bv_and(
					bv_not(
						bv_shri(upperbit_tmp3[i], 1, "tmp_upperbit_tmp4_shri_" + std::to_string(i) + _turn),
						"tmp_upperbit_tmp4_not_" + std::to_string(i) + _turn
					),
					upperbit_tmp3[i],
					"upperbit_tmp4_" + std::to_string(i) + _turn
				)
			);
		std::vector<BitVector64>upperbit_tmp7;
		for (int i = 0; i < 4; ++i)
			upperbit_tmp7.push_back(
				bv_byteswap(
					upperbit_tmp4[i],
					"upperbit_tmp7_" + std::to_string(i) + _turn
				)
			);
		std::vector<BitVector64>upperbit_tmp8;
		for (int i = 0; i < 4; ++i)
			upperbit_tmp8.push_back(
				least_significant_bit(
					upperbit_tmp7[i],
					"upperbit_tmp8_" + std::to_string(i) + _turn
				)
			);
		std::vector<BitVector64>upperbit_result;
		for (int i = 0; i < 4; ++i)
			upperbit_result.push_back(
				bv_byteswap(
					upperbit_tmp8[i],
					"upperbit_result_" + std::to_string(i) + _turn
				)
			);

		std::vector<BitVector64>outflank1;
		for (int i = 0; i < 4; ++i)
			outflank1.push_back(
				bv_and(
					upperbit_result[i],
					bb_player[turn],
					"outflank1_" + std::to_string(i) + _turn
				)
			);
		std::vector<BitVector64>flipped1;
		for (int i = 0; i < 4; ++i)
			flipped1.push_back(
				bv_and(
					bv_shli(
						bv_neg(outflank1[i], "tmp_flipped1_neg_" + std::to_string(i) + _turn),
						1,
						"tmp_flipped1_shli_" + std::to_string(i) + _turn
					),
					mask1[i],
					"flipped1_" + std::to_string(i) + _turn
				)
			);

		std::vector<BitVector64>mask2;
		for (int i = 0; i < 4; ++i)
			mask2.push_back(
				bv_shlv(
					bv_flipconst3[i],
					move_pos,
					"mask2_" + std::to_string(i) + _turn
				)
			);
		std::vector<BitVector64>outflank2;
		for (int i = 0; i < 4; ++i)
			outflank2.push_back(
				bv_and(
					increment(
						bv_or(
							OM[i],
							bv_not(mask2[i], "tmp_outflank2_not_" + std::to_string(i) + _turn),
							"tmp_outflank2_or_" + std::to_string(i) + _turn
						),
						"tmp_outflank2_increment_" + std::to_string(i) + _turn
					),
					bv_and(mask2[i], bb_player[turn], "tmp_outflank2_and2_" + std::to_string(i) + _turn),
					"outflank2_" + std::to_string(i) + _turn
				)
			);
		std::vector<BitVector64>flipped2;
		for (int i = 0; i < 4; ++i)
			flipped2.push_back(
				bv_and(
					ifthenelse(
						reduce_or(outflank2[i], "tmp_flipped2_reduce_or_" + std::to_string(i) + _turn),
						decrement(outflank2[i], "tmp_flipped2_decrement_" + std::to_string(i) + _turn),
						outflank2[i],
						"tmp_flipped2_ifthenelse_" + std::to_string(i) + _turn
					),
					mask2[i],
					"flipped2_" + std::to_string(i) + _turn
				)
			);
		BitVector64 bb_flip = bv_or(
			bv_or(
				bv_or(flipped1[0], flipped1[1], "tmp_bb_flip_or1" + _turn),
				bv_or(flipped1[2], flipped1[3], "tmp_bb_flip_or2" + _turn),
				"tmp_bb_flip_or3" + _turn
			),
			bv_or(
				bv_or(flipped2[0], flipped2[1], "tmp_bb_flip_or4" + _turn),
				bv_or(flipped2[2], flipped2[3], "tmp_bb_flip_or5" + _turn),
				"tmp_bb_flip_or6" + _turn
			),
			"bb_flip" + _turn
		);
		BitVector64 next_player_tmp = bv_xor(bb_opponent[turn], bb_flip, "next_player_tmp" + _turn);
		BitVector64 next_opponent_tmp = bv_xor(bb_player[turn], bv_or(bb_flip, move_onebit, "tmp_next_opponent_tmp_or" + _turn), "next_opponent_tmp" + _turn);
		BitVector1 move_flag = reduce_or(move_onebit, "move_flag" + _turn);
		bb_player.push_back(
			ifthenelse(move_flag, next_player_tmp, bb_opponent[turn], "tmp_bb_player" + _next_turn)
		);
		bb_opponent.push_back(
			ifthenelse(move_flag, next_opponent_tmp, bb_player[turn], "tmp_bb_opponent" + _next_turn)
		);

	}
	greater_or_equal_constraint(pop_move.back(), BitVector64("threshold", threshold), "threshold_constraint");
}

std::vector<std::string> erase_equation(const std::string& expr) {
	//等式を含む式から等式を含まない式に変換する。
	//a⇒b は (~a)|b と等価である。
	//また、a=b は (a⇒b)&(b⇒a) と等価である。
	//ゆえに、 a=b は ((~a)|b)&(a|(~b)) と等価である。
	//CNFに変換して、clauseごとに分けて返す。
	//あと、等式を含まない場合は入力をそのまま返す。

	const std::regex token = std::regex(R"([a-zA-Z_][a-zA-Z0-9_:]*)");
	const std::regex neg_token = std::regex(R"(\(?~[a-zA-Z_][a-zA-Z0-9_:]*\)?)");
	const std::regex token_token = std::regex(R"([a-zA-Z_][a-zA-Z0-9_:]*[&|][a-zA-Z_][a-zA-Z0-9_:]*)");
	const std::regex token_neg_token = std::regex(R"([a-zA-Z_][a-zA-Z0-9_:]*[&|]\(?~[a-zA-Z_][a-zA-Z0-9_:]*\)?)");

	int equal_pos = -1;
	for (int i = 0; i < expr.size(); ++i) {
		if (expr[i] == '=') {
			assert(equal_pos == -1);
			equal_pos = i;
		}
	}

	if (equal_pos == -1) {
		assert(std::regex_search(expr, token));
		return std::vector<std::string>{expr};
	}

	const std::string subl = expr.substr(0, equal_pos);
	const std::string subr = expr.substr(equal_pos + 1);
	assert(std::regex_match(subl, token));//左辺は1トークンであることを仮定する。
	assert(std::regex_search(subr, token));

	//ここで等式全体をCNFにする。
	//右辺が2トークンの場合、右側に否定がかかっている場合あり。
	//右辺が1トークンの場合、否定がかかっている場合あり。

	if (std::regex_match(subr, token)) {
		//A=B は ((~A)|B)&(A|(~B))と等価で、これはCNF。
		return std::vector<std::string>{std::string("(~" + subl + ")|" + subr), std::string(subl + "|(~" + subr + ")")};
	}
	else if (std::regex_match(subr, neg_token)) {
		std::vector<std::string>subrs;
		for (std::sregex_iterator it(std::begin(subr), std::end(subr), token), end; it != end; ++it) {
			auto&& m = *it;
			subrs.push_back(m.str());
		}
		assert(subrs.size() == 1);
		//A=~B は ((~A)|(~B))&(A|B)と等価で、これはCNF。
		return std::vector<std::string>{std::string("(~" + subl + ")|(~" + subrs[0] + ")"), std::string(subl + "|" + subrs[0])};
	}
	else if (std::regex_match(subr, token_token)) {
		int pos = -1;
		for (int i = 0; i < subr.size(); ++i) {
			if (subr[i] == '&' || subr[i] == '|') {
				pos = i;
				break;
			}
		}
		const std::string subr1 = subr.substr(0, pos);
		const std::string subr2 = subr.substr(pos + 1);
		if (subr[pos] == '&') {
			// A=B&C は ((~A)|(B&C))&(A|(~(B&C))) と等価である。
			//cnfにすると
			//(~A | B) & (~A | C) & (A | ~B | ~C)
			std::vector<std::string>result;
			result.push_back(std::string("(~" + subl + ")|" + subr1));
			result.push_back(std::string("(~" + subl + ")|" + subr2));
			result.push_back(std::string(subl + "|(~" + subr1 + ")|(~" + subr2 + ")"));
			return result;
		}
		else {
			// A=B|C は ((~A)|(B|C))&(A|(~(B|C))) と等価である。
			//cnfにすると
			//(A | ~B) & (A | ~C) & (~A | B | C)
			std::vector<std::string>result;
			result.push_back(std::string(subl + "|(~" + subr1 + ")"));
			result.push_back(std::string(subl + "|(~" + subr2 + ")"));
			result.push_back(std::string("(~" + subl + ")|" + subr1 + "|" + subr2));
			return result;
		}
	}
	else if (std::regex_match(subr, token_neg_token)) {
		std::vector<std::string>subrs;
		for (std::sregex_iterator it(std::begin(subr), std::end(subr), token), end; it != end; ++it) {
			auto&& m = *it;
			subrs.push_back(m.str());
		}
		assert(subrs.size() == 2);
		int pos = -1;
		for (int i = 0; i < subr.size(); ++i) {
			if (subr[i] == '&' || subr[i] == '|') {
				pos = i;
				break;
			}
		}
		const std::string subr1 = subrs[0];
		const std::string subr2 = subrs[1];
		if (subr[pos] == '&') {
			//上記の
			//(~A | B) & (~A | C) & (A | ~B | ~C)
			//でのCが~Cに変わるので
			//(~A | B) & (~A | ~C) & (A | ~B | C)
			std::vector<std::string>result;
			result.push_back(std::string("(~" + subl + ")|" + subr1));
			result.push_back(std::string("(~" + subl + ")|(~" + subr2 + ")"));
			result.push_back(std::string(subl + "|(~" + subr1 + ")|" + subr2));
			return result;
		}
		else {
			//上記の
			//(A | ~B) & (A | ~C) & (~A | B | C)
			//でのCが~Cに変わるので
			//(A | ~B) & (A | C) & (~A | B | ~C)
			std::vector<std::string>result;
			result.push_back(std::string("(~" + subl + ")|(~" + subr1 + ")"));
			result.push_back(std::string("(~" + subl + ")|" + subr2));
			result.push_back(std::string("(~" + subl + ")|" + subr1 + "|(~" + subr2 + ")"));
			return result;
		}
	}
	assert(0);
}

void is_clause(const std::string s) {
	const std::regex token = std::regex(R"([a-zA-Z_][a-zA-Z0-9_:]*)");
	const std::string t1 = std::regex_replace(s, token, "a");
	const std::regex neg = std::regex(R"(\(?~a\)?)");
	const std::string t2 = std::regex_replace(t1, neg, "b");
	const std::regex clause = std::regex(R"([ab](\|[ab])*)");
	assert(std::regex_match(t2, clause));
}

void solve(const int NUMBER) {

	tokenlist.clear();
	exprlist.clear();

	make(NUMBER);

	const std::regex token = std::regex(R"(~?[a-zA-Z_][a-zA-Z0-9_:]*)");
	std::vector<std::string>dimacs_literal_list;
	std::map<std::string, int>dimacs_literal_map;//tokenlistは添字からtoken名へ変換できるが、その逆変換をできるようにする。
	std::vector<std::vector<int>>dimacs_clause_list;

	for (const auto& e : exprlist) {
		const auto clauses = erase_equation(e);
		for (const auto& c : clauses) {
			is_clause(c);
			//このclauseたちをDIMACS CNF FORMATにして出力する。あとその数字とtoken名の対応表も出力する。
			std::vector<int>tokens;
			for (std::sregex_iterator it(std::begin(c), std::end(c), token), end; it != end; ++it) {
				auto&& m = *it;

				const std::string token_ = m.str();
				const bool neg = token_[0] == '~';
				const std::string token__ = neg ? token_.substr(1) : token_;
				if (dimacs_literal_map.find(token__) == dimacs_literal_map.end()) {
					dimacs_literal_map[token__] = dimacs_literal_list.size();
					dimacs_literal_list.push_back(token__);
				}
				tokens.push_back((dimacs_literal_map[token__] + 1) * (neg ? -1 : 1));
			}
			dimacs_clause_list.push_back(tokens);
		}
	}

	constexpr int BUFSIZE = 2 * 1024 * 1024;
	static char buf[BUFSIZE];//大きいのでスタック領域に置きたくないからstatic。（べつにmallocでもstd::vectorでもいいんだけど）

	{
		std::ofstream writing_file;
		writing_file.rdbuf()->pubsetbuf(buf, BUFSIZE);
		writing_file.open("dimacs_cnf_" + std::to_string(NUMBER) + "_cpp.txt", std::ios::out);
		writing_file << "p cnf " << dimacs_literal_list.size() << " " << dimacs_clause_list.size() << std::endl;
		for (const auto& v : dimacs_clause_list) {
			for (const auto& s : v) {
				assert(s != 0);
				writing_file << s << " ";
			}
			writing_file << 0 << std::endl;
		}
		writing_file.close();
	}
	{
		std::ofstream writing_file;
		writing_file.rdbuf()->pubsetbuf(buf, BUFSIZE);
		writing_file.open("dimacs_literal_2_token_" + std::to_string(NUMBER) + ".csv", std::ios::out);
		for (int i = 0; i < dimacs_literal_list.size(); ++i) {
			writing_file << dimacs_literal_list[i] << "," << i + 1 << std::endl;
		}
		writing_file.close();
	}
}

int main(int argc, char *argv[]) {

	solve(33);

	return 0;
}
