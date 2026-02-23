#include "ARPGGameplayAbility_Attack.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ARPGCharacter.h"
#include "DrawDebugHelpers.h"

UARPGGameplayAbility_Attack::UARPGGameplayAbility_Attack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

void UARPGGameplayAbility_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	CachedHandle         = Handle;
	CachedActorInfo      = ActorInfo;
	CachedActivationInfo = ActivationInfo;

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Attack] not enough mana"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	ACharacter* Character = Cast<ACharacter>(AvatarActor);
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ActorsToIgnore.Empty();
	ActorsToIgnore.AddUnique(AvatarActor);

	// figure out where to jump
	TargetActor = nullptr;
	TargetLoc_Cursor = FVector::ZeroVector;

	if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
	{
		FHitResult Hit;
		PC->GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), true, Hit);

		if (Hit.bBlockingHit && IsValid(Hit.GetActor()) &&
			Hit.GetActor() != AvatarActor &&
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Hit.GetActor()))
		{
			// clicked an enemy
			TargetActor = Hit.GetActor();
		}
		else if (Hit.bBlockingHit)
		{
			// clicked ground, move to that position
			TargetLoc_Cursor = Hit.Location;
		}
	}

	const FVector StartLoc = AvatarActor->GetActorLocation();
	const FVector TargetLoc = TargetActor
		? TargetActor->GetActorLocation()
		: (!TargetLoc_Cursor.IsZero() ? TargetLoc_Cursor : StartLoc + AvatarActor->GetActorForwardVector() * 500.f);

	// flatten direction, add Z separately so arc feels consistent
	FVector LaunchDir = (TargetLoc - StartLoc);
	LaunchDir.Z = 0.f;
	LaunchDir = LaunchDir.GetSafeNormal();

	FVector LaunchVelocity = LaunchDir * LaunchSpeed;
	LaunchVelocity.Z = LaunchZForce;

	// face target before jumping
	FRotator LookAt = UKismetMathLibrary::FindLookAtRotation(StartLoc, TargetLoc);
	LookAt.Pitch = LookAt.Roll = 0.f;
	Character->SetActorRotation(LookAt);

	Character->LaunchCharacter(LaunchVelocity, true, true);

	// debug
	DrawDebugLine(GetWorld(), StartLoc, TargetLoc, FColor::Yellow, false, 3.f, 0, 3.f);
	DrawDebugSphere(GetWorld(), StartLoc, 40.f, 12, FColor::Green, false, 3.f);
	DrawDebugSphere(GetWorld(), TargetLoc, 40.f, 12, FColor::Red, false, 3.f);

	// apply damage after delay
	GetWorld()->GetTimerManager().SetTimer(DamageTimerHandle, [this]()
	{
		if (!CachedActorInfo) return;

		AActor* Avatar = CachedActorInfo->AvatarActor.Get();
		if (!Avatar) return;

		if (UAbilitySystemComponent* ASC = CachedActorInfo->AbilitySystemComponent.Get())
		{
			FGameplayCueParameters CueParams;
			CueParams.Instigator = CueParams.EffectCauser = Avatar;
			ASC->ExecuteGameplayCue(GameplayCueTag, CueParams);
		}

		PerformAttackOverlap();

	}, DamageDelay, false);

	// end ability shortly after damage
	GetWorld()->GetTimerManager().SetTimer(EndTimerHandle, [this]()
	{
		RestoreMovement();
		CommitAbilityCooldown(CachedHandle, CachedActorInfo, CachedActivationInfo, true);
		EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, false);

	}, DamageDelay + 0.3f, false);
}

void UARPGGameplayAbility_Attack::PerformAttackOverlap()
{
	AActor* AvatarActor = CachedActorInfo ? CachedActorInfo->AvatarActor.Get() : nullptr;
	if (!AvatarActor) return;

	UAbilitySystemComponent* SourceASC = CachedActorInfo->AbilitySystemComponent.Get();
	if (!SourceASC || !DamageEffectClass) return;

	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	UKismetSystemLibrary::SphereOverlapActors(AvatarActor,AvatarActor->GetActorLocation(),AttackRadius,ObjectTypes,AARPGCharacter::StaticClass(),ActorsToIgnore,OverlappedActors);

	DrawDebugSphere(GetWorld(), AvatarActor->GetActorLocation(),AttackRadius, 24, OverlappedActors.Num() > 0 ? FColor::Red : FColor::Blue, false, 3.f, 0, 2.f);

	for (AActor* HitActor : OverlappedActors)
	{
		if (!IsValid(HitActor)) continue;

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
		if (!TargetASC) continue;

		FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
		Ctx.AddInstigator(AvatarActor, AvatarActor);

		FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.f, Ctx);
		if (!Spec.IsValid()) continue;

		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(Spec, DamageDataTag, DamageMagnitude);
		TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

		UE_LOG(LogTemp, Warning, TEXT("[Attack] hit %s for %.1f"), *HitActor->GetName(), DamageMagnitude);

		DrawDebugSphere(GetWorld(), HitActor->GetActorLocation(), 60.f, 12, FColor::Red, false, 3.f);
		DrawDebugLine(GetWorld(), AvatarActor->GetActorLocation(), HitActor->GetActorLocation(), FColor::Red, false, 3.f);
	}
}

void UARPGGameplayAbility_Attack::RestoreMovement()
{
	if (!CachedActorInfo) return;

	ACharacter* Character = Cast<ACharacter>(CachedActorInfo->AvatarActor.Get());
	if (!Character) return;

	UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
	if (!MoveComp) return;

	MoveComp->CurrentRootMotion.Clear();
	MoveComp->StopActiveMovement();
	MoveComp->SetMovementMode(MOVE_Walking);
}

void UARPGGameplayAbility_Attack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	GetWorld()->GetTimerManager().ClearTimer(DamageTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(EndTimerHandle);

	RestoreMovement();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}