
#include "ARPGGameplayAbility_Attack.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "ARPGCharacter.h"
#include "DrawDebugHelpers.h"

UARPGGameplayAbility_Attack::UARPGGameplayAbility_Attack()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    NetSecurityPolicy  = EGameplayAbilityNetSecurityPolicy::ClientOrServer;
}
void UARPGGameplayAbility_Attack::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    CachedHandle         = Handle;
    CachedActorInfo      = ActorInfo;
    CachedActivationInfo = ActivationInfo;

    UE_LOG(LogTemp, Warning, TEXT("[Attack] ====== ActivateAbility ======"));
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Yellow,
        TEXT("[Attack] ====== ActivateAbility ======"));

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        UE_LOG(LogTemp, Error, TEXT("[Attack] CommitAbility FAILED"));
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
            TEXT("[Attack] CommitAbility FAILED"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    AActor* AvatarActor = ActorInfo->AvatarActor.Get();
    ACharacter* Character = Cast<ACharacter>(AvatarActor);
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("[Attack] Avatar is not ACharacter"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    ActorsToIgnore.Empty();
    ActorsToIgnore.AddUnique(AvatarActor);

    TargetActor = nullptr;
    TargetLoc_Cursor = FVector::ZeroVector;

    APlayerController* PC = Cast<APlayerController>(Character->GetController());
    if (PC)
    {
        FHitResult Hit;
        PC->GetHitResultUnderCursorByChannel(
            UEngineTypes::ConvertToTraceType(ECC_Visibility),
            true,
            Hit
        );

        UE_LOG(LogTemp, Error, TEXT("[Attack] Hit.bBlockingHit: %s | Hit.Actor: %s | Hit.Location: %s"),
            Hit.bBlockingHit ? TEXT("YES") : TEXT("NO"),
            Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("NULL"),
            *Hit.Location.ToString());

        if (Hit.bBlockingHit && IsValid(Hit.GetActor()) &&
            Hit.GetActor() != AvatarActor &&
            UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Hit.GetActor()) != nullptr)
        {
            TargetActor = Hit.GetActor();
            UE_LOG(LogTemp, Warning, TEXT("[Attack] Target actor: %s"), *TargetActor->GetName());
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
                FString::Printf(TEXT("[Attack] Target: %s"), *TargetActor->GetName()));
        }
        else if (Hit.bBlockingHit)
        {
            TargetLoc_Cursor = Hit.Location;
            UE_LOG(LogTemp, Warning, TEXT("[Attack] Moving to cursor pos: %s"), *Hit.Location.ToString());
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange,
                TEXT("[Attack] Moving to cursor position"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[Attack] No cursor hit, jumping forward"));
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange,
                TEXT("[Attack] No cursor hit"));
        }
    }

    const FVector StartLoc = AvatarActor->GetActorLocation();
    const FVector TargetLoc = TargetActor
        ? TargetActor->GetActorLocation()
        : (!TargetLoc_Cursor.IsZero()
            ? TargetLoc_Cursor
            : StartLoc + AvatarActor->GetActorForwardVector() * 500.f);

    FVector LaunchDir = (TargetLoc - StartLoc);
    LaunchDir.Z = 0.f;
    LaunchDir = LaunchDir.GetSafeNormal();

    FVector LaunchVelocity = LaunchDir * LaunchSpeed;
    LaunchVelocity.Z = LaunchZForce;

    const float Distance = FVector::Distance(StartLoc, TargetLoc);

    UE_LOG(LogTemp, Warning, TEXT("[Attack] Distance: %.1f | LaunchVel: %s"),
        Distance, *LaunchVelocity.ToString());
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Cyan,
        FString::Printf(TEXT("[Attack] Dist: %.1f | Launch: %s"),
            Distance, *LaunchVelocity.ToString()));

    DrawDebugLine(GetWorld(), StartLoc, TargetLoc, FColor::Yellow, false, 3.f, 0, 3.f);
    DrawDebugSphere(GetWorld(), StartLoc,  40.f, 12, FColor::Green, false, 3.f);
    DrawDebugSphere(GetWorld(), TargetLoc, 40.f, 12, FColor::Red,   false, 3.f);

    FRotator LookAt = UKismetMathLibrary::FindLookAtRotation(StartLoc, TargetLoc);
    LookAt.Pitch = 0.f;
    LookAt.Roll  = 0.f;
    Character->SetActorRotation(LookAt);

    Character->LaunchCharacter(LaunchVelocity, true, true);

    UE_LOG(LogTemp, Warning, TEXT("[Attack] LaunchCharacter called"));
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
        TEXT("[Attack] LaunchCharacter called"));

    FTimerDelegate DamageDelegate;
    DamageDelegate.BindLambda([this]()
    {
        UE_LOG(LogTemp, Warning, TEXT("[Attack] Damage timer fired"));
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
            TEXT("[Attack] Damage timer fired"));

        if (!CachedActorInfo) return;

        AActor* Avatar = CachedActorInfo->AvatarActor.Get();
        if (!Avatar) return;

        if (UAbilitySystemComponent* ASC = CachedActorInfo->AbilitySystemComponent.Get())
        {
            FGameplayCueParameters CueParams;
            CueParams.Instigator   = Avatar;
            CueParams.EffectCauser = Avatar;
            ASC->ExecuteGameplayCue(GameplayCueTag, CueParams);
        }

        PerformAttackOverlap();
    });

    GetWorld()->GetTimerManager().SetTimer(
        DamageTimerHandle, DamageDelegate, DamageDelay, false);

    FTimerDelegate EndDelegate;
    EndDelegate.BindLambda([this]()
    {
        UE_LOG(LogTemp, Warning, TEXT("[Attack] End timer fired"));
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Purple,
            TEXT("[Attack] End timer fired"));

        RestoreMovement();
        CommitAbilityCooldown(CachedHandle, CachedActorInfo, CachedActivationInfo, true);
        EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, false);
    });

    GetWorld()->GetTimerManager().SetTimer(
        EndTimerHandle, EndDelegate, DamageDelay + 0.3f, false);
}

