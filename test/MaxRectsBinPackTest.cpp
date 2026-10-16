#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <stdint.h>

#if defined(__unix__) || defined(__APPLE__) || defined (__CYGWIN__)
#include <time.h>
#include <sys/time.h>
#endif

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

#ifdef WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

#include <vector>
#include <cmath>
#include <limits>
#include <type_traits>
#include <RectangleBinPack/MaxRectsBinPack.h>
#include <RectangleBinPack/GuillotineBinPack.h>
#include <RectangleBinPack/ShelfBinPack.h>
#include <RectangleBinPack/ShelfNextFitBinPack.h>
#include <RectangleBinPack/SkylineBinPack.h>

using namespace rbp;

uint64_t tick()
{
#if defined(WIN32)
	LARGE_INTEGER ddwTimer;
	BOOL success = QueryPerformanceCounter(&ddwTimer);
	return ddwTimer.QuadPart;
#elif defined(__APPLE__)
	return mach_absolute_time();
#elif defined(_POSIX_MONOTONIC_CLOCK)
	timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec * 1000 * 1000 * 1000 + t.tv_nsec;
#elif defined(_POSIX_C_SOURCE)
	timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec * 1000 * 1000 + t.tv_usec;
#else
	return clock();
#endif
}

uint64_t ticksPerSec()
{
#if defined(WIN32)
	uint64_t ddwTimerFrequency;
	QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&ddwTimerFrequency));
	return ddwTimerFrequency;
#elif defined(__APPLE__)
	mach_timebase_info_data_t timeBaseInfo;
	mach_timebase_info(&timeBaseInfo);
	return 1000000000ULL * (uint64_t)timeBaseInfo.denom / (uint64_t)timeBaseInfo.numer;
#elif defined(_POSIX_MONOTONIC_CLOCK)
	return 1000 * 1000 * 1000;
#elif defined(_POSIX_C_SOURCE) || defined(__APPLE__)
	return 1000 * 1000;
#else
	return CLOCKS_PER_SEC;
#endif
}

bool AreDisjoint(const Rect &a, const Rect &b)
{
	return a.x >= b.x + b.width || a.x + a.width <= b.x ||
		a.y >= b.y + b.height || a.y + a.height <= b.y;
}

bool AllRectsDisjoint(std::vector<Rect> &packed)
{
	for(size_t i = 0; i < packed.size(); ++i)
		for(size_t j = i+1; j < packed.size(); ++j)
		{
			if (!AreDisjoint(packed[i], packed[j]))
				return false;
		}
	return true;
}

