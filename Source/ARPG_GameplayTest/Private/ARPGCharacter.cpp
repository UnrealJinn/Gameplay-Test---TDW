#include "ARPGCharacter.h"
#include "ARPG_GameplayTest/GameplayAbilitySystem/PlayerAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "InputAction.h"

AARPGCharacter::AARPGCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    PlayerAttributeSet = CreateDefaultSubobject<UPlayerAttributeSet>(TEXT("PlayerAttributeSet"));
}

UAbilitySystemComponent* AARPGCharacter::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void AARPGCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor = true;
        PC->SetInputMode(FInputModeGameAndUI());
    }

    if (!AbilitySystemComponent) return;

    AbilitySystemComponent->InitAbilityActorInfo(this, this);

    if (GrantedAbilityClass)
    {
        FGameplayAbilitySpec AbilitySpec(GrantedAbilityClass, 1, INDEX_NONE, this);
        AbilitySystemComponent->GiveAbility(AbilitySpec);
    }

    ManaChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        UPlayerAttributeSet::GetManaAttribute()
    ).AddUObject(this, &AARPGCharacter::HandleManaChanged);

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }
}

void AARPGCharacter::HandleManaChanged(const FOnAttributeChangeData& Data)
{
    const float NewValue = Data.NewValue;
    const float OldValue = Data.OldValue;

    if (NewValue < OldValue)
    {
        AbilitySystemComponent->RemoveActiveEffectsWithGrantedTags(ManaEffectGrantedTags);

        FTimerDelegate DelayedApply;
        DelayedApply.BindLambda([this]()
        {
            if (AbilitySystemComponent && ManaRestoreEffect)
            {
                FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
                ContextHandle.AddSourceObject(this);

                FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
                    ManaRestoreEffect, 1, ContextHandle
                );

                if (SpecHandle.IsValid())
                {
                    AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
                }
            }
        });

        GetWorldTimerManager().SetTimer(ManaDelayTimerHandle, DelayedApply, 2.5f, false);
    }
    else
    {
        if (PlayerAttributeSet && NewValue >= PlayerAttributeSet->GetManaMax())
        {
            AbilitySystemComponent->RemoveActiveEffectsWithGrantedTags(ManaEffectGrantedTags);
        }
    }
}

void AARPGCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (IA_ActivateAbility)
        {
            EIC->BindAction(IA_ActivateAbility, ETriggerEvent::Started, this, &AARPGCharacter::Input_RightMouseButton);
        }
    }
}

void AARPGCharacter::Input_RightMouseButton(const FInputActionValue& Value)
{
    if (AbilitySystemComponent && GrantedAbilityClass)
    {
        AbilitySystemComponent->TryActivateAbilityByClass(GrantedAbilityClass);
    }
}

void AARPGCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
            UPlayerAttributeSet::GetManaAttribute()
        ).Remove(ManaChangedDelegateHandle);
    }

    Super::EndPlay(EndPlayReason);
}

void AARPGCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}