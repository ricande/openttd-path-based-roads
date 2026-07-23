/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file freehand_path.h Shared orthogonal freehand tile-path helpers. */

#ifndef FREEHAND_PATH_H
#define FREEHAND_PATH_H

#include <functional>
#include <vector>

#include "direction_type.h"
#include "tile_type.h"

/**
 * Orthogonal tile path for freehand drawing / removal.
 * Stores only geometry; transport-specific bits are derived by callers.
 */
struct FreehandTilePath {
	std::vector<TileIndex> tiles; ///< Ordered orthogonal path tiles.
	bool obstructed = false; ///< True if a later tile could not be added cleanly.

	void Clear();
	bool IsEmpty() const { return this->tiles.empty(); }
	size_t Length() const { return this->tiles.size(); }
	bool Contains(TileIndex tile) const;

	/** Begin a new stroke on \a tile. */
	void Start(TileIndex tile);

	/**
	 * Extend the path toward \a tile.
	 * Inserts orthogonal intermediate tiles when the cursor jumps.
	 * Moving onto a tile already in the path backtracks (truncates) to that tile.
	 */
	void ExtendTo(TileIndex tile);

	/**
	 * Iterate coalesced axis-aligned segments.
	 * Callback arguments: start_tile, end_tile, axis.
	 */
	void ForEachAxisSegment(const std::function<void(TileIndex, TileIndex, Axis)> &callback) const;
};

#endif /* FREEHAND_PATH_H */
