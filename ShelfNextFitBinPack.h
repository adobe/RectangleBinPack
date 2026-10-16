/** @file ShelfNextFitBinPack.h
	@author Jukka Jylänki

	@brief Implements the naive Shelf Next Fit bin packer algorithm.

	This algorithm is not recommended for real use at all - its only advantage is that it
	consumes only a constant amount of memory, whereas the other packers in this library
	use at least a linear amount of memory.

	This work is released to Public Domain, do whatever you want with it.
*/
#pragma once

#include "Rect.h"

namespace rbp {

class ShelfNextFitBinPack
{
public:
	/// Initializes an empty bin.
	ShelfNextFitBinPack();

	struct Node
	{
		Int x;
		Int y;
		Int width;
		Int height;

		bool flipped;
	};

	void Init(Int width, Int height);

	/// Invalid/oversized inputs leave state unchanged; an ordinary non-fit may close a shelf.
	Node Insert(Int width, Int height);

	/// Computes the ratio of used surface area.
	float Occupancy() const;

private:
	Int binWidth;
	Int binHeight;

	Int currentX;
	Int currentY;
	Int shelfHeight;

	Int usedSurfaceArea;
};

}
