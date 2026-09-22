#pragma once

namespace dawnlight::pedestal_mesh {
enum Finish { stone, trim, groove };
// Each point starts an octagonal band; finish applies to the face above it.
// Thin warm inlays follow the actual moldings. No ornamental atlas is stretched
// across bevels, and the cap stays quiet around the sword's insertion point.
struct Ring { float radius, height; Finish finish; };
inline constexpr Ring kRings[] = {
    {96, 0, stone}, {100, 4, stone},
    {100, 6, groove}, {100, 7, trim}, {100, 9, groove},
    {100, 10, stone}, {100, 12, stone},
    {90, 19, stone}, {72, 19, trim}, {68, 24, stone},
    {68, 27, trim}, {68, 29, groove}, {68, 30, stone},
    {68, 48, groove}, {68, 49, trim}, {68, 51, stone},
    {68, 54, stone}, {58, 62, stone},
    {53, 62, groove}, {52, 62, trim}, {50, 62, groove},
    {49, 62, stone}, {0, 62, stone},
};
inline constexpr unsigned kBands = sizeof(kRings) / sizeof(kRings[0]) - 1;
} // namespace dawnlight::pedestal_mesh
