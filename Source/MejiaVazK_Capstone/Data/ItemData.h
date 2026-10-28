#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemData.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class EItemType : uint8
{
	Consumable UMETA(DisplayName = "Consumable"),
	Ingredient UMETA(DisplayName = "Ingredient"),
	Material UMETA(DisplayName = "Material"),
	Key UMETA(DisplayName = "Key")
};

/**
 * Un objeto del juego (comida, ingrediente, material...).
 * Lo usan los cofres, el inventario y la cocina. Se crea como Data Asset, sin codigo.
 */
UCLASS(BlueprintType)
class MEJIAVAZK_CAPSTONE_API UItemData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EItemType ItemType = EItemType::Ingredient;

	/** Cuantos caben en un espacio del inventario. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 MaxStack = 99;

	/** Vida que restaura al consumirlo (solo Consumable). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Effects", meta = (ClampMin = "0"))
	float HealAmount = 0.f;

	/** Mana que restaura al consumirlo (solo Consumable). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Effects", meta = (ClampMin = "0"))
	float ManaAmount = 0.f;
};
