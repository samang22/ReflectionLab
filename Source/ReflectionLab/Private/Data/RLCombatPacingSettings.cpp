#include "Data/RLCombatPacingSettings.h"

float FRLCombatPacingSettings::GetAttackRate(int32 RemainingEnemies) const
{
	const int32 Threshold = FMath::Clamp(RemainingEnemyThreshold, 1, 10);
	if (!bEnabled || RemainingEnemies <= 0 || RemainingEnemies > Threshold) { return 1; }
	const float First = FMath::IsFinite(ThresholdAttackRate) ? FMath::Clamp(ThresholdAttackRate, 1.0f, 4.0f) : 1;
	const float Last = FMath::IsFinite(LastEnemyAttackRate) ? FMath::Clamp(LastEnemyAttackRate, First, 4.0f) : First;
	const float Alpha = Threshold > 1 ? static_cast<float>(Threshold - RemainingEnemies) / (Threshold - 1) : 1;
	return FMath::Lerp(First, Last, Alpha);
}

bool FRLCombatPacingSettings::IsApproaching(const FVector& ShotLocation, const FVector& Velocity, const FVector& PlayerLocation) const
{
	const FVector2D Offset(PlayerLocation - ShotLocation);
	const FVector2D Motion(Velocity);
	if (Offset.ContainsNaN() || Motion.ContainsNaN()) { return false; }
	const double Radius = FMath::IsFinite(ThreatPassDistance) ? FMath::Clamp(ThreatPassDistance, 50.0f, 600.0f) : 250;
	if (Motion.SizeSquared() < 1) { return Offset.SizeSquared() <= FMath::Square(Radius); }
	const double Arrival = FVector2D::DotProduct(Offset, Motion) / Motion.SizeSquared();
	const double Horizon = FMath::IsFinite(ThreatArrivalHorizon) ? FMath::Clamp(ThreatArrivalHorizon, 0.5f, 10.0f) : 4;
	return Arrival >= 0 && Arrival <= Horizon && (Offset - Motion * Arrival).SizeSquared() <= FMath::Square(Radius);
}
