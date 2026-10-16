This repository contains the source code used in my survey 

"A Thousand Ways to Pack the Bin - A Practical Approach to Two-Dimensional Rectangle Bin Packing."

The code can be used to solve the problem of packing a set of 2D rectangles into a larger bin. The 
algorithms are all approximations and use various heuristics, since the problem itself is intractable. 
For more information, read the paper, which is also contained as a .pdf in this repository.

The repository contains a build solution for Visual C++ 2010. It is configured to build a single static
library of the algorithms. The preferred method of embedding these algorithms into your own code is to
simply copy the code files into your own project and build them along your own build system. 

CMake provides the RectangleBinPack::RectangleBinPack target with
<RectangleBinPack/Foo.h> includes. RECTANGLE_BIN_PACK_BUILD_TESTS and
RECTANGLE_BIN_PACK_BUILD_EXAMPLES enable optional tests/examples; both default to OFF.

Coordinates, dimensions, areas and scores use signed rbp::Int (std::int64_t).
This changes the API/ABI (Rect is now 32 bytes); all consumers must rebuild.
Positive bin sizes require W*H <= INT64_MAX and W+H <= INT64_MAX/2; other sizes
reset to empty bins with occupancy 0. Invalid/oversized insertions return zero
rectangles. Batches reject any nonpositive entry without consuming input or
changing packer state (clearing the output vector where present).

To use these algorithms in Python, you can make use of Boost.Python to enable interoperability between the C++ algorithms and your Python code.

CMake and Meson builds are available for other platforms.

For more information, see a series of blog posts at

http://clb.demon.fi/projects/rectangle-bin-packing
http://clb.demon.fi/projects/more-rectangle-bin-packing
http://clb.demon.fi/projects/even-more-rectangle-bin-packing

The folder old\ contains an implementation of the bin packing algorithm as originally presented by
Jim Scott at http://www.blackpawn.com/texts/lightmaps/ . This implementation is only provided for
historical purposes, since in practice the Guillotine method (GuillotineBinPack.cpp) provides superior
results in terms of performance and packing ratio, and also offers more configuration for variants of
the algorithm.

All the code is released to Public Domain. Patches and comments are welcome.
It makes me happy to hear if someone finds the algorithms useful.

   Jukka Jylänki
