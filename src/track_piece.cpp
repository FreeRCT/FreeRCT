/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file track_piece.cpp Functions of the track pieces. */

#include "stdafx.h"
#include "sprite_store.h"
#include "fileio.h"
#include "gamecontrol.h"
#include "track_piece.h"
#include "finances.h"
#include "map.h"
#include "coaster.h"

TrackVoxel::TrackVoxel() : id(0), bg(nullptr), fg(nullptr)
{
}

/**
 * Load a track voxel.
 * @param rcd_file Data file being loaded.
 * @param length Length of the voxel (according to the file).
 */
void TrackVoxel::Load(RcdFileReader *rcd_file, size_t length)
{
	rcd_file->CheckExactLength(length, 1 + 1 + 1 + 1 + 2 * 4, "track voxel");
	this->bg = _sprite_manager.GetFrameSet(ImageSetKey(rcd_file->filename, rcd_file->GetUInt32()));
	this->fg = _sprite_manager.GetFrameSet(ImageSetKey(rcd_file->filename, rcd_file->GetUInt32()));
	this->dxyz.x = rcd_file->GetInt8();
	this->dxyz.y = rcd_file->GetInt8();
	this->dxyz.z = rcd_file->GetInt8();
	this->flags = rcd_file->GetUInt8();

	static uint32 _last_id(0);
	this->id = ++_last_id;
}

TrackCurve::TrackCurve()
= default;

TrackCurve::~TrackCurve()
= default;

/**
 * \fn double TrackCurve::GetValue(int distance)
 * Get the value of the curve at the provided \a distance.
 * @param distance Distance of the car at the curve, in 1/256 pixel.
 * @return Value of this track curve variable at the given distance.
 */

/**
 * Track curve that always has the same value.
 * @param value Constant value of the curve.
 */
ConstantTrackCurve::ConstantTrackCurve(int value) : value(value)
{
}

/**
 * Partial track curve described by a cubic Bezier spline.
 * @param start Start distance of this curve in the track piece, in 1/256 pixel.
 * @param last Last distance of this curve in the track piece, in 1/256 pixel.
 * @param a Starting value of the Bezier spline.
 * @param b First control point of the Bezier spline.
 * @param c Second intermediate control point of the Bezier spline.
 * @param d Ending value of the Bezier spline.
 */
CubicBezier::CubicBezier(uint32 start, uint32 last, int a, int b, int c, int d) : start(start), last(last), a(a), b(b), c(c), d(d)
{
}

BezierTrackCurve::BezierTrackCurve()
= default;

/**
 * Load the data of a Bezier spline into a #BezierTrackCurve.
 * @param rcd_file Data file to load from. Caller must ensure there is enough data available at the stream.
 * @return The Loaded Bezier spline.
 */
static CubicBezier LoadBezier(RcdFileReader *rcd_file)
{
	uint32 start = rcd_file->GetUInt32();
	uint32 last = rcd_file->GetUInt32();
	int16 a = rcd_file->GetInt16();
	int16 b = rcd_file->GetInt16();
	int16 c = rcd_file->GetInt16();
	int16 d = rcd_file->GetInt16();
	return CubicBezier(start, last, a, b, c, d);
}

/**
 * \fn ENSURE_LENGTH(x)
 * Make sure the RCD file being loaded has at least \a x bytes of data left.
 * @param x Minimal number of bytes required.
 */

/**
 * Load a track curve.
 * @param rcd_file Data file being loaded.
 * @param curve [out] The loaded track curve, may be \c nullptr (which indicates a not supplied track curve).
 * @param length [inout] Length of the block that is not loaded yet.
 */
