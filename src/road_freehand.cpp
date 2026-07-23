/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file road_freehand.cpp Freehand road path helpers. */

#include "stdafx.h"

#include "road_freehand.h"

#include "command_func.h"
#include "direction_func.h"
#include "map_func.h"
#include "road_cmd.h"
#include "road_func.h"
#include "table/strings.h"
#include "tile_map.h"
#include "town_type.h"

#include "safeguards.h"

RoadBits FreehandRoadPath::BitsForIndex(size_t i) const
{
	assert(i < this->tiles.size());

	const TileIndex cur = this->tiles[i];
	const TileIndex prev = (i > 0) ? this->tiles[i - 1] : INVALID_TILE;
	const TileIndex next = (i + 1 < this->tiles.size()) ? this->tiles[i + 1] : INVALID_TILE;

	RoadBits bits{};
	if (prev != INVALID_TILE) {
		const DiagDirection dir = DiagdirBetweenTiles(cur, prev);
		if (!IsValidDiagDirection(dir)) return {};
		bits.Set(DiagDirToRoadBits(dir));
	}
	if (next != INVALID_TILE) {
		const DiagDirection dir = DiagdirBetweenTiles(cur, next);
		if (!IsValidDiagDirection(dir)) return {};
		bits.Set(DiagDirToRoadBits(dir));
	}
	return bits;
}

CommandCost FreehandRoadPath::TestBuild(RoadType rt) const
{
	CommandCost total(ExpensesType::Construction);

	if (this->obstructed) return CMD_ERROR;
	if (this->tiles.size() < 2) return CMD_ERROR;
	if (!ValParamRoadType(rt)) return CMD_ERROR;

	const DoCommandFlags flags = CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRoad>());

	for (size_t i = 0; i < this->tiles.size(); i++) {
		const RoadBits bits = this->BitsForIndex(i);
		if (bits.None()) return CMD_ERROR;

		CommandCost ret = Command<Commands::BuildRoad>::Do(flags, this->tiles[i], bits, rt, {}, TownID::Invalid());
		if (ret.Failed()) {
			if (ret.GetErrorMessage() == STR_ERROR_ALREADY_BUILT) continue;
			return ret;
		}
		total.AddCost(ret.GetCost());
	}

	return total;
}

CommandCost FreehandRoadPath::TestRemove(RoadType rt) const
{
	if (this->obstructed) return CMD_ERROR;
	if (this->tiles.size() < 2) return CMD_ERROR;
	if (!ValParamRoadType(rt)) return CMD_ERROR;

	const DoCommandFlags flags = CommandFlagsToDCFlags(GetCommandFlags<Commands::RemoveRoadLong>());
	CommandCost total(ExpensesType::Construction);
	CommandCost last_error = CMD_ERROR;
	bool any_success = false;

	this->ForEachRemoveSegment([&](TileIndex start, TileIndex end, Axis axis) {
		CommandCost ret = ExtractCommandCost(Command<Commands::RemoveRoadLong>::Do(flags, end, start, rt, axis, false, false));
		if (ret.Failed()) {
			last_error = std::move(ret);
			return;
		}
		any_success = true;
		total.AddCost(ret.GetCost());
	});

	return any_success ? total : last_error;
}

bool FreehandRoadPath::IsReady(RoadType rt, bool remove) const
{
	return this->Test(rt, remove).Succeeded();
}

bool FreehandRoadPath::ShouldHighlightInvalid(RoadType rt, bool remove) const
{
	if (this->obstructed) return true;
	if (this->tiles.size() < 2) return false;
	return !this->IsReady(rt, remove);
}
