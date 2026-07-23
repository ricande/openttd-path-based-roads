/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file freehand_path.cpp Shared orthogonal freehand tile-path helpers. */

#include "stdafx.h"

#include "freehand_path.h"

#include "direction_func.h"
#include "map_func.h"
#include "tile_map.h"

#include <algorithm>

#include "safeguards.h"

void FreehandTilePath::Clear()
{
	this->tiles.clear();
	this->obstructed = false;
}

bool FreehandTilePath::Contains(TileIndex tile) const
{
	return std::find(this->tiles.begin(), this->tiles.end(), tile) != this->tiles.end();
}

void FreehandTilePath::Start(TileIndex tile)
{
	this->Clear();
	if (!IsValidTile(tile)) return;
	this->tiles.push_back(tile);
}

/**
 * Append orthogonal steps from the last path tile to \a to (exclusive of start, inclusive of end).
 */
static bool InterpolateOrthogonal(TileIndex from, TileIndex to, std::vector<TileIndex> &out)
{
	if (!IsValidTile(from) || !IsValidTile(to)) return false;

	int x = TileX(from);
	int y = TileY(from);
	const int x1 = TileX(to);
	const int y1 = TileY(to);

	while (x != x1) {
		x += (x1 > x) ? 1 : -1;
		TileIndex step = TileXY(x, y);
		if (!IsValidTile(step)) return false;
		out.push_back(step);
	}
	while (y != y1) {
		y += (y1 > y) ? 1 : -1;
		TileIndex step = TileXY(x, y);
		if (!IsValidTile(step)) return false;
		out.push_back(step);
	}
	return true;
}

/**
 * Truncate the path so that \a tile is the last tile.
 * @return True if \a tile was on the path and truncation happened (or tile was already last).
 */
static bool TruncateToTile(std::vector<TileIndex> &tiles, TileIndex tile)
{
	auto it = std::find(tiles.begin(), tiles.end(), tile);
	if (it == tiles.end()) return false;
	tiles.erase(std::next(it), tiles.end());
	return true;
}

void FreehandTilePath::ExtendTo(TileIndex tile)
{
	if (!IsValidTile(tile) || this->tiles.empty()) return;

	/* Backtrack: moving onto an existing path tile shortens the stroke. */
	if (TruncateToTile(this->tiles, tile)) {
		this->obstructed = false;
		return;
	}

	if (this->obstructed) return;
	if (tile == this->tiles.back()) return;

	std::vector<TileIndex> steps;
	if (!InterpolateOrthogonal(this->tiles.back(), tile, steps) || steps.empty()) {
		this->obstructed = true;
		return;
	}

	for (TileIndex step : steps) {
		/* Crossing an earlier tile also backtracks, then continues if needed. */
		if (TruncateToTile(this->tiles, step)) {
			this->obstructed = false;
			continue;
		}
		this->tiles.push_back(step);
	}
}

void FreehandTilePath::ForEachAxisSegment(const std::function<void(TileIndex, TileIndex, Axis)> &callback) const
{
	if (this->tiles.size() < 2) return;

	size_t i = 0;
	while (i + 1 < this->tiles.size()) {
		const DiagDirection dir = DiagdirBetweenTiles(this->tiles[i], this->tiles[i + 1]);
		if (!IsValidDiagDirection(dir)) return;
		const Axis axis = DiagDirToAxis(dir);

		size_t j = i + 1;
		while (j + 1 < this->tiles.size()) {
			const DiagDirection next_dir = DiagdirBetweenTiles(this->tiles[j], this->tiles[j + 1]);
			if (!IsValidDiagDirection(next_dir) || DiagDirToAxis(next_dir) != axis) break;
			j++;
		}

		callback(this->tiles[i], this->tiles[j], axis);
		i = j;
	}
}
