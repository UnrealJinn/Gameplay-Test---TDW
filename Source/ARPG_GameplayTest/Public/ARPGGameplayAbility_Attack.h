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
        const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,bool bReplicateEndAbility,bool bWasCancelled) override;

protected:
    // damage
    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    TSubclassOf<class UGameplayEffect> DamageEffectClass;

    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    FGameplayTag DamageDataTag;

    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    FGameplayTag GameplayCueTag;

    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    float DamageMagnitude = -25.f;

    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    float AttackRadius = 500.f;

    // movement
    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    float LaunchSpeed = 1000.f;

    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    float LaunchZForce = 400.f;

    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    float DamageDelay = 0.35f;

private:
    void PerformAttackOverlap();
    void RestoreMovement();

    UPROPERTY()
    AActor* TargetActor = nullptr;
    FVector TargetLoc_Cursor = FVector::ZeroVector;

    TArray<AActor*> ActorsToIgnore;

    FGameplayAbilitySpecHandle CachedHandle;
    const FGameplayAbilityActorInfo* CachedActorInfo = nullptr;
    FGameplayAbilityActivationInfo CachedActivationInfo;

    FTimerHandle DamageTimerHandle;
    FTimerHandle EndTimerHandle;
};