/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file rail_freehand.cpp Freehand rail path helpers. */

#include "stdafx.h"

#include "rail_freehand.h"

#include "command_func.h"
#include "direction_func.h"
#include "map_func.h"
#include "rail.h"
#include "rail_cmd.h"
#include "settings_type.h"
#include "table/strings.h"
#include "track_func.h"

#include "safeguards.h"

/**
 * Map two tile-exit DiagDirections to the Track that connects them.
 */
static Track DiagDirsToTrack(DiagDirection a, DiagDirection b)
{
	if (!IsValidDiagDirection(a) || !IsValidDiagDirection(b) || a == b) return Track::Invalid;

	if (a > b) std::swap(a, b);

	/* Opposite sides -> straight (diagonal) track. */
	if (a == DiagDirection::NE && b == DiagDirection::SW) return Track::X;
	if (a == DiagDirection::SE && b == DiagDirection::NW) return Track::Y;

	/* Adjacent sides -> corner track. */
	if (a == DiagDirection::NE && b == DiagDirection::SE) return Track::Right;
	if (a == DiagDirection::SE && b == DiagDirection::SW) return Track::Lower;
	if (a == DiagDirection::SW && b == DiagDirection::NW) return Track::Left;
	if (a == DiagDirection::NE && b == DiagDirection::NW) return Track::Upper;

	return Track::Invalid;
}

Track FreehandRailPath::TrackForIndex(size_t i) const
{
	assert(i < this->tiles.size());

	const TileIndex cur = this->tiles[i];
	const TileIndex prev = (i > 0) ? this->tiles[i - 1] : INVALID_TILE;
	const TileIndex next = (i + 1 < this->tiles.size()) ? this->tiles[i + 1] : INVALID_TILE;

	if (prev != INVALID_TILE && next != INVALID_TILE) {
		const DiagDirection to_prev = DiagdirBetweenTiles(cur, prev);
		const DiagDirection to_next = DiagdirBetweenTiles(cur, next);
		return DiagDirsToTrack(to_prev, to_next);
	}

	/* End stub: place the straight track along the connected axis. */
	const TileIndex other = (next != INVALID_TILE) ? next : prev;
	if (other == INVALID_TILE) return Track::Invalid;
	const DiagDirection dir = DiagdirBetweenTiles(cur, other);
	if (!IsValidDiagDirection(dir)) return Track::Invalid;
	return AxisToTrack(DiagDirToAxis(dir));
}

CommandCost FreehandRailPath::TestBuild(RailType rt) const
{
	CommandCost total(ExpensesType::Construction);

	if (this->obstructed) return CMD_ERROR;
	if (this->tiles.size() < 2) return CMD_ERROR;
	if (!ValParamRailType(rt)) return CMD_ERROR;

	const DoCommandFlags flags = CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRail>());
	const bool auto_remove_signals = _settings_client.gui.auto_remove_signals;

	for (size_t i = 0; i < this->tiles.size(); i++) {
		const Track track = this->TrackForIndex(i);
		if (!IsValidTrack(track)) return CMD_ERROR;

		CommandCost ret = Command<Commands::BuildRail>::Do(flags, this->tiles[i], rt, track, auto_remove_signals);
		if (ret.Failed()) {
			if (ret.GetErrorMessage() == STR_ERROR_ALREADY_BUILT) continue;
			return ret;
		}
		total.AddCost(ret.GetCost());
	}

	return total;
}

CommandCost FreehandRailPath::TestRemove() const
{
	if (this->obstructed) return CMD_ERROR;
	if (this->tiles.size() < 2) return CMD_ERROR;

	const DoCommandFlags flags = CommandFlagsToDCFlags(GetCommandFlags<Commands::RemoveRail>());
	CommandCost total(ExpensesType::Construction);
	CommandCost last_error = CMD_ERROR;
	bool any_success = false;

	for (size_t i = 0; i < this->tiles.size(); i++) {
		const Track track = this->TrackForIndex(i);
		if (!IsValidTrack(track)) return CMD_ERROR;

		CommandCost ret = Command<Commands::RemoveRail>::Do(flags, this->tiles[i], track);
		if (ret.Failed()) {
			last_error = std::move(ret);
			continue;
		}
		any_success = true;
		total.AddCost(ret.GetCost());
	}

	return any_success ? total : last_error;
}

bool FreehandRailPath::IsReady(RailType rt, bool remove) const
{
	return this->Test(rt, remove).Succeeded();
}

bool FreehandRailPath::ShouldHighlightInvalid(RailType rt, bool remove) const
{
	if (this->obstructed) return true;
	if (this->tiles.size() < 2) return false;
	return !this->IsReady(rt, remove);
}
