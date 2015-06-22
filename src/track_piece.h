/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file track_piece.h Declarations for track pieces. */

#ifndef TRACK_PIECE_H
#define TRACK_PIECE_H

#include <map>
#include <memory>
#include <vector>
#include "bitmath.h"
#include "money.h"

class ImageData;
typedef class std::map<unsigned int, ImageData *> ImageMap; ///< Loaded images.

/** Data of a voxel in a track piece. */
struct TrackVoxel {
	TrackVoxel();
	~TrackVoxel();

	bool Load(RcdFileReader *rcd_file, size_t length, const ImageMap &sprites);

	/**
	 * Does the track piece have a platform?
	 * @return Whether the track piece has a platform.
	 */
	inline bool HasPlatform() const
	{
		return GB(this->flags, 4, 3) != 0;
	}

	/**
	 * Get the 'direction' of the platform (the edge used for entering the voxel).
	 * @return Direction of the tracks (and thus the platform).
	 * @pre #HasPlatform() holds.
	 */
	inline TileEdge GetPlatformDirection() const
	{
		return static_cast<TileEdge>(GB(this->flags, 4, 3) - 1);
	}

	ImageData *back[4];  ///< Reference to the background tracks (N/E/S/W view).
	ImageData *front[4]; ///< Reference to the front tracks (N/E/S/W view).
	XYZPoint16 dxyz;     ///< Relative position of the voxel.
	uint8 flags;         ///< Flags of the voxel (space requirements, platform direction).
};

/** Banking of the track piece. */
enum TrackPieceBanking {
	TPB_NONE,  ///< Track piece does not bank.
	TPB_LEFT,  ///< Track piece banks to the left.
	TPB_RIGHT, ///< Track piece banks to the right.

	TPB_COUNT, ///< End of the banking values.

	TPB_INVALID = 0xFF, ///< Invalid banking value.
};

/** Available bends in the tracks. */
enum TrackBend {
	TBN_LEFT_WIDE,    ///< Wide bend to the left.
	TBN_LEFT_NORMAL,  ///< Normal bend to the left.
	TBN_LEFT_TIGHT,   ///< Tight bend to the left.
	TBN_STRAIGHT,     ///< No bend either way.
	TBN_RIGHT_TIGHT,  ///< Tight bend to the right.
	TBN_RIGHT_NORMAL, ///< Normal bend to the right.
	TBN_RIGHT_WIDE,   ///< Wide bend to the right.

	TBN_COUNT,        ///< Number of bend types.

	TBN_INVALID = 0xFF, ///< Invalid bend value.
};

/** Base class describing a car curve at a track piece. */
class TrackCurve {
public:
	TrackCurve();
	virtual ~TrackCurve();

	virtual double GetValue(uint32 distance) const = 0;
};

/** Car curve that is the same at every position. */
class ConstantTrackCurve : public TrackCurve {
public:
	ConstantTrackCurve(int value);

	double GetValue(uint32 distance) const override
	{
		return this->value;
	}

	const int value; ///< Value of the curve at every position.
};

/** Description of a cubic Bezier spline. */
class CubicBezier {
public:
	CubicBezier(uint32 start, uint32 last, int a, int b, int c, int d);

	/**
	 * Get the value of the curve at the provided \a distance.
	 * @param distance Distance of the car at the curve, in 1/256 pixel.
	 * @return Value of this track curve variable at the given distance.
	 * @pre \a distance must be at or between #start and #last.
	 */
	double GetValue(uint32 distance) const
	{
		assert(distance >= this->start && distance <= this->last);
		double t = (double)(distance - this->start) / (this->last - this->start); // XXX Perhaps store the length?
		double tt = t * t;
		double t1 = 1 - t;
		double tt11 = t1 * t1;

		return (tt11 * t1) * this->a + (3 * tt11 * t) * this->b + (3 * t1 * tt) * this->c + (tt * t) * this->d;
	}

	const uint32 start; ///< Start distance of this curve in the track piece, in 1/256 pixel.
	const uint32 last;  ///< Last distance of this curve in the track piece, in 1/256 pixel.
	const int a;        ///< Starting value of the Bezier spline.
	const int b;        ///< First control point of the Bezier spline.
	const int c;        ///< Second intermediate control point of the Bezier spline.
	const int d;        ///< Ending value of the Bezier spline.
};

/** Track curve of a car described with a Cubic bezier spline. */
class BezierTrackCurve : public TrackCurve {
public:
	BezierTrackCurve();

	double GetValue(uint32 distance) const override
	{
		int end = this->curve.size();
		if (end == 1) return this->curve.front().GetValue(distance); // Handle a common case quickly.

		/* Bisection to the right Bezier spline. */
		int first = 0;
		while (end > first + 1) {
			int middle = (first + end) / 2;
			const CubicBezier &bezier = this->curve[middle];
			if (bezier.start > distance) {
				end = middle;
			} else {
				first = middle;
			}
		}
		return this->curve[first].GetValue(distance);
	}

	std::vector<CubicBezier> curve; ///< Curve describing the track piece.
};

