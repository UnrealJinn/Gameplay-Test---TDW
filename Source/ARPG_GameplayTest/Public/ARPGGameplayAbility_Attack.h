
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ARPGGameplayAbility_Attack.generated.h"

UCLASS()
class ARPG_GAMEPLAYTEST_API UARPGGameplayAbility_Attack : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UARPGGameplayAbility_Attack();

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled) override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Combat")
    TSubclassOf<class UGameplayEffect> DamageEffectClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Combat")
    float AttackRadius = 500.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Combat")
    float DamageMagnitude = -25.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Movement")
    float LaunchSpeed = 1200.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Movement")
    float LaunchZForce = 400.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Movement")
    float DamageDelay = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Combat")
    FGameplayTag GameplayCueTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Combat")
    FGameplayTag DamageDataTag;

private:
    void PerformAttackOverlap();
    void RestoreMovement();

    UPROPERTY()
    AActor* TargetActor = nullptr;

    TArray<AActor*> ActorsToIgnore;

    FGameplayAbilitySpecHandle CachedHandle;
    const FGameplayAbilityActorInfo* CachedActorInfo = nullptr;
    FGameplayAbilityActivationInfo CachedActivationInfo;

    FTimerHandle DamageTimerHandle;
    FTimerHandle EndTimerHandle;
    FVector TargetLoc_Cursor = FVector::ZeroVector;
};