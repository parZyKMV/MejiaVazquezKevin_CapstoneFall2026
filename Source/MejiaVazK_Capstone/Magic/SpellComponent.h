#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpellComponent.generated.h"

class USpellData;
class UManaComponent;
class UHealthComponent;

UENUM(BlueprintType)
enum class ESpellFailReason : uint8
{
	NoSpell UMETA(DisplayName = "No Spell"),
	OnCooldown UMETA(DisplayName = "On Cooldown"),
	NotEnoughMana UMETA(DisplayName = "Not Enough Mana"),
	Busy UMETA(DisplayName = "Busy"),
	Dead UMETA(DisplayName = "Dead")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSpellCastSignature, int32, SlotIndex, USpellData*, Spell);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSpellFailedSignature, int32, SlotIndex, ESpellFailReason, Reason);

/**
 * Hechizos equipados y su lanzamiento.
 *
 * Flujo: CastSpell(slot) valida -> gasta mana (ManaComponent::TrySpend) -> reproduce el montage ->
 * el notify "Spell Release" llama ReleaseSpell() -> sale el proyectil.
 * Recibir dano mientras se lanza lo interrumpe (el mana ya gastado no se devuelve).
 */
UCLASS(ClassGroup = (Magic), meta = (BlueprintSpawnableComponent))
class MEJIAVAZK_CAPSTONE_API USpellComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpellComponent();

	/** Hechizos de cada espacio (el 0 es la tecla 1, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spells")
	TArray<TObjectPtr<USpellData>> EquippedSpells;

	UPROPERTY(BlueprintAssignable, Category = "Spells|Events")
	FOnSpellCastSignature OnSpellCast;

	UPROPERTY(BlueprintAssignable, Category = "Spells|Events")
	FOnSpellFailedSignature OnSpellFailed;

	/** Lanza el hechizo del espacio indicado. Devuelve true si empezo el lanzamiento. */
	UFUNCTION(BlueprintCallable, Category = "Spells")
	bool CastSpell(int32 SlotIndex);

	/** Lo llama el notify "Spell Release": crea el proyectil. */
	UFUNCTION(BlueprintCallable, Category = "Spells")
	void ReleaseSpell();

	UFUNCTION(BlueprintCallable, Category = "Spells")
	void InterruptCast();

	UFUNCTION(BlueprintPure, Category = "Spells")
	bool IsCasting() const { return bCasting; }

	UFUNCTION(BlueprintPure, Category = "Spells")
	float GetCooldownRemaining(int32 SlotIndex) const;

	/** 0 = listo, 1 = recien lanzado (para la barra de recarga del HUD). */
	UFUNCTION(BlueprintPure, Category = "Spells")
	float GetCooldownPercent(int32 SlotIndex) const;

	static USpellComponent* FindFromMesh(const USkeletalMeshComponent* MeshComp);

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleOwnerHealthChanged(float NewHealth, float MaxHealth, float Delta, AActor* DamageCauser);

	void HandleMontageEnded(int32 Serial, bool bInterrupted);
	void Fail(int32 SlotIndex, ESpellFailReason Reason);

	bool bCasting = false;
	bool bReleased = false;
	int32 ActiveSlot = INDEX_NONE;
	int32 CastSerial = 0;

	/** Tiempo del mundo en que termina la recarga de cada espacio. */
	TArray<float> CooldownEnd;
	TArray<float> CooldownLength;
};
