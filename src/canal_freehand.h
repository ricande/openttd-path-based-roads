/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file canal_freehand.h Freehand canal path helpers (GUI-independent). */

#ifndef CANAL_FREEHAND_H
#define CANAL_FREEHAND_H

#include "command_type.h"
#include "freehand_path.h"
#include "water_map.h"

/**
 * Orthogonal tile path for freehand canal drawing / removal.
 */
struct FreehandCanalPath : FreehandTilePath {
	/** Test-build every tile without changing the map. */
	CommandCost TestBuild(WaterClass wc) const;

	/** Test-clear every canal tile without changing the map. */
	CommandCost TestRemove() const;

	CommandCost Test(WaterClass wc, bool remove) const
	{
		return remove ? this->TestRemove() : this->TestBuild(wc);
	}

	bool IsReady(WaterClass wc, bool remove) const;

	/**
	 * Whether the current stroke should be shown as invalid for the operation.
	 * A single start tile is not invalid; only obstructed or failed multi-tile paths are.
	 */
	bool ShouldHighlightInvalid(WaterClass wc, bool remove) const;
};

#endif /* CANAL_FREEHAND_H */
