/** @file ShelfNextFitBinPack.cpp
	@author Jukka Jylänki

	@brief Implements the naive Shelf Next Fit bin packer algorithm.

	This algorithm is not recommended for real use at all - its only advantage is that it
	consumes only a constant amount of memory, whereas the other packers in this library
	use at least a linear amount of memory.

	This work is released to Public Domain, do whatever you want with it.
*/
#include <algorithm>
#include <utility>
#include <iostream>

#include <cassert>
#include <cstring>

#include "ShelfNextFitBinPack.h"
#include "BinPackCommon.h"

namespace rbp {

using namespace std;

ShelfNextFitBinPack::ShelfNextFitBinPack()
{
	Init(0, 0);
}

void ShelfNextFitBinPack::Init(Int width, Int height)
{
	if (!detail::IsValidBinSize(width, height))
		width = height = 0;

	binWidth = width;
	binHeight = height;

	currentX = 0;
	currentY = 0;
	shelfHeight = 0;
	usedSurfaceArea = 0;
}

ShelfNextFitBinPack::Node ShelfNextFitBinPack::Insert(Int width, Int height)
{
	if (!detail::FitsBin(width, height, binWidth, binHeight))
		return Node{};

	Node newNode;
	// There are three cases:
	// 1. short edge <= long edge <= shelf height. Then store the long edge vertically.
	// 2. short edge <= shelf height <= long edge. Then store the short edge vertically.
	// 3. shelf height <= short edge <= long edge. Then store the short edge vertically.

	// If the long edge of the new rectangle fits vertically onto the current shelf,
	// flip it. If the short edge is larger than the current shelf height, store
	// the short edge vertically.
	if (((width > height && width < shelfHeight) ||
		(width < height && height > shelfHeight)))
	{
		newNode.flipped = true;
		swap(width, height);
	}
	else
		newNode.flipped = false;

	if (width > binWidth - currentX)
	{
		currentX = 0;
		currentY += shelfHeight;
		shelfHeight = 0;

		// When starting a new shelf, store the new long edge of the new rectangle horizontally
		// to minimize the new shelf height.
		if (width < height)
		{
			swap(width, height);
			newNode.flipped = !newNode.flipped;
		}
	}

	// If the rectangle doesn't fit in this orientation, try flipping.
	if (width > binWidth || height > binHeight - currentY)
	{
		swap(width, height);
		newNode.flipped = !newNode.flipped;
	}

	// If flipping didn't help, return failure.
	if (width > binWidth || height > binHeight - currentY)
	{
		memset(&newNode, 0, sizeof(Node));
		return newNode;
	}

	newNode.width = width;
	newNode.height = height;
	newNode.x = currentX;
	newNode.y = currentY;

	currentX += width;
	shelfHeight = max(shelfHeight, height);

	usedSurfaceArea += width * height;

	return newNode;
}

/// Computes the ratio of used surface area.
float ShelfNextFitBinPack::Occupancy() const
{
	if (binWidth == 0 || binHeight == 0)
		return 0;

	return (float)usedSurfaceArea / (binWidth * binHeight);
}

}