void UARPGGameplayAbility_Attack::PerformAttackOverlap()
{
    AActor* AvatarActor = CachedActorInfo ? CachedActorInfo->AvatarActor.Get() : nullptr;
    if (!AvatarActor) return;

    UAbilitySystemComponent* SourceASC = CachedActorInfo->AbilitySystemComponent.Get();
    if (!SourceASC || !DamageEffectClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[Attack] SourceASC or DamageEffectClass NULL"));
        return;
    }

    TArray<AActor*> OverlappedActors;
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

    UKismetSystemLibrary::SphereOverlapActors(
        AvatarActor,
        AvatarActor->GetActorLocation(),
        AttackRadius,
        ObjectTypes,
        AARPGCharacter::StaticClass(),
        ActorsToIgnore,
        OverlappedActors
    );

    const FColor SphereColor = OverlappedActors.Num() > 0 ? FColor::Red : FColor::Blue;
    DrawDebugSphere(GetWorld(), AvatarActor->GetActorLocation(),
        AttackRadius, 24, SphereColor, false, 3.f, 0, 2.f);

    UE_LOG(LogTemp, Warning, TEXT("[Attack] Overlap hit %d actors"), OverlappedActors.Num());
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, SphereColor,
        FString::Printf(TEXT("[Attack] Hit %d actors"), OverlappedActors.Num()));

    for (AActor* HitActor : OverlappedActors)
    {
        if (!IsValid(HitActor)) continue;

        UAbilitySystemComponent* TargetASC =
            UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
        if (!TargetASC) continue;

        FGameplayEffectContextHandle CtxHandle = SourceASC->MakeEffectContext();
        CtxHandle.AddInstigator(AvatarActor, AvatarActor);

        FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
            DamageEffectClass, 1.f, CtxHandle);
        if (!SpecHandle.IsValid()) continue;

        UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
            SpecHandle, DamageDataTag, DamageMagnitude);

        TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

        DrawDebugSphere(GetWorld(), HitActor->GetActorLocation(),
            60.f, 12, FColor::Red, false, 3.f, 0, 3.f);
        DrawDebugLine(GetWorld(), AvatarActor->GetActorLocation(),
            HitActor->GetActorLocation(), FColor::Red, false, 3.f, 0, 2.f);

        UE_LOG(LogTemp, Warning, TEXT("[Attack] Damaged: %s | %.1f"),
            *HitActor->GetName(), DamageMagnitude);
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
            FString::Printf(TEXT("[Attack] Hit: %s | DMG: %.1f"),
                *HitActor->GetName(), DamageMagnitude));
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
    MoveComp->SetMovementMode(EMovementMode::MOVE_Walking);

    UE_LOG(LogTemp, Warning, TEXT("[Attack] Movement restored"));
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
        TEXT("[Attack] Movement restored"));
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

    UE_LOG(LogTemp, Warning, TEXT("[Attack] EndAbility | Cancelled: %s"),
        bWasCancelled ? TEXT("true") : TEXT("false"));
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f,
        bWasCancelled ? FColor::Red : FColor::White,
        FString::Printf(TEXT("[Attack] EndAbility | Cancelled: %s"),
            bWasCancelled ? TEXT("true") : TEXT("false")));

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}