/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file road_freehand.h Freehand road path helpers (GUI-independent). */

#ifndef ROAD_FREEHAND_H
#define ROAD_FREEHAND_H

#include <functional>

#include "command_type.h"
#include "direction_type.h"
#include "freehand_path.h"
#include "road_type.h"
#include "tile_type.h"

/**
 * Orthogonal tile path for freehand road drawing / removal.
 * Stores only geometry; road bits are derived from neighbour relations.
 */
struct FreehandRoadPath : FreehandTilePath {
	/** Road bits for path index \a i (straight, corner, or end stub). */
	RoadBits BitsForIndex(size_t i) const;

	/** Test-build every tile without changing the map. */
	CommandCost TestBuild(RoadType rt) const;

	/** Test-remove as coalesced RemoveRoadLong segments. */
	CommandCost TestRemove(RoadType rt) const;

	CommandCost Test(RoadType rt, bool remove) const
	{
		return remove ? this->TestRemove(rt) : this->TestBuild(rt);
	}

	bool IsReady(RoadType rt, bool remove) const;

	/**
	 * Whether the current stroke should be shown as invalid for the operation.
	 * A single start tile is not invalid; only obstructed or failed multi-tile paths are.
	 */
	bool ShouldHighlightInvalid(RoadType rt, bool remove) const;

	/**
	 * Iterate coalesced axis-aligned segments for RemoveRoadLong.
	 * Callback arguments: start_tile, end_tile, axis.
	 */
	void ForEachRemoveSegment(const std::function<void(TileIndex, TileIndex, Axis)> &callback) const
	{
		this->ForEachAxisSegment(callback);
	}
};

#endif /* ROAD_FREEHAND_H */
