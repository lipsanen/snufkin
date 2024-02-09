#pragma once

#include "mathlib/vector.h"
#include "mathlib/mathlib.h"

namespace utils
{
	const double M_RAD2DEG = 180 / M_PI;
	const double M_DEG2RAD = M_PI / 180;

	double NormalizeRad(double a);
	double NormalizeDeg(double a);
	float RandomFloat(float min, float max);
	void NormalizeQAngle(QAngle& angle);
	void GetMiddlePoint(const QAngle& angle1, const QAngle& angle2, QAngle& out);
}; // namespace utils
