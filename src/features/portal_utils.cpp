#include "feature.hpp"
#include "portal_utils.hpp"

#include "cdll_int.h"
#include "client_class.h"
#include "edict.h"
#include "eiface.h"
#include "engine/iserverplugin.h"
#include "mathlib/vmatrix.h"
#include "../features/ent_props.hpp"
#include "utils/game_detection.hpp"
#include "const.h"
#include "utils/interfaces.hpp"
#include "icliententity.h"
#include "portal_utils.hpp"

PortalUtilsFeature feat_portal_utils;

bool PortalUtilsFeature::ShouldLoadFeature() {
	return utils::DoesGameLookLikePortal();
}

void PortalUtilsFeature::InitHooks() {}

void PortalUtilsFeature::LoadFeature() {
	m_hPortalEnvironment = feat_entprops.GetPlayerField<int>("m_hPortalEnvironment");
}

void PortalUtilsFeature::UnloadFeature() {}

static IClientEntity* prevPortal = nullptr;
static IClientEntity* prevLinkedPortal = nullptr;

IClientEntity* FindLinkedPortal(IClientEntity* ent)
{
	int ehandle = feat_entprops.GetProperty<int>(ent->entindex(), "m_hLinkedPortal");
	int index = ehandle & ENT_ENTRY_MASK;

	// Backup linking thing for fizzled portals(not sure if you can actually properly figure it out using client portals only)
	if (index == ENT_ENTRY_MASK && prevPortal == ent && prevLinkedPortal)
	{
		index = prevLinkedPortal->entindex();
	}

	if (index != ENT_ENTRY_MASK)
	{
		IClientEntity* portal = feat_entprops.GetClientEntity(index - 1);
		prevLinkedPortal = portal;
		prevPortal = ent;

		return portal;
	}
	else
		return nullptr;
}

static bool PortalIsOrange(IClientEntity* ent)
{
	return feat_entprops.GetProperty<int>(ent->entindex() - 1, "m_bIsPortal2");
}

IClientEntity* GetEnvironmentPortal()
{
	if (!utils::DoesGameLookLikePortal())
		return nullptr;
	int handle = feat_portal_utils.m_hPortalEnvironment.GetValue();
	if (handle == -1)
		return nullptr;
	int index = (handle & ENT_ENTRY_MASK) - 1;
	return feat_entprops.GetClientEntity(index);
}

static bool invalidPortal(IClientEntity* portal)
{
	return !portal || strcmp(portal->GetClientClass()->GetName(), "CProp_Portal") != 0;
}

static IClientEntity* getPortal(const char* arg, bool verbose)
{
	int portal_index = atoi(arg);
	IClientEntity* portal = nullptr;
	bool want_blue = !strcmp(arg, "blue");
	bool want_orange = !strcmp(arg, "orange");
	bool want_auto = !strcmp(arg, "auto");

	if (want_auto)
	{
		return GetEnvironmentPortal();
	}

	if (want_blue || want_orange || want_auto)
	{
		std::vector<int> indices;

		for (int i = 0; i < MAX_EDICTS; ++i)
		{
			IClientEntity* ent = feat_entprops.GetClientEntity(i);

			if (!invalidPortal(ent))
			{
				bool is_orange_portal = PortalIsOrange(ent);

				if (is_orange_portal && want_orange)
				{
					indices.push_back(i);
				}
				else if (!is_orange_portal && want_blue)
				{
					indices.push_back(i);
				}
			}
		}

		if (indices.size() > 1)
		{
			if (verbose)
				Msg("There are multiple %s portals, please use the index:\n", arg);

			for (auto i : indices)
			{
				auto ent = feat_entprops.GetClientEntity(i);
				const auto& origin = ent->GetAbsOrigin();

				if (verbose)
					Msg("%d located at %.8f %.8f %.8f\n", i, origin.x, origin.y, origin.z);
			}
		}
		else if (indices.size() == 0)
		{
			if (verbose)
				Msg("There are no %s portals.\n", arg);
		}
		else
		{
			portal_index = indices[0];
			portal = feat_entprops.GetClientEntity(portal_index);
		}
	}
	else
	{
		portal = feat_entprops.GetClientEntity(portal_index);
	}

	return portal;
}

static bool getPortal(const char* portalStr, IClientEntity** portal_edict, Vector& new_player_origin, QAngle& new_player_angles)
{
	IClientEntity* portal = getPortal(portalStr, false);

	if (!portal)
	{
		new_player_origin = feat_entprops.GetPlayerEyePosition();
		new_player_angles = feat_entprops.GetPlayerEyeAngles();

		return false;
	}

	*portal_edict = portal;
	return true;
}

// From /game/shared/portal/prop_portal_shared.cpp
static void UpdatePortalTransformationMatrix(const matrix3x4_t& localToWorld,
	const matrix3x4_t& remoteToWorld,
	VMatrix& matrix)
{
	VMatrix matPortal1ToWorldInv, matPortal2ToWorld, matRotation;

	//inverse of this
	MatrixInverseTR(localToWorld, matPortal1ToWorldInv);

	//180 degree rotation about up
	matRotation.Identity();
	matRotation.m[0][0] = -1.0f;
	matRotation.m[1][1] = -1.0f;

	//final
	matPortal2ToWorld = remoteToWorld;
	matrix = matPortal2ToWorld * matRotation * matPortal1ToWorldInv;
}