static void LoadTrackCurve(RcdFileReader *rcd_file, std::unique_ptr<TrackCurve> *curve, int *length)
{
#define ENSURE_LENGTH(x) do { *length -= (x); rcd_file->CheckMinLength(*length, 0, "curve"); } while(false)

	ENSURE_LENGTH(1);
	uint8 type = rcd_file->GetUInt8();
	switch (type) {
		case 0: // No track curve available.
			*curve = nullptr;
			return;

		case 1: { // Curve consisting of a fixed value.
			ENSURE_LENGTH(2);
			int value = rcd_file->GetInt16();
			curve->reset(new ConstantTrackCurve(value));
			return;
		}

		case 2: {
			ENSURE_LENGTH(1);
			int count = rcd_file->GetUInt8();
			ENSURE_LENGTH(count * 16u);
			BezierTrackCurve *bezier = new BezierTrackCurve();
			bezier->curve.reserve(count);
			for (; count > 0; count--) {
				bezier->curve.push_back(LoadBezier(rcd_file));
			}
			bezier->curve.shrink_to_fit();
			curve->reset(bezier);
			return;
		}

		default: // Error.
			*curve = nullptr;
			rcd_file->Error("Unexpected curve type %u.", type);
	}
#undef ENSURE_LENGTH
}

TrackPiece::TrackPiece()
{
	this->piece_length = 0;
	this->car_xpos = nullptr;
	this->car_ypos = nullptr;
	this->car_zpos = nullptr;
	this->car_pitch = nullptr;
	this->car_roll = nullptr;
	this->car_yaw = nullptr;
}

/**
 * Immediately remove a piece of this type from all voxels it occupies.
 * @param ride_index The index of the coaster that owns the piece.
 * @param base_voxel the piece's absolute coordinates.
 */
void TrackPiece::RemoveFromWorld([[maybe_unused]] const uint16 ride_index, const XYZPoint16 base_voxel) const {
	for (const auto &subpiece : this->track_voxels) {
		Voxel *voxel = _world.GetCreateVoxel(base_voxel + subpiece->dxyz, false);
		assert(voxel);
		if (voxel->instance != SRI_FREE) {
			assert(voxel->instance == ride_index);
			voxel->ClearInstances();
		}
	}
}

/**
 * Load a track piece.
 * @param rcd_file Data file being loaded.
 */
void TrackPiece::Load(RcdFileReader *rcd_file)
{
	rcd_file->CheckVersion(7);
	int length = rcd_file->size;
	length -= 2 + 3 + 1 + 2 + 4 + 2 + 1;
	rcd_file->CheckMinLength(length, 0, "header");

	this->entry_connect = rcd_file->GetUInt8();
	this->exit_connect  = rcd_file->GetUInt8();
	this->exit_dxyz.x = rcd_file->GetInt8();
	this->exit_dxyz.y = rcd_file->GetInt8();
	this->exit_dxyz.z = rcd_file->GetInt8();
	this->speed   = rcd_file->GetInt8();
	this->track_flags = rcd_file->GetUInt16();
	this->cost = rcd_file->GetUInt32();
	uint16 voxel_count = rcd_file->GetUInt16();

	length -= 12 * voxel_count;
	rcd_file->CheckMinLength(length, 0, "voxels");

	for (uint16 i = 0; i < voxel_count; i++) {
		this->track_voxels.emplace_back(new TrackVoxel);
		this->track_voxels.back()->Load(rcd_file, 12);
	}
	length -= 4;
	rcd_file->CheckMinLength(length, 0, "pieces");
	this->piece_length = rcd_file->GetUInt32();

	LoadTrackCurve(rcd_file, &this->car_xpos,  &length);
	LoadTrackCurve(rcd_file, &this->car_ypos,  &length);
	LoadTrackCurve(rcd_file, &this->car_zpos,  &length);
	LoadTrackCurve(rcd_file, &this->car_pitch, &length);
	LoadTrackCurve(rcd_file, &this->car_roll,  &length);
	LoadTrackCurve(rcd_file, &this->car_yaw,   &length);
	if (this->car_xpos == nullptr || this->car_ypos == nullptr || this->car_zpos == nullptr || this->car_roll == nullptr) {
		rcd_file->Error("Car sprites missing");
	}

	this->internal_name = rcd_file->GetText();
	length -= this->internal_name.size();

	rcd_file->CheckExactLength(length, 0, "end of block");
}

