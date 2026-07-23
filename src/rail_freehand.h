/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file rail_freehand.h Freehand rail path helpers (GUI-independent). */

#ifndef RAIL_FREEHAND_H
#define RAIL_FREEHAND_H

#include "command_type.h"
#include "freehand_path.h"
#include "rail_type.h"
#include "track_type.h"

/**
 * Orthogonal tile path for freehand rail drawing / removal.
 * Track pieces (including corners) are derived from neighbour relations.
 */
struct FreehandRailPath : FreehandTilePath {
	/** Track piece for path index \a i (straight, corner, or end stub). */
	Track TrackForIndex(size_t i) const;

	/** Test-build every tile without changing the map. */
	CommandCost TestBuild(RailType rt) const;

	/** Test-remove every tile without changing the map. */
	CommandCost TestRemove() const;

	CommandCost Test(RailType rt, bool remove) const
	{
		return remove ? this->TestRemove() : this->TestBuild(rt);
	}

	bool IsReady(RailType rt, bool remove) const;

	/**
	 * Whether the current stroke should be shown as invalid for the operation.
	 * A single start tile is not invalid; only obstructed or failed multi-tile paths are.
	 */
	bool ShouldHighlightInvalid(RailType rt, bool remove) const;
};

#endif /* RAIL_FREEHAND_H */
