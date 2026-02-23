#include "PlayerAttributeSet.h"

UPlayerAttributeSet::UPlayerAttributeSet()
{
	Health = 100.0f;
	HealthMax = 100.0f;
	Mana = 100.0f;
	ManaMax = 100.0f;
	AttackSpeed = 1.2f;
}
void UPlayerAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}