/**
 * Get the area covered by a piece at flat ground.
 * @return Smallest rectangle surrounding all parts of the trackpiece at flat ground.
 * @note Base position may be negative.
 */
Rectangle16 TrackPiece::GetArea() const
{
	Rectangle16 rect;
	rect.AddPoint(0, 0);
	for (const auto &tv : this->track_voxels) {
		rect.AddPoint(tv->dxyz.x, tv->dxyz.y);
	}
	return rect;
}

/**
 * Constructor taking values for all its fields.
 * @param vox_pos Position of the positioned track piece.
 * @param piece Track piece to use.
 */
PositionedTrackPiece::PositionedTrackPiece(const XYZPoint16 &vox_pos, ConstTrackPiecePtr piece)
{
	this->base_voxel = vox_pos;
	this->piece = piece;
}

/** Monthly depreciation of the piece's value. */
void PositionedTrackPiece::OnNewMonth()
{
	this->return_cost = this->return_cost * (10000 - RIDE_DEPRECIATION) / 10000;
}

/**
 * Verify that all voxels of this track piece are within world boundaries.
 * @return The positioned track piece is entirely within the world boundaries.
 * @pre Positioned track piece must have a piece.
 */
bool PositionedTrackPiece::IsOnWorld() const
{
	assert(this->piece != nullptr);
	if (!IsVoxelInsideWorld(this->base_voxel)) return false;
	if (!IsVoxelInsideWorld(this->GetEndXYZ())) return false;
	XYZPoint16 base = this->base_voxel;
	return std::all_of(this->piece->track_voxels.begin(), this->piece->track_voxels.end(),
	                   [base](const std::unique_ptr<TrackVoxel> &tv){ return IsVoxelInsideWorld(base + tv->dxyz); });
}

/**
 * Can this positioned track piece be placed in the world?
 * @return \c STR_NULL if the item can be placed here; otherwise the reason why it can't.
 * @pre Positioned track piece must have a piece.
 */
StringID PositionedTrackPiece::CanBePlaced() const
{
	if (!this->IsOnWorld()) return GUI_ERROR_MESSAGE_BAD_LOCATION;
	for (const auto &tvx : this->piece->track_voxels) {
		/* Is the voxel above ground level? */
		const XYZPoint16 part_pos = this->base_voxel + tvx->dxyz;
		if (_world.GetBaseGroundHeight(part_pos.x, part_pos.y) > part_pos.z) return GUI_ERROR_MESSAGE_UNDERGROUND;
		const Voxel *vx = _world.GetVoxel(part_pos);
		if (vx != nullptr && !vx->CanPlaceInstance()) return GUI_ERROR_MESSAGE_OCCUPIED;
		if (_game_mode_mgr.InPlayMode() && _world.GetTileOwner(part_pos.x, part_pos.y) != OWN_PARK) return GUI_ERROR_MESSAGE_UNOWNED_LAND;
	}
	return STR_NULL;
}

/**
 * Can this positioned track piece function as a successor for the given exit conditions?
 * @param vox Required coordinate position.
 * @param connect Required entry connection.
 * @return This positioned track piece can be used as successor.
 */
bool PositionedTrackPiece::CanBeSuccessor(const XYZPoint16 &vox, uint8 connect) const
{
	return this->piece != nullptr && this->base_voxel == vox && this->piece->entry_connect == connect;
}

/**
 * Can this positioned track piece function as a successor of piece \a pred?
 * @param pred Previous positioned track piece.
 * @return This positioned track piece can be used as successor.
 */
bool PositionedTrackPiece::CanBeSuccessor(const PositionedTrackPiece &pred) const
{
	if (pred.piece == nullptr) return false;
	return this->CanBeSuccessor(pred.GetEndXYZ(), pred.piece->exit_connect);
}

