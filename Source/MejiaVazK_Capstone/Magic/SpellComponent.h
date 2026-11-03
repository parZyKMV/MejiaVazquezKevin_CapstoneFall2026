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
/** Un espacio cambio de hechizo (o se vacio). Para actualizar el HUD. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSpellEquippedSignature, int32, SlotIndex, USpellData*, Spell);
/** Se desbloqueo un hechizo nuevo. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpellUnlockedSignature, USpellData*, Spell);

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

	/** Cantidad de espacios equipables (teclas 1, 2 y 3). */
	static constexpr int32 NumSlots = 3;

	/** Hechizos de cada espacio (el 0 es la tecla 1, etc.). Siempre tiene NumSlots elementos; un espacio puede estar vacio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spells")
	TArray<TObjectPtr<USpellData>> EquippedSpells;

	/** Todos los hechizos que el jugador ya tiene desbloqueados (los equipados y los de reserva). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spells")
	TArray<TObjectPtr<USpellData>> UnlockedSpells;

	UPROPERTY(BlueprintAssignable, Category = "Spells|Events")
	FOnSpellEquippedSignature OnSpellEquipped;

	UPROPERTY(BlueprintAssignable, Category = "Spells|Events")
	FOnSpellUnlockedSignature OnSpellUnlocked;

	/** Desbloquea un hechizo. Si hay un espacio libre, lo equipa solo. Devuelve false si ya lo tenia. */
	UFUNCTION(BlueprintCallable, Category = "Spells")
	bool UnlockSpell(USpellData* Spell);

	/**
	 * Pone un hechizo desbloqueado en un espacio (reemplaza al que estaba).
	 * Si el hechizo ya estaba en otro espacio, los dos se intercambian.
	 * No funciona mientras se esta lanzando un hechizo.
	 */
	UFUNCTION(BlueprintCallable, Category = "Spells")
	bool EquipSpell(int32 SlotIndex, USpellData* Spell);

	/** Vacia un espacio (el hechizo sigue desbloqueado). */
	UFUNCTION(BlueprintCallable, Category = "Spells")
	bool UnequipSlot(int32 SlotIndex);

	UFUNCTION(BlueprintPure, Category = "Spells")
	USpellData* GetEquippedSpell(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Spells")
	bool IsSpellUnlocked(const USpellData* Spell) const { return UnlockedSpells.Contains(Spell); }

	/** Hechizos desbloqueados que no estan equipados (para la pantalla de cambio). */
	UFUNCTION(BlueprintPure, Category = "Spells")
	TArray<USpellData*> GetReserveSpells() const;

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
