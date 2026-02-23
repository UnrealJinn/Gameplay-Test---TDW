#include "PlayerAttributeSet.h"

UPlayerAttributeSet::UPlayerAttributeSet()
{
	Health    = 100.f;
	HealthMax = 100.f;
	Mana      = 100.f;
	ManaMax   = 100.f;
	AttackSpeed = 1.2f;
}

void UPlayerAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}