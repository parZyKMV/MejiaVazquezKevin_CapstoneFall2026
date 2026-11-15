#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/ItemData.h"
#include "InventoryComponent.generated.h"

/** Un objeto distinto del inventario y cuantos hay. Cada entrada ocupa un espacio de su categoria. */
USTRUCT(BlueprintType)
struct FInventoryEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UItemData> Item = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 Count = 0;
};

UENUM(BlueprintType)
enum class EItemUseFailReason : uint8
{
	NotConsumable UMETA(DisplayName = "Not Consumable"),
	NoneLeft UMETA(DisplayName = "None Left"),
	NoEffect UMETA(DisplayName = "No Effect (vida y mana llenos)"),
	Dead UMETA(DisplayName = "Dead")
};

// Cambio generico (para refrescar la UI). Item nulo = cambio general (por ejemplo ClearAll).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInventoryChangedSignature, UItemData*, Item, int32, NewCount, int32, Delta);
// Solo cuando entran objetos (para el aviso "+1 Gelatina").
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemAddedSignature, UItemData*, Item, int32, Amount);
// No cupo (categoria llena o maximo por objeto).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryFullSignature, UItemData*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemUsedSignature, UItemData*, Item, int32, Remaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemUseFailedSignature, UItemData*, Item, EItemUseFailReason, Reason);

/**
 * Inventario del jugador.
 *
 * Reglas:
 * - Tres categorias con espacios limitados: Consumibles, Ingredientes y Materiales (SlotsPerCategory cada una).
 * - Cada objeto DISTINTO ocupa un espacio, sin importar cuantos lleves (hasta MaxPerItem).
 * - Los objetos Clave no ocupan espacio ni se muestran en la UI.
 * - Usar un consumible cura vida y/o restaura mana (HealthComponent / ManaComponent del dueno).
 *   Si no tendria efecto (vida y mana llenos), no se gasta.
 */
UCLASS(ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class MEJIAVAZK_CAPSTONE_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	/** Espacios (objetos distintos) por categoria. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ClampMin = "1"))
	int32 SlotsPerCategory = 9;

	/** Maximo de unidades de un mismo objeto. (Se ignora ItemData::MaxStack por ahora.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ClampMin = "1"))
	int32 MaxPerItem = 9999;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryChangedSignature OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemAddedSignature OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryFullSignature OnInventoryFull;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemUsedSignature OnItemUsed;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemUseFailedSignature OnItemUseFailed;

	/** Agrega objetos. Devuelve cuantos entraron de verdad (0 si no cupo). */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItem(UItemData* Item, int32 Amount = 1);

	/** Quita objetos. Devuelve false, sin quitar nada, si no hay suficientes. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(UItemData* Item, int32 Amount = 1);

	/** Usa un consumible. Devuelve true si se uso (y se gasto uno). */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UseItem(UItemData* Item);

	/** Para activar o no el boton "Usar" de la UI. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool CanUseItem(const UItemData* Item) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetCount(const UItemData* Item) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasItem(const UItemData* Item, int32 Amount = 1) const { return GetCount(Item) >= Amount; }

	/** Objetos de una categoria, en el orden en que se obtuvieron (para la cuadrícula de la UI). */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FInventoryEntry> GetEntriesByType(EItemType Type) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FInventoryEntry>& GetAllEntries() const { return Entries; }

	/** Espacios ocupados en una categoria. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetSlotsUsed(EItemType Type) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasFreeSlot(EItemType Type) const;

	/** Vacia el inventario (por si mas adelante la muerte lo castiga). */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearAll();

private:
	FInventoryEntry* FindEntry(const UItemData* Item);
	const FInventoryEntry* FindEntry(const UItemData* Item) const;
	static bool UsesSlots(EItemType Type) { return Type != EItemType::Key; }
	bool EvaluateUse(const UItemData* Item, EItemUseFailReason& OutReason) const;

	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TArray<FInventoryEntry> Entries;
};