// These checks run even with --fast; only the historical random workload is reduced.
namespace {
const char *numericContext = "";

void NumericCheck(bool ok, const char *expression, int line)
{
	if (!ok)
	{
		std::fprintf(stderr, "Numeric regression (%s), line %d: %s\n", numericContext, line, expression);
		std::exit(1);
	}
}
#define CHECK_NUMERIC(expression) NumericCheck((expression), #expression, __LINE__)

bool SameRect(const Rect &a, const Rect &b)
{
	return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}

template<typename Rectangle>
bool IsZero(const Rectangle &r)
{
	return r.x == 0 && r.y == 0 && r.width == 0 && r.height == 0;
}

Rect AsRect(const ShelfNextFitBinPack::Node &r)
{
	return Rect{r.x, r.y, r.width, r.height};
}

Rect AsRect(const Rect &r) { return r; }

bool InBounds(const Rect &r, Int width, Int height)
{
	return r.x >= 0 && r.y >= 0 && r.width > 0 && r.height > 0 &&
		r.x <= width && r.y <= height && r.width <= width - r.x && r.height <= height - r.y;
}

template<typename Packer, typename Initialize, typename Insert>
void TestScalarNumericContract(Packer &pack, Initialize init, Insert insert)
{
	const Int max = std::numeric_limits<Int>::max();
	const Int aboveInt = Int(std::numeric_limits<int>::max()) + 123;
	CHECK_NUMERIC(pack.Occupancy() == 0);
	CHECK_NUMERIC(IsZero(insert(pack, 1, 1)));

	const RectSize invalidBins[] = {{0, 10}, {10, 0}, {-1, 10}, {10, -1},
		{max, 1}, {1, max}, {max / 2, 1}, {max / 2 - 1, 2},
		{max / 2, 3}, {7, max / 7 + 1}, {3037000500LL, 3037000500LL}, {max, max}};
	for (const RectSize &size : invalidBins)
	{
		init(pack, 10, 10);
		CHECK_NUMERIC(!IsZero(insert(pack, 2, 2)));
		init(pack, size.width, size.height);
		CHECK_NUMERIC(pack.Occupancy() == 0);
		CHECK_NUMERIC(IsZero(insert(pack, 1, 1)));
	}

	const RectSize validBins[] = {{50000, 50000}, {100000, 100000},
		{aboveInt, 2}, {2, aboveInt}, {max / 2 - 1, 1}, {1, max / 2 - 1},
		{max / 3, 3}, {7, max / 7}, {3037000499LL, 3037000499LL}};
	for (const RectSize &size : validBins)
	{
		init(pack, size.width, size.height);
		CHECK_NUMERIC(pack.Occupancy() == 0);
		CHECK_NUMERIC(InBounds(AsRect(insert(pack, size.width, size.height)), size.width, size.height));
		CHECK_NUMERIC(pack.Occupancy() == 1);
		CHECK_NUMERIC(IsZero(insert(pack, 1, 1)));
		CHECK_NUMERIC(pack.Occupancy() == 1);
	}

	// Near-limit area scores and nonzero split products must remain signed Int64.
	init(pack, 7, max / 7);
	CHECK_NUMERIC(InBounds(AsRect(insert(pack, 1, 1)), 7, max / 7));
	init(pack, 100000, 100000);
	CHECK_NUMERIC(InBounds(AsRect(insert(pack, 40000, 60000)), 100000, 100000));
	CHECK_NUMERIC(std::fabs(pack.Occupancy() - 0.24) < 0.000001);

	// Multiple contributions exceed UINT32_MAX, including on Win64.
	init(pack, 100000, 100000);
	Rect first = AsRect(insert(pack, 50000, 100000));
	Rect second = AsRect(insert(pack, 50000, 100000));
	CHECK_NUMERIC(InBounds(first, 100000, 100000));
	CHECK_NUMERIC(InBounds(second, 100000, 100000));
	CHECK_NUMERIC(AreDisjoint(first, second));
	CHECK_NUMERIC(pack.Occupancy() == 1);

	// All early rejections must leave the next placement identical to a copy.
	init(pack, 64, 64);
	CHECK_NUMERIC(!IsZero(insert(pack, 8, 8)));
	Packer before = pack;
	const double occupancy = pack.Occupancy();
	const RectSize invalidRequests[] = {{0, 1}, {1, 0}, {-1, 1}, {1, -1},
		{std::numeric_limits<Int>::min(), max}, {max, max}, {max, 1}, {1, max}, {100, 90}};
	for (const RectSize &size : invalidRequests)
	{
		auto r = insert(pack, size.width, size.height);
		CHECK_NUMERIC(IsZero(r));
		CHECK_NUMERIC(pack.Occupancy() == occupancy);
	}
	CHECK_NUMERIC(SameRect(AsRect(insert(pack, 3, 5)), AsRect(insert(before, 3, 5))));

	// Ordinary small requests: bounds, disjointness, and area accounting.
	init(pack, 64, 64);
	std::vector<Rect> placed;
	Int area = 0;
	const RectSize smallSizes[] = {{8, 12}, {13, 7}, {3, 5}, {16, 16}, {7, 9}, {20, 4}};
	for (const RectSize &size : smallSizes)
	{
		Rect r = AsRect(insert(pack, size.width, size.height));
		CHECK_NUMERIC(InBounds(r, 64, 64));
		CHECK_NUMERIC(r.width * r.height == size.width * size.height);
		area += r.width * r.height;
		placed.push_back(r);
	}
	CHECK_NUMERIC(AllRectsDisjoint(placed));
	CHECK_NUMERIC(std::fabs(pack.Occupancy() - double(area) / 4096) < 0.000001);
}

bool SameSizes(const std::vector<RectSize> &a, const std::vector<RectSize> &b)
{
	if (a.size() != b.size()) return false;
	for (size_t i = 0; i < a.size(); ++i)
		if (a[i].width != b[i].width || a[i].height != b[i].height) return false;
	return true;
}

template<typename Packer, typename Initialize, typename BatchInsert, typename Insert>
void TestBatchNumericContract(Packer &pack, Initialize init, BatchInsert batch, Insert insert, bool hasDestination = true)
{
	const Int max = std::numeric_limits<Int>::max();
	std::vector<Rect> dst(1, Rect{1, 1, 1, 1});
	std::vector<RectSize> sizes{{1, 1}};
	init(pack, 0, 0);
	batch(pack, sizes, dst);
	CHECK_NUMERIC(sizes.size() == 1 && pack.Occupancy() == 0);
	if (hasDestination) CHECK_NUMERIC(dst.empty());

	for (Int bad : {Int(0), Int(-1), std::numeric_limits<Int>::min()})
	{
		for (bool badWidth : {false, true})
		{
			init(pack, 64, 64);
			CHECK_NUMERIC(!IsZero(insert(pack, 4, 4)));
			Packer before = pack;
			const double occupancy = pack.Occupancy();
			sizes = {{2, 2}, {badWidth ? bad : 1, badWidth ? 1 : bad}};
			const std::vector<RectSize> original = sizes;
			dst.assign(1, Rect{1, 1, 1, 1});
			batch(pack, sizes, dst);
			CHECK_NUMERIC(SameSizes(sizes, original));
			CHECK_NUMERIC(pack.Occupancy() == occupancy);
			if (hasDestination) CHECK_NUMERIC(dst.empty());
			CHECK_NUMERIC(SameRect(AsRect(insert(pack, 3, 5)), AsRect(insert(before, 3, 5))));
		}
	}

	// Oversized positives stay unplaced; fitting entries still fill the bin.
	init(pack, 100000, 100000);
	sizes = {{max, max}, {50000, 100000}, {50000, 100000}, {1, max}};
	batch(pack, sizes, dst);
	CHECK_NUMERIC(sizes.size() == 2);
	CHECK_NUMERIC(pack.Occupancy() == 1);
	CHECK_NUMERIC(dst.size() == 2 && AllRectsDisjoint(dst));
	for (const Rect &r : dst) CHECK_NUMERIC(InBounds(r, 100000, 100000));

	const Int aboveInt = Int(std::numeric_limits<int>::max()) + 123;
	init(pack, aboveInt, 2);
	sizes = {{aboveInt, 1}, {aboveInt, 1}, {max, 1}};
	batch(pack, sizes, dst);
	CHECK_NUMERIC(sizes.size() == 1 && sizes[0].width == max);
	CHECK_NUMERIC(pack.Occupancy() == 1);
	CHECK_NUMERIC(dst.size() == 2 && AllRectsDisjoint(dst));
}

void TestMaxRectsNumericContract()
{
	numericContext = "MaxRects";
	for (bool flip : {false, true})
		for (int h = MaxRectsBinPack::RectBestShortSideFit; h <= MaxRectsBinPack::RectContactPointRule; ++h)
		{
			const auto heuristic = static_cast<MaxRectsBinPack::FreeRectChoiceHeuristic>(h);
			MaxRectsBinPack pack;
			auto init = [flip](MaxRectsBinPack &p, Int w, Int h) { p.Init(w, h, flip); };
			auto insert = [heuristic](MaxRectsBinPack &p, Int w, Int h) { return p.Insert(w, h, heuristic); };
			auto batch = [heuristic](MaxRectsBinPack &p, std::vector<RectSize> &s, std::vector<Rect> &d) { p.Insert(s, d, heuristic); };
			TestScalarNumericContract(pack, init, insert);
			TestBatchNumericContract(pack, init, batch, insert);
			init(pack, 10, 20);
			CHECK_NUMERIC(IsZero(insert(pack, 20, 10)) == !flip);
		}
	// Contact scores above INT_MAX with several contacting used rectangles.
	const Int wide = Int(std::numeric_limits<int>::max()) + 123;
	MaxRectsBinPack contact(wide, 4, false);
	Rect a = contact.Insert(wide, 1, MaxRectsBinPack::RectContactPointRule);
	Rect b = contact.Insert(wide, 1, MaxRectsBinPack::RectContactPointRule);
	Rect c = contact.Insert(wide, 2, MaxRectsBinPack::RectContactPointRule);
	CHECK_NUMERIC(InBounds(a, wide, 4) && InBounds(b, wide, 4) && InBounds(c, wide, 4));
	CHECK_NUMERIC(AreDisjoint(a, b) && AreDisjoint(a, c) && AreDisjoint(b, c));
	CHECK_NUMERIC(contact.Occupancy() == 1);
}

void TestGuillotineNumericContract()
{
	numericContext = "Guillotine";
	for (bool merge : {false, true})
		for (int h = GuillotineBinPack::RectBestAreaFit; h <= GuillotineBinPack::RectWorstLongSideFit; ++h)
			for (int s = GuillotineBinPack::SplitShorterLeftoverAxis; s <= GuillotineBinPack::SplitLongerAxis; ++s)
			{
				const auto heuristic = static_cast<GuillotineBinPack::FreeRectChoiceHeuristic>(h);
				const auto split = static_cast<GuillotineBinPack::GuillotineSplitHeuristic>(s);
				GuillotineBinPack pack;
				auto init = [](GuillotineBinPack &p, Int w, Int h) { p.Init(w, h); };
				auto insert = [=](GuillotineBinPack &p, Int w, Int h) { return p.Insert(w, h, merge, heuristic, split); };
				auto batch = [=](GuillotineBinPack &p, std::vector<RectSize> &r, std::vector<Rect> &d) { p.Insert(r, merge, heuristic, split); d = p.GetUsedRectangles(); };
				TestScalarNumericContract(pack, init, insert);
				TestBatchNumericContract(pack, init, batch, insert, false);
			}
}

void TestShelfNumericContract()
{
	numericContext = "Shelf";
	const Int wide = Int(std::numeric_limits<int>::max()) + 123;
	for (bool waste : {false, true})
	{
		for (int h = ShelfBinPack::ShelfNextFit; h <= ShelfBinPack::ShelfWorstWidthFit; ++h)
		{
			const auto heuristic = static_cast<ShelfBinPack::ShelfChoiceHeuristic>(h);
			ShelfBinPack pack;
			TestScalarNumericContract(pack,
				[waste](ShelfBinPack &p, Int w, Int h) { p.Init(w, h, waste); },
				[heuristic](ShelfBinPack &p, Int w, Int h) { return p.Insert(w, h, heuristic); });
			pack.Init(10, 60, waste);
			CHECK_NUMERIC(!IsZero(pack.Insert(8, 20, heuristic)));
			const double occupancy = pack.Occupancy();
			CHECK_NUMERIC(IsZero(pack.Insert(50, 5, heuristic)));
			CHECK_NUMERIC(pack.Occupancy() == occupancy);
			CHECK_NUMERIC(IsZero(pack.Insert(200, 150, heuristic)));
		}
		ShelfBinPack width(2 * wide + 10, 10, waste);
		CHECK_NUMERIC(!IsZero(width.Insert(1, 1, ShelfBinPack::ShelfBestWidthFit)));
		Rect r = width.Insert(1, 1, ShelfBinPack::ShelfBestWidthFit);
		CHECK_NUMERIC(r.x == 1 && r.y == 0);
		ShelfBinPack height(20, 2 * wide, waste);
		CHECK_NUMERIC(!IsZero(height.Insert(1, wide, ShelfBinPack::ShelfBestHeightFit)));
		r = height.Insert(1, 1, ShelfBinPack::ShelfBestHeightFit);
		CHECK_NUMERIC(r.x == 1 && r.y == 0);

		// Close a shelf and then recover its right-hand waste area.
		ShelfBinPack holes(100, 100, waste);
		CHECK_NUMERIC(!IsZero(holes.Insert(60, 40, ShelfBinPack::ShelfNextFit)));
		CHECK_NUMERIC(!IsZero(holes.Insert(100, 50, ShelfBinPack::ShelfNextFit)));
		r = holes.Insert(30, 30, ShelfBinPack::ShelfNextFit);
		CHECK_NUMERIC(!IsZero(r) == waste);
		if (waste) CHECK_NUMERIC(r.x == 60 && r.y == 0);
	}
}

void TestShelfNextFitNumericContract()
{
	numericContext = "ShelfNextFit";
	ShelfNextFitBinPack pack;
	TestScalarNumericContract(pack,
		[](ShelfNextFitBinPack &p, Int w, Int h) { p.Init(w, h); },
		[](ShelfNextFitBinPack &p, Int w, Int h) { return p.Insert(w, h); });
	CHECK_NUMERIC(!ShelfNextFitBinPack().Insert(1, 1).flipped);
	// Ordinary failed insertion still closes the shelf, as in the original API.
	pack.Init(10, 10);
	CHECK_NUMERIC(!IsZero(pack.Insert(8, 6)));
	CHECK_NUMERIC(IsZero(pack.Insert(9, 5)));
	Rect r = AsRect(pack.Insert(2, 2));
	CHECK_NUMERIC(r.x == 0 && r.y == 6);
}

void TestSkylineNumericContract()
{
	numericContext = "Skyline";
	for (bool waste : {false, true})
		for (int h = SkylineBinPack::LevelBottomLeft; h <= SkylineBinPack::LevelMinWasteFit; ++h)
		{
			const auto heuristic = static_cast<SkylineBinPack::LevelChoiceHeuristic>(h);
			SkylineBinPack pack;
			auto init = [waste](SkylineBinPack &p, Int w, Int h) { p.Init(w, h, waste); };
			auto insert = [heuristic](SkylineBinPack &p, Int w, Int h) { return p.Insert(w, h, heuristic); };
			auto batch = [heuristic](SkylineBinPack &p, std::vector<RectSize> &s, std::vector<Rect> &d) { p.Insert(s, d, heuristic); };
			TestScalarNumericContract(pack, init, insert);
			TestBatchNumericContract(pack, init, batch, insert);
		}
	// Force a bridge over a low skyline section, then recover that hole.
	SkylineBinPack holes(100, 100, true);
	CHECK_NUMERIC(!IsZero(holes.Insert(60, 60, SkylineBinPack::LevelBottomLeft)));
	CHECK_NUMERIC(!IsZero(holes.Insert(40, 20, SkylineBinPack::LevelBottomLeft)));
	CHECK_NUMERIC(!IsZero(holes.Insert(100, 10, SkylineBinPack::LevelBottomLeft)));
	SkylineBinPack recovered = holes;
	Rect r = recovered.Insert(30, 30, SkylineBinPack::LevelMinWasteFit);
	CHECK_NUMERIC(r.x == 60 && r.y == 20);
	// Scaled bridge forces wasted-area scoring/accumulation above UINT32_MAX.
	SkylineBinPack largeHoles(200000, 200000, true);
	CHECK_NUMERIC(!IsZero(largeHoles.Insert(120000, 120000, SkylineBinPack::LevelBottomLeft)));
	CHECK_NUMERIC(!IsZero(largeHoles.Insert(80000, 40000, SkylineBinPack::LevelBottomLeft)));
	CHECK_NUMERIC(!IsZero(largeHoles.Insert(200000, 20000, SkylineBinPack::LevelMinWasteFit)));
	r = largeHoles.Insert(60000, 60000, SkylineBinPack::LevelMinWasteFit);
	CHECK_NUMERIC(r.x == 120000 && r.y == 40000);
	// Previously the disabled map could return a stale, out-of-bin rectangle.
	holes.Init(10, 10, false);
	r = holes.Insert(5, 5, SkylineBinPack::LevelBottomLeft);
	CHECK_NUMERIC(InBounds(r, 10, 10) && r.x == 0 && r.y == 0);
	CHECK_NUMERIC(holes.Occupancy() == 0.25f);
	holes.Init(10, 10, true);
	CHECK_NUMERIC(!IsZero(holes.Insert(10, 10, SkylineBinPack::LevelBottomLeft)));
	CHECK_NUMERIC(holes.Occupancy() == 1);
}

void TestNumericRegressions()
{
	static_assert(std::is_same<Int, std::int64_t>::value, "Int must be signed Int64");
	static_assert(std::is_same<decltype(GuillotineBinPack().Occupancy()), float>::value, "Guillotine occupancy remains float");
	static_assert(std::is_same<decltype(ShelfBinPack().Occupancy()), float>::value, "Shelf occupancy remains float");
	static_assert(std::is_same<decltype(ShelfNextFitBinPack().Occupancy()), float>::value, "ShelfNextFit occupancy remains float");
	static_assert(std::is_same<decltype(SkylineBinPack().Occupancy()), float>::value, "Skyline occupancy remains float");
	static_assert(sizeof(Rect) == 32, "Rect uses four Int64 members");
	static_assert(std::is_same<decltype(MaxRectsBinPack().Occupancy()), double>::value, "MaxRects occupancy remains double");
	TestMaxRectsNumericContract();
	TestGuillotineNumericContract();
	TestShelfNumericContract();
	TestShelfNextFitNumericContract();
	TestSkylineNumericContract();
	std::puts("Int64 numeric regressions passed (all five packers).");
}
} // namespace

int main(int argc, char **argv)
{
	TestNumericRegressions();
	// Windows Debug CI uses --fast; other builds run the full benchmark.
	const int binSize = argc == 2 && std::strcmp(argv[1], "--fast") == 0 ? 1024 : 1024*8;
	MaxRectsBinPack pack(binSize, binSize, true);

	std::vector<Rect> packed;
	srand(12412);
	uint64_t t0 = tick();
	for(int i = 1; i < 1024*1024; ++i)
	{
		int a = (rand() % 128) + 1;
		int b = (rand() % 128) + 1;
		Rect r = pack.Insert(a, b, MaxRectsBinPack::RectBestShortSideFit);
		if (!r.width)
			break;		
		packed.push_back(r);
	}
	uint64_t t1 = tick();
	const bool allDisjoint = AllRectsDisjoint(packed);
	const double occupancy = pack.Occupancy();
	printf("Packing %d rectangles took %f msecs. All rects disjoint: %s. Occupancy: %f\n",
		(int)packed.size(), (t1 - t0) * 1000.0 / ticksPerSec(), allDisjoint ? "yes" : "NO!", occupancy);
	return !packed.empty() && allDisjoint && occupancy > 0.0 && occupancy <= 1.0 ? 0 : 1;
}
