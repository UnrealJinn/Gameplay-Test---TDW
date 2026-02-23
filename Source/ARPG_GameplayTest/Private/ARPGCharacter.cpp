#include "ARPGCharacter.h"
#include "ARPG_GameplayTest/GameplayAbilitySystem/PlayerAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
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

	// show cursor for click-to-attack
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
	}

	if (!AbilitySystemComponent) return;

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	if (GrantedAbilityClass)
	{
		FGameplayAbilitySpec Spec(GrantedAbilityClass, 1, INDEX_NONE, this);
		AbilitySystemComponent->GiveAbility(Spec);
	}

	// watch for mana changes to handle restore logic
	ManaChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UPlayerAttributeSet::GetManaAttribute()).AddUObject(this, &AARPGCharacter::HandleManaChanged);

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
	if (Data.NewValue < Data.OldValue)
	{
		// mana spent - clear any active mana effects and start restore timer
		AbilitySystemComponent->RemoveActiveEffectsWithGrantedTags(ManaEffectGrantedTags);

		GetWorldTimerManager().SetTimer(ManaDelayTimerHandle, [this]()
		{
			if (!AbilitySystemComponent || !ManaRestoreEffect) return;

			FGameplayEffectContextHandle Ctx = AbilitySystemComponent->MakeEffectContext();
			Ctx.AddSourceObject(this);

			FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(ManaRestoreEffect, 1, Ctx);
			if (Spec.IsValid())
				AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

		}, 2.5f, false);
	}
	else
	{
		// mana went up - if full, clear restore effects
		if (PlayerAttributeSet && Data.NewValue >= PlayerAttributeSet->GetManaMax())
			AbilitySystemComponent->RemoveActiveEffectsWithGrantedTags(ManaEffectGrantedTags);
	}
}

void AARPGCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (IA_ActivateAbility)
			EIC->BindAction(IA_ActivateAbility, ETriggerEvent::Started, this, &AARPGCharacter::Input_RightMouseButton);
	}
}

void AARPGCharacter::Input_RightMouseButton(const FInputActionValue& Value)
{
	if (AbilitySystemComponent && GrantedAbilityClass)
		AbilitySystemComponent->TryActivateAbilityByClass(GrantedAbilityClass);
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