/**
 * Immediately remove this piece from all voxels it occupies.
 * @param ride_index The index of the coaster that owns this piece.
 */
void PositionedTrackPiece::RemoveFromWorld(const uint16 ride_index) {
	this->piece->RemoveFromWorld(ride_index, base_voxel);
}

static const uint32 CURRENT_VERSION_PositionedTrackPiece = 2;   ///< Currently supported version of %PositionedTrackPiece.

void PositionedTrackPiece::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("pstp");
	if (version < 1 || version > CURRENT_VERSION_PositionedTrackPiece) ldr.VersionMismatch(version, CURRENT_VERSION_PositionedTrackPiece);

	uint16 x = ldr.GetWord();
	uint16 y = ldr.GetWord();
	uint16 z = ldr.GetWord();

	this->base_voxel = XYZPoint16(x, y, z);
	this->distance_base = ldr.GetLong();
	this->return_cost = (version < 2) ? 0 : ldr.GetLongLong();
	ldr.ClosePattern();
}

void PositionedTrackPiece::Save(Saver &svr)
{
	svr.StartPattern("pstp", CURRENT_VERSION_PositionedTrackPiece);
	svr.PutWord(this->base_voxel.x);
	svr.PutWord(this->base_voxel.y);
	svr.PutWord(this->base_voxel.z);
	svr.PutLong(this->distance_base);
	svr.PutLongLong(this->return_cost);
	svr.EndPattern();
}

constexpr uint32 CURRENT_VERSION_TrackedRideDesign = 2;   ///< Currently supported version of %TrackedRideDesign.

/**
 * Load a tracked ride design from a file.
 * @param ldr Loader to read from.
 */
TrackedRideDesign::TrackedRideDesign(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("FTKD");
	if (version < 1 || version > CURRENT_VERSION_TrackedRideDesign) ldr.VersionMismatch(version, CURRENT_VERSION_TrackedRideDesign);

	this->ride = ldr.GetText();
	const CoasterType *ct = static_cast<const CoasterType*>(_rides_manager.GetRideType(this->ride));

	this->name = ldr.GetText();
	this->excitement_rating = ldr.GetLong();
	this->intensity_rating  = ldr.GetLong();
	this->nausea_rating     = ldr.GetLong();

	uint32 nr_pieces = ldr.GetLong();
	this->pieces.reserve(nr_pieces);
	for (; nr_pieces > 0; --nr_pieces) {
		ldr.OpenPattern("trpc", false, true);

		std::string track_piece_name;
		if (version >= 2) {
			track_piece_name = ldr.GetText();
		} else {
			track_piece_name = ct->pieces.at(ldr.GetLong())->internal_name;
		}

		XYZPoint16 p;
		p.x = ldr.GetWord();
		p.y = ldr.GetWord();
		p.z = ldr.GetWord();

		ldr.ClosePattern();
		this->pieces.emplace_back(track_piece_name, p);
	}

	ldr.ClosePattern();
}

/**
 * Save this tracked ride design to the disk.
 * @param svr Stream to write to.
 */
void TrackedRideDesign::Save(Saver &svr) const
{
	svr.StartPattern("FTKD", CURRENT_VERSION_TrackedRideDesign);

	svr.PutText(this->ride);
	svr.PutText(this->name);
	svr.PutLong(this->excitement_rating);
	svr.PutLong(this->intensity_rating);
	svr.PutLong(this->nausea_rating);

	svr.PutLong(this->pieces.size());
	for (const AbstractTrackPiece &p : this->pieces) {
		svr.StartPattern("trpc");
		svr.PutText(p.piece_name);
		svr.PutWord(p.base_voxel.x);
		svr.PutWord(p.base_voxel.y);
		svr.PutWord(p.base_voxel.z);
		svr.EndPattern();
	}

	svr.EndPattern();
}
