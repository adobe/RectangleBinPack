/** @file SkylineBinPack.cpp
	@author Jukka Jylänki

	@brief Implements different bin packer algorithms that use the SKYLINE data structure.

	This work is released to Public Domain, do whatever you want with it.
*/
#include <algorithm>
#include <utility>
#include <iostream>
#include <limits>

#include <cassert>
#include <cstring>
#include <cmath>

#include "SkylineBinPack.h"
#include "BinPackCommon.h"

namespace rbp {

using namespace std;

SkylineBinPack::SkylineBinPack()
:binWidth(0),
binHeight(0),
usedSurfaceArea(0),
useWasteMap(false)
{
}

SkylineBinPack::SkylineBinPack(Int width, Int height, bool useWasteMap)
{
	Init(width, height, useWasteMap);
}

void SkylineBinPack::Init(Int width, Int height, bool useWasteMap_)
{
	if (!detail::IsValidBinSize(width, height))
		width = height = 0;

	binWidth = width;
	binHeight = height;

	useWasteMap = width > 0 && useWasteMap_;

#ifdef _DEBUG
	disjointRects.Clear();
#endif

	usedSurfaceArea = 0;
	skyLine.clear();
	SkylineNode node;
	node.x = 0;
	node.y = 0;
	node.width = binWidth;
	if (width > 0)
		skyLine.push_back(node);
	wasteMap.Init(0, 0);

	if (useWasteMap)
	{
		wasteMap.Init(width, height);
		wasteMap.GetFreeRectangles().clear();
	}
}

void SkylineBinPack::Insert(std::vector<RectSize> &rects, std::vector<Rect> &dst, LevelChoiceHeuristic method)
{
	dst.clear();
	if (!detail::HasValidDimensions(rects) || binWidth == 0)
		return;

	while(rects.size() > 0)
	{
		Rect bestNode;
		Int bestScore1 = std::numeric_limits<Int>::max();
		Int bestScore2 = std::numeric_limits<Int>::max();
		size_t bestSkylineIndex = skyLine.size();
		size_t bestRectIndex = rects.size();
		for(size_t i = 0; i < rects.size(); ++i)
		{
			if (!detail::FitsBin(rects[i].width, rects[i].height, binWidth, binHeight))
				continue;
			Rect newNode = {};
			Int score1;
			Int score2;
			size_t index;
			switch(method)
			{
			case LevelBottomLeft:
				newNode = FindPositionForNewNodeBottomLeft(rects[i].width, rects[i].height, score1, score2, index);
				debug_assert(disjointRects.Disjoint(newNode));
				break;
			case LevelMinWasteFit:
				newNode = FindPositionForNewNodeMinWaste(rects[i].width, rects[i].height, score2, score1, index);
				debug_assert(disjointRects.Disjoint(newNode));
				break;
			default: assert(false); break;
			}
			if (newNode.height != 0)
			{
				if (score1 < bestScore1 || (score1 == bestScore1 && score2 < bestScore2))
				{
					bestNode = newNode;
					bestScore1 = score1;
					bestScore2 = score2;
					bestSkylineIndex = index;
					bestRectIndex = i;
				}
			}
		}

		if (bestRectIndex == rects.size())
			return;

		// Perform the actual packing.
#ifdef _DEBUG
		assert(disjointRects.Disjoint(bestNode));
		disjointRects.Add(bestNode);
#endif
		AddSkylineLevel(bestSkylineIndex, bestNode);
		usedSurfaceArea += rects[bestRectIndex].width * rects[bestRectIndex].height;
		rects.erase(rects.begin() + bestRectIndex);
		dst.push_back(bestNode);
	}
}

Rect SkylineBinPack::Insert(Int width, Int height, LevelChoiceHeuristic method)
{
	if (!detail::FitsBin(width, height, binWidth, binHeight))
		return Rect{};

	// First try to pack this rectangle into the waste map, if it fits.
	Rect node = {};
	if (useWasteMap)
		node = wasteMap.Insert(width, height, true, GuillotineBinPack::RectBestShortSideFit,
			GuillotineBinPack::SplitMaximizeArea);
	debug_assert(disjointRects.Disjoint(node));

	if (node.height != 0)
	{
		Rect newNode;
		newNode.x = node.x;
		newNode.y = node.y;
		newNode.width = node.width;
		newNode.height = node.height;
		usedSurfaceArea += width * height;
#ifdef _DEBUG
		assert(disjointRects.Disjoint(newNode));
		disjointRects.Add(newNode);
#endif
		return newNode;
	}
	
	switch(method)
	{
	case LevelBottomLeft: return InsertBottomLeft(width, height);
	case LevelMinWasteFit: return InsertMinWaste(width, height);
	default: assert(false); return node;
	}
}

bool SkylineBinPack::RectangleFits(size_t skylineNodeIndex, Int width, Int height, Int &y) const
{
	Int x = skyLine[skylineNodeIndex].x;
	if (width > binWidth - x)
		return false;
	Int widthLeft = width;
	size_t i = skylineNodeIndex;
	y = skyLine[skylineNodeIndex].y;
	while(widthLeft > 0)
	{
		y = max(y, skyLine[i].y);
		if (height > binHeight - y)
			return false;
		widthLeft -= skyLine[i].width;
		++i;
		assert(i < skyLine.size() || widthLeft <= 0);
	}
	return true;
}

Int SkylineBinPack::ComputeWastedArea(size_t skylineNodeIndex, Int width, Int height, Int y) const
{
	Int wastedArea = 0;
	const Int rectLeft = skyLine[skylineNodeIndex].x;
	const Int rectRight = rectLeft + width;
	for(; skylineNodeIndex < skyLine.size() && skyLine[skylineNodeIndex].x < rectRight; ++skylineNodeIndex)
	{
		if (skyLine[skylineNodeIndex].x >= rectRight || skyLine[skylineNodeIndex].x + skyLine[skylineNodeIndex].width <= rectLeft)
			break;

		Int leftSide = skyLine[skylineNodeIndex].x;
		Int rightSide = min(rectRight, leftSide + skyLine[skylineNodeIndex].width);
		assert(y >= skyLine[skylineNodeIndex].y);
		wastedArea += (rightSide - leftSide) * (y - skyLine[skylineNodeIndex].y);
	}
	return wastedArea;
}

bool SkylineBinPack::RectangleFits(size_t skylineNodeIndex, Int width, Int height, Int &y, Int &wastedArea) const
{
	bool fits = RectangleFits(skylineNodeIndex, width, height, y);
	if (fits)
		wastedArea = ComputeWastedArea(skylineNodeIndex, width, height, y);
	
	return fits;
}

void SkylineBinPack::AddWasteMapArea(size_t skylineNodeIndex, Int width, Int height, Int y)
{
	// int wastedArea = 0; // unused
	const Int rectLeft = skyLine[skylineNodeIndex].x;
	const Int rectRight = rectLeft + width;
	for(; skylineNodeIndex < skyLine.size() && skyLine[skylineNodeIndex].x < rectRight; ++skylineNodeIndex)
	{
		if (skyLine[skylineNodeIndex].x >= rectRight || skyLine[skylineNodeIndex].x + skyLine[skylineNodeIndex].width <= rectLeft)
			break;

		Int leftSide = skyLine[skylineNodeIndex].x;
		Int rightSide = min(rectRight, leftSide + skyLine[skylineNodeIndex].width);
		assert(y >= skyLine[skylineNodeIndex].y);

		Rect waste;
		waste.x = leftSide;
		waste.y = skyLine[skylineNodeIndex].y;
		waste.width = rightSide - leftSide;
		waste.height = y - skyLine[skylineNodeIndex].y;

		debug_assert(disjointRects.Disjoint(waste));
		wasteMap.GetFreeRectangles().push_back(waste);
	}
}

void SkylineBinPack::AddSkylineLevel(size_t skylineNodeIndex, const Rect &rect)
{
	// First track all wasted areas and mark them into the waste map if we're using one.
	if (useWasteMap)
		AddWasteMapArea(skylineNodeIndex, rect.width, rect.height, rect.y);

	SkylineNode newNode;
	newNode.x = rect.x;
	newNode.y = rect.y + rect.height;
	newNode.width = rect.width;
	skyLine.insert(skyLine.begin() + skylineNodeIndex, newNode);

	assert(newNode.x + newNode.width <= binWidth);
	assert(newNode.y <= binHeight);

	for(size_t i = skylineNodeIndex+1; i < skyLine.size(); ++i)
	{
		assert(skyLine[i-1].x <= skyLine[i].x);

		if (skyLine[i].x < skyLine[i-1].x + skyLine[i-1].width)
		{
			Int shrink = skyLine[i-1].x + skyLine[i-1].width - skyLine[i].x;

			skyLine[i].x += shrink;
			skyLine[i].width -= shrink;

			if (skyLine[i].width <= 0)
			{
				skyLine.erase(skyLine.begin() + i);
				--i;
			}
			else
				break;
		}
		else
			break;
	}
	MergeSkylines();
}

void SkylineBinPack::MergeSkylines()
{
	for(size_t i = 0; i + 1 < skyLine.size(); ++i)
		if (skyLine[i].y == skyLine[i+1].y)
		{
			skyLine[i].width += skyLine[i+1].width;
			skyLine.erase(skyLine.begin() + (i+1));
			--i;
		}
}

Rect SkylineBinPack::InsertBottomLeft(Int width, Int height)
{
	Int bestHeight;
	Int bestWidth;
	size_t bestIndex;
	Rect newNode = FindPositionForNewNodeBottomLeft(width, height, bestHeight, bestWidth, bestIndex);

	if (bestIndex != skyLine.size())
	{
		debug_assert(disjointRects.Disjoint(newNode));

		// Perform the actual packing.
		AddSkylineLevel(bestIndex, newNode);

		usedSurfaceArea += width * height;
#ifdef _DEBUG
		disjointRects.Add(newNode);
#endif
	}
	else
		memset(&newNode, 0, sizeof(Rect));

	return newNode;
}

Rect SkylineBinPack::FindPositionForNewNodeBottomLeft(Int width, Int height, Int &bestHeight, Int &bestWidth, size_t &bestIndex) const
{
	bestHeight = std::numeric_limits<Int>::max();
	bestIndex = skyLine.size();
	// Used to break ties if there are nodes at the same level. Then pick the narrowest one.
	bestWidth = std::numeric_limits<Int>::max();
	Rect newNode;
	memset(&newNode, 0, sizeof(newNode));
	for(size_t i = 0; i < skyLine.size(); ++i)
	{
		Int y;
		if (RectangleFits(i, width, height, y))
		{
			if (y + height < bestHeight || (y + height == bestHeight && skyLine[i].width < bestWidth))
			{
				bestHeight = y + height;
				bestIndex = i;
				bestWidth = skyLine[i].width;
				newNode.x = skyLine[i].x;
				newNode.y = y;
				newNode.width = width;
				newNode.height = height;
				debug_assert(disjointRects.Disjoint(newNode));
			}
		}
		if (RectangleFits(i, height, width, y))
		{
			if (y + width < bestHeight || (y + width == bestHeight && skyLine[i].width < bestWidth))
			{
				bestHeight = y + width;
				bestIndex = i;
				bestWidth = skyLine[i].width;
				newNode.x = skyLine[i].x;
				newNode.y = y;
				newNode.width = height;
				newNode.height = width;
				debug_assert(disjointRects.Disjoint(newNode));
			}
		}
	}

	return newNode;
}

Rect SkylineBinPack::InsertMinWaste(Int width, Int height)
{
	Int bestHeight;
	Int bestWastedArea;
	size_t bestIndex;
	Rect newNode = FindPositionForNewNodeMinWaste(width, height, bestHeight, bestWastedArea, bestIndex);

	if (bestIndex != skyLine.size())
	{
		debug_assert(disjointRects.Disjoint(newNode));

		// Perform the actual packing.
		AddSkylineLevel(bestIndex, newNode);

		usedSurfaceArea += width * height;
#ifdef _DEBUG
		disjointRects.Add(newNode);
#endif
	}
	else
		memset(&newNode, 0, sizeof(newNode));

	return newNode;
}

Rect SkylineBinPack::FindPositionForNewNodeMinWaste(Int width, Int height, Int &bestHeight, Int &bestWastedArea, size_t &bestIndex) const
{
	bestHeight = std::numeric_limits<Int>::max();
	bestWastedArea = std::numeric_limits<Int>::max();
	bestIndex = skyLine.size();
	Rect newNode;
	memset(&newNode, 0, sizeof(newNode));
	for(size_t i = 0; i < skyLine.size(); ++i)
	{
		Int y;
		Int wastedArea;

		if (RectangleFits(i, width, height, y, wastedArea))
		{
			if (wastedArea < bestWastedArea || (wastedArea == bestWastedArea && y + height < bestHeight))
			{
				bestHeight = y + height;
				bestWastedArea = wastedArea;
				bestIndex = i;
				newNode.x = skyLine[i].x;
				newNode.y = y;
				newNode.width = width;
				newNode.height = height;
				debug_assert(disjointRects.Disjoint(newNode));
			}
		}
		if (RectangleFits(i, height, width, y, wastedArea))
		{
			if (wastedArea < bestWastedArea || (wastedArea == bestWastedArea && y + width < bestHeight))
			{
				bestHeight = y + width;
				bestWastedArea = wastedArea;
				bestIndex = i;
				newNode.x = skyLine[i].x;
				newNode.y = y;
				newNode.width = height;
				newNode.height = width;
				debug_assert(disjointRects.Disjoint(newNode));
			}
		}
	}

	return newNode;
}

/// Computes the ratio of used surface area.
float SkylineBinPack::Occupancy() const
{
	if (binWidth == 0 || binHeight == 0)
		return 0;

	return (float)usedSurfaceArea / (binWidth * binHeight);
}

}