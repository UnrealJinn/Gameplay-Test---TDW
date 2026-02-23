#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "InputActionValue.h"
#include "ARPGCharacter.generated.h"

UCLASS()
class ARPG_GAMEPLAYTEST_API AARPGCharacter : public ACharacter, public IAbilitySystemInterface 
{
    GENERATED_BODY()

public:
    AARPGCharacter();
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // GAS
    UPROPERTY(VisibleAnywhere, Category = "GAS")
    class UAbilitySystemComponent* AbilitySystemComponent;

    UPROPERTY(BlueprintReadOnly, Category = "GAS")
    class UPlayerAttributeSet* PlayerAttributeSet;

    UPROPERTY(EditDefaultsOnly, Category = "GAS")
    TSubclassOf<class UGameplayAbility> GrantedAbilityClass;

    // mana restore stuff
    UPROPERTY(EditDefaultsOnly, Category = "GAS")
    TSubclassOf<class UGameplayEffect> ManaRestoreEffect;

    UPROPERTY(EditDefaultsOnly, Category = "GAS")
    FGameplayTagContainer ManaEffectGrantedTags;

    // input
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    class UInputMappingContext* DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    class UInputAction* IA_ActivateAbility;

private:
    void Input_RightMouseButton(const FInputActionValue& Value);
    void HandleManaChanged(const FOnAttributeChangeData& Data);

    FDelegateHandle ManaChangedDelegateHandle;
    FTimerHandle ManaDelayTimerHandle;
};