static void UpdatePortalTransformationMatrix(IClientEntity* localPortal, IClientEntity* remotePortal, VMatrix& matrix)
{
	Vector localForward, localRight, localUp;
	Vector remoteForward, remoteRight, remoteUp;
	AngleVectors(localPortal->GetAbsAngles(), &localForward, &localRight, &localUp);
	AngleVectors(remotePortal->GetAbsAngles(), &remoteForward, &remoteRight, &remoteUp);

	matrix3x4_t localToWorld(localForward, -localRight, localUp, localPortal->GetAbsOrigin());
	matrix3x4_t remoteToWorld(remoteForward, -remoteRight, remoteUp, remotePortal->GetAbsOrigin());
	UpdatePortalTransformationMatrix(localToWorld, remoteToWorld, matrix);
}

bool isInfrontOfPortal(IClientEntity* saveglitch_portal, const Vector& player_origin)
{
	const auto& portal_origin = saveglitch_portal->GetAbsOrigin();
	const auto& portal_angles = saveglitch_portal->GetAbsAngles();

	Vector delta = player_origin - portal_origin;
	Vector normal;
	AngleVectors(portal_angles, &normal);
	float dot = DotProduct(normal, delta);

	return dot >= 0;
}

static void transformThroughPortal(IClientEntity* saveglitch_portal,
	const Vector& start_pos,
	const QAngle start_angles,
	Vector& transformed_origin,
	QAngle& transformed_angles)
{
	VMatrix matrix;
	matrix.Identity();

	if (isInfrontOfPortal(saveglitch_portal, start_pos))
	{
		transformed_origin = start_pos;
		transformed_angles = start_angles;
	}
	else
	{
		edict_t* e = interfaces::g_pEngine->PEntityOfEntIndex(saveglitch_portal->entindex());
		static int offset = feat_entprops.GetFieldOffset("CProp_Portal", "m_matrixThisToLinked", true);
		if (e && offset != -1)
		{
			uintptr_t serverEnt = reinterpret_cast<uintptr_t>(e->GetUnknown());
			matrix = *reinterpret_cast<VMatrix*>(serverEnt + offset);
		}
		else
		{
			IClientEntity* linkedPortal = FindLinkedPortal(saveglitch_portal);

			if (linkedPortal)
			{
				UpdatePortalTransformationMatrix(saveglitch_portal, linkedPortal, matrix);
			}
		}
	}

	auto eye_origin = start_pos;
	auto new_eye_origin = matrix * eye_origin;
	transformed_origin = new_eye_origin;

	transformed_angles = TransformAnglesToWorldSpace(start_angles, matrix.As3x4());
	transformed_angles.x = AngleNormalizePositive(transformed_angles.x);
	transformed_angles.y = AngleNormalizePositive(transformed_angles.y);
	transformed_angles.z = AngleNormalizePositive(transformed_angles.z);
}

static void calculateOffsetPlayer(IClientEntity* saveglitch_portal, Vector& new_player_origin, QAngle& new_player_angles)
{
	const auto& player_origin = feat_entprops.GetPlayerEyePosition();
	const auto& player_angles = feat_entprops.GetPlayerEyeAngles();
	transformThroughPortal(saveglitch_portal, player_origin, player_angles, new_player_origin, new_player_angles);
}

static void calculateAGOffsetPortal(IClientEntity* enter_portal,
	IClientEntity* exit_portal,
	Vector& new_player_origin,
	QAngle& new_player_angles)
{
	const auto enter_origin = enter_portal->GetAbsOrigin();
	const auto enter_angles = enter_portal->GetAbsAngles();
	const auto exit_origin = exit_portal->GetAbsOrigin();
	const auto exit_angles = exit_portal->GetAbsAngles();

	Vector exitForward, exitRight, exitUp;
	Vector enterForward, enterRight, enterUp;
	AngleVectors(enter_angles, &enterForward, &enterRight, &enterUp);
	AngleVectors(exit_angles, &exitForward, &exitRight, &exitUp);

	auto delta = enter_origin - exit_origin;

	Vector exit_portal_coords;
	exit_portal_coords.x = delta.Dot(exitForward);
	exit_portal_coords.y = delta.Dot(exitRight);
	exit_portal_coords.z = delta.Dot(exitUp);

	Vector transition(0, 0, 0);
	transition -= exit_portal_coords.x * enterForward;
	transition -= exit_portal_coords.y * enterRight;
	transition += exit_portal_coords.z * enterUp;

	auto new_eye_origin = enter_origin + transition;
	new_player_origin = new_eye_origin;
	new_player_angles = feat_entprops.GetPlayerEyeAngles();
}

void PortalUtilsFeature::calculateAGPosition(const char* portalStr, Vector& new_player_origin, QAngle& new_player_angles) {
	if (!utils::DoesGameLookLikePortal()) {
		return;
	}

	IClientEntity* enter_portal = NULL;
	IClientEntity* exit_portal = NULL;

	if (getPortal(portalStr, &enter_portal, new_player_origin, new_player_angles))
	{
		exit_portal = FindLinkedPortal(enter_portal);
		if (exit_portal)
		{
			calculateAGOffsetPortal(enter_portal, exit_portal, new_player_origin, new_player_angles);
		}
	}
}

void PortalUtilsFeature::calculateSGPosition(const char* portalStr, Vector& new_player_origin, QAngle& new_player_angles) {
	if (!utils::DoesGameLookLikePortal()) {
		return;
	}

	IClientEntity* portal;
	if (getPortal(portalStr, &portal, new_player_origin, new_player_angles))
		calculateOffsetPlayer(portal, new_player_origin, new_player_angles);
}
