/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file canal_freehand.cpp Freehand canal path helpers. */

#include "stdafx.h"

#include "canal_freehand.h"

#include "command_func.h"
#include "landscape_cmd.h"
#include "table/strings.h"
#include "water_cmd.h"
#include "water_map.h"

#include "safeguards.h"

CommandCost FreehandCanalPath::TestBuild(WaterClass wc) const
{
	CommandCost total(ExpensesType::Construction);

	if (this->obstructed) return CMD_ERROR;
	if (this->tiles.size() < 2) return CMD_ERROR;

	const DoCommandFlags flags = CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildCanal>());

	for (TileIndex tile : this->tiles) {
		CommandCost ret = Command<Commands::BuildCanal>::Do(flags, tile, tile, wc, false);
		if (ret.Failed()) {
			if (ret.GetErrorMessage() == STR_ERROR_ALREADY_BUILT) continue;
			return ret;
		}
		total.AddCost(ret.GetCost());
	}

	return total;
}

CommandCost FreehandCanalPath::TestRemove() const
{
	if (this->obstructed) return CMD_ERROR;
	if (this->tiles.size() < 2) return CMD_ERROR;

	const DoCommandFlags flags = CommandFlagsToDCFlags(GetCommandFlags<Commands::LandscapeClear>());
	CommandCost total(ExpensesType::Construction);
	CommandCost last_error = CMD_ERROR;
	bool any_success = false;

	for (TileIndex tile : this->tiles) {
		if (!IsWaterTile(tile) || !IsCanal(tile)) {
			last_error = CommandCost(STR_ERROR_CAN_T_CLEAR_THIS_AREA);
			continue;
		}

		CommandCost ret = Command<Commands::LandscapeClear>::Do(flags, tile);
		if (ret.Failed()) {
			last_error = std::move(ret);
			continue;
		}
		any_success = true;
		total.AddCost(ret.GetCost());
	}

	return any_success ? total : last_error;
}

bool FreehandCanalPath::IsReady(WaterClass wc, bool remove) const
{
	return this->Test(wc, remove).Succeeded();
}

bool FreehandCanalPath::ShouldHighlightInvalid(WaterClass wc, bool remove) const
{
	if (this->obstructed) return true;
	if (this->tiles.size() < 2) return false;
	return !this->IsReady(wc, remove);
}