/** One track piece (type) of a roller coaster track. */
class TrackPiece {
public:
	TrackPiece();
	~TrackPiece();

	bool Load(RcdFileReader *rcd_file, const ImageMap &sprites);
	Rectangle16 GetArea() const;

	uint8 entry_connect;      ///< Entry connection code
	uint8 exit_connect;       ///< Exit connection code
	XYZPoint16 exit_dxyz;     ///< Relative position of the exit voxel.
	int8 speed;               ///< If non-zero, the minimal speed of cars at the track.
	uint16 track_flags;       ///< Flags of the track piece.
	Money cost;               ///< Cost of this track piece.
	std::vector<TrackVoxel *> track_voxels; ///< Track voxels of this piece.

	uint32 piece_length;      ///< Length of the track piece for the cars, in 1/256 pixel.
	TrackCurve *car_xpos;     ///< X position of cars over this track piece.
	TrackCurve *car_ypos;     ///< Y position of cars over this track piece.
	TrackCurve *car_zpos;     ///< Z position of cars over this track piece.
	TrackCurve *car_pitch;    ///< Pitch of cars over this track piece, may be \c nullptr.
	TrackCurve *car_roll;     ///< Roll of cars over this track piece.
	TrackCurve *car_yaw;      ///< Yaw of cars over this track piece, may be \c null.

	/**
	 * Check whether the track piece is powered.
	 * @return Whether the track piece enforces a non-zero speed.
	 */
	inline bool HasPower() const
	{
		return this->speed != 0;
	}

	/**
	 * Check whether the track piece has a platform associated with it.
	 * @return Whether a platform is available.
	 */
	inline bool HasPlatform() const
	{
		return std::any_of(this->track_voxels.cbegin(), this->track_voxels.cend(), [](TrackVoxel *tv){ return tv->HasPlatform(); });
	}

	/**
	 * Can the track piece be used as the first piece of a roller coaster?
	 * @return Whether the track piece can be used as the first piece of a roller coaster.
	 */
	inline bool IsStartingPiece() const
	{
		return GB(this->track_flags, 3, 1) != 0;
	}

	/**
	 * Get the direction of the initial track piece. Should be used to match with the build arrow direction.
	 * @return Direction of the initial track piece.
	 * @pre #IsStartingPiece() holds.
	 */
	inline TileEdge GetStartDirection() const
	{
		return (TileEdge)GB(this->track_flags, 4, 2);
	}

	/**
	 * Get banking of the track piece.
	 * @return Banking of the track piece.
	 */
	inline TrackPieceBanking GetBanking() const
	{
		uint8 banking = GB(this->track_flags, 6, 2);
		assert(banking < TPB_COUNT);
		return (TrackPieceBanking)banking;
	}

	/**
	 * Get the slope of the track piece.
	 * @return the bend of the track piece.
	 */
	inline TrackSlope GetSlope() const
	{
		int slope = GB(this->track_flags, 8, 3);
		if ((slope & 4) != 0) slope |= ~7;
		switch (slope + 3) {
			case -3 + 3: return TSL_STRAIGHT_DOWN;
			case -2 + 3: return TSL_STEEP_DOWN;
			case -1 + 3: return TSL_DOWN;
			case 0 + 3:  return TSL_FLAT;
			case 1 + 3:  return TSL_UP;
			case 2 + 3:  return TSL_STEEP_UP;
			case 3 + 3:  return TSL_STRAIGHT_UP;
			default: return TSL_FLAT;
		}
	}

	/**
	 * Get the bend of the track piece.
	 * @return the bend of the track piece.
	 */
	inline TrackBend GetBend() const
	{
		int bend = GB(this->track_flags, 11, 3);
		if ((bend & 4) != 0) bend |= ~7;
		return (TrackBend)(bend + 3);
	}
};

/** Shared pointer to a const #TrackPiece. */
typedef std::shared_ptr<const TrackPiece> ConstTrackPiecePtr;

/**
 * Track piece with a position. Used in roller coasters to define their path in the world.
 * @note The #piece value is owned by the coaster type, do not free it.
 */
class PositionedTrackPiece {
public:
	PositionedTrackPiece() = default;
	PositionedTrackPiece(const XYZPoint16 &vox_pos, ConstTrackPiecePtr piece);

	bool IsOnWorld() const;
	bool CanBePlaced() const;
	bool CanBeSuccessor(const XYZPoint16 &vox, uint8 connect) const;
	bool CanBeSuccessor(const PositionedTrackPiece &pred) const;

	/**
	 * Get the position of the exit voxel.
	 * @return The position of the exit voxel.
	 */
	inline XYZPoint16 GetEndXYZ() const
	{
		return this->base_voxel + this->piece->exit_dxyz;
	}

	XYZPoint16 base_voxel; ///< Position (in voxels) of the entry point of the track piece.
	uint32 distance_base; ///< Base distance of this track piece in its roller coaster.

	ConstTrackPiecePtr piece; ///< Track piece placed at the given position, may be \c nullptr.
};

#endif
