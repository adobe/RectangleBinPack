#include "../MaxRectsBinPack.h"
#include <cstdio>
#include <cerrno>
#include <cinttypes>
#include <limits>

namespace {
bool ParseDimension(const char *text, rbp::Int &value)
{
	errno = 0;
	char *end = 0;
	const std::intmax_t parsed = std::strtoimax(text, &end, 10);
	if (errno == ERANGE || end == text || *end != '\0' ||
		parsed < std::numeric_limits<rbp::Int>::min() ||
		parsed > std::numeric_limits<rbp::Int>::max())
		return false;
	value = static_cast<rbp::Int>(parsed);
	return true;
}
}


int main(int argc, char **argv)
{
	
	if (argc < 5 || argc % 2 != 1)
	{
		printf("Usage: MaxRectsBinPackTest binWidth binHeight w_0 h_0 w_1 h_1 w_2 h_2 ... w_n h_n\n");
		printf("       where binWidth and binHeight define the size of the bin.\n");
		printf("       w_i is the width of the i'th rectangle to pack, and h_i the height.\n");
		printf("Example: MaxRectsBinPackTest 256 256 30 20 50 20 10 80 90 20\n");
		return 0;
	}
	
	using namespace rbp;
	
	// Create a bin to pack to, use the bin size from command line.
	MaxRectsBinPack bin;
	Int binWidth, binHeight;
	if (!ParseDimension(argv[1], binWidth) || !ParseDimension(argv[2], binHeight))
	{
		fprintf(stderr, "Invalid Int64 bin dimension.\n");
		return 1;
	}
	printf("Initializing bin to size %" PRId64 "x%" PRId64 ".\n", binWidth, binHeight);
	bin.Init(binWidth, binHeight);
	
	// Pack each rectangle (w_i, h_i) the user inputted on the command line.
	for(int i = 3; i < argc; i += 2)
	{
		// Read next rectangle to pack.
		Int rectWidth, rectHeight;
		if (!ParseDimension(argv[i], rectWidth) || !ParseDimension(argv[i+1], rectHeight))
		{
			fprintf(stderr, "Invalid Int64 rectangle dimension.\n");
			return 1;
		}
		printf("Packing rectangle of size %" PRId64 "x%" PRId64 ": ", rectWidth, rectHeight);

		// Perform the packing.
		MaxRectsBinPack::FreeRectChoiceHeuristic heuristic = MaxRectsBinPack::RectBestShortSideFit; // This can be changed individually even for each rectangle packed.
		Rect packedRect = bin.Insert(rectWidth, rectHeight, heuristic);

		// Test success or failure.
		if (packedRect.height > 0)
			printf("Packed to (x,y)=(%" PRId64 ",%" PRId64 "), (w,h)=(%" PRId64 ",%" PRId64 "). Free space left: %.2f%%\n", packedRect.x, packedRect.y, packedRect.width, packedRect.height, 100.f - bin.Occupancy()*100.f);
		else
			printf("Failed! Could not find a proper position to pack this rectangle into. Skipping this one.\n");
	}
	printf("Done. All rectangles packed.\n");
}
