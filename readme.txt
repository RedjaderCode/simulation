NEXT THINGS TO DO:

 - render pipeline is causing a bottleneck. Implement different pipeline.
 - perhaps it's time to retry d3dx11...
 - Further back-end cell matrix optimizations. at 60 fps target framerate, cpu runs at about 75%
 - bitwise key states and yaw and pitch transformation problem


enum CellFlags : uint8_t {
    CELL_DIRTY   = 1 << 0,  // Needs update/redraw
    CELL_ACTIVE  = 1 << 1,  // In motion / simulated
    CELL_STATIC  = 1 << 2,  // Locked/static
    CELL_VISIBLE = 1 << 3,  // For rendering
    CELL_SOLID   = 1 << 4,  // Used in raymarch/DDA
    // Bits 5–7 reserved for future use
};

Why Use Bitwise Flags?

uint8_t flags		8 flags in 1 byte → minimal memory overhead
Bitwise AND/OR/XOR	Extremely fast, branchless updates
Packed array layout	Maximizes cache locality
SIMD-friendly		Compatible with AVX/AVX2 vectorization
Easy to clear/set	`flags


cell.flags |= CELL_DIRTY; Set a flag
cell.flags &= ~CELL_DIRTY; Clear flag
cell.flags ^= CELL_DIRTY; Toggle flag
if (cell.flags & CELL_DIRTY) {
    // Only process dirty cells
}
if (!(cell.flags & CELL_DIRTY)) {
    // Cell is clean
}

 - Set an array of w*h*d size; cellflags separate from main cell structure. Cache efficient.
 - use SIMD like cell checking
 - use SIMD step with raymarch DDA