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

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
    class UAbilitySystemComponent* AbilitySystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
    class UPlayerAttributeSet* PlayerAttributeSet;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
    TSubclassOf<class UGameplayAbility> GrantedAbilityClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
    FGameplayTagContainer ManaEffectGrantedTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
    TSubclassOf<class UGameplayEffect> ManaRestoreEffect;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputMappingContext* DefaultMappingContext;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* IA_ActivateAbility;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    FDelegateHandle ManaChangedDelegateHandle;
    FTimerHandle ManaDelayTimerHandle;

    void HandleManaChanged(const FOnAttributeChangeData& Data);

public:
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
    void Input_RightMouseButton(const FInputActionValue& Value);
};