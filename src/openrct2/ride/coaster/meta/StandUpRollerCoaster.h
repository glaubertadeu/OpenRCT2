/*****************************************************************************
 * Copyright (c) 2014-2020 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "../../RideData.h"
#include "../../ShopItem.h"
#include "../../Track.h"

// clang-format off
constexpr const RideTypeDescriptor StandUpRollerCoasterRTD =
{
    SET_FIELD(AlternateType, RIDE_TYPE_NULL),
    SET_FIELD(Category, RIDE_CATEGORY_ROLLERCOASTER),
    SET_FIELD(EnabledTrackPieces, (1ULL << TRACK_STRAIGHT) | (1ULL << TRACK_STATION_END) | (1ULL << TRACK_LIFT_HILL) | (1ULL << TRACK_FLAT_ROLL_BANKING) | (1ULL << TRACK_VERTICAL_LOOP) | (1ULL << TRACK_SLOPE) | (1ULL << TRACK_SLOPE_STEEP) | (1ULL << TRACK_SLOPE_CURVE) | (1ULL << TRACK_S_BEND) | (1ULL << TRACK_CURVE_SMALL) | (1ULL << TRACK_CURVE) | (1ULL << TRACK_HALF_LOOP) | (1ULL << TRACK_CORKSCREW) | (1ULL << TRACK_HELIX_SMALL) | (1ULL << TRACK_BRAKES) | (1ULL << TRACK_ON_RIDE_PHOTO) | (1ULL << TRACK_BLOCK_BRAKES)),
    SET_FIELD(ExtraTrackPieces, 0),
    SET_FIELD(CoveredTrackPieces, 0),
    SET_FIELD(StartTrackPiece, TRACK_ELEM_END_STATION),
    SET_FIELD(TrackPaintFunction, get_track_paint_function_stand_up_rc),
    SET_FIELD(Flags, RIDE_TYPE_FLAGS_TRACK_HAS_3_COLOURS | RIDE_TYPE_FLAG_HAS_LEAVE_WHEN_ANOTHER_VEHICLE_ARRIVES_AT_STATION |
                     RIDE_TYPE_FLAGS_COMMON_COASTER | RIDE_TYPE_FLAGS_COMMON_COASTER_NON_ALT | RIDE_TYPE_FLAG_HAS_LARGE_CURVES |
                     RIDE_TYPE_FLAG_PEEP_CHECK_GFORCES | RIDE_TYPE_FLAG_ALLOW_MULTIPLE_CIRCUITS),
    SET_FIELD(RideModes, (1ULL << RIDE_MODE_CONTINUOUS_CIRCUIT) | (1ULL << RIDE_MODE_CONTINUOUS_CIRCUIT_BLOCK_SECTIONED) | (1ULL << RIDE_MODE_REVERSE_INCLINE_LAUNCHED_SHUTTLE) | (1ULL << RIDE_MODE_POWERED_LAUNCH_PASSTROUGH) | (1ULL << RIDE_MODE_POWERED_LAUNCH)),
    SET_FIELD(DefaultMode, RIDE_MODE_CONTINUOUS_CIRCUIT),
    SET_FIELD(OperatingSettings, { 7, 27, 0, 0, 0, 0 }),
    SET_FIELD(Naming, { STR_RIDE_NAME_STAND_UP_ROLLER_COASTER, STR_RIDE_DESCRIPTION_STAND_UP_ROLLER_COASTER }),
    SET_FIELD(NameConvention, { RIDE_COMPONENT_TYPE_TRAIN, RIDE_COMPONENT_TYPE_TRACK, RIDE_COMPONENT_TYPE_STATION }),
    SET_FIELD(EnumName, nameof(RIDE_TYPE_STAND_UP_ROLLER_COASTER)),
    SET_FIELD(AvailableBreakdowns, (1 << BREAKDOWN_SAFETY_CUT_OUT) | (1 << BREAKDOWN_RESTRAINTS_STUCK_CLOSED) | (1 << BREAKDOWN_RESTRAINTS_STUCK_OPEN) | (1 << BREAKDOWN_VEHICLE_MALFUNCTION) | (1 << BREAKDOWN_BRAKES_FAILURE)),
    SET_FIELD(Heights, { 25, 24, 9, 11, }),
    SET_FIELD(MaxMass, 18),
    SET_FIELD(LiftData, { SoundId::LiftClassic, 4, 6 }),
    SET_FIELD(RatingsCalculationFunction, ride_ratings_calculate_stand_up_roller_coaster),
    SET_FIELD(RatingsMultipliers, { 50, 30, 10 }),
    SET_FIELD(UpkeepCosts, { 40, 20, 80, 10, 3, 10 }),
    SET_FIELD(BuildCosts, { 100, 5, 50, }),
    SET_FIELD(DefaultPrices, { 20, 20 }),
    SET_FIELD(DefaultMusic, MUSIC_STYLE_TECHNO),
    SET_FIELD(PhotoItem, SHOP_ITEM_PHOTO),
    SET_FIELD(BonusValue, 90),
    SET_FIELD(ColourPresets, TRACK_COLOUR_PRESETS(
        { COLOUR_BRIGHT_RED, COLOUR_BRIGHT_RED, COLOUR_BLACK },
        { COLOUR_BLACK, COLOUR_LIGHT_BLUE, COLOUR_BLACK },
        { COLOUR_LIGHT_ORANGE, COLOUR_WHITE, COLOUR_LIGHT_ORANGE },
        { COLOUR_BORDEAUX_RED, COLOUR_YELLOW, COLOUR_BORDEAUX_RED },
        { COLOUR_MOSS_GREEN, COLOUR_MOSS_GREEN, COLOUR_WHITE },
        { COLOUR_SATURATED_BROWN, COLOUR_LIGHT_ORANGE, COLOUR_SATURATED_BROWN },
    )),
    SET_FIELD(ColourKey, RideColourKey::Ride),
};
// clang-format on
