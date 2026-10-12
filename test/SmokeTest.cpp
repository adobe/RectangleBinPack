// Exercise both existing flat includes and CMake's namespaced includes.
#include "Rect.h"
#include <RectangleBinPack/GuillotineBinPack.h>
#include <RectangleBinPack/MaxRectsBinPack.h>
#include <RectangleBinPack/Rect.h>
#include <RectangleBinPack/ShelfBinPack.h>
#include <RectangleBinPack/ShelfNextFitBinPack.h>
#include <RectangleBinPack/SkylineBinPack.h>

#include <cstdio>

namespace {

template <typename Rectangle>
bool Check(const char *name, const Rectangle &rect, double occupancy)
{
    const bool ok = rect.x >= 0 && rect.y >= 0 &&
        rect.x + rect.width <= 64 && rect.y + rect.height <= 64 &&
        rect.width == 16 && rect.height == 16 && occupancy == 0.0625;
    if (!ok)
        std::fprintf(stderr, "%s: unexpected placement or occupancy\n", name);
    return ok;
}

} // namespace

int main()
{
    using namespace rbp;
    bool ok = true;

    MaxRectsBinPack maxRects(64, 64, false);
    const Rect maxRectsRect = maxRects.Insert(16, 16, MaxRectsBinPack::RectBestShortSideFit);
    ok = Check("MaxRects", maxRectsRect, maxRects.Occupancy()) && ok;

    GuillotineBinPack guillotine(64, 64);
    const Rect guillotineRect = guillotine.Insert(16, 16, true,
        GuillotineBinPack::RectBestShortSideFit, GuillotineBinPack::SplitShorterAxis);
    ok = Check("Guillotine", guillotineRect, guillotine.Occupancy()) && ok;

    ShelfBinPack shelf(64, 64, true);
    const Rect shelfRect = shelf.Insert(16, 16, ShelfBinPack::ShelfNextFit);
    ok = Check("Shelf", shelfRect, shelf.Occupancy()) && ok;

    ShelfNextFitBinPack nextFit;
    nextFit.Init(64, 64);
    const ShelfNextFitBinPack::Node nextFitRect = nextFit.Insert(16, 16);
    ok = Check("ShelfNextFit", nextFitRect, nextFit.Occupancy()) && ok;

    SkylineBinPack skyline(64, 64, true);
    const Rect skylineRect = skyline.Insert(16, 16, SkylineBinPack::LevelBottomLeft);
    ok = Check("Skyline", skylineRect, skyline.Occupancy()) && ok;

    return ok ? 0 : 1;
}
